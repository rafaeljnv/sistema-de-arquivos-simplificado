#ifndef MINIFS_H
#define MINIFS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "disk.h"

#define MINIFS_MAGIC 0x4D494653 /* "MIFS" em ASCII */

#define MINIFS_MAX_FILENAME 32
#define MINIFS_DIRECT_BLOCKS 18
#define MINIFS_INODE_SIZE 128
#define MINIFS_INODES_PER_BLOCK (MINIFS_BLOCK_SIZE / MINIFS_INODE_SIZE) /* 512 / 128 = 4 */

/* Limite maximo de tamanho de arquivo suportado: 18 blocos * 512 = 9216 bytes */
#define MINIFS_MAX_FILE_SIZE (MINIFS_DIRECT_BLOCKS * MINIFS_BLOCK_SIZE)

/* Codigos de retorno e erros */
typedef enum {
    MINIFS_SUCCESS = 0,
    MINIFS_ERR_NOT_FORMATTED = -1,
    MINIFS_ERR_DISK_FULL = -2,
    MINIFS_ERR_FILE_NOT_FOUND = -3,
    MINIFS_ERR_FILE_EXISTS = -4,
    MINIFS_ERR_FILE_TOO_LARGE = -5,
    MINIFS_ERR_INODE_TABLE_FULL = -6,
    MINIFS_ERR_IO = -7,
    MINIFS_ERR_INVALID_PARAM = -8
} minifs_error_t;

/**
 * Superblock do MiniFS (armazenado no Bloco 0 do disco).
 * Ocupa exatamente 1 bloco (512 bytes).
 * Responsabilidade: Carlos Alberto Magalhaes Neto (Atribuicao 1)
 */
typedef struct {
    uint32_t magic;              /* Identificador do MiniFS (MINIFS_MAGIC) */
    uint32_t block_size;         /* Tamanho do bloco (512 bytes) */
    uint32_t total_blocks;       /* Quantidade total de blocos do disco */
    uint32_t bitmap_start_block; /* Bloco inicial do bitmap */
    uint32_t bitmap_blocks;      /* Quantidade de blocos do bitmap */
    uint32_t inode_start_block;  /* Bloco inicial da tabela de inodes */
    uint32_t inode_blocks;       /* Quantidade de blocos da tabela de inodes */
    uint32_t data_start_block;   /* Bloco inicial da area de dados */
    uint32_t data_blocks;        /* Quantidade de blocos de dados */
    uint32_t free_blocks_count;  /* Quantidade de blocos de dados livres */
    uint32_t total_inodes;       /* Quantidade maxima de arquivos suportados */
    uint32_t free_inodes_count;  /* Quantidade de inodes livres */
    uint32_t next_file_id;       /* Gerador incremental de IDs de arquivo */
    uint8_t  padding[460];       /* Preenchimento para alcancar exatamente 512 bytes */
} __attribute__((packed)) minifs_superblock_t;

/**
 * Estrutura de Metadados / Inode do arquivo (Alocacao indexada simplificada).
 * Cada arquivo possui 1 inode com referencias diretas aos blocos de dados alocados.
 * Tamanho: exatamente 128 bytes.
 * Responsabilidade: Rafael Jeronimo Nunes Vasconcelos (Atribuicao 2)
 */
typedef struct {
    uint32_t id;                          /* Identificador numerico unico */
    uint8_t  used;                        /* 1 = arquivo ativo; 0 = entrada livre */
    char     name[MINIFS_MAX_FILENAME];   /* Nome do arquivo (null-terminated) */
    uint32_t size;                        /* Tamanho exato em bytes */
    uint32_t blocks[MINIFS_DIRECT_BLOCKS];/* Indices dos blocos de dados ocupados */
    uint8_t  padding[15];                 /* Preenchimento para alcancar 128 bytes */
} __attribute__((packed)) minifs_inode_t;

/* Estrutura de estado em memoria do sistema de arquivos montado */
typedef struct {
    bool is_mounted;
    minifs_superblock_t sb;
    uint8_t *bitmap; /* Cache em memoria do bitmap de blocos */
} minifs_fs_t;

extern minifs_fs_t g_minifs;

/* Constantes e funcoes utilitarias de conversao de erro */
const char *minifs_strerror(minifs_error_t err);

#endif /* MINIFS_H */
