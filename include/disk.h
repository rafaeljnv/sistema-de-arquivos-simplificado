#ifndef MINIFS_DISK_H
#define MINIFS_DISK_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define MINIFS_BLOCK_SIZE 512
#define MINIFS_DEFAULT_DISK_FILE "minifs.bin"

/**
 * Responsabilidade: Carlos Alberto Magalhaes Neto (Atribuicao 1)
 * Camada de abstração do disco virtual (arquivo binário em blocos de tamanho fixo).
 */

int disk_init(const char *disk_path, uint32_t num_blocks);
int disk_open(const char *disk_path);
int disk_close(void);
int disk_read_block(uint32_t block_num, void *buffer);
int disk_write_block(uint32_t block_num, const void *buffer);
uint32_t disk_get_num_blocks(void);
bool disk_is_open(void);

#endif /* MINIFS_DISK_H */
