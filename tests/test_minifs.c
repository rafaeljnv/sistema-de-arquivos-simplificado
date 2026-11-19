#include "minifs.h"
#include "metadata.h"
#include "fs_ops.h"
#include "bitmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/**
 * Responsabilidade: Rafael Jeronimo Nunes Vasconcelos (Atribuicao 3)
 * Bateria de Testes Automatizados e Integracao do MiniFS.
 *
 * Valida:
 * 1. Formatacao e inicializacao das estruturas internas.
 * 2. Operacoes create, write e read (integridade de dados de 1 e multiplos blocos).
 * 3. Operacoes ls e stat (verificacao da alocacao indexada de blocos).
 * 4. Persistencia: fechar o disco, desmontar, remontar e conferir dados.
 * 5. Remocao e REUTILIZACAO DE BLOCOS liberados pelo bitmap.
 * 6. Tratamento de todos os erros obrigatorios:
 *    - Arquivo inexistente
 *    - Nome duplicado
 *    - Arquivo acima do limite suportado
 *    - Disco cheio
 */

#define TEST_PASS() printf("  --> [PASSOU]\n\n")

void test_format(void) {
    printf("[TESTE 1] Formatacao inicial do disco virtual (2048 blocos)...\n");
    int res = fs_format(2048);
    assert(res == MINIFS_SUCCESS);
    assert(g_minifs.is_mounted == true);
    assert(g_minifs.sb.magic == MINIFS_MAGIC);
    assert(g_minifs.sb.total_blocks == 2048);
    assert(g_minifs.sb.free_inodes_count == g_minifs.sb.total_inodes);
    assert(g_minifs.sb.free_blocks_count == g_minifs.sb.data_blocks);
    printf("  Superblock verificado: %u blocos totais, %u blocos de dados livres.\n",
           g_minifs.sb.total_blocks, g_minifs.sb.free_blocks_count);
    TEST_PASS();
}

void test_create_write_read(void) {
    printf("[TESTE 2] Criacao, escrita e leitura com integridade de dados...\n");

    /* Cria arquivo */
    int res = fs_create("doc.txt");
    assert(res == MINIFS_SUCCESS);

    /* Escreve texto pequeno (< 512 bytes, 1 bloco) */
    const char *msg = "Sistemas Operacionais - Teste de arquivo no MiniFS!";
    res = fs_write("doc.txt", msg, strlen(msg));
    assert(res == MINIFS_SUCCESS);

    /* Le de volta e compara */
    char buffer[512];
    size_t out_len = 0;
    res = fs_read("doc.txt", buffer, sizeof(buffer), &out_len);
    assert(res == MINIFS_SUCCESS);
    assert(out_len == strlen(msg));
    assert(strcmp(buffer, msg) == 0);
    printf("  Arquivo 'doc.txt' lido com sucesso: \"%s\"\n", buffer);

    /* Escreve arquivo maior (1200 bytes, ocupando exatamente 3 blocos) */
    res = fs_create("grande.dat");
    assert(res == MINIFS_SUCCESS);

    char large_data[1200];
    for (int i = 0; i < 1200; i++) {
        large_data[i] = (char)('A' + (i % 26));
    }

    res = fs_write("grande.dat", large_data, 1200);
    assert(res == MINIFS_SUCCESS);

    char read_large[1200];
    res = fs_read("grande.dat", read_large, 1200, &out_len);
    assert(res == MINIFS_SUCCESS);
    assert(out_len == 1200);
    assert(memcmp(large_data, read_large, 1200) == 0);
    printf("  Arquivo 'grande.dat' (1200 bytes, 3 blocos) lido e validado byte a byte.\n");

    TEST_PASS();
}

void test_stat_and_ls(void) {
    printf("[TESTE 3] Listagem (ls) e Inspecao de Metadados/Indices (stat)...\n");
    fs_ls();
    printf("\nInspecionando metadados de 'grande.dat':\n");
    int res = fs_stat("grande.dat");
    assert(res == MINIFS_SUCCESS);
    TEST_PASS();
}

void test_persistence(void) {
    printf("[TESTE 4] Persistencia: Encerrando execucao e reabrindo o disco...\n");

    /* Desmonta o sistema e fecha o arquivo */
    fs_unmount();
    assert(g_minifs.is_mounted == false);
    printf("  MiniFS desmontado e arquivo de disco fechado.\n");

    /* Remonta o sistema como se fosse um novo processo */
    int res = fs_mount(MINIFS_DEFAULT_DISK_FILE);
    assert(res == MINIFS_SUCCESS);
    assert(g_minifs.is_mounted == true);
    printf("  MiniFS remontado com sucesso a partir de '%s'.\n", MINIFS_DEFAULT_DISK_FILE);

    /* Verifica se os arquivos e seus conteudos persistem */
    char buffer[512];
    size_t out_len = 0;
    const char *expected = "Sistemas Operacionais - Teste de arquivo no MiniFS!";
    res = fs_read("doc.txt", buffer, sizeof(buffer), &out_len);
    assert(res == MINIFS_SUCCESS);
    assert(strcmp(buffer, expected) == 0);

    /* Verifica 'grande.dat' */
    minifs_inode_t node;
    res = metadata_find_by_name("grande.dat", NULL, &node);
    assert(res == MINIFS_SUCCESS);
    assert(node.size == 1200);

    printf("  Validado: Dados e metadados persistiram perfeitamente apos encerramento!\n");
    TEST_PASS();
}

