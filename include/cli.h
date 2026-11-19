#ifndef MINIFS_CLI_H
#define MINIFS_CLI_H

/**
 * Responsabilidade: Estudante 3
 * Interface de linha de comando (CLI) do MiniFS.
 */

int cli_execute_line(char *line);
void cli_run_interactive(void);
int cli_run_script(const char *filename);

#endif /* MINIFS_CLI_H */
