/*
 * partida.h — jogadores, rounds, placar e o laço da física.
 *
 * Jogador: dura o jogo inteiro (personagem escolhido e combo montado).
 * Partida: dura de "3, 2, 1" do round 1 até alguém vencer.
 * Lutador: dura um round (recriado a cada round).
 *
 * Regras de round:
 *   - perde o round quem SAIR do ringue (bacia além de RINGUE_RAIO)
 *     ou CAIR (LUTADOR_CAIDO: qualquer parte além dos pés encostando no
 *     chão por CORPO_TEMPO_QUEDA), como no sumô de verdade;
 *   - se os dois perderem no mesmo passo, o round é repetido (empate);
 *   - se a física "explodir" (corpo_valido falso), o round é repetido;
 *   - vence a partida quem fizer PARTIDA_ROUNDS_PARA_VENCER rounds.
 *
 * Relógio: relogio só anda durante PARTIDA_LUTA e sem pausa. Hematomas e
 * golpes usam esse relógio, então nada cura durante a contagem ou a pausa.
 *
 * Dono: Principal.
 */
#ifndef PARTIDA_H
#define PARTIDA_H

#include <stdbool.h>
#include "fisica.h"
#include "lutador.h"
#include "combate.h"
#include "hud.h"

typedef struct {
    int   personagem;      /* índice no catálogo                         */
    Combo combo;           /* montado na tela de montagem; dura o jogo   */
} Jogador;

typedef enum {
    PARTIDA_CONTAGEM,      /* 3, 2, 1 — corpos parados de pé             */
    PARTIDA_LUTA,
    PARTIDA_FIM_ROUND,     /* mostra quem perdeu, depois novo round      */
    PARTIDA_FIM            /* alguém venceu; main.c vai para o resultado */
} EstadoPartida;

/* Eventos do último quadro, para o main.c tocar sons. */
typedef struct {
    bool acerto_leve;
    bool acerto_forte;
    bool queda;            /* alguém caiu ou saiu do ringue */
    bool contagem;         /* o número da contagem mudou    */
    bool lute;             /* a luta começou                */
    bool fim_partida;
} EventosPartida;

typedef struct {
    Jogador *jogadores;          /* vetor de 2, do main.c (não liberado aqui) */
    Fisica   fisica;
    Lutador  lutadores[2];
    int      vitorias[2];
    int      round;              /* 1, 2, 3...                                */
    EstadoPartida estado;
    float    tempo_no_estado;    /* (s)                                       */
    double   relogio;            /* tempo de luta (s), só anda na luta        */
    float    acumulador;         /* tempo ainda não simulado (s)              */
    bool     pausada;
    int      perdedor_round;     /* 0, 1, ou -1 (empate / repetir)            */
    const char *mensagem;        /* "FORA!", "CAIU!", "EMPATE"... ou NULL      */
    EventosPartida eventos;
    bool     round_ativo;        /* true enquanto física e lutadores existem  */
    float    congelado;          /* pausa no impacto ("hitstop") restante (s) */
    float    tremor;             /* intensidade do tremor de câmera (m)       */
    int      contagem_anterior;  /* último número mostrado (evento contagem)  */
} Partida;

/* Cria a física e o round 1. jogadores[i].combo deve estar cheio. */
bool partida_iniciar(Partida *p, Jogador jogadores[2]);

/* Um quadro: entradas, passos fixos de física (forças -> passo -> combate),
 * lógica dos lutadores, regras e troca de estado. Não faz nada se pausada. */
void partida_atualizar(Partida *p, const Intencao in[2], float dt);

void partida_pausar(Partida *p, bool pausada);

/* 0 ou 1 quando estado == PARTIDA_FIM; -1 antes disso. */
int  partida_vencedor(const Partida *p);

/* Preenche o InfoHud para hud_desenhar. */
void partida_preencher_hud(const Partida *p, InfoHud *info);

/* Destrói lutadores e física. Os combos dos jogadores continuam. */
void partida_liberar(Partida *p);

#endif /* PARTIDA_H */
