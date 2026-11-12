/*
 * entrada.h — lê o controle (ou o teclado) e devolve uma Intencao.
 *
 * O resto do jogo nunca chama funções de entrada da raylib: só usa a
 * Intencao. Assim, controle e teclado funcionam igual em todo lugar.
 *
 * Controle (gamepad) do jogador i = gamepad i da raylib:
 *   analógico esquerdo  -> mover
 *   D-pad               -> direcional_x / direcional_y (menus)
 *   A                   -> a       (golpe / confirmar)
 *   B                   -> b       (voltar / remover)
 *   Y                   -> y       (trocar golpe de lugar na montagem)
 *   Start               -> start   (pausa / pronto)
 *   gatilho direito     -> firmar  (segurado)
 *
 * Teclado (reserva, quando o controle do jogador não está conectado):
 *   Jogador 1: WASD mover/direcional, C = a, X = b, Z = y, V = firmar, TAB = start
 *   Jogador 2: setas mover/direcional, K = a, J = b, U = y, L = firmar, ENTER = start
 *
 * Dono: Integrante B.
 */
#ifndef ENTRADA_H
#define ENTRADA_H

#include <stdbool.h>
#include "raylib.h"

typedef struct {
    Vector2 mover;          /* direção de movimento, -1 a 1 em x e y, já com zona
                               morta. x > 0 = direita; y > 0 = PARA CIMA
                               (atenção: na raylib o analógico para cima é negativo;
                               aqui é invertido). Teclado: W/seta cima = y +1.   */
    int  direcional_x;      /* -1, 0 ou 1: esquerda/direita APERTADO neste quadro  */
    int  direcional_y;      /* -1, 0 ou 1: -1 = cima, 1 = baixo, APERTADO neste quadro */
    bool a;                 /* botão A apertado neste quadro (borda, não segurado) */
    bool b;                 /* botão B apertado neste quadro                       */
    bool y;                 /* botão Y apertado neste quadro                       */
    bool start;             /* Start apertado neste quadro                         */
    bool firmar;            /* gatilho direito SEGURADO                            */
    bool usando_controle;   /* true = leu do controle; false = leu do teclado      */
} Intencao;

/* Lê a entrada do jogador (0 ou 1) neste quadro.
 * Usa o controle se IsGamepadAvailable(jogador); senão, o teclado.
 * Aplica ENTRADA_ZONA_MORTA no analógico e ENTRADA_LIMIAR_GATILHO no gatilho.
 * Chamar UMA vez por quadro por jogador (as bordas "apertado neste quadro"
 * vêm da raylib). */
Intencao entrada_ler(int jogador);

/* Intencao "vazia": tudo zero/false. Útil para pausas e testes. */
Intencao entrada_vazia(void);

#endif /* ENTRADA_H */
