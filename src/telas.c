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

/* Medidas da tela de seleção (pixels e tamanhos de fonte). */
static const float SEL_PAINEL_Y       = 110.0f;
static const float SEL_PAINEL_ALTURA  = 590.0f;
static const float SEL_MARGEM         = 24.0f;   /* da borda do painel até o conteúdo */
static const float SEL_GRADE_Y        = 56.0f;   /* do topo do painel até a grade */
static const float SEL_CARTAO_ALTURA  = 56.0f;
static const float SEL_CARTAO_ESPACO  = 10.0f;   /* entre um cartão e outro */
static const float SEL_FICHA_ESPACO   = 24.0f;   /* da grade até a ficha */
static const float SEL_ROTULO_LARGURA = 150.0f;  /* coluna com "Peso", "Velocidade"... */
static const float SEL_VALOR_LARGURA  = 100.0f;  /* coluna com o número da barra */
static const float SEL_BARRA_ALTURA   = 18.0f;
static const float SEL_BARRA_PASSO    = 42.0f;   /* de uma barra à seguinte */
static const int   SEL_FONTE_CARTAO   = 22;
static const int   SEL_FONTE_NOME     = 44;
static const int   SEL_FONTE_FRASE    = 22;
static const int   SEL_FONTE_ROTULO   = 22;
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

/* Grade com todos os personagens; devolve a altura ocupada. */
static float desenhar_grade(Rectangle area, int escolhido, bool pronto, Color cor)
{
    int qtd = personagem_quantidade();
    for (int k = 0; k < qtd; k++)
        desenhar_cartao(retangulo_cartao(area, k, qtd), personagem_dados(k)->nome,
                        k == escolhido, pronto, cor);
    int linhas = linhas_da_grade(qtd);
    return linhas * SEL_CARTAO_ALTURA + (linhas - 1) * SEL_CARTAO_ESPACO;
}

/* Uma linha da ficha: rótulo, barra comparada ao máximo e o número. */
static void desenhar_quesito(Rectangle area, float y, const char *rotulo, float fracao,
                             const char *valor, Color cor)
{
    DrawText(rotulo, (int)area.x, (int)y, SEL_FONTE_ROTULO, UI_TEXTO_FRACO);
    float x_barra = area.x + SEL_ROTULO_LARGURA;
    float w_barra = area.width - SEL_ROTULO_LARGURA - SEL_VALOR_LARGURA;
    ui_barra((Rectangle){ x_barra, y + 2.0f, w_barra, SEL_BARRA_ALTURA }, fracao, cor);
    int tamanho = fonte_que_cabe(valor, SEL_FONTE_VALOR, SEL_VALOR_LARGURA - 10.0f);
    DrawText(valor, (int)(area.x + area.width) - MeasureText(valor, tamanho), (int)y + 1,
             tamanho, UI_TEXTO);
}

/* Ficha do personagem escolhido: nome, jeito de lutar e as três barras. */
static void desenhar_ficha(Rectangle area, const Personagem *p, Color cor)
{
    MaximosCatalogo max = maximos_do_catalogo();
    int cx = (int)(area.x + area.width / 2);
    float y = area.y;

    int fonte_nome = fonte_que_cabe(p->nome, SEL_FONTE_NOME, area.width);
    ui_texto_centro(p->nome, cx, (int)y, fonte_nome, UI_TEXTO);
    y += SEL_FONTE_NOME + 12.0f;

    const char *frase = frase_do_personagem(p->nome);
    ui_texto_centro(frase, cx, (int)y, fonte_que_cabe(frase, SEL_FONTE_FRASE, area.width),
                    UI_DESTAQUE);
    y += SEL_FONTE_FRASE + 22.0f;

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
    float x = (i == 0) ? 80.0f : (float)CENTRO_X + 40.0f;
    float w = (float)CENTRO_X - 120.0f;
    Color cor = ui_cor_jogador(i);
    Rectangle painel = { x, SEL_PAINEL_Y, w, SEL_PAINEL_ALTURA };
    ui_painel(painel, UI_PAINEL, t->pronto[i] ? cor : UI_BORDA);

    int cx = (int)(x + w / 2);
    ui_texto_centro(TextFormat("Jogador %d", i + 1), cx, (int)SEL_PAINEL_Y + 14, 26, cor);

    const Personagem *p = personagem_dados(t->escolha[i]);
    if (p == NULL) {
        ui_texto_centro("Nenhum personagem", cx, 300, 30, UI_TEXTO_FRACO);
        return;
    }

    /* Grade em cima; a ficha começa logo abaixo, onde quer que a grade termine
     * (com mais personagens a grade ganha linhas e a ficha desce junto). */
    Rectangle conteudo = { x + SEL_MARGEM, SEL_PAINEL_Y + SEL_GRADE_Y,
                           w - 2.0f * SEL_MARGEM, 0.0f };
    float altura_grade = desenhar_grade(conteudo, t->escolha[i], t->pronto[i], cor);
    conteudo.y += altura_grade + SEL_FICHA_ESPACO;
    desenhar_ficha(conteudo, p, cor);

    int y_rodape = (int)(SEL_PAINEL_Y + SEL_PAINEL_ALTURA) - 64;
    if (t->pronto[i])
        ui_texto_centro("PRONTO!", cx, y_rodape - 6, 36, cor);
    else {
        const char *ajuda = "esq/dir: escolher   A: confirmar   B: voltar";
        ui_texto_centro(ajuda, cx, y_rodape, fonte_que_cabe(ajuda, 20, w - 2.0f * SEL_MARGEM),
                        UI_TEXTO_FRACO);
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
