#ifndef MINIFS_BITMAP_H
#define MINIFS_BITMAP_H

#include "minifs.h"

/**
 * Responsabilidade: Carlos Alberto Magalhaes Neto (Atribuicao 1)
 * Gerenciamento do Bitmap de blocos.
 * Cada bit representa um bloco do disco:
 * 0 = livre, 1 = ocupado.
 */

int bitmap_init_empty(uint8_t *bitmap, uint32_t total_blocks, uint32_t data_start);
int bitmap_load_from_disk(void);
int bitmap_save_to_disk(void);
int bitmap_alloc_block(uint32_t *out_block);
int bitmap_free_block(uint32_t block_num);
bool bitmap_is_allocated(uint32_t block_num);

#endif /* MINIFS_BITMAP_H */
