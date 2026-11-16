/*
 * montagem.c — tela de montagem do combo.
 * Especificação: montagem.h (controles). Usa só as funções de combo.h.
 *
 * Dono: Integrante B.
 */
#include <math.h>
#include <stdio.h>
#include "montagem.h"
#include "config.h"
#include "ui.h"
#include "raymath.h"

static const TipoGolpe GOLPES_DISPONIVEIS[] = {
    GOLPE_LEVANTADOR, GOLPE_TAPA_LATERAL, GOLPE_TAPA_BAIXO,
    GOLPE_BARRIGADA, GOLPE_INVESTIDA, GOLPE_EMPURRAR_FORA,
    GOLPE_FORCAR_FORA, GOLPE_PROJECAO_CIMA, GOLPE_PROJECAO_BAIXO
};
#define QTD_DISPONIVEIS ((int)(sizeof GOLPES_DISPONIVEIS / sizeof GOLPES_DISPONIVEIS[0]))

/* Consultas usadas também nas prévias 3D do executável. */
int montagem_catalogo_quantidade(void)
{
    return QTD_DISPONIVEIS;
}

TipoGolpe montagem_catalogo_golpe(int indice)
{
    return indice >= 0 && indice < QTD_DISPONIVEIS ? GOLPES_DISPONIVEIS[indice] : GOLPE_QTD;
}

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
    t->cursor_catalogo[i] = ((t->cursor_catalogo[i] + in->direcional_y) % QTD_DISPONIVEIS + QTD_DISPONIVEIS) % QTD_DISPONIVEIS;
    t->cursor_combo[i] += in->direcional_x;
    if (t->cursor_combo[i] < 0) t->cursor_combo[i] = 0;
    if (t->cursor_combo[i] > COMBO_MAX - 1) t->cursor_combo[i] = COMBO_MAX - 1;

    int pos = t->cursor_combo[i];
    TipoGolpe escolhido = montagem_catalogo_golpe(t->cursor_catalogo[i]);

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

/* Medidas da tela (pixels e tamanhos de fonte). Quantos golpes aparecem de
 * uma vez fica em config.h (MONTAGEM_GOLPES_VISIVEIS). */
static const float MON_PAINEL_Y          = 100.0f;
static const float MON_PAINEL_ALTURA     = 610.0f;
static const float MON_LISTA_Y           = 160.0f;  /* topo da primeira linha da lista */
static const float MON_LINHA_ALTURA      = 34.0f;
static const float MON_LINHA_MARGEM      = 16.0f;   /* da borda do painel até a linha */
static const float MON_SETA_LARGURA      = 16.0f;   /* setas de "tem mais golpes" */
static const float MON_SETA_ALTURA       = 10.0f;
static const float MON_CATALOGO_LARGURA  = 220.0f;
static const float MON_COLUNAS_ESPACO    = 14.0f;
static const float MON_PREVIA_ALTURA     = 327.0f;
static const float MON_ESPACO_COMBO      = 70.0f;   /* altura dos espaços do combo */
static const int   MON_FONTE_NOME        = 20;
static const int   MON_FONTE_FICHA       = 16;
static const int   MON_FONTE_DESCRICAO   = 20;
static const int   MON_FONTE_ESPACO      = 18;      /* nome do golpe num espaço do combo */
static const int   MON_FONTE_MINIMA      = 12;
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

/* A ficha completa fica ao lado da lista. */
static void desenhar_linha_golpe(Rectangle r, const Golpe *d, bool selecionado, Color cor)
{
    ui_painel(r, selecionado ? Fade(cor, 0.25f) : Fade(BLACK, 0.20f),
              selecionado ? cor : UI_BORDA);
    int fonte = fonte_que_cabe(d->nome, MON_FONTE_NOME, r.width - 20.0f);
    DrawText(d->nome, (int)r.x + 10, (int)(r.y + (r.height - fonte) / 2), fonte,
             selecionado ? UI_TEXTO : UI_TEXTO_FRACO);
}

