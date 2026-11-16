/*
 * combo.h — LISTA CIRCULAR DUPLAMENTE ENCADEADA de golpes.
 *
 * É a estrutura de dados central do jogo (conteúdo da disciplina).
 * Cada jogador monta um combo de até COMBO_MAX golpes antes da luta.
 * Na luta, cada aperto do botão A executa o golpe "atual" e avança para o
 * próximo; depois do último vem o primeiro de novo (por isso é circular).
 *
 * Invariantes (devem valer SEMPRE, depois de qualquer função):
 *   - combo vazio:  inicio == NULL, atual == NULL, tamanho == 0
 *   - combo com n >= 1 nós:
 *       percorrer n vezes por ->prox a partir de inicio volta ao inicio;
 *       para todo nó x: x->prox->ant == x  e  x->ant->prox == x;
 *       atual aponta para um dos nós da lista (nunca NULL).
 *   - com 1 nó: inicio->prox == inicio e inicio->ant == inicio.
 *
 * Posições: os nós são numerados de 0 a tamanho-1, a partir de inicio,
 * seguindo ->prox.
 *
 * C puro: sem raylib e sem ODE (testado em testes/teste_combo.c).
 * Dono: Integrante A.
 */
#ifndef COMBO_H
#define COMBO_H

#include <stdbool.h>
#include "golpe.h"

typedef struct NoCombo {
    TipoGolpe golpe;          /* guarda só o tipo; os dados vêm de golpe_dados() */
    struct NoCombo *prox;     /* próximo nó; o último aponta para o primeiro     */
    struct NoCombo *ant;      /* nó anterior; o primeiro aponta para o último    */
} NoCombo;

typedef struct {
    NoCombo *inicio;          /* nó da posição 0 (NULL se vazio)                 */
    NoCombo *atual;           /* golpe que sai no próximo aperto de A            */
    int tamanho;              /* quantidade de nós, de 0 a COMBO_MAX             */
} Combo;

/* Deixa o combo vazio. Não libera nada: use em um Combo novo
 * (ou depois de combo_liberar).  O(1) */
void combo_iniciar(Combo *c);

/* Cria um nó com o golpe g e o coloca na posição pos (0 a tamanho).
 * pos == 0 coloca no início; pos == tamanho coloca no fim.
 * Se o combo estava vazio, atual passa a ser o novo nó; senão, atual
 * continua apontando para o mesmo nó de antes.
 * Retorna false (sem alterar nada) se: o combo estiver cheio (COMBO_MAX),
 * pos for inválida, g for inválido (>= GOLPE_QTD ou < 0) ou faltar memória.
 * O(n) */
bool combo_inserir(Combo *c, int pos, TipoGolpe g);

/* Remove e libera (free) o nó da posição pos (0 a tamanho-1).
 * Se o nó removido era o atual, atual passa para o próximo dele.
 * Se o nó removido era o inicio, inicio passa para o próximo dele.
 * Se o combo ficar vazio, inicio e atual viram NULL.
 * Retorna false (sem alterar nada) se pos for inválida.  O(n) */
bool combo_remover(Combo *c, int pos);

/* Troca os golpes das posições pos e (pos + 1) % tamanho.
 * Com pos == tamanho-1, troca o último com o primeiro.
 * Religa os ponteiros (os nós mudam de lugar; o golpe de cada nó não muda).
 * Não muda atual: ele continua no mesmo nó, então combo_golpe_atual
 * devolve o mesmo golpe de antes, mesmo que o atual seja um dos trocados.
 * Retorna false (sem alterar nada) se tamanho < 2 ou pos for inválida.
 * O(n) para achar a posição; a religação em si é O(1). */
bool combo_trocar_com_proximo(Combo *c, int pos);

/* Golpe do nó atual. Devolve GOLPE_QTD se o combo estiver vazio.  O(1) */
TipoGolpe combo_golpe_atual(const Combo *c);

/* Golpe da posição pos. Devolve GOLPE_QTD se pos for inválida.  O(n) */
TipoGolpe combo_golpe_em(const Combo *c, int pos);

/* Posição do nó atual (0 a tamanho-1). Devolve -1 se vazio.  O(n) */
int combo_indice_atual(const Combo *c);

/* atual = atual->prox. Não faz nada se o combo estiver vazio.  O(1) */
void combo_avancar(Combo *c);

/* atual = inicio. Não faz nada se o combo estiver vazio.  O(1) */
void combo_reiniciar(Combo *c);

/* Libera (free) todos os nós e deixa o combo vazio.
 * Seguro com combo vazio. Cuidado: a lista é circular, então "andar até
 * NULL" nunca termina; use o tamanho.  O(n) */
void combo_liberar(Combo *c);

#endif /* COMBO_H */