void test_block_reuse(void) {
    printf("[TESTE 5] Remocao (rm) e Comprovacao de Reutilizacao de Blocos Liberados...\n");

    /* 1. Cria um arquivo 'alvo.txt' e grava 1024 bytes (2 blocos) */
    fs_create("alvo.txt");
    char dummy[1024];
    memset(dummy, 'X', 1024);
    fs_write("alvo.txt", dummy, 1024);

    minifs_inode_t alvo_node;
    metadata_find_by_name("alvo.txt", NULL, &alvo_node);
    uint32_t b1 = alvo_node.blocks[0];
    uint32_t b2 = alvo_node.blocks[1];
    printf("  'alvo.txt' alocado nos blocos de dados: [%u, %u]\n", b1, b2);

    /* 2. Remove o arquivo 'alvo.txt' */
    int res = fs_rm("alvo.txt");
    assert(res == MINIFS_SUCCESS);
    printf("  'alvo.txt' removido com sucesso via fs_rm.\n");

    /* Verifica se os blocos foram marcados como livres no bitmap */
    assert(bitmap_is_allocated(b1) == false);
    assert(bitmap_is_allocated(b2) == false);
    printf("  Bitmap confirmado: blocos %u e %u estao LIVRES no bitmap.\n", b1, b2);

    /* 3. Cria um novo arquivo 'novo.txt' e aloca 2 blocos */
    fs_create("novo.txt");
    fs_write("novo.txt", dummy, 1024);

    minifs_inode_t novo_node;
    metadata_find_by_name("novo.txt", NULL, &novo_node);
    printf("  'novo.txt' alocado nos blocos de dados: [%u, %u]\n",
           novo_node.blocks[0], novo_node.blocks[1]);

    /* Comprova que os blocos reutilizados sao exatamente b1 e b2 */
    assert(novo_node.blocks[0] == b1);
    assert(novo_node.blocks[1] == b2);
    printf("  PROVA CONCLUIDA: O novo arquivo reutilizou exatamente os blocos liberados [%u, %u]!\n", b1, b2);

    TEST_PASS();
}

void test_error_handling(void) {
    printf("[TESTE 6] Tratamento de Erros Obrigatorios...\n");

    /* 1. Arquivo inexistente */
    printf("  1. Testando leitura de arquivo inexistente:\n");
    char buf[128];
    int res = fs_read("inexistente.txt", buf, sizeof(buf), NULL);
    assert(res == MINIFS_ERR_FILE_NOT_FOUND);
    printf("     Retorno: %s (%d) [CORRETO]\n", minifs_strerror(res), res);

    /* 2. Nome duplicado */
    printf("  2. Testando criacao com nome duplicado:\n");
    res = fs_create("doc.txt"); /* 'doc.txt' ja existe */
    assert(res == MINIFS_ERR_FILE_EXISTS);
    printf("     Retorno: %s (%d) [CORRETO]\n", minifs_strerror(res), res);

    /* 3. Arquivo acima do limite suportado (> 18 blocos * 512 = 9216 bytes) */
    printf("  3. Testando arquivo acima do limite suportado (10000 bytes > 9216 max):\n");
    char *too_large = (char *)malloc(10000);
    res = fs_write("doc.txt", too_large, 10000);
    assert(res == MINIFS_ERR_FILE_TOO_LARGE);
    printf("     Retorno: %s (%d) [CORRETO]\n", minifs_strerror(res), res);
    free(too_large);

    /* 4. Disco cheio */
    printf("  4. Testando condicao de disco cheio:\n");
    /* Formata um disco com 10 blocos:
       Superblock (1), Bitmap (1), Inodes (2) = 4 blocos de metadados.
       Restam exatamente 6 blocos de dados (6 * 512 = 3072 bytes). */
    res = fs_format(10);
    assert(res == MINIFS_SUCCESS);
    assert(g_minifs.sb.free_blocks_count == 6);
    printf("     Disco formatado com 10 blocos. Blocos de dados disponiveis: %u\n",
           g_minifs.sb.free_blocks_count);

    /* Cria arquivo 1 preenchendo todos os 6 blocos disponiveis (3072 bytes) */
    res = fs_create("cheio1.txt");
    assert(res == MINIFS_SUCCESS);
    char buf6[3072];
    memset(buf6, 'A', 3072);
    res = fs_write("cheio1.txt", buf6, 3072);
    assert(res == MINIFS_SUCCESS);
    assert(g_minifs.sb.free_blocks_count == 0);
    printf("     'cheio1.txt' gravado ocupando todos os 6 blocos de dados livres (disco 100%% cheio).\n");

    /* Tenta gravar em um segundo arquivo sem blocos de dados livres */
    res = fs_create("cheio2.txt");
    assert(res == MINIFS_SUCCESS);
    res = fs_write("cheio2.txt", "dados extras", 12);
    assert(res == MINIFS_ERR_DISK_FULL);
    printf("     Retorno ao tentar escrever sem blocos disponiveis: %s (%d) [CORRETO]\n",
           minifs_strerror(res), res);

    TEST_PASS();
}

int main(void) {
    printf("====================================================\n");
    printf("   MiniFS - BATERIA DE TESTES DE INTEGRACAO         \n");
    printf("   Carlos Alberto Magalhaes Neto (Disco / Bitmap / E/S)\n");
    printf("   Rafael Jeronimo Nunes Vasconcelos (Metadados / CLI)\n");
    printf("====================================================\n\n");

    test_format();
    test_create_write_read();
    test_stat_and_ls();
    test_persistence();
    test_block_reuse();
    test_error_handling();

    fs_unmount();

    printf("====================================================\n");
    printf("   TODOS OS TESTES PASSARAM COM SUCESSO (100%%)!     \n");
    printf("====================================================\n");
    return 0;
}
