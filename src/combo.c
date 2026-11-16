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
 * Quem chama garante que pos é válida e que o combo não está vazio.
 * O(n): anda pos nós a partir do início. */
static NoCombo *no_na_posicao(const Combo *c, int pos)
{
    NoCombo *n = c->inicio;
    for (int i = 0; i < pos; i++)
        n = n->prox;
    return n;
}

/* O(1): só zera os campos, não percorre nada. */
void combo_iniciar(Combo *c)
{
    c->inicio = NULL;
    c->atual = NULL;
    c->tamanho = 0;
}

/* O(n): precisa andar até a posição; a ligação do nó em si é O(1).
 * Inserir no fim também é O(1), porque o último é inicio->ant (circular). */
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

/* O(n): precisa andar até a posição; religar os vizinhos é O(1),
 * porque cada nó conhece o anterior (lista dupla). */
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

/* O(n): precisa andar até a posição; a religação dos dois nós é O(1). */
bool combo_trocar_com_proximo(Combo *c, int pos)
{
    if (c->tamanho < 2 || pos < 0 || pos >= c->tamanho)
        return false;

    /* Move os NÓS de lugar (religando ponteiros) em vez de trocar os golpes
     * entre eles. Assim cada nó continua com o seu golpe, e o atual, que
     * aponta para um nó, continua no mesmo golpe depois da troca.
     *
     * a e b são vizinhos (b = a->prox). Trocar dois vizinhos é tirar o a
     * de onde está e colocá-lo logo depois do b:
     *     antes:  ... -> X -> a -> b -> Y -> ...
     *     depois: ... -> X -> b -> a -> Y -> ...
     * pos == tamanho-1 troca o último com o primeiro, porque o prox do
     * último é o início. Com 2 nós (X == b e Y == b) as mesmas linhas
     * também funcionam. */
    NoCombo *a = no_na_posicao(c, pos);
    NoCombo *b = a->prox;

    /* 1. Tira o a do círculo: o anterior dele (X) passa a apontar para o b. */
    NoCombo *antes = a->ant;
    antes->prox = b;
    b->ant = antes;

    /* 2. Coloca o a entre o b e o próximo do b (Y). */
    NoCombo *depois = b->prox;
    a->ant = b;
    a->prox = depois;
    b->prox = a;
    depois->ant = a;

    /* 3. Se um dos dois era o início, o outro passa a ser: o início é a
     * posição 0, e os dois trocaram de posição. O atual não muda. */
    if (c->inicio == a)
        c->inicio = b;
    else if (c->inicio == b)
        c->inicio = a;

    return true;
}

/* O(1): o atual já é um ponteiro para o nó. */
TipoGolpe combo_golpe_atual(const Combo *c)
{
    return (c->atual != NULL) ? c->atual->golpe : GOLPE_QTD;
}

/* O(n): anda até a posição. */
TipoGolpe combo_golpe_em(const Combo *c, int pos)
{
    if (pos < 0 || pos >= c->tamanho)
        return GOLPE_QTD;
    return no_na_posicao(c, pos)->golpe;
}

/* O(n): anda pela lista contando até achar o atual. */
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

/* O(1): segue o prox; depois do último vem o primeiro (circular). */
void combo_avancar(Combo *c)
{
    if (c->atual != NULL)
        c->atual = c->atual->prox;
}

/* O(1): volta o atual para o início. */
void combo_reiniciar(Combo *c)
{
    if (c->inicio != NULL)
        c->atual = c->inicio;
}

/* O(n): libera nó por nó. */
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
