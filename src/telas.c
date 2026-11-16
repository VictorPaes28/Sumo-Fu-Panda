/*
 * telas.c — menu, seleção de personagem e resultado.
 * Especificação: telas.h
 *
 * Dono: Integrante B.
 */
#include <stddef.h>
#include <string.h>
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

/* Medidas e cores da tela inicial. */
static const float MENU_LUTADOR_L     = 330.0f;  /* área de cada lutador 3D */
static const float MENU_LUTADOR_A     = 480.0f;
static const float MENU_LUTADOR_Y     = 196.0f;
static const float MENU_LUTADOR_BORDA = 40.0f;   /* da borda da tela */
static const float MENU_BOTAO_L       = 300.0f;
static const float MENU_BOTAO_A       = 58.0f;
static const float MENU_BOTAO_Y       = 400.0f;
static const float MENU_BOTAO_PASSO   = 74.0f;
static const Color MENU_CEU_CIMA      = { 26, 18, 34, 255 };
static const Color MENU_CEU_BAIXO     = { 86, 44, 36, 255 };
static const Color MENU_ARGILA        = { 196, 158, 110, 255 };
static const Color MENU_PALHA         = { 186, 164, 98, 255 };

Rectangle menu_area_lutador(int lado)
{
    float x = (lado == 0) ? MENU_LUTADOR_BORDA
                          : JANELA_LARGURA - MENU_LUTADOR_BORDA - MENU_LUTADOR_L;
    return (Rectangle){ x, MENU_LUTADOR_Y, MENU_LUTADOR_L, MENU_LUTADOR_A };
}

/* Fundo: céu em degradê e, embaixo, o ringue (dohyô) visto de lado: a argila
 * clara e o anel de palha. */
static void desenhar_fundo_menu(void)
{
    DrawRectangleGradientV(0, 0, JANELA_LARGURA, JANELA_ALTURA, MENU_CEU_CIMA, MENU_CEU_BAIXO);
    Vector2 centro = { (float)CENTRO_X, JANELA_ALTURA - 40.0f };
    DrawEllipse((int)centro.x, (int)centro.y, 640.0f, 120.0f, Fade(MENU_ARGILA, 0.85f));
    for (int k = 0; k < 6; k++)               /* anel de palha, grosso */
        DrawEllipseLines((int)centro.x, (int)centro.y, 560.0f - k, 98.0f - k * 0.3f, MENU_PALHA);
    DrawEllipse((int)centro.x, (int)centro.y, 420.0f, 70.0f, Fade(WHITE, 0.08f));
}

/* Título grande com contorno escuro (desenhado deslocado em volta) e sombra. */
static void desenhar_titulo_menu(void)
{
    const int tamanho = 96, y = 96;
    for (int dx = -3; dx <= 3; dx += 3)
        for (int dy = -3; dy <= 3; dy += 3)
            ui_texto_centro(NOME_JOGO, CENTRO_X + dx, y + dy + 6, tamanho, (Color){ 20, 10, 16, 255 });
    ui_texto_centro(NOME_JOGO, CENTRO_X, y, tamanho, UI_DESTAQUE);
    ui_texto_centro_sombra("Sumô com física, no Palácio de Jade", CENTRO_X, y + 112, 26, UI_TEXTO);
}

/* Botão do menu: o escolhido fica cheio, com borda dourada e setas dos lados. */
static void desenhar_botao(const char *texto, float y, bool escolhido)
{
    Rectangle r = { CENTRO_X - MENU_BOTAO_L / 2.0f, y, MENU_BOTAO_L, MENU_BOTAO_A };
    if (escolhido) {
        DrawRectangleRounded(r, 0.35f, 8, Fade(UI_DESTAQUE, 0.22f));
        DrawRectangleRoundedLinesEx(r, 0.35f, 8, 3.0f, UI_DESTAQUE);
        float cy = y + MENU_BOTAO_A / 2.0f, s = 10.0f;
        DrawTriangle((Vector2){ r.x - 26, cy - s }, (Vector2){ r.x - 26, cy + s },
                     (Vector2){ r.x - 10, cy }, UI_DESTAQUE);
        DrawTriangle((Vector2){ r.x + r.width + 26, cy - s }, (Vector2){ r.x + r.width + 10, cy },
                     (Vector2){ r.x + r.width + 26, cy + s }, UI_DESTAQUE);
    } else {
        DrawRectangleRounded(r, 0.35f, 8, Fade(BLACK, 0.35f));
        DrawRectangleRoundedLinesEx(r, 0.35f, 8, 2.0f, UI_BORDA);
    }
    ui_texto_centro(texto, CENTRO_X, (int)(y + (MENU_BOTAO_A - 36) / 2), 36,
                    escolhido ? UI_TEXTO : UI_TEXTO_FRACO);
}

