#ifndef MINIFS_METADATA_H
#define MINIFS_METADATA_H

#include "minifs.h"

/**
 * Responsabilidade: Estudante 2
 * Metadados dos arquivos, referencias aos blocos e operacoes:
 * - create: criacao de arquivos nos metadados
 * - rm: remocao de arquivos e liberacao de blocos
 * - stat: exibicao dos metadados e blocos alocados
 */

/* Operacoes internas sobre a tabela de metadados */
int metadata_read_inode(uint32_t inode_idx, minifs_inode_t *inode);
int metadata_write_inode(uint32_t inode_idx, const minifs_inode_t *inode);
int metadata_find_by_name(const char *name, uint32_t *out_idx, minifs_inode_t *out_inode);
int metadata_find_free_slot(uint32_t *out_idx);

/* Operacoes de alto nivel exigidas para o Estudante 2 */
int fs_create(const char *name);
int fs_rm(const char *name);
int fs_stat(const char *name);

#endif /* MINIFS_METADATA_H */
