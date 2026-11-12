/*
 * montagem.c — tela de montagem do combo.
 * Especificação: montagem.h (controles). Usa só as funções de combo.h.
 *
 * Dono: Integrante B.
 */
#include <stdio.h>
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

    if (in->y && pos < combo->tamanho && combo_trocar_com_proximo(combo, pos))
        t->cursor_combo[i] = (pos + 1) % combo->tamanho;   /* o cursor acompanha o golpe */

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

/* Medidas da tela (pixels e tamanhos de fonte). Quantos golpes aparecem de
 * uma vez fica em config.h (MONTAGEM_GOLPES_VISIVEIS). */
static const float MON_PAINEL_Y          = 100.0f;
static const float MON_PAINEL_ALTURA     = 610.0f;
static const float MON_LISTA_Y           = 160.0f;  /* topo da primeira linha da lista */
static const float MON_LINHA_ALTURA      = 34.0f;
static const float MON_LINHA_MARGEM      = 16.0f;   /* da borda do painel até a linha */
static const float MON_SETA_LARGURA      = 16.0f;   /* setas de "tem mais golpes" */
static const float MON_SETA_ALTURA       = 10.0f;
static const float MON_DESCRICAO_ALTURA  = 76.0f;   /* caixa fixa: cabem 3 linhas */
static const float MON_ESPACO            = 10.0f;   /* entre os blocos da tela */
static const float MON_ESPACO_COMBO      = 70.0f;   /* altura dos espaços do combo */
static const int   MON_FONTE_NOME        = 20;
static const int   MON_FONTE_FICHA       = 16;
static const int   MON_FONTE_SELO        = 12;
static const int   MON_FONTE_DESCRICAO   = 20;
static const int   MON_FONTE_ESPACO      = 18;      /* nome do golpe num espaço do combo */
static const int   MON_FONTE_MINIMA      = 12;
static const Color MON_COR_FORTE         = { 214, 84, 64, 255 };   /* selo "FORTE" */
static const Color MON_COR_AGARRA        = { 64, 170, 140, 255 };  /* selo "FAIXA" */

/* Maior fonte (até "maximo") em que o texto cabe na largura: texto comprido
 * encolhe em vez de sair do lugar. */
static int fonte_que_cabe(const char *texto, int maximo, float largura)
{
    int tamanho = maximo;
    while (tamanho > MON_FONTE_MINIMA && MeasureText(texto, tamanho) > (int)largura)
        tamanho--;
    return tamanho;
}

/* Primeiro golpe da janela visível da lista. A janela fica centrada no
 * cursor e encosta nas pontas: assim ela acompanha o cursor nas duas
 * direções, inclusive quando ele dá a volta do último para o primeiro, sem
 * guardar nada a mais no estado da tela. */
static int primeiro_visivel(int cursor, int qtd, int visiveis)
{
    if (qtd <= visiveis)
        return 0;
    int primeiro = cursor - visiveis / 2;
    if (primeiro < 0)
        primeiro = 0;
    if (primeiro > qtd - visiveis)
        primeiro = qtd - visiveis;
    return primeiro;
}

/* Triângulo apontando para cima ou para baixo, centrado em cx. Desenhado
 * (e não escrito): a fonte padrão da raylib não tem os caracteres ▲ ▼. */
static void desenhar_seta(float cx, float y, bool para_cima, Color cor)
{
    float meia = MON_SETA_LARGURA / 2.0f;
    if (para_cima)
        DrawTriangle((Vector2){ cx, y }, (Vector2){ cx - meia, y + MON_SETA_ALTURA },
                     (Vector2){ cx + meia, y + MON_SETA_ALTURA }, cor);
    else
        DrawTriangle((Vector2){ cx - meia, y }, (Vector2){ cx, y + MON_SETA_ALTURA },
                     (Vector2){ cx + meia, y }, cor);
}

/* Selo pequeno ("FORTE", "FAIXA") com o centro vertical em cy. Devolve a
 * largura, para o próximo selo vir ao lado. */
static float desenhar_selo(const char *texto, float x, float cy, Color cor)
{
    float w = (float)MeasureText(texto, MON_FONTE_SELO) + 10.0f;
    float h = (float)MON_FONTE_SELO + 4.0f;
    DrawRectangleRounded((Rectangle){ x, cy - h / 2.0f, w, h }, 0.5f, 6, cor);
    DrawText(texto, (int)x + 5, (int)(cy - MON_FONTE_SELO / 2.0f), MON_FONTE_SELO, UI_FUNDO);
    return w;
}

