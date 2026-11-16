/*
 * teste_hematoma.c — testes da LISTA ORDENADA + INSERTION SORT (hematoma.c).
 *
 * Roda no terminal, sem raylib e sem ODE. hematoma.c está pronto quando
 * todos os testes passam (saída "TODOS OS TESTES PASSARAM" e código 0).
 *
 * NÃO altere este arquivo para fazer um teste passar: corrija hematoma.c.
 * Se achar que um teste está errado, fale com o grupo antes.
 */
#include <stdio.h>
#include <math.h>
#include "hematoma.h"
#include "config.h"

static int verificacoes = 0;
static int falhas = 0;

#define VERIFICA(cond)                                                       \
    do {                                                                     \
        verificacoes++;                                                      \
        if (!(cond)) {                                                       \
            falhas++;                                                        \
            printf("    FALHOU (linha %d): %s\n", __LINE__, #cond);          \
        }                                                                    \
    } while (0)

#define TESTE(nome) printf("- %s\n", nome)

static int quase_igual(float a, float b)
{
    return fabsf(a - b) < 0.001f;
}

/* Confere invariantes: ordenada, quantidade e soma batem com os nós. */
static int lista_consistente(const ListaHematomas *l)
{
    int n = 0;
    float soma = 0.0f;
    for (const NoHematoma *x = l->inicio; x != NULL; x = x->prox) {
        if (x->prox != NULL && x->momento_cura > x->prox->momento_cura) return 0;
        soma += x->dano;
        n++;
        if (n > HEMATOMAS_MAX) return 0;
    }
    return n == l->quantidade && quase_igual(soma, l->dano_total);
}

/* Confere a ordem dos momentos de cura. */
static int momentos_iguais(const ListaHematomas *l, const double *esperado, int n)
{
    const NoHematoma *x = l->inicio;
    for (int i = 0; i < n; i++) {
        if (x == NULL || x->momento_cura != esperado[i]) return 0;
        x = x->prox;
    }
    return x == NULL;
}

/* Confere o ponteiro fim: NULL com a lista vazia; senão, aponta para o
 * último nó (o que se acha andando por ->prox) e fim->prox == NULL. */
static int fim_correto(const ListaHematomas *l)
{
    if (l->inicio == NULL)
        return l->fim == NULL;
    const NoHematoma *ultimo = l->inicio;
    while (ultimo->prox != NULL)
        ultimo = ultimo->prox;
    return l->fim == ultimo && l->fim->prox == NULL;
}

static void teste_iniciar_vazia(void)
{
    TESTE("lista nova fica vazia");
    ListaHematomas l;
    hematomas_iniciar(&l);
    VERIFICA(l.inicio == NULL);
    VERIFICA(l.quantidade == 0);
    VERIFICA(l.dano_total == 0.0f);
    VERIFICA(hematomas_dano_total(&l) == 0.0f);
}

static void teste_insere_fora_de_ordem(void)
{
    TESTE("inserir fora de ordem (8, 3, 5) fica ordenado (3, 5, 8)");
    ListaHematomas l;
    hematomas_iniciar(&l);
    VERIFICA(hematomas_inserir(&l, 10.0f, 8.0));
    VERIFICA(hematomas_inserir(&l, 5.0f, 3.0));
    VERIFICA(hematomas_inserir(&l, 7.0f, 5.0));
    double esperado[3] = { 3.0, 5.0, 8.0 };
    VERIFICA(momentos_iguais(&l, esperado, 3));
    VERIFICA(l.quantidade == 3);
    VERIFICA(quase_igual(hematomas_dano_total(&l), 22.0f));
    VERIFICA(lista_consistente(&l));
    hematomas_liberar(&l);
}

static void teste_insere_inicio_meio_fim(void)
{
    TESTE("inserir no inicio, no meio e no fim");
    ListaHematomas l;
    hematomas_iniciar(&l);
    hematomas_inserir(&l, 1.0f, 5.0);
    hematomas_inserir(&l, 1.0f, 1.0);   /* inicio */
    hematomas_inserir(&l, 1.0f, 9.0);   /* fim    */
    hematomas_inserir(&l, 1.0f, 7.0);   /* meio   */
    hematomas_inserir(&l, 1.0f, 2.0);   /* meio   */
    double esperado[5] = { 1.0, 2.0, 5.0, 7.0, 9.0 };
    VERIFICA(momentos_iguais(&l, esperado, 5));
    VERIFICA(lista_consistente(&l));
    hematomas_liberar(&l);
}

static void teste_estavel(void)
{
    TESTE("momentos iguais mantem a ordem de chegada (estavel)");
    ListaHematomas l;
    hematomas_iniciar(&l);
    hematomas_inserir(&l, 1.0f, 4.0);
    hematomas_inserir(&l, 2.0f, 4.0);
    hematomas_inserir(&l, 3.0f, 4.0);
    hematomas_inserir(&l, 9.0f, 1.0);
    VERIFICA(l.inicio != NULL && l.inicio->dano == 9.0f);
    const NoHematoma *x = l.inicio ? l.inicio->prox : NULL;
    VERIFICA(x != NULL && x->dano == 1.0f);
    x = x ? x->prox : NULL;
    VERIFICA(x != NULL && x->dano == 2.0f);
    x = x ? x->prox : NULL;
    VERIFICA(x != NULL && x->dano == 3.0f);
    VERIFICA(lista_consistente(&l));
    hematomas_liberar(&l);
}

static void teste_dano_invalido(void)
{
    TESTE("dano zero ou negativo nao e inserido");
    ListaHematomas l;
    hematomas_iniciar(&l);
    VERIFICA(!hematomas_inserir(&l, 0.0f, 1.0));
    VERIFICA(!hematomas_inserir(&l, -5.0f, 1.0));
    VERIFICA(l.quantidade == 0);
    VERIFICA(l.inicio == NULL);
    VERIFICA(lista_consistente(&l));
}

static void teste_curar_no_tempo_certo(void)
{
    TESTE("curar remove so os que ja venceram (momento <= agora)");
    ListaHematomas l;
    hematomas_iniciar(&l);
    hematomas_inserir(&l, 5.0f, 3.0);
    hematomas_inserir(&l, 7.0f, 5.0);
    hematomas_inserir(&l, 10.0f, 8.0);

    VERIFICA(hematomas_curar(&l, 2.9) == 0);
    VERIFICA(l.quantidade == 3);

    VERIFICA(hematomas_curar(&l, 4.0) == 1);          /* sai o de 3 s */
    VERIFICA(l.quantidade == 2);
    VERIFICA(quase_igual(hematomas_dano_total(&l), 17.0f));

    VERIFICA(hematomas_curar(&l, 5.0) == 1);          /* exatamente 5: sai */
    VERIFICA(quase_igual(hematomas_dano_total(&l), 10.0f));
    VERIFICA(lista_consistente(&l));
    hematomas_liberar(&l);
}

static void teste_curar_todos(void)
{
    TESTE("curar todos deixa a lista vazia com dano exatamente 0");
    ListaHematomas l;
    hematomas_iniciar(&l);
    hematomas_inserir(&l, 0.1f, 1.0);
    hematomas_inserir(&l, 0.2f, 2.0);
    hematomas_inserir(&l, 0.3f, 3.0);
    VERIFICA(hematomas_curar(&l, 100.0) == 3);
    VERIFICA(l.inicio == NULL);
    VERIFICA(l.quantidade == 0);
    VERIFICA(hematomas_dano_total(&l) == 0.0f);       /* sem resto de arredondamento */
}

static void teste_curar_vazia(void)
{
    TESTE("curar lista vazia retorna 0");
    ListaHematomas l;
    hematomas_iniciar(&l);
    VERIFICA(hematomas_curar(&l, 10.0) == 0);
    VERIFICA(lista_consistente(&l));
}

static void teste_limite(void)
{
    TESTE("nao passa de HEMATOMAS_MAX");
    ListaHematomas l;
    hematomas_iniciar(&l);
    for (int i = 0; i < HEMATOMAS_MAX; i++)
        VERIFICA(hematomas_inserir(&l, 1.0f, (double)(HEMATOMAS_MAX - i)));
    VERIFICA(!hematomas_inserir(&l, 1.0f, 0.5));
    VERIFICA(l.quantidade == HEMATOMAS_MAX);
    VERIFICA(quase_igual(hematomas_dano_total(&l), (float)HEMATOMAS_MAX));
    VERIFICA(lista_consistente(&l));
    hematomas_liberar(&l);
}

static void teste_sequencia_de_luta(void)
{
    TESTE("sequencia de luta: insere e cura intercalado");
    ListaHematomas l;
    hematomas_iniciar(&l);
    hematomas_inserir(&l, 15.0f, 0.0 + 8.0);   /* barrigada em t=0, cura em 8  */
    hematomas_inserir(&l, 5.0f, 1.0 + 3.0);    /* empurrao  em t=1, cura em 4  */
    VERIFICA(l.inicio != NULL && l.inicio->momento_cura == 4.0); /* o novo passou na frente */
    hematomas_curar(&l, 4.5);
    VERIFICA(quase_igual(hematomas_dano_total(&l), 15.0f));
    hematomas_inserir(&l, 5.0f, 5.0 + 3.0);    /* empurrao  em t=5, cura em 8 (empate) */
    VERIFICA(l.quantidade == 2);
    VERIFICA(l.inicio != NULL && l.inicio->dano == 15.0f);       /* estavel no empate */
    VERIFICA(hematomas_curar(&l, 8.0) == 2);
    VERIFICA(hematomas_dano_total(&l) == 0.0f);
    VERIFICA(lista_consistente(&l));
}

static void teste_liberar_e_reusar(void)
{
    TESTE("liberar deixa vazia, pode liberar de novo e reusar");
    ListaHematomas l;
    hematomas_iniciar(&l);
    hematomas_inserir(&l, 3.0f, 1.0);
    hematomas_inserir(&l, 4.0f, 2.0);
    hematomas_liberar(&l);
    VERIFICA(l.inicio == NULL);
    VERIFICA(l.quantidade == 0);
    VERIFICA(l.dano_total == 0.0f);
    hematomas_liberar(&l);
    VERIFICA(hematomas_inserir(&l, 2.0f, 1.0));
    VERIFICA(lista_consistente(&l));
    hematomas_liberar(&l);
}

static void teste_fim_crescente(void)
{
    TESTE("fim: inserir em ordem crescente (caso comum) liga sempre no fim");
    ListaHematomas l;
    hematomas_iniciar(&l);
    VERIFICA(fim_correto(&l));                        /* vazia: fim == NULL */
    for (int i = 1; i <= 5; i++) {
        VERIFICA(hematomas_inserir(&l, 1.0f, (double)i));
        VERIFICA(fim_correto(&l));
        VERIFICA(l.fim != NULL && l.fim->momento_cura == (double)i); /* o novo é o último */
    }
    double esperado[5] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    VERIFICA(momentos_iguais(&l, esperado, 5));
    VERIFICA(lista_consistente(&l));
    hematomas_liberar(&l);
}

static void teste_fim_decrescente(void)
{
    TESTE("fim: inserir em ordem decrescente nao muda o fim");
    ListaHematomas l;
    hematomas_iniciar(&l);
    for (int i = 5; i >= 1; i--) {
        VERIFICA(hematomas_inserir(&l, 1.0f, (double)i));
        VERIFICA(fim_correto(&l));
        VERIFICA(l.fim != NULL && l.fim->momento_cura == 5.0); /* o primeiro continua no fim */
    }
    double esperado[5] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    VERIFICA(momentos_iguais(&l, esperado, 5));
    VERIFICA(lista_consistente(&l));
    hematomas_liberar(&l);
}

static void teste_fim_iguais(void)
{
    TESTE("fim: valores iguais vao para o fim, na ordem de chegada (estavel)");
    ListaHematomas l;
    hematomas_iniciar(&l);
    for (int i = 1; i <= 3; i++) {
        VERIFICA(hematomas_inserir(&l, (float)i, 4.0));
        VERIFICA(fim_correto(&l));
        VERIFICA(l.fim != NULL && l.fim->dano == (float)i);    /* o mais novo é o último */
    }
    const NoHematoma *x = l.inicio;
    for (int i = 1; i <= 3; i++) {
        VERIFICA(x != NULL && x->dano == (float)i);
        x = x ? x->prox : NULL;
    }
    VERIFICA(lista_consistente(&l));
    hematomas_liberar(&l);
}

static void teste_fim_meio(void)
{
    TESTE("fim: inserir no inicio e no meio mantem o fim");
    ListaHematomas l;
    hematomas_iniciar(&l);
    hematomas_inserir(&l, 1.0f, 5.0);
    hematomas_inserir(&l, 1.0f, 9.0);                 /* fim */
    hematomas_inserir(&l, 1.0f, 1.0);                 /* inicio */
    VERIFICA(fim_correto(&l));
    hematomas_inserir(&l, 1.0f, 7.0);                 /* meio */
    VERIFICA(fim_correto(&l));
    VERIFICA(l.fim != NULL && l.fim->momento_cura == 9.0);
    VERIFICA(lista_consistente(&l));
    hematomas_liberar(&l);
}

static void teste_fim_curar_e_liberar(void)
{
    TESTE("fim: curar e liberar mantem o fim certo e a lista reutilizavel");
    ListaHematomas l;
    hematomas_iniciar(&l);
    hematomas_inserir(&l, 5.0f, 3.0);
    hematomas_inserir(&l, 7.0f, 5.0);
    hematomas_inserir(&l, 10.0f, 8.0);

    VERIFICA(hematomas_curar(&l, 4.0) == 1);          /* sai do inicio, fim igual */
    VERIFICA(fim_correto(&l));
    VERIFICA(l.fim != NULL && l.fim->momento_cura == 8.0);

    VERIFICA(hematomas_curar(&l, 7.0) == 1);          /* sobra 1: inicio == fim */
    VERIFICA(fim_correto(&l));
    VERIFICA(l.inicio == l.fim);

    VERIFICA(hematomas_curar(&l, 8.0) == 1);          /* esvaziou: fim == NULL */
    VERIFICA(l.fim == NULL);
    VERIFICA(fim_correto(&l));

    VERIFICA(hematomas_inserir(&l, 2.0f, 10.0));      /* reusa depois de esvaziar */
    VERIFICA(fim_correto(&l));
    VERIFICA(l.inicio == l.fim);

    hematomas_liberar(&l);
    VERIFICA(l.fim == NULL);
    VERIFICA(fim_correto(&l));
}

int main(void)
{
    printf("Testes dos hematomas (lista ordenada + insertion sort)\n");

    teste_iniciar_vazia();
    teste_insere_fora_de_ordem();
    teste_insere_inicio_meio_fim();
    teste_estavel();
    teste_dano_invalido();
    teste_curar_no_tempo_certo();
    teste_curar_todos();
    teste_curar_vazia();
    teste_limite();
    teste_sequencia_de_luta();
    teste_liberar_e_reusar();
    teste_fim_crescente();
    teste_fim_decrescente();
    teste_fim_iguais();
    teste_fim_meio();
    teste_fim_curar_e_liberar();

    printf("\n%d verificacoes, %d falhas\n", verificacoes, falhas);
    if (falhas == 0) {
        printf("TODOS OS TESTES PASSARAM\n");
        return 0;
    }
    return 1;
}
