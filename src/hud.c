/*
 * hud.c — informações por cima da luta.
 * Especificação: hud.h
 *
 * Cada jogador tem uma faixa no seu canto, espelhada (o jogador 1 à
 * esquerda, o 2 à direita), como nos jogos de luta:
 *   - nome e rounds vencidos;
 *   - barra de vida (o quanto ainda aguenta: esvazia com o dano, do verde
 *     ao vermelho, e pisca branco ao levar golpe) com a porcentagem de dano;
 *   - stamina;
 *   - os golpes do combo, com o próximo pulsando e uma faixa de cor nos
 *     golpes fortes (vermelha) e nos que seguram a faixa (verde).
 *
 * Dono: Integrante B.
 */
#include <math.h>
#include <stddef.h>
#include "hud.h"
#include "raylib.h"
#include "ui.h"

/* Medidas da faixa de cada jogador (pixels). */
#define FAIXA_L          430.0f   /* largura                                  */
#define FAIXA_A          172.0f   /* altura                                   */
#define FAIXA_MARGEM     16.0f    /* da borda da tela                         */
#define FAIXA_CORTE      34.0f    /* quanto a ponta de dentro é inclinada     */
#define FAIXA_FUNDO      (Color){ 24, 20, 30, 215 }
#define VIDA_Y           52.0f    /* topo da barra de vida, a partir da faixa */
#define VIDA_A           24.0f
#define FOLEGO_A         8.0f     /* altura da barra de stamina               */
#define COMBO_Y          128.0f
#define COMBO_A          34.0f
#define COR_FORTE        (Color){ 214, 84, 64, 255 }   /* como os selos da montagem */
#define COR_AGARRA       (Color){ 64, 170, 140, 255 }

/* Vida: verde com pouco dano, amarela no meio, vermelha perto do fim. */
static Color cor_da_vida(float vida)
{
    if (vida > 0.5f)
        return ColorLerp((Color){ 236, 200, 60, 255 }, (Color){ 96, 200, 110, 255 }, (vida - 0.5f) * 2.0f);
    return ColorLerp((Color){ 220, 64, 50, 255 }, (Color){ 236, 200, 60, 255 }, vida * 2.0f);
}

/* x da tela para uma posição medida a partir da borda de fora da faixa:
 * o jogador 2 é o espelho do 1. */
static float x_de(int i, float de_fora, float largura)
{
    return (i == 0) ? FAIXA_MARGEM + de_fora : JANELA_LARGURA - FAIXA_MARGEM - de_fora - largura;
}

/* Fundo da faixa: retângulo com a ponta de dentro cortada na diagonal e uma
 * linha na cor do jogador embaixo. */
static void desenhar_fundo(int i, Color cor)
{
    float fora = (i == 0) ? FAIXA_MARGEM : JANELA_LARGURA - FAIXA_MARGEM;
    float dentro = (i == 0) ? fora + FAIXA_L : fora - FAIXA_L;
    float corte = (i == 0) ? -FAIXA_CORTE : FAIXA_CORTE;
    float y0 = FAIXA_MARGEM, y1 = FAIXA_MARGEM + FAIXA_A;
    Vector2 a = { fora, y0 }, b = { dentro, y0 }, c = { dentro + corte, y1 }, d = { fora, y1 };
    /* DrawTriangle quer os vértices no sentido anti-horário. */
    if (i == 0) {
        DrawTriangle(a, d, c, FAIXA_FUNDO);
        DrawTriangle(a, c, b, FAIXA_FUNDO);
    } else {
        DrawTriangle(a, b, c, FAIXA_FUNDO);
        DrawTriangle(a, c, d, FAIXA_FUNDO);
    }
    DrawLineEx(d, c, 4.0f, cor);
    DrawLineEx(b, c, 2.0f, Fade(cor, 0.6f));
}

/* Nome do lutador e os rounds vencidos. */
static void desenhar_titulo(const InfoHud *h, int i, Color cor)
{
    const char *nome = h->nome[i] ? h->nome[i] : "?";
    int w = MeasureText(nome, 28);
    ui_texto_sombra(nome, (int)x_de(i, 16.0f, (float)w), (int)FAIXA_MARGEM + 12, 28, cor);

    /* Bolinhas dos rounds, do lado de dentro do nome. */
    for (int r = 0; r < h->rounds_para_vencer; r++) {
        float de_fora = 24.0f + (float)w + 22.0f + r * 24.0f;
        Vector2 c = { x_de(i, de_fora, 0.0f), FAIXA_MARGEM + 26.0f };
        if (r < h->vitorias[i])
            DrawCircleV(c, 8.0f, UI_DESTAQUE);
        DrawCircleLinesV(c, 8.0f, r < h->vitorias[i] ? UI_DESTAQUE : UI_TEXTO_FRACO);
    }
}

/* Barra de vida: cheia sem dano, vazia em DANO_MAX. Esvazia pelo lado de
 * dentro (perto do centro da tela), como nos jogos de luta. */
