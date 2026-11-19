#include "fs_ops.h"
#include "metadata.h"
#include "bitmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Responsabilidade: Carlos Alberto Magalhaes Neto e Rafael Jeronimo Nunes Vasconcelos (Atribuicao 3)
 * Implementacao das operacoes de alto nivel:
 * - format: formatacao do disco e inicializacao das estruturas
 * - write: escrita indexada em blocos de dados
 * - read: leitura indexada a partir dos blocos
 * - ls: listagem de diretorio/arquivos
 * - df: relatorio de espaco livre e ocupado
 * - mount / unmount: montagem e persistencia
 */

/* Variavel global do sistema de arquivos */
minifs_fs_t g_minifs = {
    .is_mounted = false,
    .bitmap = NULL
};

/* Mensagens de erro legiveis */
const char *minifs_strerror(minifs_error_t err) {
    switch (err) {
        case MINIFS_SUCCESS:
            return "Sucesso";
        case MINIFS_ERR_NOT_FORMATTED:
            return "Erro: Disco nao formatado ou corrompido";
        case MINIFS_ERR_DISK_FULL:
            return "Erro: Disco cheio (sem blocos de dados livres)";
        case MINIFS_ERR_FILE_NOT_FOUND:
            return "Erro: Arquivo inexistente";
        case MINIFS_ERR_FILE_EXISTS:
            return "Erro: Nome duplicado (arquivo ja existe)";
        case MINIFS_ERR_FILE_TOO_LARGE:
            return "Erro: Arquivo acima do limite suportado";
        case MINIFS_ERR_INODE_TABLE_FULL:
            return "Erro: Tabela de metadados cheia (limite maximo de arquivos atingido)";
        case MINIFS_ERR_IO:
            return "Erro de E/S no disco virtual";
        case MINIFS_ERR_INVALID_PARAM:
            return "Erro: Parametro invalido";
        default:
            return "Erro desconhecido";
    }
}

static int sync_superblock(void) {
    uint8_t sb_block[MINIFS_BLOCK_SIZE];
    memset(sb_block, 0, sizeof(sb_block));
    memcpy(sb_block, &g_minifs.sb, sizeof(minifs_superblock_t));
    if (disk_write_block(0, sb_block) != 0) {
        return MINIFS_ERR_IO;
    }
    return MINIFS_SUCCESS;
}

int fs_mount(const char *disk_path) {
    if (g_minifs.is_mounted) {
        fs_unmount();
    }

    if (disk_open(disk_path) != 0) {
        return MINIFS_ERR_IO;
    }

    uint8_t sb_block[MINIFS_BLOCK_SIZE];
    if (disk_read_block(0, sb_block) != 0) {
        disk_close();
        return MINIFS_ERR_IO;
    }

    memcpy(&g_minifs.sb, sb_block, sizeof(minifs_superblock_t));

    /* Validacao da assinatura (magic number) */
    if (g_minifs.sb.magic != MINIFS_MAGIC) {
        disk_close();
        return MINIFS_ERR_NOT_FORMATTED;
    }

    /* Aloca memoria para o bitmap e carrega do disco */
    size_t bitmap_size_bytes = g_minifs.sb.bitmap_blocks * MINIFS_BLOCK_SIZE;
    g_minifs.bitmap = (uint8_t *)malloc(bitmap_size_bytes);
    if (!g_minifs.bitmap) {
        disk_close();
        return MINIFS_ERR_IO;
    }

    g_minifs.is_mounted = true;

    if (bitmap_load_from_disk() != MINIFS_SUCCESS) {
        free(g_minifs.bitmap);
        g_minifs.bitmap = NULL;
        g_minifs.is_mounted = false;
        disk_close();
        return MINIFS_ERR_IO;
    }

    return MINIFS_SUCCESS;
}

int fs_unmount(void) {
    if (!g_minifs.is_mounted) {
        return MINIFS_SUCCESS;
    }

    /* Garante a sincronizacao de superblock e bitmap */
    sync_superblock();
    bitmap_save_to_disk();

    if (g_minifs.bitmap) {
        free(g_minifs.bitmap);
        g_minifs.bitmap = NULL;
    }

    g_minifs.is_mounted = false;
    disk_close();
    return MINIFS_SUCCESS;
}

