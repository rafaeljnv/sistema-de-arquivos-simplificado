# MiniFS

Trabalho pratico da disciplina de Sistemas Operacionais (referencia conceitual: Tanenbaum).

Implementacao de um sistema de arquivos simplificado sobre disco virtual simulado em arquivo binario (`minifs.bin`), com blocos de tamanho fixo de 512 bytes e alocacao indexada simplificada.

## Estrutura do Projeto

- `include/`: Arquivos de cabecalho (.h)
  - `minifs.h`: Estruturas do superblock, inode e constantes
  - `disk.h`: Abstracao de blocos fisicos
  - `bitmap.h`: Gerenciamento do bitmap de blocos livres
  - `metadata.h`: Metadados dos arquivos e operacoes create, rm e stat
  - `fs_ops.h`: Operacoes read, write, ls, df e format
  - `cli.h`: Interface de linha de comando
- `src/`: Codigo-fonte (.c)
  - `disk.c`: Leitura e escrita de blocos de 512 bytes no disco virtual
  - `bitmap.c`: Alocacao e liberacao de blocos no bitmap (First Fit)
  - `metadata.c`: Controle de metadados, criacao, remocao e inspecao de arquivos
  - `fs_ops.c`: Formatacao, leitura, escrita indexada, listagem e persistencia
  - `cli.c`: Interpretador de comandos
  - `main.c`: Ponto de entrada do programa
- `tests/test_minifs.c`: Bateria de testes automatizados

## Compilacao

### Windows (GCC / MinGW):
```cmd
build.bat
```
Para compilar e rodar a suite de testes:
```cmd
build.bat test
```

### Linux / GCC:
```bash
make
./test_minifs
```

## Execucao

Para abrir o terminal interativo:
```bash
./minifs
```

Tambem e possivel passar comandos diretamente por script ou stdin.

## Comandos Suportados

- `format [n_blocos]`: Formata o disco virtual (padrao: 2048 blocos de 512 bytes)
- `create <arquivo>`: Cria arquivo vazio
- `write <arquivo> "<conteudo>"`: Escreve dados no arquivo
- `read <arquivo>`: Le e exibe o conteudo do arquivo
- `stat <arquivo>`: Exibe metadados e os blocos alocados
- `rm <arquivo>`: Remove o arquivo e libera seus blocos
- `ls`: Lista arquivos cadastrados
- `df`: Exibe informacoes de uso e espaco livre do disco
- `help`: Lista comandos disponiveis
- `exit`: Encerra o programa

## Testes

O executavel `test_minifs` valida:
1. Formatacao inicial e consistencia do superblock
2. Criacao, escrita e leitura de dados
3. Listagem (`ls`) e metadados (`stat`)
4. Persistencia dos arquivos apos fechar e reabrir o sistema
5. Remocao e reutilizacao dos blocos liberados no bitmap
6. Tratamento de erros (arquivo inexistente, nome duplicado, limite maximo de tamanho excedido e disco cheio)