/* Uma linha da lista: nome, selos e a ficha curta (dano e stamina) à direita. */
static void desenhar_linha_golpe(Rectangle r, const Golpe *d, bool selecionado, Color cor)
{
    if (selecionado)
        DrawRectangleRec(r, Fade(cor, 0.25f));

    float cy = r.y + r.height / 2.0f;
    int x = (int)r.x + 12;
    DrawText(d->nome, x, (int)(cy - MON_FONTE_NOME / 2.0f), MON_FONTE_NOME,
             selecionado ? UI_TEXTO : UI_TEXTO_FRACO);

    float x_selo = (float)(x + MeasureText(d->nome, MON_FONTE_NOME) + 10);
    if (d->forte)
        x_selo += desenhar_selo("FORTE", x_selo, cy, MON_COR_FORTE) + 6.0f;
    if (d->agarra)
        desenhar_selo("FAIXA", x_selo, cy, MON_COR_AGARRA);

    const char *ficha = TextFormat("dano %.0f%%  stamina %.0f", d->dano, d->custo_stamina);
    DrawText(ficha, (int)(r.x + r.width) - 12 - MeasureText(ficha, MON_FONTE_FICHA),
             (int)(cy - MON_FONTE_FICHA / 2.0f), MON_FONTE_FICHA,
             selecionado ? UI_DESTAQUE : UI_TEXTO_FRACO);
}

/* Lista com rolagem: só os golpes que cabem, com setas quando há mais
 * golpes acima ou abaixo. A quantidade vem de GOLPE_QTD. */
static void desenhar_catalogo(float x, float w, int cursor, Color cor)
{
    int visiveis = (GOLPE_QTD < MONTAGEM_GOLPES_VISIVEIS) ? GOLPE_QTD : MONTAGEM_GOLPES_VISIVEIS;
    int primeiro = primeiro_visivel(cursor, GOLPE_QTD, visiveis);

    for (int k = 0; k < visiveis; k++) {
        int g = primeiro + k;
        Rectangle linha = { x + MON_LINHA_MARGEM, MON_LISTA_Y + k * MON_LINHA_ALTURA,
                            w - 2.0f * MON_LINHA_MARGEM, MON_LINHA_ALTURA };
        desenhar_linha_golpe(linha, golpe_dados((TipoGolpe)g), g == cursor, cor);
    }

    float cx = x + w / 2.0f;
    if (primeiro > 0)
        desenhar_seta(cx, MON_LISTA_Y - MON_SETA_ALTURA - 4.0f, true, UI_DESTAQUE);
    if (primeiro + visiveis < GOLPE_QTD)
        desenhar_seta(cx, MON_LISTA_Y + visiveis * MON_LINHA_ALTURA + 4.0f, false, UI_DESTAQUE);
}

/* Quebra o texto em linhas que cabem na largura (por palavras) e, se
 * "desenhar", desenha a partir de (x, y). Devolve quantas linhas deu.
 * A mesma função mede e desenha, para as duas coisas nunca discordarem. */
static int quebrar_texto(const char *texto, float largura, int tamanho, bool desenhar,
                         int x, int y, Color cor)
{
    char linha[256] = "";
    char tentativa[256];
    int linhas = 0;
    const char *p = texto;

    while (*p != '\0') {
        while (*p == ' ')
            p++;
        const char *fim = p;
        while (*fim != '\0' && *fim != ' ')
            fim++;
        if (fim == p)
            break;

        int escrito = snprintf(tentativa, sizeof tentativa, "%s%s%.*s",
                               linha, (linha[0] != '\0') ? " " : "", (int)(fim - p), p);
        /* Não cabe na largura (ou nem no buffer): a palavra vai para a próxima linha. */
        bool nao_cabe = escrito >= (int)sizeof tentativa ||
                        MeasureText(tentativa, tamanho) > (int)largura;
        if (linha[0] != '\0' && nao_cabe) {
            /* A palavra não cabe: fecha a linha atual e ela começa a próxima. */
            if (desenhar)
                DrawText(linha, x, y + linhas * (tamanho + 4), tamanho, cor);
            linhas++;
            snprintf(linha, sizeof linha, "%.*s", (int)(fim - p), p);
        } else {
            snprintf(linha, sizeof linha, "%s", tentativa);
        }
        p = fim;
    }
    if (linha[0] != '\0') {
        if (desenhar)
            DrawText(linha, x, y + linhas * (tamanho + 4), tamanho, cor);
        linhas++;
    }
    return linhas;
}

/* Descrição do golpe numa caixa de tamanho fixo. Se ela for longa demais
 * para a fonte normal, a fonte diminui até caber inteira. */
static void desenhar_descricao(Rectangle caixa, const char *descricao)
{
    ui_painel(caixa, Fade(BLACK, 0.30f), UI_BORDA);
    float largura = caixa.width - 20.0f;
    float altura = caixa.height - 12.0f;
    int tamanho = MON_FONTE_DESCRICAO;
    while (tamanho > MON_FONTE_MINIMA &&
           quebrar_texto(descricao, largura, tamanho, false, 0, 0, UI_TEXTO) * (tamanho + 4) > altura)
        tamanho--;
    quebrar_texto(descricao, largura, tamanho, true, (int)caixa.x + 10, (int)caixa.y + 8, UI_TEXTO);
}

