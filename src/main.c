#include "cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Responsabilidade: Rafael Jeronimo Nunes Vasconcelos (Atribuicao 3)
 * Ponto de entrada do executavel do MiniFS.
 */

int main(int argc, char *argv[]) {
    if (argc > 1) {
        /* Se for arquivo de script existente, executa */
        FILE *f = fopen(argv[1], "r");
        if (f) {
            fclose(f);
            return cli_run_script(argv[1]);
        }

        /* Caso contrario, interpreta a string passada */
        char *cmd_str = strdup(argv[1]);
        char *line = strtok(cmd_str, "\n");
        while (line) {
            if (cli_execute_line(line)) {
                break;
            }
            line = strtok(NULL, "\n");
        }
        free(cmd_str);
        return 0;
    }

    /* Modo interativo ou stdin */
    cli_run_interactive();
    return 0;
}