/* Lista com rolagem: só os golpes que cabem, com setas quando há mais
 * golpes acima ou abaixo no catálogo disponível. */
static void desenhar_catalogo(float x, float w, int cursor, Color cor)
{
    int visiveis = (QTD_DISPONIVEIS < MONTAGEM_GOLPES_VISIVEIS) ? QTD_DISPONIVEIS : MONTAGEM_GOLPES_VISIVEIS;
    int primeiro = primeiro_visivel(cursor, QTD_DISPONIVEIS, visiveis);

    for (int k = 0; k < visiveis; k++) {
        int g = primeiro + k;
        Rectangle linha = { x, MON_LISTA_Y + k * MON_LINHA_ALTURA,
                            w, MON_LINHA_ALTURA - 4.0f };
        desenhar_linha_golpe(linha, golpe_dados(montagem_catalogo_golpe(g)), g == cursor, cor);
    }

    float cx = x + w / 2.0f;
    if (primeiro > 0)
        desenhar_seta(cx, MON_LISTA_Y - MON_SETA_ALTURA - 4.0f, true, UI_DESTAQUE);
    if (primeiro + visiveis < QTD_DISPONIVEIS)
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

static void desenhar_previa(Rectangle caixa, const Golpe *g, float tempo, Texture2D imagem, Color cor)
{
    float fim_ativo = g->tempo_preparo + g->tempo_ativo;
    bool ativo = tempo >= g->tempo_preparo && tempo < fim_ativo;
    const char *fase = tempo < g->tempo_preparo ? "Preparo"
                     : ativo ? "Ataque" : tempo < g->duracao ? "Volta" : "Guarda";

    ui_painel(caixa, Fade(BLACK, 0.30f), UI_BORDA);
    int x = (int)caixa.x + 12;
    int y = (int)caixa.y;
    float largura = caixa.width - 24.0f;
    DrawText(g->nome, x, y + 12, fonte_que_cabe(g->nome, 25, largura), UI_TEXTO);
    if (imagem.id != 0)
        DrawText(fase, x, y + 45, MON_FONTE_FICHA, ativo ? UI_DESTAQUE : cor);

    Rectangle area = { (float)x, caixa.y + 66.0f, largura, 139.0f };
    DrawRectangleRounded(area, 0.12f, 6, Fade(cor, 0.11f));
    if (imagem.id != 0) {
        float escala = fminf(largura / imagem.width, area.height / imagem.height);
        float w = imagem.width * escala, h = imagem.height * escala;
        Rectangle origem = { 0, 0, (float)imagem.width, -(float)imagem.height };
        Rectangle destino = { area.x + (area.width - w) / 2, area.y + (area.height - h) / 2, w, h };
        DrawTexturePro(imagem, origem, destino, (Vector2){0}, 0.0f, WHITE);
    }

    Rectangle barra = { (float)x, caixa.y + 212.0f, largura, 6.0f };
    DrawRectangleRec(barra, UI_BORDA);
    float inicio = barra.width * g->tempo_preparo / g->duracao;
    float janela = barra.width * g->tempo_ativo / g->duracao;
    DrawRectangleRec((Rectangle){ barra.x + inicio, barra.y, janela, barra.height },
                     Fade(UI_DESTAQUE, 0.65f));
    DrawRectangle((int)(barra.x + Clamp(tempo / g->duracao, 0.0f, 1.0f) * (barra.width - 2)),
                  (int)barra.y - 2, 2, 10, UI_TEXTO);

    DrawText(TextFormat("Dano %.0f%%       Stamina %.0f", g->dano, g->custo_stamina),
             x, y + 229, 18, UI_TEXTO);
    const char *tipo = g->forte && g->agarra ? "Forte · Pegada na faixa"
                     : g->forte ? "Golpe forte" : g->agarra ? "Pegada na faixa" : "";
    const char *efeito = g->impulso_vertical >= 80.0f ? "Ergue o alvo"
                       : g->impulso_vertical <= -80.0f ? "Força para baixo"
                       : fabsf(g->impulso_lateral) >= fabsf(g->impulso) && g->impulso_lateral != 0.0f ? "Empurra de lado"
                       : g->impulso < 0.0f ? "Puxa o alvo" : "Empurra o alvo";
    const char *rotulo = tipo[0] ? TextFormat("%s  |  %s", efeito, tipo) : efeito;
    DrawText(rotulo, x, y + 253, fonte_que_cabe(rotulo, MON_FONTE_FICHA, largura),
             g->agarra ? MON_COR_AGARRA : UI_DESTAQUE);
    int fonte = MON_FONTE_DESCRICAO;
    while (fonte > MON_FONTE_MINIMA &&
           quebrar_texto(g->descricao, largura, fonte, false, 0, 0, UI_TEXTO) * (fonte + 4) > 48)
        fonte--;
    quebrar_texto(g->descricao, largura, fonte, true, x, y + 273, UI_TEXTO_FRACO);
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

/* Catálogo e prévia lado a lado, com o combo e os controles abaixo. */
static void desenhar_lado(const TelaMontagem *t, int i, const Combo *combo, float tempo, Texture2D imagem)
{
    float x = (i == 0) ? 40.0f : (float)JANELA_LARGURA / 2 + 20.0f;
    float w = (float)JANELA_LARGURA / 2 - 60.0f;
    Color cor = ui_cor_jogador(i);
    int cx = (int)(x + w / 2);

    ui_painel((Rectangle){ x, MON_PAINEL_Y, w, MON_PAINEL_ALTURA }, UI_PAINEL,
              t->pronto[i] ? cor : UI_BORDA);
    ui_texto_centro(TextFormat("Jogador %d", i + 1), cx, (int)MON_PAINEL_Y + 10, 28, cor);

    float inicio = x + MON_LINHA_MARGEM;
    desenhar_catalogo(inicio, MON_CATALOGO_LARGURA, t->cursor_catalogo[i], cor);
    float previa_x = inicio + MON_CATALOGO_LARGURA + MON_COLUNAS_ESPACO;
    Rectangle previa = { previa_x, MON_LISTA_Y, x + w - MON_LINHA_MARGEM - previa_x,
                          MON_PREVIA_ALTURA };
    desenhar_previa(previa, golpe_dados(montagem_catalogo_golpe(t->cursor_catalogo[i])), tempo, imagem, cor);
    float y = previa.y + previa.height + 14.0f;

    desenhar_espacos_combo(t, i, combo, x, w, y);
    y += MON_ESPACO_COMBO + 32.0f;   /* espaços + a linha "(depois do 4º...)" */

    desenhar_rodape(t, i, combo, cx, (int)y, w - 2.0f * MON_LINHA_MARGEM);
}

/* Entrada usada pelo jogo com as texturas renderizadas pelo main. */
void montagem_desenhar_com_previas(const TelaMontagem *t, const Combo *combo_j1,
                                  const Combo *combo_j2, const float tempo[2],
                                  const Texture2D imagem[2])
{
    ClearBackground(UI_FUNDO);
    ui_texto_centro("Montem seus combos", JANELA_LARGURA / 2, 40, 44, UI_TEXTO);
    desenhar_lado(t, 0, combo_j1, tempo[0], imagem[0]);
    desenhar_lado(t, 1, combo_j2, tempo[1], imagem[1]);
}

void montagem_desenhar(const TelaMontagem *t, const Combo *combo_j1, const Combo *combo_j2)
{
    const float tempo[2] = {0};
    const Texture2D imagem[2] = {0};
    montagem_desenhar_com_previas(t, combo_j1, combo_j2, tempo, imagem);
}
