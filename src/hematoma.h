/*
 * hematoma.h — LISTA ENCADEADA ORDENADA de hematomas + INSERTION SORT.
 *
 * Conteúdo da disciplina. Cada golpe que acerta cria um hematoma no alvo,
 * com um dano (%) e o instante em que ele cura. A lista fica SEMPRE
 * ordenada pelo momento_cura (o que cura primeiro fica no início), então
 * para curar basta olhar o início da lista.
 *
 * Inserção = insertion sort: o novo hematoma é encaixado na posição certa
 * de uma lista que já está ordenada (melhor caso do algoritmo).
 * Estável: com momentos de cura iguais, o hematoma mais novo fica DEPOIS
 * dos que já estavam lá.
 *
 * O tempo é sempre passado como parâmetro (relógio da partida, em segundos):
 * este módulo não chama GetTime(). Assim ele pode ser testado no terminal.
 *
 * Invariantes:
 *   - lista vazia: inicio == NULL, quantidade == 0, dano_total == 0
 *   - para todo nó x com x->prox != NULL:  x->momento_cura <= x->prox->momento_cura
 *   - quantidade == número de nós; dano_total == soma dos danos dos nós
 *   - quantidade <= HEMATOMAS_MAX
 *
 * C puro: sem raylib e sem ODE (testado em testes/teste_hematoma.c).
 * Dono: Integrante A.
 */
#ifndef HEMATOMA_H
#define HEMATOMA_H

#include <stdbool.h>
#include "config.h"

typedef struct NoHematoma {
    float  dano;                 /* porcentagem deste hematoma          (%) */
    double momento_cura;         /* instante em que ele some            (s) */
    struct NoHematoma *prox;     /* próximo a curar (NULL no último)        */
} NoHematoma;

typedef struct {
    NoHematoma *inicio;          /* o que cura primeiro (NULL se vazia)     */
    int   quantidade;            /* número de hematomas ativos              */
    float dano_total;            /* soma dos danos, atualizada a cada
                                    inserção e remoção (não recalcular)     */
} ListaHematomas;

/* Deixa a lista vazia. Não libera nada: use em uma lista nova
 * (ou depois de hematomas_liberar).  O(1) */
void hematomas_iniciar(ListaHematomas *l);

/* Cria um hematoma e o insere na posição certa pelo momento_cura
 * (insertion sort, estável). Atualiza quantidade e dano_total.
 * Retorna false (sem alterar nada) se: dano <= 0, a lista estiver cheia
 * (HEMATOMAS_MAX) ou faltar memória.
 * O(n) no pior caso; O(1) quando o novo cura antes de todos. */
bool hematoma_inserir(ListaHematomas *l, float dano, double momento_cura);

/* Remove e libera, a partir do início, todos os hematomas com
 * momento_cura <= agora. Atualiza quantidade e dano_total
 * (com a lista vazia, dano_total volta a ser exatamente 0).
 * Retorna quantos foram removidos.  O(k), k = removidos */
int hematomas_curar(ListaHematomas *l, double agora);

/* Soma dos danos ativos (%).  O(1) */
float hematomas_dano_total(const ListaHematomas *l);

/* Libera (free) todos os nós e deixa a lista vazia. Seguro com lista vazia.
 * O(n) */
void hematomas_liberar(ListaHematomas *l);

#endif /* HEMATOMA_H */
