/*
 * hematoma.c — LISTA ENCADEADA ORDENADA de hematomas + INSERTION SORT.
 * Especificação completa: hematoma.h e docs/tarefas/TAREFA-A-estruturas.md
 * Testes: testes/teste_hematoma.c
 *
 * Dono: Integrante A.
 */
#include <stdlib.h>
#include "hematoma.h"

void hematomas_iniciar(ListaHematomas *l)
{
    l->inicio = NULL;
    l->quantidade = 0;
    l->dano_total = 0.0f;
}

bool hematoma_inserir(ListaHematomas *l, float dano, double momento_cura)
{
    if (dano <= 0.0f || l->quantidade >= HEMATOMAS_MAX)
        return false;

    NoHematoma *novo = malloc(sizeof *novo);
    if (novo == NULL)
        return false;
    novo->dano = dano;
    novo->momento_cura = momento_cura;

    /* Passo do insertion sort: a lista já está ordenada, então basta achar o
     * primeiro nó que cura DEPOIS do novo e encaixar o novo antes dele.
     * Usar "<=" para continuar andando nos empates deixa o novo depois dos
     * iguais que já estavam lá: é isso que torna a ordenação estável.
     * 'onde' aponta para o ponteiro que será trocado (l->inicio ou x->prox),
     * o que trata o início da lista sem caso especial. */
    NoHematoma **onde = &l->inicio;
    while (*onde != NULL && (*onde)->momento_cura <= momento_cura)
        onde = &(*onde)->prox;

    novo->prox = *onde;
    *onde = novo;

    l->quantidade++;
    l->dano_total += dano;
    return true;
}

int hematomas_curar(ListaHematomas *l, double agora)
{
    int removidos = 0;

    /* Ordenada pelo momento de cura: os que já curaram estão todos no início. */
    while (l->inicio != NULL && l->inicio->momento_cura <= agora) {
        NoHematoma *curado = l->inicio;
        l->inicio = curado->prox;
        l->dano_total -= curado->dano;
        l->quantidade--;
        free(curado);
        removidos++;
    }

    /* Somar e subtrair float deixa resto (ex.: 0.0000001). Vazia = zero exato. */
    if (l->quantidade == 0)
        l->dano_total = 0.0f;

    return removidos;
}

float hematomas_dano_total(const ListaHematomas *l)
{
    return l->dano_total;
}

void hematomas_liberar(ListaHematomas *l)
{
    while (l->inicio != NULL) {
        NoHematoma *prox = l->inicio->prox;
        free(l->inicio);
        l->inicio = prox;
    }
    hematomas_iniciar(l);
}
