/*
 * telas.c — menu, seleção de personagem e resultado.
 * Especificação: telas.h
 *
 * Dono: Integrante B.
 */
#include <stddef.h>
#include "telas.h"
#include "config.h"
#include "personagem.h"
#include "ui.h"

#define CENTRO_X  (JANELA_LARGURA / 2)

/* Soma -1/0/+1 com volta: de 0 para trás vai para qtd-1. */
static int girar_indice(int atual, int passo, int qtd)
{
    if (qtd <= 0)
        return 0;
    return ((atual + passo) % qtd + qtd) % qtd;
}

/* Desenha uma lista vertical de opções com a escolhida em destaque. */
static void desenhar_opcoes(const char *const *opcoes, int qtd, int escolhida, int y0)
{
    for (int i = 0; i < qtd; i++) {
        int y = y0 + i * 60;
        if (i == escolhida) {
            int w = MeasureText(opcoes[i], 36) + 60;
            ui_painel((Rectangle){ (float)(CENTRO_X - w / 2), (float)(y - 8), (float)w, 52.0f },
                      UI_PAINEL, UI_DESTAQUE);
            ui_texto_centro(opcoes[i], CENTRO_X, y, 36, UI_DESTAQUE);
        } else {
            ui_texto_centro(opcoes[i], CENTRO_X, y, 32, UI_TEXTO_FRACO);
        }
    }
}

/* ------------------------------------------------------------------ Menu */

static const char *const OPCOES_MENU[] = { "Jogar", "Sair" };
#define QTD_OPCOES_MENU 2

void menu_iniciar(TelaMenu *t)
{
    t->opcao = 0;
}

AcaoMenu menu_atualizar(TelaMenu *t, const Intencao in[2])
{
    for (int i = 0; i < 2; i++) {
        t->opcao = girar_indice(t->opcao, in[i].direcional_y, QTD_OPCOES_MENU);
        if (in[i].a)
            return (t->opcao == 0) ? MENU_JOGAR : MENU_SAIR;
    }
    return MENU_NENHUMA;
}

void menu_desenhar(const TelaMenu *t)
{
    ClearBackground(UI_FUNDO);
    ui_texto_centro(NOME_JOGO, CENTRO_X, 150, 84, UI_DESTAQUE);
    ui_texto_centro("Sumô com física, no Palácio de Jade", CENTRO_X, 250, 26, UI_TEXTO);
    desenhar_opcoes(OPCOES_MENU, QTD_OPCOES_MENU, t->opcao, 380);
    ui_texto_centro("Controle: D-pad escolhe, A confirma   |   Teclado: W/S e C (J1), setas e K (J2)",
                    CENTRO_X, JANELA_ALTURA - 50, 18, UI_TEXTO_FRACO);
}

/* ------------------------------------------------------------- Seleção */

void selecao_iniciar(TelaSelecao *t)
{
    *t = (TelaSelecao){0};
    if (personagem_quantidade() > 1)
        t->escolha[1] = 1;           /* começa com personagens diferentes */
}

AcaoSelecao selecao_atualizar(TelaSelecao *t, const Intencao in[2])
{
    int qtd = personagem_quantidade();
    for (int i = 0; i < 2; i++) {
        if (!t->pronto[i])
            t->escolha[i] = girar_indice(t->escolha[i], in[i].direcional_x, qtd);
        if (in[i].a && qtd > 0)
            t->pronto[i] = true;
        if (in[i].b) {
            if (t->pronto[i])
                t->pronto[i] = false;
            else
                return SELECAO_VOLTAR;
        }
    }
    return (t->pronto[0] && t->pronto[1]) ? SELECAO_CONCLUIDA : SELECAO_CONTINUA;
}

static float massa_total(const Personagem *p)
{
    float m = 0.0f;
    for (int i = 0; i < PARTE_QTD; i++)
        m += p->massa[i];
    return m;
}

void selecao_desenhar(const TelaSelecao *t)
{
    ClearBackground(UI_FUNDO);
    ui_texto_centro("Escolham seus lutadores", CENTRO_X, 50, 44, UI_TEXTO);

    for (int i = 0; i < 2; i++) {
        float x = (i == 0) ? 80.0f : (float)CENTRO_X + 40.0f;
        float w = (float)CENTRO_X - 120.0f;
        Color cor = ui_cor_jogador(i);
        Rectangle painel = { x, 140.0f, w, 440.0f };
        ui_painel(painel, UI_PAINEL, t->pronto[i] ? cor : UI_BORDA);

        int cx = (int)(x + w / 2);
        ui_texto_centro(TextFormat("Jogador %d", i + 1), cx, 165, 26, cor);

        const Personagem *p = personagem_dados(t->escolha[i]);
        if (p == NULL) {
            ui_texto_centro("Nenhum personagem", cx, 300, 30, UI_TEXTO_FRACO);
            continue;
        }
        ui_texto_centro(TextFormat("<  %s  >", p->nome), cx, 250, 52, UI_TEXTO);

        /* Barras comparativas: peso e velocidade (escalas fixas para comparar). */
        DrawText("Peso", (int)x + 40, 360, 22, UI_TEXTO_FRACO);
        ui_barra((Rectangle){ x + 170, 362, w - 220, 18 }, massa_total(p) / 110.0f, cor);
        DrawText("Velocidade", (int)x + 40, 400, 22, UI_TEXTO_FRACO);
        ui_barra((Rectangle){ x + 170, 402, w - 220, 18 }, p->velocidade_max / 2.0f, cor);
        DrawText("Equilíbrio", (int)x + 40, 440, 22, UI_TEXTO_FRACO);
        ui_barra((Rectangle){ x + 170, 442, w - 220, 18 }, p->forca_equilibrio / 1000.0f, cor);

        if (t->pronto[i])
            ui_texto_centro("PRONTO!", cx, 510, 36, cor);
        else
            ui_texto_centro("A: confirmar   B: voltar", cx, 515, 22, UI_TEXTO_FRACO);
    }
}

/* ------------------------------------------------------------ Resultado */

static const char *const OPCOES_RESULTADO[] = { "Revanche", "Remontar combos", "Menu" };
#define QTD_OPCOES_RESULTADO 3

void resultado_iniciar(TelaResultado *t, int vencedor, const char *nome_vencedor)
{
    t->vencedor = vencedor;
    t->nome_vencedor = nome_vencedor;
    t->opcao = 0;
}

AcaoResultado resultado_atualizar(TelaResultado *t, const Intencao in[2])
{
    for (int i = 0; i < 2; i++) {
        t->opcao = girar_indice(t->opcao, in[i].direcional_y, QTD_OPCOES_RESULTADO);
        if (in[i].a) {
            if (t->opcao == 0) return RESULTADO_REVANCHE;
            if (t->opcao == 1) return RESULTADO_REMONTAR;
            return RESULTADO_MENU;
        }
    }
    return RESULTADO_CONTINUA;
}

void resultado_desenhar(const TelaResultado *t)
{
    ClearBackground(UI_FUNDO);
    Color cor = ui_cor_jogador(t->vencedor < 0 ? 0 : t->vencedor);
    ui_texto_centro(TextFormat("Vitória de %s!", t->nome_vencedor ? t->nome_vencedor : "?"),
                    CENTRO_X, 150, 72, cor);
    ui_texto_centro(TextFormat("Jogador %d venceu a partida", t->vencedor + 1), CENTRO_X, 245, 28,
                    UI_TEXTO);
    desenhar_opcoes(OPCOES_RESULTADO, QTD_OPCOES_RESULTADO, t->opcao, 360);
}
