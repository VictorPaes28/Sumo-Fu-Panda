/*
 * previa.h — o lutador escolhido, ao vivo, na tela de seleção.
 *
 * Cada jogador tem uma prévia: um mundo de física só dela, com o corpo do
 * personagem escolhido em pé na guarda de sumô. É o mesmo corpo da luta
 * (corpo.h), então ele respira, se equilibra e aparece com a mesma pele.
 * A câmera vai e volta em volta dele (PREVIA_GIRO).
 *
 * ATENÇÃO: como Fisica e Corpo, uma Previa nunca pode ser copiada por valor
 * (o ODE guarda ponteiros para dentro dela). Use sempre ponteiros.
 *
 * Dono: Principal.
 */
#ifndef PREVIA_H
#define PREVIA_H

#include <stdbool.h>
#include "raylib.h"
#include "corpo.h"
#include "fisica.h"
#include "recursos.h"

typedef struct {
    Fisica fisica;            /* mundo só da prévia (chão e um corpo)          */
    Corpo  corpo;
    int    personagem;        /* índice no catálogo; -1 = nenhum corpo criado  */
    int    jogador;           /* 0 ou 1: cores do jogador                      */
    float  acumulador;        /* tempo ainda não simulado (s)                  */
    float  tempo;             /* desde que a prévia foi criada: giro da câmera (s) */
    RenderTexture2D imagem;   /* onde o lutador é desenhado                    */
} Previa;

/* Prepara a prévia do jogador (0 ou 1), sem corpo ainda. Chamar depois de
 * InitWindow(). Retorna false se faltar memória para a física. */
bool previa_criar(Previa *pv, int jogador);

/* Uma vez por quadro: troca o corpo se o personagem mudou (e já o deixa de pé)
 * e avança a física do tempo dt (s). */
void previa_atualizar(Previa *pv, int personagem, float dt);

/* Desenha a prévia no retângulo 'area' da tela. Chamar entre BeginDrawing e
 * EndDrawing. Recursos não é const pelo mesmo motivo de render_cena. */
void previa_desenhar(Previa *pv, Recursos *r, Rectangle area);

/* Destrói o corpo, a física e a imagem. */
void previa_liberar(Previa *pv);

#endif /* PREVIA_H */
