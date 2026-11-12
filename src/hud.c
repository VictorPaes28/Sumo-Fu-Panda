/*
 * hud.c — informações por cima da luta.
 * Especificação: hud.h
 *
 * Dono: Integrante B.
 */
#include <stddef.h>
#include "hud.h"
#include "raylib.h"
#include "ui.h"

#define PAINEL_L   380.0f
#define PAINEL_A   150.0f

/* Branco com pouco dano, vermelho com muito (o perigo aparece na cor). */
static Color cor_do_dano(float dano)
{
    float t = dano / DANO_MAX;
    if (t > 1.0f) t = 1.0f;
    return (Color){ 255, (unsigned char)(240 - 200 * t), (unsigned char)(230 - 210 * t), 255 };
}

static void desenhar_jogador(const InfoHud *h, int i)
{
    bool esquerda = (i == 0);
    float x = esquerda ? 20.0f : JANELA_LARGURA - 20.0f - PAINEL_L;
    Color cor = ui_cor_jogador(i);
    ui_painel((Rectangle){ x, 16, PAINEL_L, PAINEL_A }, Fade(UI_PAINEL, 0.85f), cor);

    DrawText(h->nome[i] ? h->nome[i] : "?", (int)x + 14, 26, 26, cor);

    /* Rounds vencidos. */
    for (int r = 0; r < h->rounds_para_vencer; r++) {
        Vector2 c = { x + PAINEL_L - 24 - r * 26.0f, 39 };
        if (r < h->vitorias[i]) DrawCircleV(c, 9, UI_DESTAQUE);
        DrawCircleLinesV(c, 9, UI_TEXTO_FRACO);
    }

    /* Porcentagem de dano, grande. Ao levar golpe, ela "pula" e treme. */
    float imp = h->impacto[i];
    int tam = 48 + (int)(16.0f * imp);
    int tremor = (imp > 0.0f) ? (int)(imp * 5.0f) * ((GetRandomValue(0, 1) * 2) - 1) : 0;
    Color cor_dano = cor_do_dano(h->dano[i]);
    if (imp > 0.5f) cor_dano = (Color){ 255, 70, 50, 255 };
    DrawText(TextFormat("%.0f%%", h->dano[i]), (int)x + 14 + tremor, 58 - (tam - 48) / 2, tam, cor_dano);

    DrawText("stamina", (int)x + 150, 66, 16, UI_TEXTO_FRACO);
    ui_barra((Rectangle){ x + 150, 86, PAINEL_L - 170, 14 }, h->stamina[i],
             h->stamina[i] < 0.3f ? (Color){ 220, 80, 60, 255 } : (Color){ 110, 200, 120, 255 });

    /* Combo: o próximo golpe em destaque. */
    float bw = (PAINEL_L - 28) / COMBO_MAX;
    for (int k = 0; k < COMBO_MAX; k++) {
        Rectangle r = { x + 14 + k * bw, 116, bw - 6, 40 };
        bool proximo = (k == h->combo_atual[i]);
        ui_painel(r, proximo ? Fade(cor, 0.45f) : Fade(BLACK, 0.4f), proximo ? UI_DESTAQUE : UI_BORDA);
        if (k < h->combo_tamanho[i]) {
            const Golpe *g = golpe_dados(h->combo[i][k]);
            if (g != NULL) {
                /* Diminui a fonte até o nome caber no quadro. */
                int tam_fonte = 14;
                while (tam_fonte > 8 && MeasureText(g->nome, tam_fonte) > (int)r.width - 8)
                    tam_fonte--;
                ui_texto_centro(g->nome, (int)(r.x + r.width / 2), (int)r.y + (40 - tam_fonte) / 2, tam_fonte,
                                proximo ? UI_TEXTO : UI_TEXTO_FRACO);
            }
        }
    }
}

void hud_desenhar(const InfoHud *info)
{
    desenhar_jogador(info, 0);
    desenhar_jogador(info, 1);
    ui_texto_centro_sombra(TextFormat("Round %d", info->round), JANELA_LARGURA / 2, 24, 30, UI_TEXTO);

    if (info->contagem > 0)
        ui_texto_centro_sombra(TextFormat("%d", info->contagem), JANELA_LARGURA / 2, 230, 140, UI_DESTAQUE);
    else if (info->contagem == 0)
        ui_texto_centro_sombra("LUTE!", JANELA_LARGURA / 2, 250, 110, UI_DESTAQUE);

    if (info->mensagem != NULL)
        ui_texto_centro_sombra(info->mensagem, JANELA_LARGURA / 2, 400, 80, UI_TEXTO);

    if (info->pausado) {
        DrawRectangle(0, 0, JANELA_LARGURA, JANELA_ALTURA, Fade(BLACK, 0.6f));
        ui_texto_centro("PAUSA", JANELA_LARGURA / 2, 260, 90, UI_TEXTO);
        ui_texto_centro("Start: continuar     B: sair para o menu", JANELA_LARGURA / 2, 380, 26,
                        UI_TEXTO_FRACO);
    }
}