void menu_desenhar(const TelaMenu *t)
{
    desenhar_fundo_menu();
    desenhar_titulo_menu();
    for (int i = 0; i < QTD_OPCOES_MENU; i++)
        desenhar_botao(OPCOES_MENU[i], MENU_BOTAO_Y + i * MENU_BOTAO_PASSO, i == t->opcao);
    ui_texto_centro("Controle: D-pad escolhe, A confirma   |   Teclado: W/S e C (J1), setas e K (J2)",
                    CENTRO_X, JANELA_ALTURA - 36, 18, (Color){ 70, 44, 30, 255 });
}

/* ------------------------------------------------------------- Seleção */

/* Medidas da tela de seleção (pixels e tamanhos de fonte). */
static const float SEL_PAINEL_Y       = 110.0f;
static const float SEL_PAINEL_ALTURA  = 590.0f;
static const float SEL_MARGEM         = 24.0f;   /* da borda do painel até o conteúdo */
static const float SEL_GRADE_Y        = 56.0f;   /* do topo do painel até a grade */
static const float SEL_CARTAO_ALTURA  = 56.0f;
static const float SEL_CARTAO_ESPACO  = 10.0f;   /* entre um cartão e outro */
static const float SEL_FICHA_ESPACO   = 24.0f;   /* da grade até a ficha */
static const float SEL_BLOCO_ESPACO   = 16.0f;   /* entre os blocos da ficha */
static const float SEL_PREVIA_LARGURA = 220.0f;  /* coluna da prévia 3D (previa.h) */
static const float SEL_BARRA_ALTURA   = 16.0f;
static const float SEL_BARRA_PASSO    = 62.0f;   /* de um quesito ao seguinte */
static const int   SEL_FONTE_CARTAO   = 22;
static const int   SEL_FONTE_NOME     = 44;
static const int   SEL_FONTE_FRASE    = 22;
static const int   SEL_FONTE_ROTULO   = 20;
static const int   SEL_FONTE_VALOR    = 20;
static const int   FONTE_MINIMA       = 10;      /* o texto encolhe até aqui para caber */

/* Jeito de lutar de cada personagem, procurado pelo nome. Fica aqui, e não
 * no Personagem, para não mudar o contrato de personagem.h. Personagem sem
 * frase na tabela aparece sem frase (a tela continua certa). */
typedef struct {
    const char *nome;
    const char *frase;
} FrasePersonagem;

static const FrasePersonagem FRASES[] = {
    { "Po",        "Pesado e macio: empurra com a barriga." },
    { "Tai Lung",  "Leve e rápido, empurra forte." },
    { "Oogway",    "Lento, mas difícil de tirar do lugar." },
    { "Minotauro", "O mais pesado e firme: avança como um muro." },
};

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

/* O maior valor de cada quesito no catálogo: é a barra cheia da ficha.
 * Calculado a cada vez (são poucos personagens) para valer com qualquer
 * catálogo, sem escala fixa que um personagem novo possa ultrapassar. */
typedef struct {
    float peso;        /* (kg)  */
    float velocidade;  /* (m/s) */
    float firmeza;     /* (N.m) */
} MaximosCatalogo;

static MaximosCatalogo maximos_do_catalogo(void)
{
    MaximosCatalogo max = {0};
    for (int i = 0; i < personagem_quantidade(); i++) {
        const Personagem *p = personagem_dados(i);
        if (massa_total(p) > max.peso)          max.peso = massa_total(p);
        if (p->velocidade_max > max.velocidade) max.velocidade = p->velocidade_max;
        if (p->forca_equilibrio > max.firmeza)  max.firmeza = p->forca_equilibrio;
    }
    return max;
}

/* Fração de 0 a 1 em relação ao máximo; protege contra divisão por zero. */
static float fracao_do_maximo(float valor, float maximo)
{
    return (maximo > 0.0f) ? valor / maximo : 0.0f;
}

static const char *frase_do_personagem(const char *nome)
{
    for (size_t i = 0; i < sizeof FRASES / sizeof FRASES[0]; i++)
        if (strcmp(FRASES[i].nome, nome) == 0)
            return FRASES[i].frase;
    return "";
}

/* Maior fonte (até "maximo") em que o texto cabe na largura. Assim um nome
 * comprido encolhe em vez de sair do cartão. */
static int fonte_que_cabe(const char *texto, int maximo, float largura)
{
    int tamanho = maximo;
    while (tamanho > FONTE_MINIMA && MeasureText(texto, tamanho) > (int)largura)
        tamanho--;
    return tamanho;
}

