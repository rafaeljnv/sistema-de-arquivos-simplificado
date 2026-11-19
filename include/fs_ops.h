#ifndef MINIFS_FS_OPS_H
#define MINIFS_FS_OPS_H

#include "minifs.h"

/**
 * Responsabilidade: Estudante 3
 * Interface de comandos e operacoes:
 * - format: formatacao do disco virtual
 * - write: escrita de dados em blocos indexados
 * - read: leitura de dados a partir dos blocos indexados
 * - ls: listagem de arquivos
 * - df: relatorio de espaco em disco
 * - mount / unmount: montagem e persistencia
 */

int fs_mount(const char *disk_path);
int fs_unmount(void);
int fs_format(uint32_t total_blocks);

int fs_write(const char *name, const void *data, size_t length);
int fs_read(const char *name, void *buffer, size_t max_len, size_t *out_len);
int fs_ls(void);
int fs_df(void);

#endif /* MINIFS_FS_OPS_H */
