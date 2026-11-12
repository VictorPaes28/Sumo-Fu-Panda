/*
 * telas.h — telas 2D simples: menu, seleção de personagem e resultado.
 *
 * Padrão de toda tela do jogo:
 *   xxx_iniciar(tela)              prepara o estado
 *   acao = xxx_atualizar(tela, in) lê as intenções dos 2 jogadores e
 *                                  devolve o que o main.c deve fazer
 *   xxx_desenhar(tela)             só desenha (entre BeginDrawing e EndDrawing,
 *                                  chamado pelo main.c); nunca altera o estado
 *
 * As telas não tocam sons nem trocam de tela sozinhas: elas devolvem uma
 * ação, e o main.c decide. As telas só desenham em 2D (DrawText, DrawRectangle...).
 *
 * Dono: Integrante B.
 */
#ifndef TELAS_H
#define TELAS_H

#include <stdbool.h>
#include "entrada.h"

/* ------------------------------------------------------------------ Menu */
/* Opções: 0 = "Jogar", 1 = "Sair". Qualquer jogador navega. */
typedef struct {
    int opcao;
} TelaMenu;

typedef enum { MENU_NENHUMA, MENU_JOGAR, MENU_SAIR } AcaoMenu;

void     menu_iniciar(TelaMenu *t);
/* direcional_y muda a opção (com volta: de baixo pula para cima);
 * a de qualquer jogador confirma. */
AcaoMenu menu_atualizar(TelaMenu *t, const Intencao in[2]);
void     menu_desenhar(const TelaMenu *t);

/* ------------------------------------------------------------- Seleção */
/* Cada jogador escolhe um personagem do catálogo (personagem.h).
 * Os dois podem escolher o mesmo personagem. */
typedef struct {
    int  escolha[2];   /* índice no catálogo de personagens */
    bool pronto[2];
} TelaSelecao;

typedef enum { SELECAO_CONTINUA, SELECAO_CONCLUIDA, SELECAO_VOLTAR } AcaoSelecao;

void        selecao_iniciar(TelaSelecao *t);
/* Para cada jogador i:
 *   direcional_x muda escolha[i] (com volta), só se não estiver pronto;
 *   a -> pronto[i] = true;
 *   b -> se pronto[i], desfaz; se não, devolve SELECAO_VOLTAR.
 * Os dois prontos -> SELECAO_CONCLUIDA. */
AcaoSelecao selecao_atualizar(TelaSelecao *t, const Intencao in[2]);
void        selecao_desenhar(const TelaSelecao *t);

/* ------------------------------------------------------------ Resultado */
/* Fim da partida. Opções: 0 = "Revanche" (mesmos combos),
 * 1 = "Remontar combos", 2 = "Menu". Qualquer jogador navega. */
typedef struct {
    int vencedor;               /* 0 ou 1 */
    const char *nome_vencedor;  /* nome do personagem vencedor */
    int opcao;
} TelaResultado;

typedef enum {
    RESULTADO_CONTINUA, RESULTADO_REVANCHE, RESULTADO_REMONTAR, RESULTADO_MENU
} AcaoResultado;

void          resultado_iniciar(TelaResultado *t, int vencedor, const char *nome_vencedor);
AcaoResultado resultado_atualizar(TelaResultado *t, const Intencao in[2]);
void          resultado_desenhar(const TelaResultado *t);

#endif /* TELAS_H */
