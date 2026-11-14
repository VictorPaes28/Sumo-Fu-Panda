/*
 * montagem.c — tela de montagem do combo.
 * Especificação: montagem.h (controles). Usa só as funções de combo.h.
 *
 * Dono: Integrante B.
 */
#include "montagem.h"
#include "config.h"
#include "ui.h"

void montagem_iniciar(TelaMontagem *t)
{
    *t = (TelaMontagem){0};
}

/* Aplica os comandos de um jogador no próprio combo. Devolve true se ele
 * pediu para voltar (B com o combo vazio). */
static bool atualizar_jogador(TelaMontagem *t, int i, Combo *combo, const Intencao *in)
{
    if (in->start) {
        if (t->pronto[i])
            t->pronto[i] = false;
        else if (combo->tamanho == COMBO_MAX)
            t->pronto[i] = true;
        return false;
    }
    if (t->pronto[i])
        return false;

    /* Cursores. O do catálogo dá a volta; o do combo para nas pontas. */
    t->cursor_catalogo[i] = ((t->cursor_catalogo[i] + in->direcional_y) % GOLPE_QTD + GOLPE_QTD) % GOLPE_QTD;
    t->cursor_combo[i] += in->direcional_x;
    if (t->cursor_combo[i] < 0) t->cursor_combo[i] = 0;
    if (t->cursor_combo[i] > COMBO_MAX - 1) t->cursor_combo[i] = COMBO_MAX - 1;

    int pos = t->cursor_combo[i];
    TipoGolpe escolhido = (TipoGolpe)t->cursor_catalogo[i];

    if (in->a) {
        if (combo->tamanho == COMBO_MAX) {
            combo_remover(combo, pos);                       /* cheio: substitui */
            combo_inserir(combo, pos, escolhido);
        } else {
            int onde = (pos > combo->tamanho) ? combo->tamanho : pos;
            if (combo_inserir(combo, onde, escolhido))
                t->cursor_combo[i] = (onde + 1 < COMBO_MAX) ? onde + 1 : onde;
        }
    }

    if (in->b) {
        if (combo->tamanho == 0)
            return true;
        if (pos < combo->tamanho)
            combo_remover(combo, pos);
        else
            combo_remover(combo, combo->tamanho - 1);       /* cursor no espaço vazio: tira o último */
        if (t->cursor_combo[i] > combo->tamanho)
            t->cursor_combo[i] = combo->tamanho;
    }

    if (in->y && pos < combo->tamanho && combo_trocar_com_proximo(combo, pos)) {
        combo_reiniciar(combo);                             /* reorganizou: o combo recomeça */
        t->cursor_combo[i] = (pos + 1) % combo->tamanho;   /* o cursor acompanha o golpe */
    }

    return false;
}

AcaoMontagem montagem_atualizar(TelaMontagem *t, Combo *combo_j1, Combo *combo_j2,
                                const Intencao in[2])
{
    Combo *combos[2] = { combo_j1, combo_j2 };
    for (int i = 0; i < 2; i++)
        if (atualizar_jogador(t, i, combos[i], &in[i]))
            return MONTAGEM_VOLTAR;
    return (t->pronto[0] && t->pronto[1]) ? MONTAGEM_CONCLUIDA : MONTAGEM_CONTINUA;
}

/* ------------------------------------------------------------ Desenho */

static void desenhar_lado(const TelaMontagem *t, int i, const Combo *combo)
{
    float x = (i == 0) ? 40.0f : (float)JANELA_LARGURA / 2 + 20.0f;
    float w = (float)JANELA_LARGURA / 2 - 60.0f;
    Color cor = ui_cor_jogador(i);
    int cx = (int)(x + w / 2);

    ui_painel((Rectangle){ x, 110, w, 560 }, UI_PAINEL, t->pronto[i] ? cor : UI_BORDA);
    ui_texto_centro(TextFormat("Jogador %d", i + 1), cx, 125, 28, cor);

    /* Catálogo. */
    for (int g = 0; g < GOLPE_QTD; g++) {
        const Golpe *d = golpe_dados((TipoGolpe)g);
        float y = 175.0f + g * 46.0f;
        bool sel = (g == t->cursor_catalogo[i]);
        if (sel)
            DrawRectangleRec((Rectangle){ x + 20, y - 4, w - 40, 40 }, Fade(cor, 0.25f));
        DrawText(d->nome, (int)x + 36, (int)y + 4, 24, sel ? UI_TEXTO : UI_TEXTO_FRACO);
        DrawText(TextFormat("dano %.0f%%  stamina %.0f", d->dano, d->custo_stamina),
                 (int)(x + w) - 250, (int)y + 8, 18, sel ? UI_DESTAQUE : UI_TEXTO_FRACO);
    }
    const Golpe *sel = golpe_dados((TipoGolpe)t->cursor_catalogo[i]);
    ui_texto_centro(sel->descricao, cx, 412, 20, UI_TEXTO);

    /* Espaços do combo. */
    float sw = (w - 60.0f) / COMBO_MAX;
    for (int k = 0; k < COMBO_MAX; k++) {
        Rectangle r = { x + 30 + k * sw, 455, sw - 10, 70 };
        bool cursor = (k == t->cursor_combo[i]) && !t->pronto[i];
        ui_painel(r, Fade(BLACK, 0.35f), cursor ? UI_DESTAQUE : UI_BORDA);
        DrawText(TextFormat("%d", k + 1), (int)r.x + 6, (int)r.y + 4, 16, UI_TEXTO_FRACO);
        TipoGolpe gk = combo_golpe_em(combo, k);
        if (gk != GOLPE_QTD)
            ui_texto_centro(golpe_dados(gk)->nome, (int)(r.x + r.width / 2), (int)r.y + 30, 18, UI_TEXTO);
        if (k < COMBO_MAX - 1)
            DrawText(">", (int)(r.x + r.width + 1), (int)r.y + 26, 16, UI_TEXTO_FRACO);
    }
    DrawText("(depois do 4º, volta ao 1º)", (int)x + 30, 532, 16, UI_TEXTO_FRACO);

    if (t->pronto[i]) {
        ui_texto_centro("PRONTO!", cx, 575, 40, cor);
        ui_texto_centro("Start: desfazer", cx, 625, 18, UI_TEXTO_FRACO);
    } else if (combo->tamanho < COMBO_MAX) {
        int faltam = COMBO_MAX - combo->tamanho;
        ui_texto_centro(faltam == 1 ? "Falta 1 golpe" : TextFormat("Faltam %d golpes", faltam),
                        cx, 575, 26, UI_TEXTO);
        ui_texto_centro("cima/baixo: golpe  esq/dir: espaço  A: pôr  B: tirar  Y: trocar",
                        cx, 620, 16, UI_TEXTO_FRACO);
    } else {
        ui_texto_centro("Start: pronto!", cx, 575, 30, UI_DESTAQUE);
        ui_texto_centro("A: substituir  B: tirar  Y: trocar com o próximo", cx, 620, 16, UI_TEXTO_FRACO);
    }
}

void montagem_desenhar(const TelaMontagem *t, const Combo *combo_j1, const Combo *combo_j2)
{
    ClearBackground(UI_FUNDO);
    ui_texto_centro("Montem seus combos", JANELA_LARGURA / 2, 40, 44, UI_TEXTO);
    desenhar_lado(t, 0, combo_j1);
    desenhar_lado(t, 1, combo_j2);
}
