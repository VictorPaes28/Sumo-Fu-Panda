/*
 * ui.c — cores e funções de desenho 2D comuns.
 * Especificação: ui.h
 *
 * Dono: Integrante B.
 */
#include "ui.h"

Color ui_cor_jogador(int jogador)
{
    return (jogador == 0) ? (Color){ 235, 120, 40, 255 } : (Color){ 60, 150, 235, 255 };
}

void ui_texto_centro(const char *texto, int cx, int y, int tamanho, Color cor)
{
    DrawText(texto, cx - MeasureText(texto, tamanho) / 2, y, tamanho, cor);
}

void ui_texto_sombra(const char *texto, int x, int y, int tamanho, Color cor)
{
    int d = (tamanho >= 40) ? 3 : 2;
    DrawText(texto, x + d, y + d, tamanho, Fade(BLACK, 0.7f));
    DrawText(texto, x, y, tamanho, cor);
}

void ui_texto_centro_sombra(const char *texto, int cx, int y, int tamanho, Color cor)
{
    ui_texto_sombra(texto, cx - MeasureText(texto, tamanho) / 2, y, tamanho, cor);
}

void ui_painel(Rectangle r, Color fundo, Color borda)
{
    DrawRectangleRec(r, fundo);
    DrawRectangleLinesEx(r, 2.0f, borda);
}

void ui_barra(Rectangle r, float fracao, Color cor)
{
    if (fracao < 0.0f) fracao = 0.0f;
    if (fracao > 1.0f) fracao = 1.0f;
    DrawRectangleRec(r, Fade(BLACK, 0.5f));
    DrawRectangleRec((Rectangle){ r.x, r.y, r.width * fracao, r.height }, cor);
    DrawRectangleLinesEx(r, 1.0f, Fade(WHITE, 0.4f));
}