static int linhas_da_grade(int qtd)
{
    return (qtd + SELECAO_CARTOES_POR_LINHA - 1) / SELECAO_CARTOES_POR_LINHA;
}

/* Retângulo do cartão "indice" dentro da área da grade. Todos os cartões têm
 * a mesma largura; uma linha incompleta fica centralizada. */
static Rectangle retangulo_cartao(Rectangle area, int indice, int qtd)
{
    int colunas = SELECAO_CARTOES_POR_LINHA;
    int linha = indice / colunas;
    int coluna = indice % colunas;
    int na_linha = qtd - linha * colunas;
    if (na_linha > colunas)
        na_linha = colunas;

    float w = (area.width - (colunas - 1) * SEL_CARTAO_ESPACO) / colunas;
    float largura_linha = na_linha * w + (na_linha - 1) * SEL_CARTAO_ESPACO;
    float x0 = area.x + (area.width - largura_linha) / 2.0f;
    return (Rectangle){ x0 + coluna * (w + SEL_CARTAO_ESPACO),
                        area.y + linha * (SEL_CARTAO_ALTURA + SEL_CARTAO_ESPACO),
                        w, SEL_CARTAO_ALTURA };
}

static void desenhar_cartao(Rectangle r, const char *nome, bool escolhido, bool pronto, Color cor)
{
    if (escolhido) {
        /* Mais cheio depois de confirmar, para o jogador ver que travou. */
        ui_painel(r, Fade(cor, pronto ? 0.55f : 0.30f), cor);
        DrawRectangleLinesEx(r, 4.0f, cor);
    } else {
        ui_painel(r, Fade(BLACK, 0.25f), UI_BORDA);
    }
    int tamanho = fonte_que_cabe(nome, SEL_FONTE_CARTAO, r.width - 12.0f);
    ui_texto_centro(nome, (int)(r.x + r.width / 2), (int)(r.y + (r.height - tamanho) / 2),
                    tamanho, escolhido ? UI_TEXTO : UI_TEXTO_FRACO);
}

/* Altura da grade com todos os personagens (cresce uma linha a cada
 * SELECAO_CARTOES_POR_LINHA personagens). */
static float altura_da_grade(void)
{
    int linhas = linhas_da_grade(personagem_quantidade());
    return linhas * SEL_CARTAO_ALTURA + (linhas - 1) * SEL_CARTAO_ESPACO;
}

static void desenhar_grade(Rectangle area, int escolhido, bool pronto, Color cor)
{
    int qtd = personagem_quantidade();
    for (int k = 0; k < qtd; k++)
        desenhar_cartao(retangulo_cartao(area, k, qtd), personagem_dados(k)->nome,
                        k == escolhido, pronto, cor);
}

/* Onde fica cada bloco de um lado da tela, de cima para baixo: grade,
 * cabeçalho (nome e frase) e, lado a lado, a prévia 3D e as barras. Calculado
 * num lugar só, para o desenho e o main.c (que desenha a prévia) concordarem. */
typedef struct {
    Rectangle painel, grade, cabecalho, previa, barras;
    float y_rodape;
} LayoutSelecao;

static LayoutSelecao layout_do_lado(int jogador)
{
    LayoutSelecao l;
    float x = (jogador == 0) ? 80.0f : (float)CENTRO_X + 40.0f;
    float w = (float)CENTRO_X - 120.0f;
    l.painel = (Rectangle){ x, SEL_PAINEL_Y, w, SEL_PAINEL_ALTURA };
    l.grade = (Rectangle){ x + SEL_MARGEM, SEL_PAINEL_Y + SEL_GRADE_Y, w - 2.0f * SEL_MARGEM,
                           altura_da_grade() };
    float y = l.grade.y + l.grade.height + SEL_FICHA_ESPACO;
    l.cabecalho = (Rectangle){ l.grade.x, y, l.grade.width,
                               SEL_FONTE_NOME + 12.0f + SEL_FONTE_FRASE };
    y += l.cabecalho.height + SEL_BLOCO_ESPACO;
    l.y_rodape = SEL_PAINEL_Y + SEL_PAINEL_ALTURA - 64.0f;
    float altura = l.y_rodape - SEL_BLOCO_ESPACO - y;
    l.previa = (Rectangle){ l.grade.x, y, SEL_PREVIA_LARGURA, altura };
    float x_barras = l.previa.x + l.previa.width + SEL_BLOCO_ESPACO;
    l.barras = (Rectangle){ x_barras, y, l.grade.x + l.grade.width - x_barras, altura };
    return l;
}

