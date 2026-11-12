/*
 * ui.h — cores e funções de desenho 2D comuns a todas as telas e ao HUD.
 *
 * Só desenho: nenhuma função altera estado. Usado por telas.c, montagem.c
 * e hud.c, para todas as telas terem o mesmo visual.
 *
 * Dono: Integrante B.
 */
#ifndef UI_H
#define UI_H

#include "raylib.h"

/* Paleta do jogo. */
#define UI_FUNDO        (Color){ 32, 28, 36, 255 }
#define UI_PAINEL       (Color){ 52, 46, 58, 235 }
#define UI_BORDA        (Color){ 92, 84, 100, 255 }
#define UI_TEXTO        (Color){ 240, 234, 224, 255 }
#define UI_TEXTO_FRACO  (Color){ 160, 152, 150, 255 }
#define UI_DESTAQUE     (Color){ 246, 196, 72, 255 }

/* Cor de cada jogador (0 = laranja, 1 = azul), a mesma do render 3D. */
Color ui_cor_jogador(int jogador);

/* Texto centralizado horizontalmente em cx (fonte padrão da raylib). */
void ui_texto_centro(const char *texto, int cx, int y, int tamanho, Color cor);

/* Texto com sombra, para ler por cima da cena 3D. */
void ui_texto_sombra(const char *texto, int x, int y, int tamanho, Color cor);
void ui_texto_centro_sombra(const char *texto, int cx, int y, int tamanho, Color cor);

/* Painel retangular com borda. */
void ui_painel(Rectangle r, Color fundo, Color borda);

/* Barra horizontal de 0 a 1. */
void ui_barra(Rectangle r, float fracao, Color cor);

#endif /* UI_H */
