#include "bitmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Responsabilidade: Estudante 1
 * Implementacao do Bitmap de controle de blocos livres/ocupados.
 */

int bitmap_init_empty(uint8_t *bitmap, uint32_t total_blocks, uint32_t data_start) {
    if (!bitmap) return MINIFS_ERR_INVALID_PARAM;

    uint32_t total_bytes = (total_blocks + 7) / 8;
    memset(bitmap, 0, total_bytes);

    /* Marca blocos de sistema (Superblock, Bitmap, Inodes) como ocupados (1) */
    for (uint32_t i = 0; i < data_start && i < total_blocks; i++) {
        uint32_t byte_idx = i / 8;
        uint8_t bit_mask = (uint8_t)(1 << (i % 8));
        bitmap[byte_idx] |= bit_mask;
    }

    return MINIFS_SUCCESS;
}

int bitmap_load_from_disk(void) {
    if (!g_minifs.bitmap || !disk_is_open()) {
        return MINIFS_ERR_IO;
    }

    uint32_t start_blk = g_minifs.sb.bitmap_start_block;
    uint32_t num_blks = g_minifs.sb.bitmap_blocks;

    for (uint32_t i = 0; i < num_blks; i++) {
        int res = disk_read_block(start_blk + i, g_minifs.bitmap + (i * MINIFS_BLOCK_SIZE));
        if (res != 0) {
            return MINIFS_ERR_IO;
        }
    }
    return MINIFS_SUCCESS;
}

int bitmap_save_to_disk(void) {
    if (!g_minifs.bitmap || !disk_is_open()) {
        return MINIFS_ERR_IO;
    }

    uint32_t start_blk = g_minifs.sb.bitmap_start_block;
    uint32_t num_blks = g_minifs.sb.bitmap_blocks;

    for (uint32_t i = 0; i < num_blks; i++) {
        int res = disk_write_block(start_blk + i, g_minifs.bitmap + (i * MINIFS_BLOCK_SIZE));
        if (res != 0) {
            return MINIFS_ERR_IO;
        }
    }
    return MINIFS_SUCCESS;
}

int bitmap_alloc_block(uint32_t *out_block) {
    if (!g_minifs.is_mounted || !g_minifs.bitmap || !out_block) {
        return MINIFS_ERR_IO;
    }

    if (g_minifs.sb.free_blocks_count == 0) {
        return MINIFS_ERR_DISK_FULL;
    }

    /* Busca o primeiro bloco de dados livre (First Fit) */
    for (uint32_t b = g_minifs.sb.data_start_block; b < g_minifs.sb.total_blocks; b++) {
        uint32_t byte_idx = b / 8;
        uint8_t bit_mask = (uint8_t)(1 << (b % 8));

        if (!(g_minifs.bitmap[byte_idx] & bit_mask)) {
            /* Encontrou bloco livre: aloca */
            g_minifs.bitmap[byte_idx] |= bit_mask;
            g_minifs.sb.free_blocks_count--;
            *out_block = b;

            /* Zera o bloco de dados no disco */
            uint8_t zero[MINIFS_BLOCK_SIZE];
            memset(zero, 0, sizeof(zero));
            disk_write_block(b, zero);

            /* Persiste alteracao do bitmap no disco */
            bitmap_save_to_disk();
            return MINIFS_SUCCESS;
        }
    }

    return MINIFS_ERR_DISK_FULL;
}

int bitmap_free_block(uint32_t block_num) {
    if (!g_minifs.is_mounted || !g_minifs.bitmap) {
        return MINIFS_ERR_IO;
    }

    if (block_num < g_minifs.sb.data_start_block || block_num >= g_minifs.sb.total_blocks) {
        return MINIFS_ERR_INVALID_PARAM;
    }

    uint32_t byte_idx = block_num / 8;
    uint8_t bit_mask = (uint8_t)(1 << (block_num % 8));

    if (g_minifs.bitmap[byte_idx] & bit_mask) {
        g_minifs.bitmap[byte_idx] &= ~bit_mask;
        g_minifs.sb.free_blocks_count++;

        /* Persiste alteracao no disco */
        bitmap_save_to_disk();
    }

    return MINIFS_SUCCESS;
}

bool bitmap_is_allocated(uint32_t block_num) {
    if (!g_minifs.bitmap || block_num >= g_minifs.sb.total_blocks) {
        return true; /* Trata fora dos limites como ocupado */
    }

    uint32_t byte_idx = block_num / 8;
    uint8_t bit_mask = (uint8_t)(1 << (block_num % 8));
    return (g_minifs.bitmap[byte_idx] & bit_mask) != 0;
}