/**
 * Operacao: format <quantidade_blocos>
 * Inicializa o disco virtual, gravando Superblock, Bitmap zerado,
 * tabela de Inodes zerada e blocos de dados.
 */
int fs_format(uint32_t total_blocks) {
    if (total_blocks == 0) {
        total_blocks = 2048; /* Padrao sugerido: 2048 blocos (1 MB) */
    }

    if (g_minifs.is_mounted) {
        fs_unmount();
    }

    /* Calculo de layout */
    uint32_t sb_block = 0;
    uint32_t bitmap_start = 1;
    /* Quantidade de blocos para o bitmap: cada bloco de 512B tem 4096 bits */
    uint32_t bitmap_blocks = (total_blocks + 4095) / 4096;
    uint32_t inode_start = bitmap_start + bitmap_blocks;
    uint32_t inode_blocks = (total_blocks <= 64) ? 2 : 16;
    uint32_t data_start = inode_start + inode_blocks;

    if (data_start >= total_blocks) {
        return MINIFS_ERR_INVALID_PARAM;
    }

    if (disk_init(MINIFS_DEFAULT_DISK_FILE, total_blocks) != 0) {
        return MINIFS_ERR_IO;
    }

    uint32_t data_blocks = total_blocks - data_start;

    /* Prepara o Superblock */
    memset(&g_minifs.sb, 0, sizeof(minifs_superblock_t));
    g_minifs.sb.magic = MINIFS_MAGIC;
    g_minifs.sb.block_size = MINIFS_BLOCK_SIZE;
    g_minifs.sb.total_blocks = total_blocks;
    g_minifs.sb.bitmap_start_block = bitmap_start;
    g_minifs.sb.bitmap_blocks = bitmap_blocks;
    g_minifs.sb.inode_start_block = inode_start;
    g_minifs.sb.inode_blocks = inode_blocks;
    g_minifs.sb.data_start_block = data_start;
    g_minifs.sb.data_blocks = data_blocks;
    g_minifs.sb.free_blocks_count = data_blocks;
    g_minifs.sb.total_inodes = inode_blocks * MINIFS_INODES_PER_BLOCK;
    g_minifs.sb.free_inodes_count = g_minifs.sb.total_inodes;
    g_minifs.sb.next_file_id = 1;

    /* Escreve Superblock no Bloco 0 */
    uint8_t block_buf[MINIFS_BLOCK_SIZE];
    memset(block_buf, 0, MINIFS_BLOCK_SIZE);
    memcpy(block_buf, &g_minifs.sb, sizeof(minifs_superblock_t));
    disk_write_block(sb_block, block_buf);

    /* Prepara Bitmap */
    size_t bitmap_bytes = bitmap_blocks * MINIFS_BLOCK_SIZE;
    uint8_t *temp_bitmap = (uint8_t *)calloc(1, bitmap_bytes);
    if (!temp_bitmap) {
        disk_close();
        return MINIFS_ERR_IO;
    }

    bitmap_init_empty(temp_bitmap, total_blocks, data_start);
    for (uint32_t i = 0; i < bitmap_blocks; i++) {
        disk_write_block(bitmap_start + i, temp_bitmap + (i * MINIFS_BLOCK_SIZE));
    }
    free(temp_bitmap);

    /* Zera a tabela de Inodes */
    memset(block_buf, 0, MINIFS_BLOCK_SIZE);
    for (uint32_t i = 0; i < inode_blocks; i++) {
        disk_write_block(inode_start + i, block_buf);
    }

    /* Monta o sistema recem-formatado */
    return fs_mount(MINIFS_DEFAULT_DISK_FILE);
}

/**
 * Operacao: write <arquivo> <conteudo>
 * Grava dados no arquivo utilizando alocacao indexada simplificada.
 * Trata erros:
 * - arquivo inexistente
 * - arquivo acima do limite suportado
 * - disco cheio (falta de blocos de dados)
 */
