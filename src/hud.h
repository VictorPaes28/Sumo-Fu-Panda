/*
 * hud.h — informações desenhadas por cima da luta (2D).
 *
 * O HUD não conhece a Partida: o Principal preenche um InfoHud
 * (partida_preencher_hud) e o HUD só desenha o que recebeu.
 *
 * Layout sugerido:
 *   cantos superiores: nome, porcentagem de dano (grande), barra de stamina,
 *                      bolinhas de rounds vencidos, os golpes do combo com o
 *                      próximo destacado
 *   centro superior:   "Round N"
 *   centro:            contagem (3, 2, 1, LUTE!) e mensagens (FORA!, CAIU!, EMPATE)
 *   pausado:           tela escurecida, "PAUSA", "Start: continuar  B: menu"
 *
 * Dono: Integrante B.
 */
#ifndef HUD_H
#define HUD_H

#include <stdbool.h>
#include "config.h"
#include "golpe.h"

typedef struct {
    const char *nome[2];              /* nome dos personagens                  */
    float dano[2];                    /* porcentagem de dano (%)               */
    float stamina[2];                 /* 0 a 1 (fração de STAMINA_MAX)         */
    int   vitorias[2];                /* rounds vencidos                       */
    int   rounds_para_vencer;         /* para desenhar as bolinhas             */
    int   round;                      /* número do round atual (1, 2, 3...)    */
    TipoGolpe combo[2][COMBO_MAX];    /* golpes do combo de cada jogador       */
    int   combo_tamanho[2];
    int   combo_atual[2];             /* posição do próximo golpe (-1 = vazio) */
    int   contagem;                   /* 3, 2, 1 = contagem; 0 = "LUTE!"; -1 = nada */
    const char *mensagem;             /* texto grande no centro, ou NULL       */
    bool  pausado;
    float impacto[2];                 /* 1 logo depois de levar golpe, cai até 0:
                                         a porcentagem "pula" no HUD           */
} InfoHud;

/* Desenha o HUD. Chamar depois de desenhar a cena 3D (fora de BeginMode3D). */
void hud_desenhar(const InfoHud *info);

#endif /* HUD_H */
