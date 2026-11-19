#include "metadata.h"
#include "bitmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Responsabilidade: Estudante 2
 * Gerenciamento de Metadados dos Arquivos, Referencias aos Blocos de Dados
 * e Operacoes: create, rm, stat.
 */

/* Leitura de um inode especifico da tabela de inodes no disco */
int metadata_read_inode(uint32_t inode_idx, minifs_inode_t *inode) {
    if (!g_minifs.is_mounted || !inode || inode_idx >= g_minifs.sb.total_inodes) {
        return MINIFS_ERR_INVALID_PARAM;
    }

    uint32_t block_offset = inode_idx / MINIFS_INODES_PER_BLOCK;
    uint32_t block_num = g_minifs.sb.inode_start_block + block_offset;
    uint32_t slot_in_block = inode_idx % MINIFS_INODES_PER_BLOCK;

    uint8_t buffer[MINIFS_BLOCK_SIZE];
    if (disk_read_block(block_num, buffer) != 0) {
        return MINIFS_ERR_IO;
    }

    minifs_inode_t *table = (minifs_inode_t *)buffer;
    memcpy(inode, &table[slot_in_block], sizeof(minifs_inode_t));
    return MINIFS_SUCCESS;
}

/* Gravacao de um inode especifico na tabela de inodes no disco */
int metadata_write_inode(uint32_t inode_idx, const minifs_inode_t *inode) {
    if (!g_minifs.is_mounted || !inode || inode_idx >= g_minifs.sb.total_inodes) {
        return MINIFS_ERR_INVALID_PARAM;
    }

    uint32_t block_offset = inode_idx / MINIFS_INODES_PER_BLOCK;
    uint32_t block_num = g_minifs.sb.inode_start_block + block_offset;
    uint32_t slot_in_block = inode_idx % MINIFS_INODES_PER_BLOCK;

    uint8_t buffer[MINIFS_BLOCK_SIZE];
    if (disk_read_block(block_num, buffer) != 0) {
        return MINIFS_ERR_IO;
    }

    minifs_inode_t *table = (minifs_inode_t *)buffer;
    memcpy(&table[slot_in_block], inode, sizeof(minifs_inode_t));

    if (disk_write_block(block_num, buffer) != 0) {
        return MINIFS_ERR_IO;
    }

    return MINIFS_SUCCESS;
}

/* Localiza um arquivo na tabela de inodes pelo nome */
int metadata_find_by_name(const char *name, uint32_t *out_idx, minifs_inode_t *out_inode) {
    if (!g_minifs.is_mounted || !name) {
        return MINIFS_ERR_IO;
    }

    for (uint32_t i = 0; i < g_minifs.sb.total_inodes; i++) {
        minifs_inode_t node;
        if (metadata_read_inode(i, &node) == MINIFS_SUCCESS) {
            if (node.used && strncmp(node.name, name, MINIFS_MAX_FILENAME) == 0) {
                if (out_idx) *out_idx = i;
                if (out_inode) *out_inode = node;
                return MINIFS_SUCCESS;
            }
        }
    }
    return MINIFS_ERR_FILE_NOT_FOUND;
}

/* Localiza uma entrada livre na tabela de inodes */
int metadata_find_free_slot(uint32_t *out_idx) {
    if (!g_minifs.is_mounted || !out_idx) {
        return MINIFS_ERR_IO;
    }

    for (uint32_t i = 0; i < g_minifs.sb.total_inodes; i++) {
        minifs_inode_t node;
        if (metadata_read_inode(i, &node) == MINIFS_SUCCESS) {
            if (!node.used) {
                *out_idx = i;
                return MINIFS_SUCCESS;
            }
        }
    }
    return MINIFS_ERR_INODE_TABLE_FULL;
}

/* Persiste o Superblock atualizado no Bloco 0 */
static int sync_superblock(void) {
    uint8_t sb_block[MINIFS_BLOCK_SIZE];
    memset(sb_block, 0, sizeof(sb_block));
    memcpy(sb_block, &g_minifs.sb, sizeof(minifs_superblock_t));
    if (disk_write_block(0, sb_block) != 0) {
        return MINIFS_ERR_IO;
    }
    return MINIFS_SUCCESS;
}

/**
 * Operacao: create <arquivo>
 * Cria uma entrada para o arquivo com tamanho 0 e nenhum bloco alocado.
 * Valida: nome duplicado, limite de entradas na tabela de metadados.
 */