static void desenhar_vida(const InfoHud *h, int i)
{
    float largura = FAIXA_L - 32.0f - FAIXA_CORTE;
    float x = x_de(i, 16.0f, largura);
    float y = FAIXA_MARGEM + VIDA_Y;
    float vida = 1.0f - h->dano[i] / DANO_MAX;
    if (vida < 0.0f) vida = 0.0f;

    DrawRectangle((int)x - 2, (int)y - 2, (int)largura + 4, (int)VIDA_A + 4, (Color){ 10, 8, 14, 255 });
    float cheia = largura * vida;
    float x_cheia = (i == 0) ? x : x + largura - cheia;
    Color cor = cor_da_vida(vida);
    DrawRectangleGradientV((int)x_cheia, (int)y, (int)cheia, (int)VIDA_A, ColorBrightness(cor, 0.25f), cor);
    /* Ao levar golpe, a barra pisca branco por um instante. */
    if (h->impacto[i] > 0.0f)
        DrawRectangle((int)x_cheia, (int)y, (int)cheia, (int)VIDA_A, Fade(WHITE, 0.6f * h->impacto[i]));
    DrawRectangleLines((int)x - 2, (int)y - 2, (int)largura + 4, (int)VIDA_A + 4, UI_BORDA);
}

/* Porcentagem de dano (pula e treme ao levar golpe) e a stamina. */
static void desenhar_dano_e_folego(const InfoHud *h, int i)
{
    float imp = h->impacto[i];
    int tam = 34 + (int)(12.0f * imp);
    int tremor = (imp > 0.0f) ? (int)(imp * 4.0f) * ((GetRandomValue(0, 1) * 2) - 1) : 0;
    const char *texto = TextFormat("%.0f%%", h->dano[i]);
    float vida = 1.0f - h->dano[i] / DANO_MAX;
    Color cor = (imp > 0.5f) ? (Color){ 255, 70, 50, 255 } : cor_da_vida(vida < 0.0f ? 0.0f : vida);
    int w = MeasureText(texto, tam);
    ui_texto_sombra(texto, (int)x_de(i, 16.0f, (float)w) + tremor,
                    (int)(FAIXA_MARGEM + VIDA_Y + VIDA_A + 6.0f) - (tam - 34) / 2, tam, cor);

    float largura = 170.0f;
    float y = FAIXA_MARGEM + VIDA_Y + VIDA_A + 24.0f;
    float x = x_de(i, 120.0f, largura);
    ui_texto_sombra("stamina", (int)x, (int)y - 14, 16, UI_TEXTO_FRACO);
    Color cor_folego = h->stamina[i] < 0.3f ? (Color){ 220, 80, 60, 255 } : (Color){ 110, 190, 230, 255 };
    ui_barra((Rectangle){ x, y + 4.0f, largura, FOLEGO_A }, h->stamina[i], cor_folego);
}

/* Os golpes do combo. O próximo pulsa na cor do jogador; golpes fortes e
 * que seguram a faixa ganham uma faixinha de cor embaixo. */
static void desenhar_combo(const InfoHud *h, int i, Color cor)
{
    float largura = FAIXA_L - 32.0f - FAIXA_CORTE;
    float bw = largura / COMBO_MAX;
    float pulso = 0.5f + 0.5f * sinf((float)GetTime() * 6.0f);
    for (int k = 0; k < COMBO_MAX; k++) {
        /* O jogador 2 lê o combo da direita para a esquerda (espelhado). */
        float de_fora = 16.0f + k * bw;
        Rectangle r = { x_de(i, de_fora, bw - 6.0f), FAIXA_MARGEM + COMBO_Y, bw - 6.0f, COMBO_A };
        bool proximo = (k == h->combo_atual[i]);
        Color fundo = proximo ? Fade(cor, 0.35f + 0.35f * pulso) : Fade(BLACK, 0.45f);
        ui_painel(r, fundo, proximo ? UI_DESTAQUE : UI_BORDA);
        if (k >= h->combo_tamanho[i])
            continue;
        const Golpe *g = golpe_dados(h->combo[i][k]);
        if (g == NULL)
            continue;
        if (g->forte || g->agarra) {
            Color faixa = g->agarra ? COR_AGARRA : COR_FORTE;
            DrawRectangle((int)r.x + 2, (int)(r.y + r.height) - 5, (int)r.width - 4, 3, faixa);
        }
        int tam_fonte = 14;
        while (tam_fonte > 8 && MeasureText(g->nome, tam_fonte) > (int)r.width - 10)
            tam_fonte--;
        ui_texto_centro(g->nome, (int)(r.x + r.width / 2), (int)r.y + (int)(COMBO_A - tam_fonte) / 2,
                        tam_fonte, proximo ? UI_TEXTO : UI_TEXTO_FRACO);
    }
}

static void desenhar_jogador(const InfoHud *h, int i)
{
    Color cor = ui_cor_jogador(i);
    desenhar_fundo(i, cor);
    desenhar_titulo(h, i, cor);
    desenhar_vida(h, i);
    desenhar_dano_e_folego(h, i);
    desenhar_combo(h, i, cor);
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