/* Os COMBO_MAX espaços do combo, a partir de y. */
static void desenhar_espacos_combo(const TelaMontagem *t, int i, const Combo *combo,
                                   float x, float w, float y)
{
    float sw = (w - 60.0f) / COMBO_MAX;
    for (int k = 0; k < COMBO_MAX; k++) {
        Rectangle r = { x + 30 + k * sw, y, sw - 10, MON_ESPACO_COMBO };
        bool cursor = (k == t->cursor_combo[i]) && !t->pronto[i];
        ui_painel(r, Fade(BLACK, 0.35f), cursor ? UI_DESTAQUE : UI_BORDA);
        DrawText(TextFormat("%d", k + 1), (int)r.x + 6, (int)r.y + 4, 16, UI_TEXTO_FRACO);
        TipoGolpe gk = combo_golpe_em(combo, k);
        if (gk != GOLPE_QTD) {
            /* Nomes longos ("Projeção baixo") encolhem para não passar do espaço. */
            const char *nome = golpe_dados(gk)->nome;
            ui_texto_centro(nome, (int)(r.x + r.width / 2), (int)r.y + 30,
                            fonte_que_cabe(nome, MON_FONTE_ESPACO, r.width - 8.0f), UI_TEXTO);
        }
        if (k < COMBO_MAX - 1)
            DrawText(">", (int)(r.x + r.width + 1), (int)r.y + 26, 16, UI_TEXTO_FRACO);
    }
    DrawText("(depois do 4º, volta ao 1º)", (int)x + 30, (int)(y + MON_ESPACO_COMBO + 6), 16,
             UI_TEXTO_FRACO);
}

/* Linha de ajuda dos botões: encolhe para caber na largura do painel. */
static void desenhar_ajuda(const char *texto, int cx, int y, float largura)
{
    ui_texto_centro(texto, cx, y, fonte_que_cabe(texto, MON_FONTE_FICHA, largura), UI_TEXTO_FRACO);
}

/* Situação do jogador e os botões que valem agora, a partir de y. */
static void desenhar_rodape(const TelaMontagem *t, int i, const Combo *combo, int cx, int y,
                           float largura)
{
    Color cor = ui_cor_jogador(i);
    if (t->pronto[i]) {
        ui_texto_centro("PRONTO!", cx, y - 4, 40, cor);
        ui_texto_centro("Start: desfazer", cx, y + 44, 18, UI_TEXTO_FRACO);
    } else if (combo->tamanho < COMBO_MAX) {
        int faltam = COMBO_MAX - combo->tamanho;
        ui_texto_centro(faltam == 1 ? "Falta 1 golpe" : TextFormat("Faltam %d golpes", faltam),
                        cx, y, 26, UI_TEXTO);
        desenhar_ajuda("cima/baixo: golpe  esq/dir: espaço  A: pôr  B: tirar  Y: trocar",
                       cx, y + 44, largura);
    } else {
        ui_texto_centro("Start: pronto!", cx, y, 30, UI_DESTAQUE);
        desenhar_ajuda("A: substituir  B: tirar  Y: trocar com o próximo", cx, y + 44, largura);
    }
}

/* Um lado da tela, de cima para baixo: lista, descrição, combo e rodapé.
 * Cada bloco começa onde o anterior termina, então nada se sobrepõe. */
static void desenhar_lado(const TelaMontagem *t, int i, const Combo *combo)
{
    float x = (i == 0) ? 40.0f : (float)JANELA_LARGURA / 2 + 20.0f;
    float w = (float)JANELA_LARGURA / 2 - 60.0f;
    Color cor = ui_cor_jogador(i);
    int cx = (int)(x + w / 2);

    ui_painel((Rectangle){ x, MON_PAINEL_Y, w, MON_PAINEL_ALTURA }, UI_PAINEL,
              t->pronto[i] ? cor : UI_BORDA);
    ui_texto_centro(TextFormat("Jogador %d", i + 1), cx, (int)MON_PAINEL_Y + 10, 28, cor);

    desenhar_catalogo(x, w, t->cursor_catalogo[i], cor);

    /* A caixa fica abaixo do espaço da lista CHEIA (e da seta de baixo),
     * mesmo com menos golpes: assim ela não pula de lugar. */
    float y = MON_LISTA_Y + MONTAGEM_GOLPES_VISIVEIS * MON_LINHA_ALTURA + MON_SETA_ALTURA + MON_ESPACO;
    Rectangle caixa = { x + MON_LINHA_MARGEM, y, w - 2.0f * MON_LINHA_MARGEM, MON_DESCRICAO_ALTURA };
    desenhar_descricao(caixa, golpe_dados((TipoGolpe)t->cursor_catalogo[i])->descricao);
    y += MON_DESCRICAO_ALTURA + MON_ESPACO;

    desenhar_espacos_combo(t, i, combo, x, w, y);
    y += MON_ESPACO_COMBO + 32.0f;   /* espaços + a linha "(depois do 4º...)" */

    desenhar_rodape(t, i, combo, cx, (int)y, w - 2.0f * MON_LINHA_MARGEM);
}

void montagem_desenhar(const TelaMontagem *t, const Combo *combo_j1, const Combo *combo_j2)
{
    ClearBackground(UI_FUNDO);
    ui_texto_centro("Montem seus combos", JANELA_LARGURA / 2, 40, 44, UI_TEXTO);
    desenhar_lado(t, 0, combo_j1);
    desenhar_lado(t, 1, combo_j2);
}