int fs_write(const char *name, const void *data, size_t length) {
    if (!g_minifs.is_mounted) {
        return MINIFS_ERR_NOT_FORMATTED;
    }

    /* 1. Trata erro: arquivo acima do limite suportado */
    if (length > MINIFS_MAX_FILE_SIZE) {
        return MINIFS_ERR_FILE_TOO_LARGE;
    }

    /* 2. Trata erro: arquivo inexistente */
    uint32_t file_idx;
    minifs_inode_t inode;
    if (metadata_find_by_name(name, &file_idx, &inode) != MINIFS_SUCCESS) {
        return MINIFS_ERR_FILE_NOT_FOUND;
    }

    /* Quantidade de blocos necessarios */
    uint32_t needed_blocks = (length == 0) ? 0 : (uint32_t)((length + MINIFS_BLOCK_SIZE - 1) / MINIFS_BLOCK_SIZE);
    uint32_t current_blocks = (inode.size == 0) ? 0 : (uint32_t)((inode.size + MINIFS_BLOCK_SIZE - 1) / MINIFS_BLOCK_SIZE);

    /* 3. Trata erro: disco cheio (verifica antes se ha espaco suficiente para a expansao) */
    if (needed_blocks > current_blocks) {
        uint32_t extra_needed = needed_blocks - current_blocks;
        if (g_minifs.sb.free_blocks_count < extra_needed) {
            return MINIFS_ERR_DISK_FULL;
        }

        /* Aloca novos blocos no bitmap e indexa no inode */
        for (uint32_t i = current_blocks; i < needed_blocks; i++) {
            uint32_t new_block = 0;
            int res = bitmap_alloc_block(&new_block);
            if (res != MINIFS_SUCCESS) {
                /* Caso inesperado de falha: reverte blocos alocados nesta tentativa */
                for (uint32_t j = current_blocks; j < i; j++) {
                    bitmap_free_block(inode.blocks[j]);
                    inode.blocks[j] = 0;
                }
                return res;
            }
            inode.blocks[i] = new_block;
        }
    } else if (needed_blocks < current_blocks) {
        /* Truncamento: libera blocos excedentes no bitmap */
        for (uint32_t i = needed_blocks; i < current_blocks; i++) {
            bitmap_free_block(inode.blocks[i]);
            inode.blocks[i] = 0;
        }
    }

    /* 4. Escreve os dados nos blocos alocados */
    const uint8_t *src = (const uint8_t *)data;
    size_t remaining = length;

    for (uint32_t i = 0; i < needed_blocks; i++) {
        uint8_t blk_buf[MINIFS_BLOCK_SIZE];
        memset(blk_buf, 0, MINIFS_BLOCK_SIZE);

        size_t chunk = (remaining > MINIFS_BLOCK_SIZE) ? MINIFS_BLOCK_SIZE : remaining;
        if (src && chunk > 0) {
            memcpy(blk_buf, src, chunk);
            src += chunk;
            remaining -= chunk;
        }

        if (disk_write_block(inode.blocks[i], blk_buf) != 0) {
            return MINIFS_ERR_IO;
        }
    }

    /* 5. Atualiza metadados e persiste */
    inode.size = (uint32_t)length;
    if (metadata_write_inode(file_idx, &inode) != MINIFS_SUCCESS) {
        return MINIFS_ERR_IO;
    }

    sync_superblock();
    return MINIFS_SUCCESS;
}

/**
 * Operacao: read <arquivo>
 * Le o conteudo dos blocos indexados pelo arquivo.
 * Trata erro: arquivo inexistente.
 */