Rectangle selecao_area_previa(int jogador)
{
    return layout_do_lado(jogador).previa;
}

/* Nome grande e, embaixo, o jeito de lutar. */
static void desenhar_cabecalho(Rectangle area, const Personagem *p)
{
    int cx = (int)(area.x + area.width / 2);
    ui_texto_centro(p->nome, cx, (int)area.y, fonte_que_cabe(p->nome, SEL_FONTE_NOME, area.width),
                    UI_TEXTO);
    const char *frase = frase_do_personagem(p->nome);
    ui_texto_centro(frase, cx, (int)(area.y + SEL_FONTE_NOME + 12.0f),
                    fonte_que_cabe(frase, SEL_FONTE_FRASE, area.width), UI_DESTAQUE);
}

/* Moldura da prévia: o lutador (desenhado pelo main.c) fica por cima. */
static void desenhar_moldura_previa(Rectangle area, Color cor)
{
    ui_painel(area, Fade(BLACK, 0.30f), UI_BORDA);
    DrawRectangle((int)area.x, (int)(area.y + area.height) - 4, (int)area.width, 4, cor);
}

/* Um quesito em coluna: rótulo e número na mesma linha, a barra embaixo. */
static void desenhar_quesito(Rectangle area, float y, const char *rotulo, float fracao,
                             const char *valor, Color cor)
{
    DrawText(rotulo, (int)area.x, (int)y, SEL_FONTE_ROTULO, UI_TEXTO_FRACO);
    int tamanho = fonte_que_cabe(valor, SEL_FONTE_VALOR, area.width / 2.0f);
    DrawText(valor, (int)(area.x + area.width) - MeasureText(valor, tamanho), (int)y + 1,
             tamanho, UI_TEXTO);
    ui_barra((Rectangle){ area.x, y + SEL_FONTE_ROTULO + 6.0f, area.width, SEL_BARRA_ALTURA },
             fracao, cor);
}

/* As três barras, comparadas ao máximo do catálogo, centradas na área. */
static void desenhar_barras(Rectangle area, const Personagem *p, Color cor)
{
    MaximosCatalogo max = maximos_do_catalogo();
    float y = area.y + (area.height - 3.0f * SEL_BARRA_PASSO) / 2.0f;
    float massa = massa_total(p);
    desenhar_quesito(area, y, "Peso", fracao_do_maximo(massa, max.peso),
                     TextFormat("%.0f kg", massa), cor);
    y += SEL_BARRA_PASSO;
    desenhar_quesito(area, y, "Velocidade", fracao_do_maximo(p->velocidade_max, max.velocidade),
                     TextFormat("%.1f m/s", p->velocidade_max), cor);
    y += SEL_BARRA_PASSO;
    desenhar_quesito(area, y, "Firmeza", fracao_do_maximo(p->forca_equilibrio, max.firmeza),
                     TextFormat("%.0f", p->forca_equilibrio), cor);
}

static void desenhar_lado_selecao(const TelaSelecao *t, int i)
{
    LayoutSelecao l = layout_do_lado(i);
    Color cor = ui_cor_jogador(i);
    ui_painel(l.painel, UI_PAINEL, t->pronto[i] ? cor : UI_BORDA);

    int cx = (int)(l.painel.x + l.painel.width / 2);
    ui_texto_centro(TextFormat("Jogador %d", i + 1), cx, (int)SEL_PAINEL_Y + 14, 26, cor);

    const Personagem *p = personagem_dados(t->escolha[i]);
    if (p == NULL) {
        ui_texto_centro("Nenhum personagem", cx, 300, 30, UI_TEXTO_FRACO);
        return;
    }

    desenhar_grade(l.grade, t->escolha[i], t->pronto[i], cor);
    desenhar_cabecalho(l.cabecalho, p);
    desenhar_moldura_previa(l.previa, cor);
    desenhar_barras(l.barras, p, cor);

    if (t->pronto[i])
        ui_texto_centro("PRONTO!", cx, (int)l.y_rodape - 6, 36, cor);
    else {
        const char *ajuda = "esq/dir: escolher   A: confirmar   B: voltar";
        ui_texto_centro(ajuda, cx, (int)l.y_rodape,
                        fonte_que_cabe(ajuda, 20, l.grade.width), UI_TEXTO_FRACO);
    }
}

void selecao_desenhar(const TelaSelecao *t)
{
    ClearBackground(UI_FUNDO);
    ui_texto_centro("Escolham seus lutadores", CENTRO_X, 50, 44, UI_TEXTO);
    for (int i = 0; i < 2; i++)
        desenhar_lado_selecao(t, i);
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
