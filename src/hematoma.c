/*
 * hematoma.c — LISTA ENCADEADA ORDENADA de hematomas + INSERTION SORT.
 * Especificação completa: hematoma.h e docs/tarefas/TAREFA-A-estruturas.md
 * Testes: testes/teste_hematoma.c
 *
 * Dono: Integrante A.
 */
#include <stdlib.h>
#include "hematoma.h"

/* O(1): só zera os campos, não percorre nada. */
void hematomas_iniciar(ListaHematomas *l)
{
    l->inicio = NULL;
    l->fim = NULL;
    l->quantidade = 0;
    l->dano_total = 0.0f;
}

/* O(1) no caso comum (cura depois de todos) e quando cura antes de todos;
 * O(n) no pior caso (cai no meio da lista). */
bool hematomas_inserir(ListaHematomas *l, float dano, double momento_cura)
{
    if (dano <= 0.0f || l->quantidade >= HEMATOMAS_MAX)
        return false;

    NoHematoma *novo = malloc(sizeof *novo);
    if (novo == NULL)
        return false;
    novo->dano = dano;
    novo->momento_cura = momento_cura;

    if (l->fim == NULL || momento_cura >= l->fim->momento_cura) {
        /* Caso comum: o novo cura depois de todos (ou a lista está vazia).
         * Liga direto depois do fim, sem percorrer: O(1).
         * O ">=" deixa o novo depois de um último com o mesmo momento de
         * cura, o que mantém a inserção estável. */
        novo->prox = NULL;
        if (l->fim == NULL)
            l->inicio = novo;
        else
            l->fim->prox = novo;
        l->fim = novo;
    } else {
        /* Passo do insertion sort: a lista já está ordenada, então basta achar
         * o primeiro nó que cura DEPOIS do novo e encaixar o novo antes dele.
         * Usar "<=" para continuar andando nos empates deixa o novo depois dos
         * iguais que já estavam lá: é isso que torna a ordenação estável.
         * 'onde' aponta para o ponteiro que será trocado (l->inicio ou x->prox),
         * o que trata o início da lista sem caso especial.
         * Aqui o fim não muda: o último cura depois do novo, então o laço
         * sempre para antes dele e o novo nunca vira o último. */
        NoHematoma **onde = &l->inicio;
        while ((*onde)->momento_cura <= momento_cura)
            onde = &(*onde)->prox;

        novo->prox = *onde;
        *onde = novo;
    }

    l->quantidade++;
    l->dano_total += dano;
    return true;
}

/* O(k), k = quantos curaram: só olha o início da lista. */
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

    /* Remover pelo início só mexe no fim quando a lista esvazia
     * (o último nó foi removido). */
    if (l->inicio == NULL)
        l->fim = NULL;

    /* Somar e subtrair float deixa resto (ex.: 0.0000001). Vazia = zero exato. */
    if (l->quantidade == 0)
        l->dano_total = 0.0f;

    return removidos;
}

/* O(1): a soma é mantida a cada inserção e remoção, não é recalculada. */
float hematomas_dano_total(const ListaHematomas *l)
{
    return l->dano_total;
}

/* O(n): libera nó por nó. A lista termina em NULL, então dá para andar até ele. */
void hematomas_liberar(ListaHematomas *l)
{
    while (l->inicio != NULL) {
        NoHematoma *prox = l->inicio->prox;
        free(l->inicio);
        l->inicio = prox;
    }
    hematomas_iniciar(l);   /* inicio, fim, quantidade e dano_total zerados */
}