int fs_read(const char *name, void *buffer, size_t max_len, size_t *out_len) {
    if (!g_minifs.is_mounted) {
        return MINIFS_ERR_NOT_FORMATTED;
    }

    if (!buffer) {
        return MINIFS_ERR_INVALID_PARAM;
    }

    /* 1. Trata erro: arquivo inexistente */
    uint32_t file_idx;
    minifs_inode_t inode;
    if (metadata_find_by_name(name, &file_idx, &inode) != MINIFS_SUCCESS) {
        return MINIFS_ERR_FILE_NOT_FOUND;
    }

    if (inode.size == 0) {
        if (out_len) *out_len = 0;
        ((char *)buffer)[0] = '\0';
        return MINIFS_SUCCESS;
    }

    uint32_t num_blocks = (inode.size + MINIFS_BLOCK_SIZE - 1) / MINIFS_BLOCK_SIZE;
    size_t to_read = inode.size;
    if (to_read > max_len) {
        to_read = max_len;
    }

    uint8_t *dst = (uint8_t *)buffer;
    size_t remaining = to_read;

    for (uint32_t i = 0; i < num_blocks && remaining > 0; i++) {
        uint8_t blk_buf[MINIFS_BLOCK_SIZE];
        if (disk_read_block(inode.blocks[i], blk_buf) != 0) {
            return MINIFS_ERR_IO;
        }

        size_t chunk = (remaining > MINIFS_BLOCK_SIZE) ? MINIFS_BLOCK_SIZE : remaining;
        memcpy(dst, blk_buf, chunk);
        dst += chunk;
        remaining -= chunk;
    }

    if (to_read < max_len) {
        ((char *)buffer)[to_read] = '\0';
    }

    if (out_len) {
        *out_len = to_read;
    }

    return MINIFS_SUCCESS;
}

/**
 * Operacao: ls
 * Lista todos os arquivos existentes com ID, Nome, Tamanho e contagem de blocos.
 */
int fs_ls(void) {
    if (!g_minifs.is_mounted) {
        return MINIFS_ERR_NOT_FORMATTED;
    }

    printf("ID\tTAMANHO\tBLOCOS\tNOME\n");
    printf("----------------------------------------\n");

    int found_count = 0;
    for (uint32_t i = 0; i < g_minifs.sb.total_inodes; i++) {
        minifs_inode_t node;
        if (metadata_read_inode(i, &node) == MINIFS_SUCCESS) {
            if (node.used) {
                uint32_t blks = (node.size == 0) ? 0 : (node.size + MINIFS_BLOCK_SIZE - 1) / MINIFS_BLOCK_SIZE;
                printf("%u\t%u B\t%u\t%s\n", node.id, node.size, blks, node.name);
                found_count++;
            }
        }
    }

    if (found_count == 0) {
        printf("(nenhum arquivo armazenado)\n");
    } else {
        printf("----------------------------------------\n");
        printf("Total: %d arquivo(s)\n", found_count);
    }

    return MINIFS_SUCCESS;
}

/**
 * Operacao: df
 * Exibe estatisticas detalhadas de utilizacao do disco virtual.
 */
int fs_df(void) {
    if (!g_minifs.is_mounted) {
        return MINIFS_ERR_NOT_FORMATTED;
    }

    uint32_t total_blks = g_minifs.sb.total_blocks;
    uint32_t data_blks = g_minifs.sb.data_blocks;
    uint32_t free_blks = g_minifs.sb.free_blocks_count;
    uint32_t used_blks = data_blks - free_blks;
    uint32_t meta_blks = g_minifs.sb.data_start_block;

    double pct_used = (data_blks > 0) ? ((double)used_blks * 100.0 / data_blks) : 0.0;

    printf("--- Uso do Disco Virtual (df) ---\n");
    printf("Tamanho do bloco       : %u bytes\n", g_minifs.sb.block_size);
    printf("Total de blocos no disco: %u (%u bytes)\n", total_blks, total_blks * MINIFS_BLOCK_SIZE);
    printf("Blocos de metadados    : %u (Superblock: 1, Bitmap: %u, Inodes: %u)\n",
           meta_blks, g_minifs.sb.bitmap_blocks, g_minifs.sb.inode_blocks);
    printf("Blocos de dados totais : %u\n", data_blks);
    printf("Blocos de dados usados : %u\n", used_blks);
    printf("Blocos de dados livres : %u\n", free_blks);
    printf("Percentual de dados em uso: %.2f%%\n", pct_used);
    printf("Inodes totais          : %u\n", g_minifs.sb.total_inodes);
    printf("Inodes em uso          : %u\n", g_minifs.sb.total_inodes - g_minifs.sb.free_inodes_count);
    printf("Inodes livres          : %u\n", g_minifs.sb.free_inodes_count);
    printf("----------------------------------\n");

    return MINIFS_SUCCESS;
}