int fs_create(const char *name) {
    if (!g_minifs.is_mounted) {
        return MINIFS_ERR_NOT_FORMATTED;
    }

    if (!name || strlen(name) == 0) {
        return MINIFS_ERR_INVALID_PARAM;
    }

    if (strlen(name) >= MINIFS_MAX_FILENAME) {
        return MINIFS_ERR_INVALID_PARAM;
    }

    /* 1. Trata erro: nome duplicado */
    uint32_t existing_idx;
    if (metadata_find_by_name(name, &existing_idx, NULL) == MINIFS_SUCCESS) {
        return MINIFS_ERR_FILE_EXISTS;
    }

    /* 2. Trata erro: tabela de inodes cheia */
    uint32_t free_slot;
    if (metadata_find_free_slot(&free_slot) != MINIFS_SUCCESS) {
        return MINIFS_ERR_INODE_TABLE_FULL;
    }

    /* 3. Preenche metadados do arquivo */
    minifs_inode_t new_file;
    memset(&new_file, 0, sizeof(minifs_inode_t));
    new_file.id = g_minifs.sb.next_file_id++;
    new_file.used = 1;
    strncpy(new_file.name, name, MINIFS_MAX_FILENAME - 1);
    new_file.size = 0;
    /* new_file.blocks ja esta zerado pelo memset */

    /* 4. Persiste o inode no disco */
    if (metadata_write_inode(free_slot, &new_file) != MINIFS_SUCCESS) {
        return MINIFS_ERR_IO;
    }

    /* 5. Atualiza contadores do superblock e persiste */
    g_minifs.sb.free_inodes_count--;
    sync_superblock();

    return MINIFS_SUCCESS;
}

/**
 * Operacao: rm <arquivo>
 * Remove o arquivo, liberando seus metadados e todos os blocos de dados
 * referenciados no bitmap de blocos (tornando-os disponiveis para reutilizacao).
 * Valida: arquivo inexistente.
 */
int fs_rm(const char *name) {
    if (!g_minifs.is_mounted) {
        return MINIFS_ERR_NOT_FORMATTED;
    }

    if (!name || strlen(name) == 0) {
        return MINIFS_ERR_INVALID_PARAM;
    }

    /* 1. Trata erro: arquivo inexistente */
    uint32_t file_idx;
    minifs_inode_t inode;
    if (metadata_find_by_name(name, &file_idx, &inode) != MINIFS_SUCCESS) {
        return MINIFS_ERR_FILE_NOT_FOUND;
    }

    /* 2. Libera todos os blocos de dados indexados no bitmap */
    uint32_t num_blocks = (inode.size + MINIFS_BLOCK_SIZE - 1) / MINIFS_BLOCK_SIZE;
    if (inode.size == 0) {
        num_blocks = 0;
    }

    for (uint32_t i = 0; i < num_blocks && i < MINIFS_DIRECT_BLOCKS; i++) {
        uint32_t blk = inode.blocks[i];
        if (blk >= g_minifs.sb.data_start_block && blk < g_minifs.sb.total_blocks) {
            bitmap_free_block(blk);
        }
    }

    /* 3. Limpa o registro do inode (marca como livre) */
    memset(&inode, 0, sizeof(minifs_inode_t));
    if (metadata_write_inode(file_idx, &inode) != MINIFS_SUCCESS) {
        return MINIFS_ERR_IO;
    }

    /* 4. Atualiza contadores do superblock e persiste */
    g_minifs.sb.free_inodes_count++;
    sync_superblock();

    return MINIFS_SUCCESS;
}

/**
 * Operacao: stat <arquivo>
 * Exibe informacoes detalhadas dos metadados do arquivo:
 * ID, nome, tamanho em bytes, quantidade de blocos alocados e
 * a lista com os indices exatos dos blocos de dados utilizados.
 * Valida: arquivo inexistente.
 */
int fs_stat(const char *name) {
    if (!g_minifs.is_mounted) {
        return MINIFS_ERR_NOT_FORMATTED;
    }

    if (!name || strlen(name) == 0) {
        return MINIFS_ERR_INVALID_PARAM;
    }

    /* 1. Trata erro: arquivo inexistente */
    uint32_t file_idx;
    minifs_inode_t inode;
    if (metadata_find_by_name(name, &file_idx, &inode) != MINIFS_SUCCESS) {
        return MINIFS_ERR_FILE_NOT_FOUND;
    }

    /* 2. Calcula blocos ocupados */
    uint32_t num_blocks = (inode.size + MINIFS_BLOCK_SIZE - 1) / MINIFS_BLOCK_SIZE;
    if (inode.size == 0) {
        num_blocks = 0;
    }

    /* 3. Imprime as informacoes formatadas */
    printf("--- Informacoes do Arquivo (stat) ---\n");
    printf("Identificador (ID): %u\n", inode.id);
    printf("Nome do Arquivo   : %s\n", inode.name);
    printf("Tamanho em bytes  : %u bytes\n", inode.size);
    printf("Blocos alocados   : %u de %u maximo\n", num_blocks, MINIFS_DIRECT_BLOCKS);
    printf("Indices dos blocos: ");
    if (num_blocks == 0) {
        printf("(nenhum bloco alocado - arquivo vazio)\n");
    } else {
        printf("[");
        for (uint32_t i = 0; i < num_blocks; i++) {
            printf("%u%s", inode.blocks[i], (i + 1 < num_blocks) ? ", " : "");
        }
        printf("]\n");
    }
    printf("-------------------------------------\n");

    return MINIFS_SUCCESS;
}
