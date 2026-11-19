#include "cli.h"
#include "minifs.h"
#include "metadata.h"
#include "fs_ops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/**
 * Responsabilidade: Estudante 3
 * Parser e despachante de comandos da interface do MiniFS.
 */

static void print_help(void) {
    printf("=== MiniFS - Comandos Disponiveis ===\n");
    printf("  format [num_blocos]        - Formata o disco virtual (padrao: 2048)\n");
    printf("  create <nome>              - Cria um arquivo vazio\n");
    printf("  write <nome> \"<conteudo>\"  - Escreve conteudo no arquivo especificado\n");
    printf("  read <nome>                - Le e exibe o conteudo do arquivo\n");
    printf("  rm <nome>                  - Remove o arquivo e libera seus blocos\n");
    printf("  stat <nome>                - Exibe metadados e blocos alocados do arquivo\n");
    printf("  ls                         - Lista todos os arquivos existentes\n");
    printf("  df                         - Exibe o uso de blocos e inodes do disco\n");
    printf("  help                       - Exibe esta mensagem de ajuda\n");
    printf("  exit / quit                - Encerra o programa MiniFS\n");
    printf("=====================================\n");
}

/* Divide a linha em tokens respeitando aspas */
static int tokenize(char *line, char **argv, int max_args) {
    int argc = 0;
    char *p = line;

    while (*p && argc < max_args) {
        while (*p && isspace((unsigned char)*p)) {
            p++;
        }
        if (!*p) break;

        if (*p == '"') {
            p++;
            argv[argc++] = p;
            while (*p && *p != '"') {
                p++;
            }
            if (*p == '"') {
                *p = '\0';
                p++;
            }
        } else {
            argv[argc++] = p;
            while (*p && !isspace((unsigned char)*p)) {
                p++;
            }
            if (*p) {
                *p = '\0';
                p++;
            }
        }
    }
    return argc;
}

int cli_execute_line(char *line) {
    if (!line) return 0;

    /* Remove quebra de linha no final */
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        line[len - 1] = '\0';
        len--;
    }

    if (len == 0) return 0;

    char *argv[16];
    int argc = tokenize(line, argv, 16);
    if (argc == 0) return 0;

    char *cmd = argv[0];

    if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
        return 1; /* Sinaliza saida */
    }

    if (strcmp(cmd, "help") == 0) {
        print_help();
        return 0;
    }

    if (strcmp(cmd, "format") == 0) {
        uint32_t blocks = 2048;
        if (argc >= 2) {
            blocks = (uint32_t)atoi(argv[1]);
        }
        int res = fs_format(blocks);
        if (res == MINIFS_SUCCESS) {
            printf("[OK] Disco formatado com sucesso (%u blocos de %d bytes).\n", blocks, MINIFS_BLOCK_SIZE);
        } else {
            printf("[ERRO] %s\n", minifs_strerror(res));
        }
        return 0;
    }

    /* Os demais comandos exigem que o disco esteja montado */
    if (!g_minifs.is_mounted) {
        /* Tenta montagem automatica caso o arquivo ja exista */
        if (fs_mount(MINIFS_DEFAULT_DISK_FILE) != MINIFS_SUCCESS) {
            printf("[ERRO] Sistema nao formatado. Execute o comando 'format' primeiro.\n");
            return 0;
        }
    }

    if (strcmp(cmd, "create") == 0) {
        if (argc < 2) {
            printf("[ERRO] Uso: create <nome_arquivo>\n");
            return 0;
        }
        int res = fs_create(argv[1]);
        if (res == MINIFS_SUCCESS) {
            printf("[OK] Arquivo '%s' criado com sucesso.\n", argv[1]);
        } else {
            printf("[ERRO] %s\n", minifs_strerror(res));
        }
    } else if (strcmp(cmd, "write") == 0) {
        if (argc < 3) {
            printf("[ERRO] Uso: write <nome_arquivo> \"<conteudo>\"\n");
            return 0;
        }
        const char *filename = argv[1];
        const char *content = argv[2];
        size_t content_len = strlen(content);

        int res = fs_write(filename, content, content_len);
        if (res == MINIFS_SUCCESS) {
            printf("[OK] %zu bytes gravados com sucesso em '%s'.\n", content_len, filename);
        } else {
            printf("[ERRO] %s\n", minifs_strerror(res));
        }
    } else if (strcmp(cmd, "read") == 0) {
        if (argc < 2) {
            printf("[ERRO] Uso: read <nome_arquivo>\n");
            return 0;
        }
        char read_buf[MINIFS_MAX_FILE_SIZE + 1];
        size_t out_len = 0;
        int res = fs_read(argv[1], read_buf, MINIFS_MAX_FILE_SIZE, &out_len);
        if (res == MINIFS_SUCCESS) {
            read_buf[out_len] = '\0';
            printf("%s\n", read_buf);
        } else {
            printf("[ERRO] %s\n", minifs_strerror(res));
        }
    } else if (strcmp(cmd, "rm") == 0) {
        if (argc < 2) {
            printf("[ERRO] Uso: rm <nome_arquivo>\n");
            return 0;
        }
        int res = fs_rm(argv[1]);
        if (res == MINIFS_SUCCESS) {
            printf("[OK] Arquivo '%s' removido com sucesso.\n", argv[1]);
        } else {
            printf("[ERRO] %s\n", minifs_strerror(res));
        }
    } else if (strcmp(cmd, "stat") == 0) {
        if (argc < 2) {
            printf("[ERRO] Uso: stat <nome_arquivo>\n");
            return 0;
        }
        int res = fs_stat(argv[1]);
        if (res != MINIFS_SUCCESS) {
            printf("[ERRO] %s\n", minifs_strerror(res));
        }
    } else if (strcmp(cmd, "ls") == 0) {
        fs_ls();
    } else if (strcmp(cmd, "df") == 0) {
        fs_df();
    } else {
        printf("[ERRO] Comando desconhecido: '%s'. Digite 'help' para ajuda.\n", cmd);
    }

    return 0;
}

void cli_run_interactive(void) {
    char line_buffer[1024];

    printf("==================================================\n");
    printf("   MiniFS - Sistema de Arquivos Simplificado      \n");
    printf("   Referencia conceitual: Tanenbaum (SO)         \n");
    printf("   Digite 'help' para comandos ou 'exit' para sair\n");
    printf("==================================================\n\n");

    /* Tenta montar automaticamente se o disco ja existir */
    if (fs_mount(MINIFS_DEFAULT_DISK_FILE) == MINIFS_SUCCESS) {
        printf("[INFO] Disco virtual '%s' montado com sucesso.\n", MINIFS_DEFAULT_DISK_FILE);
    } else {
        printf("[INFO] Nenhum disco montado. Digite 'format 2048' para iniciar.\n");
    }

    while (1) {
        printf("minifs> ");
        fflush(stdout);

        if (!fgets(line_buffer, sizeof(line_buffer), stdin)) {
            break; /* EOF */
        }

        if (cli_execute_line(line_buffer)) {
            break;
        }
    }

    fs_unmount();
    printf("\nMiniFS finalizado. Dados persistidos no disco virtual.\n");
}

int cli_run_script(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Erro ao abrir script: %s\n", filename);
        return -1;
    }

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        if (cli_execute_line(line)) {
            break;
        }
    }

    fclose(f);
    fs_unmount();
    return 0;
}
