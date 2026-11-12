/*
 * combo.c — LISTA CIRCULAR DUPLAMENTE ENCADEADA de golpes.
 * Especificação completa: combo.h e docs/tarefas/TAREFA-A-estruturas.md
 * Testes: testes/teste_combo.c
 *
 * Dono: Integrante A.
 */
#include <stdlib.h>
#include "combo.h"
#include "config.h"

/* Nó da posição pos (0 a tamanho-1), andando por ->prox a partir do início.
 * Quem chama garante que pos é válida e que o combo não está vazio. */
static NoCombo *no_na_posicao(const Combo *c, int pos)
{
    NoCombo *n = c->inicio;
    for (int i = 0; i < pos; i++)
        n = n->prox;
    return n;
}

void combo_iniciar(Combo *c)
{
    c->inicio = NULL;
    c->atual = NULL;
    c->tamanho = 0;
}

bool combo_inserir(Combo *c, int pos, TipoGolpe g)
{
    if (c->tamanho >= COMBO_MAX || pos < 0 || pos > c->tamanho)
        return false;
    if ((int)g < 0 || g >= GOLPE_QTD)
        return false;

    NoCombo *novo = malloc(sizeof *novo);
    if (novo == NULL)
        return false;
    novo->golpe = g;

    if (c->tamanho == 0) {
        /* Único nó: aponta para ele mesmo nos dois sentidos. */
        novo->prox = novo;
        novo->ant = novo;
        c->inicio = novo;
        c->atual = novo;
    } else {
        /* O novo entra ANTES do nó que hoje ocupa a posição pos.
         * Inserir no fim (pos == tamanho) é entrar antes do início,
         * porque a lista é circular. */
        NoCombo *depois = (pos == c->tamanho) ? c->inicio : no_na_posicao(c, pos);
        NoCombo *antes = depois->ant;

        novo->prox = depois;
        novo->ant = antes;
        antes->prox = novo;
        depois->ant = novo;

        if (pos == 0)
            c->inicio = novo;
    }

    c->tamanho++;
    return true;
}

bool combo_remover(Combo *c, int pos)
{
    if (pos < 0 || pos >= c->tamanho)
        return false;

    NoCombo *alvo = no_na_posicao(c, pos);

    if (c->tamanho == 1) {
        free(alvo);
        combo_iniciar(c);
        return true;
    }

    /* Religa os vizinhos, pulando o alvo. */
    alvo->ant->prox = alvo->prox;
    alvo->prox->ant = alvo->ant;

    if (c->atual == alvo)
        c->atual = alvo->prox;
    if (c->inicio == alvo)
        c->inicio = alvo->prox;

    free(alvo);
    c->tamanho--;
    return true;
}

bool combo_trocar_com_proximo(Combo *c, int pos)
{
    if (c->tamanho < 2 || pos < 0 || pos >= c->tamanho)
        return false;

    /* Troca os valores dos dois nós: o resultado é o mesmo que religar os
     * ponteiros, com muito menos chance de erro. pos == tamanho-1 troca o
     * último com o primeiro, porque o prox do último é o início. */
    NoCombo *a = no_na_posicao(c, pos);
    NoCombo *b = a->prox;
    TipoGolpe temp = a->golpe;
    a->golpe = b->golpe;
    b->golpe = temp;

    c->atual = c->inicio;
    return true;
}

TipoGolpe combo_golpe_atual(const Combo *c)
{
    return (c->atual != NULL) ? c->atual->golpe : GOLPE_QTD;
}

TipoGolpe combo_golpe_em(const Combo *c, int pos)
{
    if (pos < 0 || pos >= c->tamanho)
        return GOLPE_QTD;
    return no_na_posicao(c, pos)->golpe;
}

int combo_indice_atual(const Combo *c)
{
    const NoCombo *n = c->inicio;
    for (int i = 0; i < c->tamanho; i++) {
        if (n == c->atual)
            return i;
        n = n->prox;
    }
    return -1;
}

void combo_avancar(Combo *c)
{
    if (c->atual != NULL)
        c->atual = c->atual->prox;
}

void combo_reiniciar(Combo *c)
{
    if (c->inicio != NULL)
        c->atual = c->inicio;
}

void combo_liberar(Combo *c)
{
    /* A lista é circular: nunca chega em NULL. Conta os nós pelo tamanho. */
    NoCombo *n = c->inicio;
    for (int i = 0; i < c->tamanho; i++) {
        NoCombo *prox = n->prox;
        free(n);
        n = prox;
    }
    combo_iniciar(c);
}
