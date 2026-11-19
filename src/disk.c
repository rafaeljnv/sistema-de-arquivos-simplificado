#include "disk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Responsabilidade: Estudante 1
 * Implementacao da camada fisica do disco virtual simulado em arquivo binario.
 */

static FILE *s_disk_file = NULL;
static uint32_t s_num_blocks = 0;

int disk_init(const char *disk_path, uint32_t num_blocks) {
    if (s_disk_file) {
        disk_close();
    }

    s_disk_file = fopen(disk_path, "wb+");
    if (!s_disk_file) {
        return -1;
    }

    uint8_t zero_block[MINIFS_BLOCK_SIZE];
    memset(zero_block, 0, sizeof(zero_block));

    for (uint32_t i = 0; i < num_blocks; i++) {
        if (fwrite(zero_block, 1, MINIFS_BLOCK_SIZE, s_disk_file) != MINIFS_BLOCK_SIZE) {
            fclose(s_disk_file);
            s_disk_file = NULL;
            return -1;
        }
    }

    fflush(s_disk_file);
    s_num_blocks = num_blocks;
    return 0;
}

int disk_open(const char *disk_path) {
    if (s_disk_file) {
        disk_close();
    }

    s_disk_file = fopen(disk_path, "rb+");
    if (!s_disk_file) {
        return -1;
    }

    if (fseek(s_disk_file, 0, SEEK_END) != 0) {
        fclose(s_disk_file);
        s_disk_file = NULL;
        return -1;
    }

    long size = ftell(s_disk_file);
    if (size < (long)MINIFS_BLOCK_SIZE) {
        fclose(s_disk_file);
        s_disk_file = NULL;
        return -1;
    }

    s_num_blocks = (uint32_t)(size / MINIFS_BLOCK_SIZE);
    fseek(s_disk_file, 0, SEEK_SET);
    return 0;
}

int disk_close(void) {
    if (s_disk_file) {
        fflush(s_disk_file);
        fclose(s_disk_file);
        s_disk_file = NULL;
        s_num_blocks = 0;
    }
    return 0;
}

int disk_read_block(uint32_t block_num, void *buffer) {
    if (!s_disk_file || block_num >= s_num_blocks || !buffer) {
        return -1;
    }

    if (fseek(s_disk_file, (long)block_num * MINIFS_BLOCK_SIZE, SEEK_SET) != 0) {
        return -1;
    }

    if (fread(buffer, 1, MINIFS_BLOCK_SIZE, s_disk_file) != MINIFS_BLOCK_SIZE) {
        return -1;
    }

    return 0;
}

int disk_write_block(uint32_t block_num, const void *buffer) {
    if (!s_disk_file || block_num >= s_num_blocks || !buffer) {
        return -1;
    }

    if (fseek(s_disk_file, (long)block_num * MINIFS_BLOCK_SIZE, SEEK_SET) != 0) {
        return -1;
    }

    if (fwrite(buffer, 1, MINIFS_BLOCK_SIZE, s_disk_file) != MINIFS_BLOCK_SIZE) {
        return -1;
    }

    fflush(s_disk_file);
    return 0;
}

uint32_t disk_get_num_blocks(void) {
    return s_num_blocks;
}

bool disk_is_open(void) {
    return (s_disk_file != NULL);
}
