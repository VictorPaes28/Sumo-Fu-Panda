/*
 * main.c — janela, laço principal e troca de telas.
 *
 * Fluxo de telas:
 *   MENU -> ENTRADA -> SELECAO -> ARENA -> MONTAGEM -> VERSUS -> LUTA -> RESULTADO
 *   Torneio: INSCRICAO -> CHAVE -> ENTRADA; RESULTADO avança a chave.
 *                |          +--- remontar ----+
 *                +---- (voltar)          revanche -> LUTA, menu -> MENU
 *
 * O main.c é o único lugar que: lê a entrada (uma vez por quadro), troca
 * de tela, toca sons a partir dos eventos e chama as funções de desenho.
 * As telas e a partida só devolvem ações/eventos.
 *
 * Teclas de desenvolvimento: F1 = modo de depuração (caixas físicas e FPS).
 *
 * Dono: Principal.
 */
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "rlgl.h"
#include "config.h"
#include "entrada.h"
#include "recursos.h"
#include "telas.h"
#include "montagem.h"
#include "hud.h"
#include "partida.h"
#include "personagem.h"
#include "render.h"
#include "previa.h"

/* Entrada com dispositivo escolhido; as demos mantêm entrada_ler. */
Intencao entrada_ler_dispositivo(int jogador, int controle);
void render_cena_arena(const Partida *p, Recursos *r, const CameraLuta *c, bool depuracao, int arena);
void render_arena_previa(RenderTexture2D alvo, int arena);

static const char *const ARENAS[] = { "Ginásio Imperial", "Pátio de Jade", "Bosque de Bambu" };

static const float PAUSA_PREVIA_GOLPE = 0.65f;
static const float DISTANCIA_GOLPE_PREVIA = 0.85f;
static const float APROXIMACAO_PREVIA_MAX = 1.1f;
static const int PREVIA_GOLPE_LARGURA = 580;
static const int PREVIA_GOLPE_ALTURA = 278;

typedef struct {
    Partida partida;
    Jogador jogadores[2];
    RenderTexture2D imagem;
    bool ativa;
    bool atacou;
    float aproximacao;
    float fim;
} PreviaGolpe;

typedef enum {
    TELA_MENU, TELA_CONFIGURACOES, TELA_SELECAO, TELA_MONTAGEM, TELA_LUTA, TELA_RESULTADO,
    TELA_ENTRADA, TELA_INSCRICAO, TELA_CHAVE, TELA_VERSUS, TELA_AJUDA, TELA_ARENA
} Tela;

typedef struct {
    char nomes[8][48];
    int quantidade, campo, atual;
    int dupla[7][2], vencedor[7];
    bool ativo, substituir;
} Torneio;

typedef struct {
    int acertos[2], placar[2];
    float dano[2];             /* dano acumulado causado (%) */
    double duracao;           /* tempo efetivo de luta (s) */
} Estatisticas;

typedef struct {
    Tela tela;
    bool sair;
    bool depuracao;
    Jogador jogadores[2];        /* personagem e combo: duram o jogo inteiro */
    TelaMenu menu;
    int opcao_configuracoes;
    RenderTexture2D quadro;      /* imagem em alta resolução; layout lógico preservado */
    TelaSelecao selecao;
    TelaMontagem montagem;
    TelaResultado resultado;
    Partida partida;
    bool partida_ativa;          /* true entre partida_iniciar e partida_liberar */
    bool controle_antes[2];      /* para pausar quando um controle desconecta */
    Recursos recursos;
    CameraLuta camera;           /* câmera da luta (acompanha e treme) */
    Previa previa[2];            /* lutador ao vivo: no menu e na seleção */
    PreviaGolpe previa_golpe[2];
    float tempo_golpe_previa[2]; /* tempo do golpe destacado (s) */
    float tempo_menu;            /* desde que o menu abriu: troca os lutadores (s) */
    Font fonte_ui;
    Music musica;
    Sound torcida;
    bool tem_musica, tem_torcida;
    int volume_musica, volume_efeitos;
    int dispositivo[2];          /* -2 aguardando, -1 teclado, >= 0 controle */
    char nomes[2][48];
    bool em_torneio;
    Torneio torneio;
    Estatisticas estatisticas;
    float tempo_versus;          /* segundos na apresentação */
    int arena;
    bool arena_pronto[2];
    RenderTexture2D imagem_arena;
} Jogo;

static void apresentacao_volumes(Jogo *j)
{
    if (j->tem_musica) SetMusicVolume(j->musica, j->volume_musica / 10.0f);
    if (j->tem_torcida) SetSoundVolume(j->torcida, j->volume_efeitos / 10.0f);
    for (int i = 0; i < SOM_QTD; i++)
        if (j->recursos.tem_som[i]) SetSoundVolume(j->recursos.sons[i], j->volume_efeitos / 10.0f);
}

static void apresentacao_salvar_config(const Jogo *j)
{
    FILE *f = fopen(TextFormat("%sconfiguracoes-locais.txt", GetApplicationDirectory()), "w");
    if (!f) { TraceLog(LOG_WARNING, "Nao foi possivel salvar as configuracoes locais"); return; }
    fprintf(f, "%d %d %d\n", j->volume_musica, j->volume_efeitos, IsWindowFullscreen() ? 1 : 0);
    fclose(f);
}

static void torneio_criar(Torneio *t)
{
    for (int i = 0; i < 7; i++) {
        t->dupla[i][0] = t->dupla[i][1] = -1;
        t->vencedor[i] = -1;
    }
    for (int i = 0; i < t->quantidade / 2; i++) {
        t->dupla[i][0] = i * 2;
        t->dupla[i][1] = i * 2 + 1;
    }
    t->atual = 0;
    t->ativo = true;
}

/* Só confirma o vencedor ao sair do resultado, nunca durante o desenho. */
static bool torneio_avancar(Torneio *t, int lado)
{
    int m = t->atual;
    if (!t->ativo || lado < 0 || lado > 1 || m >= t->quantidade - 1 ||
        t->vencedor[m] >= 0 || t->dupla[m][lado] < 0) return false;
    t->vencedor[m] = t->dupla[m][lado];
    if (m < t->quantidade - 2)
        t->dupla[t->quantidade / 2 + m / 2][m % 2] = t->vencedor[m];
    t->atual++;
    return true;
}

static void apresentacao_entrada(Jogo *j)
{
    for (int i = 0; i < 2; i++) {
        j->dispositivo[i] = -2;
        combo_liberar(&j->jogadores[i].combo);
        if (j->em_torneio) {
            int p = j->torneio.dupla[j->torneio.atual][i];
            snprintf(j->nomes[i], sizeof j->nomes[i], "%s", j->torneio.nomes[p]);
        } else snprintf(j->nomes[i], sizeof j->nomes[i], "Jogador %d", i + 1);
    }
    j->tela = TELA_ENTRADA;
}

static void apresentacao_versus(Jogo *j)
{
    j->tempo_versus = 0;
    j->tela = TELA_VERSUS;
}

static void tocar_sons_de_menu(const Jogo *j, const Intencao in[2])
{
    for (int i = 0; i < 2; i++) {
        if (in[i].direcional_x != 0 || in[i].direcional_y != 0)
            recursos_tocar(&j->recursos, SOM_MENU_MOVER);
        if (in[i].a)
            recursos_tocar(&j->recursos, SOM_MENU_CONFIRMAR);
    }
}

static void tocar_sons_da_partida(const Jogo *j)
{
    const EventosPartida *e = &j->partida.eventos;
    if (e->acerto_forte)      recursos_tocar(&j->recursos, SOM_GOLPE_FORTE);
    else if (e->acerto_leve)  recursos_tocar(&j->recursos, SOM_GOLPE_LEVE);
    if (e->queda)             recursos_tocar(&j->recursos, SOM_QUEDA);
    if (e->queda && j->tem_torcida) PlaySound(j->torcida);
    if (e->contagem)          recursos_tocar(&j->recursos, SOM_CONTAGEM);
    if (e->lute)              recursos_tocar(&j->recursos, SOM_LUTE);
    if (e->fim_partida)       recursos_tocar(&j->recursos, SOM_VITORIA);
}

static void encerrar_luta(Jogo *j)
{
    if (j->partida_ativa) {
        partida_liberar(&j->partida);
        j->partida_ativa = false;
    }
}

static void iniciar_luta(Jogo *j)
{
    encerrar_luta(j);
    if (partida_iniciar(&j->partida, j->jogadores)) {
        j->estatisticas = (Estatisticas){0};
        j->partida_ativa = true;
        for (int i = 0; i < 2; i++)
            j->controle_antes[i] = j->dispositivo[i] >= 0 && IsGamepadAvailable(j->dispositivo[i]);
        render_iniciar_camera(&j->camera);
        j->tela = TELA_LUTA;
    } else {
        TraceLog(LOG_ERROR, "Falha ao iniciar a partida");
        menu_iniciar(&j->menu);
        j->tela = TELA_MENU;
    }
}

/* O n-ésimo lutador da vitrine do menu, com volta. Só entram os personagens
 * com modelo 3D (a primeira tela fica bonita); sem nenhum, entram todos. */
static int lutador_do_menu(const Jogo *j, int n)
{
    int qtd = personagem_quantidade(), com_modelo = 0;
    for (int k = 0; k < qtd; k++)
        com_modelo += j->recursos.peles[k].carregada;
    if (com_modelo == 0)
        return n % qtd;
    n %= com_modelo;
    for (int k = 0; k < qtd; k++)
        if (j->recursos.peles[k].carregada && n-- == 0)
            return k;
    return 0;
}

/* Poses da vitrine na ordem do catálogo. Os ângulos são somados à
 * guarda, sem mexer no estilo que o personagem usa no ringue. */
static const Pose POSES_MENU[] = {
    { .alvo = { /* Po: postura relaxada, uma mao junto a faixa. */
        [ART_CINTURA] = { -0.09f, 0.05f },
        [ART_PESCOCO] = { 0.02f, -0.05f },
        [ART_OMBRO_E] = { -0.14f, -0.29f },
        [ART_COTOVELO_E] = { 0.22f },
        [ART_OMBRO_D] = { -0.20f, -0.36f },
        [ART_COTOVELO_D] = { -0.28f },
    } },
    { .alvo = { /* Tai Lung: postura ereta, maos baixas e ombros firmes. */
        [ART_CINTURA] = { -0.13f, -0.04f },
        [ART_PESCOCO] = { 0.10f },
        [ART_OMBRO_E] = { -0.44f, -0.53f },
        [ART_COTOVELO_E] = { -0.29f },
        [ART_OMBRO_D] = { -0.35f, -0.49f },
        [ART_COTOVELO_D] = { -0.20f },
    } },
    { .alvo = { /* Oogway: guarda fechada, como uma saudação. */
        [ART_PESCOCO] = { 0.18f },
        [ART_OMBRO_E] = { 0.38f, 0.45f },
        [ART_OMBRO_D] = { 0.38f, 0.45f },
        [ART_COTOVELO_E] = { 0.80f },
        [ART_COTOVELO_D] = { 0.80f },
    } },
    { .alvo = { /* Kai: o peito aberto e os braços separados. */
        [ART_CINTURA] = { -0.12f },
        [ART_OMBRO_E] = { 0.39f, -0.43f },
        [ART_OMBRO_D] = { 0.39f, -0.43f },
        [ART_COTOVELO_E] = { 0.20f },
        [ART_COTOVELO_D] = { 0.20f },
    } },
    { .alvo = { /* Macaco: postura assimétrica, com uma mão alta. */
        [ART_CINTURA] = { 0.02f, -0.11f },
        [ART_PESCOCO] = { -0.03f, 0.13f },
        [ART_OMBRO_E] = { 0.15f, -0.19f },
        [ART_OMBRO_D] = { 0.80f, 0.32f },
        [ART_COTOVELO_D] = { 0.53f },
    } },
};

static void atualizar_vitrine(Previa *pv, int personagem, float dt)
{
    if (pv->personagem != personagem)
        previa_atualizar(pv, personagem, 0.0f);
    if (pv->personagem < 0)
        return;
    if ((unsigned)personagem < sizeof POSES_MENU / sizeof POSES_MENU[0]) {
        corpo_definir_movimento_extra(&pv->corpo, &POSES_MENU[personagem]);
        corpo_definir_rigidez(&pv->corpo, RIGIDEZ_PREPARO);
    }
    previa_atualizar(pv, personagem, dt);
}

static void atualizar_luta(Jogo *j, const Intencao in[2], float dt)
{
    Partida *p = &j->partida;

    /* Controle desconectou no meio da luta: pausa. */
    bool desconectou = false;
    for (int i = 0; i < 2; i++) {
        if (j->controle_antes[i] && !in[i].usando_controle)
            desconectou = true;
        j->controle_antes[i] = in[i].usando_controle;
    }
    if (desconectou) { partida_pausar(p, true); return; }

    if (in[0].start || in[1].start)
        partida_pausar(p, !p->pausada);

    if (p->pausada) {
        if (in[0].b || in[1].b) {          /* pausado + B = sair para o menu */
            encerrar_luta(j);
            menu_iniciar(&j->menu);
            j->tela = TELA_MENU;
        }
        return;
    }

    /* Copia apenas dados dos hematomas: nós podem ser liberados na atualização.
     * A diferença entre listas mede dano novo mesmo se outro hematoma curar. */
    double cura[2][HEMATOMAS_MAX], ultimo[2];
    float dano[2][HEMATOMAS_MAX];
    int qtd[2] = {0};
    int round_anterior = p->round;
    bool estava_lutando = p->estado == PARTIDA_LUTA;
    for (int i = 0; i < 2; i++) {
        ultimo[i] = p->lutadores[i].ultimo_golpe_recebido;
        for (const NoHematoma *n = p->lutadores[i].hematomas.inicio; n; n = n->prox) {
            if (qtd[i] == HEMATOMAS_MAX) break;
            cura[i][qtd[i]] = n->momento_cura;
            dano[i][qtd[i]++] = n->dano;
        }
    }
    partida_atualizar(p, in, dt);
    if (estava_lutando && p->round == round_anterior) {
        for (int i = 0; i < 2; i++) {
            if (p->lutadores[i].ultimo_golpe_recebido != ultimo[i])
                j->estatisticas.acertos[1-i]++;
            bool usados[HEMATOMAS_MAX] = {0};
            for (const NoHematoma *n = p->lutadores[i].hematomas.inicio; n; n = n->prox) {
                int k;
                for (k = 0; k < qtd[i]; k++)
                    if (!usados[k] && cura[i][k] == n->momento_cura && dano[i][k] == n->dano) break;
                if (k < qtd[i]) usados[k] = true;
                else j->estatisticas.dano[1-i] += n->dano;
            }
        }
    }
    j->estatisticas.duracao = p->relogio;
    for (int i = 0; i < 2; i++) j->estatisticas.placar[i] = p->vitorias[i];
    render_atualizar_camera(&j->camera, p, dt);
    tocar_sons_da_partida(j);

    if (p->estado == PARTIDA_FIM) {
        int v = partida_vencedor(p);
        const Personagem *pv = personagem_dados(j->jogadores[v < 0 ? 0 : v].personagem);
        resultado_iniciar(&j->resultado, v, pv ? pv->nome : "?");
        encerrar_luta(j);
        j->tela = TELA_RESULTADO;
    }
}

static void fechar_previa_golpe(PreviaGolpe *pv)
{
    if (pv->ativa)
        partida_liberar(&pv->partida);
    pv->ativa = false;
    combo_liberar(&pv->jogadores[0].combo);
}

static bool reiniciar_previa_golpe(PreviaGolpe *pv, int atacante, int alvo, TipoGolpe golpe)
{
    fechar_previa_golpe(pv);
    pv->jogadores[0].personagem = atacante;
    pv->jogadores[1].personagem = alvo;
    if (!combo_inserir(&pv->jogadores[0].combo, 0, golpe))
        return false;
    pv->ativa = partida_iniciar(&pv->partida, pv->jogadores);
    if (!pv->ativa)
        return false;

    /* Assenta a guarda usando a mesma contagem da luta, sem exibi-la. */
    for (float t = 0; t < PREVIA_ASSENTAR; t += FISICA_DT)
        partida_atualizar(&pv->partida, (Intencao[2]){0}, FISICA_DT);
    pv->partida.estado = PARTIDA_LUTA;
    pv->partida.tempo_no_estado = 0;
    pv->aproximacao = 0;
    pv->fim = 0;
    pv->atacou = false;
    return true;
}

static void iniciar_montagem(Jogo *j)
{
    montagem_iniciar(&j->montagem);
    for (int i = 0; i < 2; i++)
        j->tempo_golpe_previa[i] = 0.0f;
    j->tela = TELA_MONTAGEM;
}

static void iniciar_selecao_arena(Jogo *j)
{
    j->arena_pronto[0] = j->arena_pronto[1] = false;
    j->tela = TELA_ARENA;
}

/* Mudar a escolha desfaz as confirmações; os dois aprovam a mesma arena. */
static void atualizar_selecao_arena(Jogo *j, const Intencao in[2])
{
    if (in[0].b || in[1].b) {
        selecao_iniciar(&j->selecao);
        for (int i = 0; i < 2; i++) j->selecao.escolha[i] = j->jogadores[i].personagem;
        j->tela = TELA_SELECAO;
        return;
    }
    int direcao = in[0].direcional_x ? in[0].direcional_x : in[1].direcional_x;
    if (direcao) {
        int qtd = (int)(sizeof ARENAS / sizeof ARENAS[0]);
        j->arena = (j->arena + (direcao > 0 ? 1 : -1) + qtd) % qtd;
        j->arena_pronto[0] = j->arena_pronto[1] = false;
        return;
    }
    for (int i = 0; i < 2; i++) if (in[i].a) j->arena_pronto[i] = true;
    if (j->arena_pronto[0] && j->arena_pronto[1]) iniciar_montagem(j);
}

static void atualizar_previas_golpes(Jogo *j, const int anterior[2], float dt)
{
    for (int i = 0; i < 2; i++) {
        PreviaGolpe *pv = &j->previa_golpe[i];
        int cursor = j->montagem.cursor_catalogo[i];
        const Golpe *g = golpe_dados((TipoGolpe)cursor);
        float *tempo = &j->tempo_golpe_previa[i];
        if (!pv->ativa || cursor != anterior[i] || pv->fim >= PAUSA_PREVIA_GOLPE) {
            *tempo = 0.0f;
            if (!reiniciar_previa_golpe(pv, j->jogadores[i].personagem,
                                       j->jogadores[1-i].personagem, (TipoGolpe)cursor))
                continue;
        }

        Partida *p = &pv->partida;
        Lutador *l = &p->lutadores[0];
        Intencao in[2] = {0};
        if (!pv->atacou && p->estado == PARTIDA_LUTA) {
            Vector3 a = corpo_posicao(&l->corpo);
            Vector3 b = corpo_posicao(&p->lutadores[1].corpo);
            pv->aproximacao += dt;
            if (b.x - a.x > DISTANCIA_GOLPE_PREVIA &&
                pv->aproximacao < APROXIMACAO_PREVIA_MAX) {
                in[0].mover.x = 1.0f;
                in[1].mover.x = -1.0f;
            } else {
                in[0].a = true;
            }
        }

        partida_atualizar(p, in, dt);
        if (p->estado != PARTIDA_LUTA ||
            (pv->atacou && l->golpe_atual == NULL) ||
            pv->aproximacao > 2.0f * APROXIMACAO_PREVIA_MAX) {
            pv->fim += dt;
            *tempo = g->duracao;
        } else if (l->golpe_atual != NULL) {
            pv->atacou = true;
            *tempo = l->tempo_golpe;
        }
    }
}

static void limpar_previas_golpes(Jogo *j)
{
    for (int i = 0; i < 2; i++)
        fechar_previa_golpe(&j->previa_golpe[i]);
}

/* Troca somente depois de alocar: uma falha mantém a imagem anterior válida. */
static void redimensionar_imagem(RenderTexture2D *imagem, int largura, int altura)
{
    if (imagem->texture.width == largura && imagem->texture.height == altura)
        return;
    RenderTexture2D nova = LoadRenderTexture(largura, altura);
    if (!IsRenderTextureValid(nova)) {
        if (nova.id) UnloadRenderTexture(nova);
        return;
    }
    SetTextureFilter(nova.texture, TEXTURE_FILTER_BILINEAR);
    if (imagem->id) UnloadRenderTexture(*imagem);
    *imagem = nova;
}

static void atualizar_resolucao(Jogo *j)
{
    /* Dois samples por eixo suavizam as bordas ao reduzir. MSAA da janela
     * não se aplica às RenderTextures. Em telas maiores, acompanha os pixels
     * físicos da área 16:9, inclusive quando há escala de DPI no Windows. */
    float escala = fmaxf(2.0f, fminf((float)GetRenderWidth() / JANELA_LARGURA,
                                   (float)GetRenderHeight() / JANELA_ALTURA));
    redimensionar_imagem(&j->quadro, (int)ceilf(JANELA_LARGURA * escala),
                         (int)ceilf(JANELA_ALTURA * escala));
    redimensionar_imagem(&j->imagem_arena, (int)ceilf(840*escala), (int)ceilf(430*escala));
    for (int i = 0; i < 2; i++) {
        redimensionar_imagem(&j->previa[i].imagem, (int)ceilf(PREVIA_LARGURA * escala),
                             (int)ceilf(PREVIA_ALTURA * escala));
        redimensionar_imagem(&j->previa_golpe[i].imagem,
                             (int)ceilf(PREVIA_GOLPE_LARGURA * escala),
                             (int)ceilf(PREVIA_GOLPE_ALTURA * escala));
    }
}

/* O viewport usa todos os pixels; as coordenadas dos menus continuam iguais. */
static void projetar_interface(void)
{
    rlDrawRenderBatchActive();
    rlMatrixMode(RL_PROJECTION);
    rlLoadIdentity();
    rlOrtho(0, JANELA_LARGURA, JANELA_ALTURA, 0, 0.0, 1.0);
    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();
}

static void filtrar_modelos(Recursos *r)
{
    for (int i = 0; i < MAX_PERSONAGENS; i++) {
        if (!r->peles[i].carregada) continue;
        Model *modelo = &r->peles[i].modelo;
        for (int m = 0; m < modelo->materialCount; m++) {
            Texture2D *textura = &modelo->materials[m].maps[MATERIAL_MAP_ALBEDO].texture;
            if (!textura->id || textura->id == rlGetTextureIdDefault()) continue;
            /* Mipmaps evitam cintilação dos pelos e manchas quando distantes. */
            if (textura->mipmaps == 1) GenTextureMipmaps(textura);
            SetTextureFilter(*textura, TEXTURE_FILTER_TRILINEAR);
        }
    }
}

static void atualizar(Jogo *j, float dt)
{
    bool atribuido = j->tela == TELA_SELECAO || j->tela == TELA_ARENA || j->tela == TELA_MONTAGEM ||
                    j->tela == TELA_LUTA || j->tela == TELA_VERSUS || j->tela == TELA_RESULTADO;
    Intencao in[2];
    for (int i = 0; i < 2; i++) {
        in[i] = atribuido ? entrada_ler_dispositivo(i, j->dispositivo[i]) : entrada_ler(i);
        if (!atribuido && in[i].usando_controle) {
            Intencao teclado = entrada_ler_dispositivo(i, -1);
            in[i].a |= teclado.a; in[i].b |= teclado.b;
            in[i].direcional_y += teclado.direcional_y;
            in[i].direcional_x += teclado.direcional_x;
        }
    }
    if (j->tem_musica) UpdateMusicStream(j->musica);

    if (IsKeyPressed(KEY_F11)) {
        ToggleFullscreen();
        apresentacao_salvar_config(j);
    }

    if (IsKeyPressed(KEY_F1))
        j->depuracao = !j->depuracao;

    if (j->tela != TELA_LUTA)
        tocar_sons_de_menu(j, in);

    switch (j->tela) {
    case TELA_MENU: {
        /* Uma dupla diferente de lutadores a cada MENU_TROCA_LUTADOR. */
        j->tempo_menu += dt;
        int dupla = (int)(j->tempo_menu / MENU_TROCA_LUTADOR);
        for (int i = 0; i < 2; i++)
            atualizar_vitrine(&j->previa[i], lutador_do_menu(j, dupla * 2 + i), dt);
        j->menu.opcao = (j->menu.opcao + in[0].direcional_y + in[1].direcional_y + 8) % 4;
        if (in[0].a || in[1].a || IsKeyPressed(KEY_ENTER)) {
            if (j->menu.opcao == 0) { j->em_torneio = false; apresentacao_entrada(j); }
            if (j->menu.opcao == 1) {
                j->em_torneio = true;
                if (j->torneio.ativo) j->tela = TELA_CHAVE;
                else {
                    j->torneio.quantidade = 4; j->torneio.campo = -1;
                    for (int i = 0; i < 8; i++)
                        snprintf(j->torneio.nomes[i], sizeof j->torneio.nomes[i], "Jogador %d", i + 1);
                    j->tela = TELA_INSCRICAO;
                }
            }
            if (j->menu.opcao == 2) { j->opcao_configuracoes = 0; j->tela = TELA_CONFIGURACOES; }
            if (j->menu.opcao == 3) j->sair = true;
        }
    } break;

    case TELA_CONFIGURACOES:
        for (int i = 0; i < 2; i++) {
            j->opcao_configuracoes = (j->opcao_configuracoes + in[i].direcional_y + 5) % 5;
            if (in[i].b || IsKeyPressed(KEY_ESCAPE) || (in[i].a && j->opcao_configuracoes == 4)) {
                j->tela = TELA_MENU;
                break;
            }
            if (j->opcao_configuracoes == 0 && (in[i].a || in[i].direcional_x)) {
                ToggleFullscreen();
                apresentacao_salvar_config(j);
                break;
            }
            if (j->opcao_configuracoes == 1 || j->opcao_configuracoes == 2) {
                int *v = j->opcao_configuracoes == 1 ? &j->volume_musica : &j->volume_efeitos;
                *v += in[i].direcional_x;
                if (*v < 0) *v = 0;
                if (*v > 10) *v = 10;
                apresentacao_volumes(j);
                if (in[i].direcional_x) apresentacao_salvar_config(j);
            }
            if (j->opcao_configuracoes == 3 && in[i].a) j->tela = TELA_AJUDA;
        }
        break;

    case TELA_SELECAO: {
        AcaoSelecao a = selecao_atualizar(&j->selecao, in);
        for (int i = 0; i < 2; i++)
            previa_atualizar(&j->previa[i], j->selecao.escolha[i], dt);
        if (a == SELECAO_CONCLUIDA) {
            for (int i = 0; i < 2; i++)
                j->jogadores[i].personagem = j->selecao.escolha[i];
            iniciar_selecao_arena(j);
        } else if (a == SELECAO_VOLTAR) {
            menu_iniciar(&j->menu);
            j->tela = TELA_MENU;
        }
    } break;

    case TELA_MONTAGEM: {
        int anterior[2] = { j->montagem.cursor_catalogo[0], j->montagem.cursor_catalogo[1] };
        AcaoMontagem a = montagem_atualizar(&j->montagem, &j->jogadores[0].combo,
                                            &j->jogadores[1].combo, in);
        if (a == MONTAGEM_CONCLUIDA) {
            limpar_previas_golpes(j);
            for (int i = 0; i < 2; i++) j->controle_antes[i] = in[i].usando_controle;
            apresentacao_versus(j);
        } else if (a == MONTAGEM_VOLTAR) {
            limpar_previas_golpes(j);
            iniciar_selecao_arena(j);
        } else {
            atualizar_previas_golpes(j, anterior, dt);
        }
    } break;

    case TELA_LUTA:
        atualizar_luta(j, in, dt);
        break;

    case TELA_ARENA:
        atualizar_selecao_arena(j, in);
        break;

    case TELA_RESULTADO: {
        int n = j->em_torneio ? 2 : 4;
        j->resultado.opcao = (j->resultado.opcao + in[0].direcional_y + in[1].direcional_y + 8) % n;
        if (in[0].a || in[1].a) {
            int o = j->resultado.opcao;
            if (j->em_torneio) {
                torneio_avancar(&j->torneio, j->resultado.vencedor);
                j->tela = o == 0 ? TELA_CHAVE : TELA_MENU;
            } else if (o == 0) iniciar_luta(j);
            else if (o == 1) { selecao_iniciar(&j->selecao); j->tela = TELA_SELECAO; }
            else if (o == 2) iniciar_montagem(j);
            else j->tela = TELA_MENU;
        }
    } break;
    case TELA_ENTRADA: {
        if (IsKeyPressed(KEY_ESCAPE)) { j->tela = j->em_torneio ? TELA_CHAVE : TELA_MENU; break; }
        if (j->dispositivo[0] != -2 && j->dispositivo[1] != -2) {
            selecao_iniciar(&j->selecao); j->tela = TELA_SELECAO; break;
        }
        if (IsKeyPressed(KEY_C)) j->dispositivo[0] = -1;
        if (IsKeyPressed(KEY_K)) j->dispositivo[1] = -1;
        for (int g = 0; g < 4; g++) {
            if (!IsGamepadAvailable(g) || j->dispositivo[0] == g || j->dispositivo[1] == g) continue;
            if (IsGamepadButtonPressed(g, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
                int lado = j->dispositivo[0] == -2 ? 0 : 1;
                if (j->dispositivo[lado] == -2) j->dispositivo[lado] = g;
            }
        }
    } break;
    case TELA_INSCRICAO: {
        Torneio *t = &j->torneio;
        if (IsKeyPressed(KEY_ESCAPE)) { j->tela = TELA_MENU; break; }
        if (t->campo == -1) {
            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) t->quantidade = 12 - t->quantidade;
        } else if (t->campo < t->quantidade) {
            char *nome = t->nomes[t->campo];
            int cp;
            while ((cp = GetCharPressed()) != 0) {
                if (cp < 32 || cp > 255) continue;
                if (t->substituir) { nome[0] = 0; t->substituir = false; }
                int bytes = 0;
                const char *utf8 = CodepointToUTF8(cp, &bytes);
                size_t len = strlen(nome);
                if (len + bytes < sizeof t->nomes[0]) { memcpy(nome + len, utf8, bytes); nome[len + bytes] = 0; }
            }
            if (IsKeyPressed(KEY_BACKSPACE)) {
                size_t len = strlen(nome);
                if (t->substituir) { nome[0] = 0; t->substituir = false; }
                else if (len) { do { len--; } while (len && ((unsigned char)nome[len] & 0xc0) == 0x80); nome[len] = 0; }
            }
        }
        bool anterior = IsKeyPressed(KEY_UP) || (IsKeyPressed(KEY_TAB) && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)));
        if (anterior) {
            if (t->campo > -1) t->campo--;
            t->substituir = true;
        } else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_DOWN)) {
            if (t->campo >= 0 && t->campo < t->quantidade && t->nomes[t->campo][0] == 0)
                snprintf(t->nomes[t->campo], sizeof t->nomes[0], "Jogador %d", t->campo + 1);
            t->campo++;
            t->substituir = true;
            if (t->campo > t->quantidade) {
                for (int i = 0; i < t->quantidade; i++)
                    if (t->nomes[i][0] == 0) snprintf(t->nomes[i], sizeof t->nomes[i], "Jogador %d", i + 1);
                torneio_criar(t); j->tela = TELA_CHAVE;
            }
        }
    } break;
    case TELA_CHAVE:
        if (in[0].b || in[1].b || IsKeyPressed(KEY_ESCAPE)) j->tela = TELA_MENU;
        else if (IsKeyPressed(KEY_N)) { j->torneio.ativo = false; j->torneio.campo = -1; j->tela = TELA_INSCRICAO; }
        else if (in[0].a || in[1].a || IsKeyPressed(KEY_ENTER)) {
            if (j->torneio.atual < j->torneio.quantidade - 1) apresentacao_entrada(j);
        }
        break;
    case TELA_VERSUS:
        j->tempo_versus += dt;
        if (j->tempo_versus >= 2.0f || in[0].a || in[1].a) iniciar_luta(j);
        break;
    case TELA_AJUDA:
        if (in[0].b || in[1].b || IsKeyPressed(KEY_ESCAPE)) j->tela = TELA_CONFIGURACOES;
        break;
    }
}

/* Encaixa sem esticar e vira a textura renderizada do OpenGL. */
static void desenhar_textura(RenderTexture2D imagem, Rectangle area)
{
    if (!imagem.id)
        return;
    float escala = fminf(area.width / imagem.texture.width, area.height / imagem.texture.height);
    float w = imagem.texture.width * escala, h = imagem.texture.height * escala;
    Rectangle origem = { 0, 0, (float)imagem.texture.width, -(float)imagem.texture.height };
    Rectangle destino = { area.x + (area.width - w) / 2, area.y + (area.height - h) / 2, w, h };
    DrawTexturePro(imagem.texture, origem, destino, (Vector2){0}, 0.0f, WHITE);
}

/* As prévias são renderizadas antes do quadro, pois trocar de textura de
 * render durante o quadro restaura o framebuffer da janela. */
static void renderizar_previas(Jogo *j)
{
    if (j->tela == TELA_ARENA && j->imagem_arena.id)
        render_arena_previa(j->imagem_arena, j->arena);
    for (int i = 0; i < 2; i++) {
        if (j->tela == TELA_MENU || j->tela == TELA_SELECAO) {
            Previa *pv = &j->previa[i];
            if (pv->personagem < 0 || pv->imagem.id == 0)
                continue;
            float giro = j->tela == TELA_MENU ? MENU_GIRO : PREVIA_GIRO;
            float virar = j->tela == TELA_MENU ? (i == 0 ? -MENU_VIRAR : MENU_VIRAR) * 0.65f : 0.0f;
            float angulo = virar + giro * sinf(2.0f * PI * pv->tempo / PREVIA_GIRO_PERIODO);
            render_previa(&pv->corpo, pv->personagem, i, &j->recursos, pv->imagem, angulo);
        } else if (j->tela == TELA_MONTAGEM) {
            PreviaGolpe *pv = &j->previa_golpe[i];
            if (!pv->ativa || pv->imagem.id == 0)
                continue;
            CameraLuta camera;
            render_iniciar_camera(&camera);
            camera.camera.position = (Vector3){ 0.25f, 1.95f, 3.8f };
            camera.camera.target = (Vector3){ 0.0f, 0.85f, 0.0f };
            camera.camera.fovy = 37.0f;
            BeginTextureMode(pv->imagem);
            ClearBackground((Color){ 38, 31, 39, 255 });
            render_cena_arena(&pv->partida, &j->recursos, &camera, false, j->arena);
            EndTextureMode();
        }
    }
}

/* Interface de apresentação: recebe estado pronto e nunca o modifica. */
static void apresentacao_texto(const Jogo *j, const char *s, float x, float y, float tam, Color cor)
{
    DrawTextEx(j->fonte_ui, s, (Vector2){x, y}, tam, 0.5f, cor);
}

static void apresentacao_centro(const Jogo *j, const char *s, float y, float tam, Color cor)
{
    while (tam > 14 && MeasureTextEx(j->fonte_ui, s, tam, 0.5f).x > JANELA_LARGURA-80) tam--;
    float w = MeasureTextEx(j->fonte_ui, s, tam, 0.5f).x;
    apresentacao_texto(j, s, (JANELA_LARGURA - w) / 2, y, tam, cor);
}

static void apresentacao_ajustado(const Jogo *j, const char *s, Rectangle r, float tam, Color cor)
{
    while (tam > 14 && MeasureTextEx(j->fonte_ui, s, tam, 0.5f).x > r.width) tam--;
    apresentacao_texto(j, s, r.x, r.y, tam, cor);
}

static Color apresentacao_cor(int i)
{
    return i == 0 ? (Color){255, 184, 92, 255} : (Color){108, 204, 225, 255};
}

static void apresentacao_painel(Rectangle r, bool selecionado)
{
    DrawRectangleRounded(r, 0.12f, 8, selecionado ? (Color){74, 55, 40, 245} : (Color){28, 32, 42, 235});
    DrawRectangleRoundedLinesEx(r, 0.12f, 8, selecionado ? 2.0f : 1.0f,
                               selecionado ? (Color){248, 193, 99, 255} : (Color){66, 73, 87, 255});
}

static void apresentacao_fundo(const Jogo *j, const char *titulo, const char *subtitulo)
{
    DrawRectangleGradientV(0, 0, JANELA_LARGURA, JANELA_ALTURA,
                          (Color){15, 23, 33, 255}, (Color){43, 28, 31, 255});
    DrawCircleLines(1130, 130, 210, (Color){80, 61, 45, 255});
    DrawCircleLines(1130, 130, 200, (Color){80, 61, 45, 255});
    apresentacao_texto(j, "SUMÔ-FU PANDA / ARENA", 40, 22, 18, apresentacao_cor(0));
    apresentacao_texto(j, titulo, 40, 51, 42, RAYWHITE);
    apresentacao_texto(j, subtitulo, 42, 101, 20, (Color){185, 193, 205, 255});
}

static bool apresentacao_playstation(int g)
{
    if (g < 0 || !IsGamepadAvailable(g)) return false;
    const char *s = GetGamepadName(g);
    return s && (strstr(s, "DualSense") || strstr(s, "Wireless Controller") ||
                 strstr(s, "Sony") || strstr(s, "PS5") || strstr(s, "PS4"));
}

static void apresentacao_botao(const Jogo *j, int jogador, int botao, float x, float y, const char *acao)
{
    int g = j->dispositivo[jogador];
    bool controle = g >= 0 && IsGamepadAvailable(g);
    bool ps = apresentacao_playstation(g);
    Rectangle r = {x, y, 50, 28};
    DrawRectangleRounded(r, 0.3f, 6, (Color){53, 61, 75, 255});
    const char *teclas[2][4] = {{"C", "X", "Z", "Tab"}, {"K", "J", "U", "Enter"}};
    const char *xbox[4] = {"A", "B", "Y", "Start"};
    if (ps && botao < 3) {
        Vector2 c = {x + 25, y + 14};
        if (botao == 0) {
            DrawLineEx((Vector2){c.x-7,c.y-7}, (Vector2){c.x+7,c.y+7}, 2, SKYBLUE);
            DrawLineEx((Vector2){c.x-7,c.y+7}, (Vector2){c.x+7,c.y-7}, 2, SKYBLUE);
        } else if (botao == 1) DrawCircleLines((int)c.x, (int)c.y, 9, PINK);
        else DrawTriangleLines((Vector2){c.x,c.y-9}, (Vector2){c.x-9,c.y+7}, (Vector2){c.x+9,c.y+7}, GREEN);
    } else {
        const char *s = controle ? (ps ? "Opts" : xbox[botao]) : teclas[jogador][botao];
        apresentacao_texto(j, s, x + 7, y + 3, 19, RAYWHITE);
    }
    apresentacao_texto(j, acao, x + 58, y + 3, 19, RAYWHITE);
}

static void apresentacao_menu_desenhar(const Jogo *j)
{
    apresentacao_fundo(j, "", "");
    const char *titulo = "SUMÔ-FU PANDA";
    float w = MeasureTextEx(j->recursos.fonte_titulo, titulo, 96, 2).x;
    DrawTextEx(j->recursos.fonte_titulo, titulo, (Vector2){(1280-w)/2, 82}, 96, 2, apresentacao_cor(0));
    apresentacao_centro(j, "Empurre. Derrube. Vença.", 198, 26, RAYWHITE);
    for (int i = 0; i < 2; i++)
        desenhar_textura(j->previa[i].imagem, (Rectangle){i == 0 ? 24 : 946, 240, 310, 430});
    const char *opcoes[] = {"Partida rápida", j->torneio.ativo ? "Continuar torneio" : "Torneio da turma", "Configurações", "Sair"};
    for (int k = 0; k < 4; k++) {
        apresentacao_painel((Rectangle){420, 294 + k*76, 440, 62}, j->menu.opcao == k);
        apresentacao_centro(j, opcoes[k], 306 + k*76, 30, RAYWHITE);
    }
    apresentacao_centro(j, "W/S ou setas: escolher   ·   C / K / Enter: confirmar   ·   Controle: direcional + botão inferior", 674, 18, LIGHTGRAY);
}

static void apresentacao_config_desenhar(const Jogo *j)
{
    apresentacao_fundo(j, "Configurações", "Ajuste a apresentação para a sua tela e para a sala.");
    const char *nomes[] = {"Tela cheia", "Música", "Efeitos e torcida", "Como jogar", "Voltar"};
    for (int k = 0; k < 5; k++) {
        float y = 165 + k*88;
        apresentacao_painel((Rectangle){230, y, 820, 70}, j->opcao_configuracoes == k);
        apresentacao_texto(j, nomes[k], 258, y+20, 27, RAYWHITE);
        if (k == 0) apresentacao_texto(j, IsWindowFullscreen() ? "ATIVADA" : "JANELA", 820, y+23, 22, apresentacao_cor(0));
        if (k == 1 || k == 2) {
            int valor = k == 1 ? j->volume_musica : j->volume_efeitos;
            for (int b = 0; b < 10; b++) DrawRectangle(650+b*25, (int)y+27, 18, 16, b < valor ? apresentacao_cor(0) : DARKGRAY);
            apresentacao_texto(j, TextFormat("%d%%", valor*10), 930, y+22, 23, RAYWHITE);
        }
    }
    apresentacao_centro(j, "C / K: confirmar   ·   Esquerda/direita: ajustar volume   ·   X / J / Esc: voltar   ·   F11: tela cheia", 654, 19, LIGHTGRAY);
}

static void apresentacao_entrada_desenhar(const Jogo *j)
{
    apresentacao_fundo(j, "Quem entra no ringue?", "Cada participante confirma o próprio dispositivo. Um controle pertence a um jogador.");
    for (int i = 0; i < 2; i++) {
        float x = 50 + i*620;
        bool pronto = j->dispositivo[i] != -2;
        apresentacao_painel((Rectangle){x, 180, 560, 365}, pronto);
        apresentacao_ajustado(j, j->nomes[i], (Rectangle){x+28, 210, 500, 40}, 40, apresentacao_cor(i));
        apresentacao_texto(j, pronto ? "CONECTADO" : "AGUARDANDO JOGADOR", x+28, 292, 24, pronto ? GREEN : LIGHTGRAY);
        const char *s = j->dispositivo[i] >= 0 ? GetGamepadName(j->dispositivo[i]) : "Teclado";
        if (pronto) apresentacao_ajustado(j, s ? s : "Controle", (Rectangle){x+28, 344, 500, 40}, 27, RAYWHITE);
        else {
            apresentacao_texto(j, i == 0 ? "Aperte C no teclado" : "Aperte K no teclado", x+28, 344, 28, RAYWHITE);
            apresentacao_texto(j, "ou o botão inferior do controle (X / A)", x+28, 392, 23, RAYWHITE);
        }
        apresentacao_texto(j, i == 0 ? "Teclado: WASD + C, X, Z, V, Tab" : "Teclado: setas + K, J, U, L, Enter", x+28, 478, 21, LIGHTGRAY);
    }
    apresentacao_centro(j, "Os dois conectados? A seleção abre automaticamente.   ·   Esc: voltar", 620, 23, LIGHTGRAY);
}

static void apresentacao_inscricao_desenhar(const Jogo *j)
{
    const Torneio *t = &j->torneio;
    apresentacao_fundo(j, "Torneio da turma", "Digite os nomes na ordem dos confrontos. O vencedor avança até a final.");
    apresentacao_painel((Rectangle){40, 148, 1200, 54}, t->campo == -1);
    apresentacao_texto(j, TextFormat("<   %d participantes   >", t->quantidade), 66, 159, 27, apresentacao_cor(0));
    for (int i = 0; i < t->quantidade; i++) {
        float x = 40 + (i/4)*620, y = 225 + (i%4)*82;
        apresentacao_painel((Rectangle){x, y, 580, 64}, t->campo == i);
        apresentacao_texto(j, TextFormat("%02d", i+1), x+18, y+19, 24, apresentacao_cor(i%2));
        apresentacao_ajustado(j, t->nomes[i], (Rectangle){x+70, y+16, 480, 36}, 30, RAYWHITE);
    }
    apresentacao_painel((Rectangle){390, 575, 500, 64}, t->campo == t->quantidade);
    apresentacao_centro(j, "Criar chave", 590, 30, RAYWHITE);
    apresentacao_centro(j, "Enter / Tab: próximo   ·   Shift+Tab: anterior   ·   Digite o nome   ·   Setas: 4 ou 8   ·   Esc: voltar", 668, 20, LIGHTGRAY);
}

static const char *torneio_nome(const Torneio *t, int p)
{
    return p >= 0 && p < t->quantidade ? t->nomes[p] : "A definir";
}

static void apresentacao_chave_desenhar(const Jogo *j)
{
    const Torneio *t = &j->torneio;
    bool terminou = t->atual == t->quantidade - 1;
    apresentacao_fundo(j, terminou ? "Temos um campeão!" : "Caminho até a final", "Eliminação simples · Os confrontos seguem a ordem de inscrição.");
    int indice = 0, coluna = 0;
    for (int n = t->quantidade/2; n >= 1; n /= 2, coluna++) {
        float x = 40 + coluna*410;
        apresentacao_texto(j, n == 4 ? "QUARTAS" : n == 2 ? "SEMIFINAL" : "FINAL", x, 157, 22, apresentacao_cor(0));
        for (int k = 0; k < n; k++, indice++) {
            float y = 200 + (k + 0.5f)*380/n - 42;
            if (n > 1) {
                float destino = 199 + (k/2 + 0.5f)*380/(n/2);
                Color linha = (Color){92, 85, 74, 255};
                DrawLineEx((Vector2){x+370, y+41}, (Vector2){x+390, y+41}, 2, linha);
                DrawLineEx((Vector2){x+390, y+41}, (Vector2){x+390, destino}, 2, linha);
                DrawLineEx((Vector2){x+390, destino}, (Vector2){x+410, destino}, 2, linha);
            }
            apresentacao_painel((Rectangle){x, y, 370, 82}, indice == t->atual);
            for (int lado = 0; lado < 2; lado++) {
                int p = t->dupla[indice][lado];
                bool venceu = p >= 0 && t->vencedor[indice] == p;
                apresentacao_ajustado(j, torneio_nome(t, p), (Rectangle){x+18, y+10+lado*36, 310, 28}, 24, venceu ? GREEN : RAYWHITE);
                if (venceu) apresentacao_texto(j, "+", x+338, y+10+lado*36, 24, GREEN);
            }
        }
    }
    if (terminou) apresentacao_centro(j, torneio_nome(t, t->vencedor[t->quantidade-2]), 601, 36, apresentacao_cor(0));
    else apresentacao_centro(j, "C / K / Enter ou X / A no controle: próximo confronto", 610, 26, RAYWHITE);
    apresentacao_centro(j, "Esc: menu (mantém a chave nesta sessão)   ·   N: iniciar outro torneio", 672, 21, LIGHTGRAY);
}

static void apresentacao_selecao_desenhar(const Jogo *j)
{
    apresentacao_fundo(j, "Escolham seus lutadores", "Esquerda e direita mudam o personagem. Cada jogador confirma a sua escolha.");
    for (int i = 0; i < 2; i++) {
        float x = 40 + i*630;
        apresentacao_painel((Rectangle){x, 151, 570, 492}, j->selecao.pronto[i]);
        apresentacao_ajustado(j, j->nomes[i], (Rectangle){x+22, 170, 520, 40}, 30, apresentacao_cor(i));
        desenhar_textura(j->previa[i].imagem, (Rectangle){x+130, 207, 300, 353});
        const Personagem *p = personagem_dados(j->selecao.escolha[i]);
        apresentacao_texto(j, p ? p->nome : "?", x+26, 555, 36, RAYWHITE);
        apresentacao_texto(j, j->selecao.pronto[i] ? "PRONTO" : "<  Escolher  >", x+365, 568, 24, j->selecao.pronto[i] ? GREEN : LIGHTGRAY);
        apresentacao_botao(j, i, 0, x+22, 602, "Confirmar");
        apresentacao_botao(j, i, 1, x+300, 602, "Voltar");
    }
}

static void apresentacao_arena_desenhar(const Jogo *j)
{
    apresentacao_fundo(j, "Escolham a arena", "Usem esquerda e direita para ver os cenários. Os dois jogadores confirmam.");
    desenhar_textura(j->imagem_arena, (Rectangle){220, 139, 840, 430});
    for (int i = 0; i < (int)(sizeof ARENAS / sizeof ARENAS[0]); i++) {
        float x = 92 + 368*i;
        apresentacao_painel((Rectangle){x, 578, 360, 54}, j->arena == i);
        apresentacao_texto(j, ARENAS[i], x+22, 590, 26, RAYWHITE);
    }
    for (int i = 0; i < 2; i++) {
        float x = 95 + i*580;
        apresentacao_botao(j, i, 0, x, 663, j->arena_pronto[i] ? "Confirmado" : "Confirmar");
        apresentacao_botao(j, i, 1, x+250, 663, "Voltar");
    }
}

static void apresentacao_montagem_desenhar(const Jogo *j)
{
    apresentacao_fundo(j, "Montem seus combos", "Cima/baixo: catálogo   ·   Esquerda/direita: posição   ·   Cada ataque avança; o último volta ao primeiro.");
    for (int i = 0; i < 2; i++) {
        float x = 28 + i*630;
        const Combo *c = &j->jogadores[i].combo;
        int cursor = j->montagem.cursor_catalogo[i];
        const Golpe *g = golpe_dados((TipoGolpe)cursor);
        apresentacao_painel((Rectangle){x, 144, 594, 520}, j->montagem.pronto[i]);
        apresentacao_ajustado(j, j->nomes[i], (Rectangle){x+16, 158, 410, 30}, 27, apresentacao_cor(i));
        apresentacao_texto(j, j->montagem.pronto[i] ? "PRONTO" : TextFormat("%d/%d golpes", c->tamanho, COMBO_MAX), x+442, 164, 21, j->montagem.pronto[i] ? GREEN : LIGHTGRAY);
        int primeiro = cursor - 3;
        if (primeiro < 0) primeiro = 0;
        if (primeiro > GOLPE_QTD-7) primeiro = GOLPE_QTD-7;
        for (int k = primeiro; k < primeiro+7 && k < GOLPE_QTD; k++) {
            float y = 205 + (k-primeiro)*32;
            if (k == cursor) DrawRectangleRounded((Rectangle){x+12, y-2, 198, 30}, 0.15f, 4, (Color){87, 63, 40, 255});
            apresentacao_ajustado(j, golpe_dados((TipoGolpe)k)->nome, (Rectangle){x+20,y,180,28}, 22, k == cursor ? apresentacao_cor(0) : LIGHTGRAY);
        }
        apresentacao_texto(j, TextFormat("%d-%d de %d  (cima/baixo)", primeiro+1, primeiro+7, GOLPE_QTD), x+19, 433, 16, LIGHTGRAY);
        if (j->previa_golpe[i].ativa) desenhar_textura(j->previa_golpe[i].imagem, (Rectangle){x+218, 203, 360, 174});
        apresentacao_texto(j, TextFormat("Energia %.0f   ·   Dano base %.0f%%", g->custo_stamina, g->dano), x+224, 386, 20, apresentacao_cor(i));
        apresentacao_texto(j, TextFormat("Preparo %.2fs   /   Total %.2fs", g->tempo_preparo, g->duracao), x+224, 415, 19, LIGHTGRAY);
        apresentacao_ajustado(j, g->descricao, (Rectangle){x+18, 455, 554, 36}, 22, RAYWHITE);
        float custo = 0;
        for (int k = 0; k < COMBO_MAX; k++) {
            const Golpe *item = golpe_dados(combo_golpe_em(c, k));
            float sx = x+14+k*142;
            apresentacao_painel((Rectangle){sx, 495, 136, 64}, j->montagem.cursor_combo[i] == k);
            apresentacao_texto(j, TextFormat("%d", k+1), sx+9, 500, 18, apresentacao_cor(i));
            apresentacao_ajustado(j, item ? item->nome : "Vazio", (Rectangle){sx+9,527,119,24}, 19, RAYWHITE);
            if (item) custo += item->custo_stamina;
        }
        apresentacao_texto(j, TextFormat("Ciclo completo: %.0f de energia · Recupera durante a luta", custo), x+17, 570, 18, LIGHTGRAY);
        apresentacao_botao(j, i, 0, x+16, 602, "Inserir");
        apresentacao_botao(j, i, 1, x+158, 602, "Remover");
        apresentacao_botao(j, i, 2, x+310, 602, "Trocar");
        apresentacao_botao(j, i, 3, x+450, 602, "Pronto");
    }
    apresentacao_centro(j, "Para começar: os dois precisam de um combo completo e confirmar Pronto.", 681, 20, LIGHTGRAY);
}

static void apresentacao_versus_desenhar(const Jogo *j)
{
    apresentacao_fundo(j, j->em_torneio ? "Confronto do torneio" : "O próximo confronto", "Derrube o adversário ou empurre para fora do ringue.");
    apresentacao_centro(j, "VS", 310, 100, apresentacao_cor(0));
    for (int i = 0; i < 2; i++) {
        float x = i == 0 ? 70 : 760;
        apresentacao_painel((Rectangle){x, 230, 450, 240}, false);
        apresentacao_ajustado(j, j->nomes[i], (Rectangle){x+24, 270, 400, 60}, 45, apresentacao_cor(i));
        apresentacao_texto(j, personagem_dados(j->jogadores[i].personagem)->nome, x+24, 352, 34, RAYWHITE);
        apresentacao_texto(j, TextFormat("Primeiro a vencer %d rounds", PARTIDA_ROUNDS_PARA_VENCER), x+24, 420, 22, LIGHTGRAY);
    }
    apresentacao_centro(j, ARENAS[j->arena], 570, 32, RAYWHITE);
}

static void apresentacao_hud_desenhar(const Jogo *j, const InfoHud *h)
{
    for (int i = 0; i < 2; i++) {
        float x = i == 0 ? 24 : 836;
        apresentacao_painel((Rectangle){x, 18, 420, 144}, false);
        apresentacao_ajustado(j, j->nomes[i], (Rectangle){x+16, 28, 265, 30}, 27, apresentacao_cor(i));
        apresentacao_texto(j, TextFormat("%.0f%%", h->dano[i]), x+290, 36, 40+6*h->impacto[i], RAYWHITE);
        apresentacao_texto(j, h->nome[i], x+16, 60, 20, LIGHTGRAY);
        DrawRectangle((int)x+16, 94, 388, 10, DARKGRAY);
        DrawRectangle((int)x+16, 94, (int)(388*fmaxf(0, fminf(1, h->stamina[i]))), 10, apresentacao_cor(i));
        apresentacao_texto(j, TextFormat("ENERGIA %.0f%%", h->stamina[i]*100), x+16, 113, 18, LIGHTGRAY);
        apresentacao_texto(j, TextFormat("%d / %d rounds", h->vitorias[i], h->rounds_para_vencer), x+270, 113, 20, RAYWHITE);
        apresentacao_painel((Rectangle){x, 613, 420, 87}, false);
        int pos = h->combo_atual[i];
        apresentacao_texto(j, "SEQUÊNCIA", x+14, 620, 16, LIGHTGRAY);
        for (int k = 0; k < h->combo_tamanho[i]; k++) {
            const Golpe *g = golpe_dados(h->combo[i][k]);
            float sx = x+10+k*101;
            Color texto = k == pos ? BLACK : RAYWHITE;
            DrawRectangleRounded((Rectangle){sx, 642, 97, 47}, 0.18f, 4,
                                 k == pos ? apresentacao_cor(i) : (Color){49,54,65,255});
            apresentacao_texto(j, k == pos ? "PRÓXIMO" : TextFormat("%d", k+1), sx+6, 645, 13, texto);
            apresentacao_ajustado(j, g ? g->nome : "", (Rectangle){sx+6,663,85,22}, 18, texto);
        }
    }
    apresentacao_centro(j, TextFormat("ROUND %d", h->round), 30, 28, RAYWHITE);
    if (h->contagem >= 0) apresentacao_centro(j, h->contagem == 0 ? "LUTE!" : TextFormat("%d", h->contagem), 280, 96, apresentacao_cor(0));
    if (h->mensagem) apresentacao_centro(j, h->mensagem, 250, 60, RAYWHITE);
    if (h->pausado) {
        DrawRectangle(0, 0, 1280, 720, Fade(BLACK, 0.8f));
        apresentacao_centro(j, "PAUSA", 210, 64, RAYWHITE);
        apresentacao_centro(j, "Reconecte o controle ou use o teclado do jogador.", 304, 26, LIGHTGRAY);
        for (int i = 0; i < 2; i++) {
            apresentacao_botao(j, i, 3, 270+i*430, 380, "Continuar");
            apresentacao_botao(j, i, 1, 270+i*430, 434, "Menu");
        }
    }
}

static void apresentacao_resultado_desenhar(const Jogo *j)
{
    int v = j->resultado.vencedor;
    apresentacao_fundo(j, "Resultado da partida", "Os acertos e o dano acumulam todos os rounds; a duração exclui pausas e contagens.");
    apresentacao_centro(j, v >= 0 ? TextFormat("%s venceu!", j->nomes[v]) : "Empate", 150, 44, apresentacao_cor(v == 1));
    apresentacao_centro(j, TextFormat("%d  :  %d", j->estatisticas.placar[0], j->estatisticas.placar[1]), 208, 58, RAYWHITE);
    for (int i = 0; i < 2; i++) {
        float x = 120+i*570;
        apresentacao_painel((Rectangle){x, 295, 470, 142}, v == i);
        apresentacao_ajustado(j, j->nomes[i], (Rectangle){x+22, 310, 420, 30}, 27, apresentacao_cor(i));
        apresentacao_texto(j, TextFormat("%d acertos", j->estatisticas.acertos[i]), x+22, 356, 25, RAYWHITE);
        apresentacao_texto(j, TextFormat("%.1f%% de dano causado", j->estatisticas.dano[i]), x+22, 394, 24, LIGHTGRAY);
    }
    int segundos = (int)j->estatisticas.duracao;
    apresentacao_centro(j, TextFormat("Tempo de combate   %02d:%02d", segundos/60, segundos%60), 453, 25, LIGHTGRAY);
    const char *rapida[] = {"Revanche imediata", "Trocar personagens", "Remontar combos", "Menu inicial"};
    const char *torneio[] = {"Avançar no torneio", "Menu inicial"};
    int n = j->em_torneio ? 2 : 4;
    for (int k = 0; k < n; k++) {
        float y = 500+k*44;
        apresentacao_painel((Rectangle){390,y,500,38}, j->resultado.opcao == k);
        apresentacao_centro(j, j->em_torneio ? torneio[k] : rapida[k], y+5, 25, RAYWHITE);
    }
}

static void apresentacao_ajuda_desenhar(const Jogo *j)
{
    apresentacao_fundo(j, "Como jogar", "Faça o adversário cair ou sair do ringue. Cada ataque usa o próximo golpe do seu combo.");
    /* Colunas fixas por conteúdo, não pela largura variável dos espaços. */
    const char *acao[] = {"Mover", "Atacar / confirmar", "Voltar / remover", "Trocar ordem", "Firmar o corpo", "Pronto / pausa"};
    const char *j1[] = {"WASD", "C", "X", "Z", "V (segurar)", "Tab"};
    const char *j2[] = {"Setas", "K", "J", "U", "L (segurar)", "Enter"};
    const char *pad[] = {"Analógico esquerdo", "X / A", "Círculo / B", "Triângulo / Y", "R2 / RT (segurar)", "Options / Start"};
    apresentacao_texto(j, "AÇÃO", 70, 174, 20, LIGHTGRAY);
    apresentacao_texto(j, "JOGADOR 1", 420, 174, 20, apresentacao_cor(0));
    apresentacao_texto(j, "JOGADOR 2", 650, 174, 20, apresentacao_cor(1));
    apresentacao_texto(j, "PS5 / XBOX", 890, 174, 20, LIGHTGRAY);
    for (int k = 0; k < 6; k++) {
        float y = 220+k*61;
        apresentacao_painel((Rectangle){48,y-8,1184,52}, false);
        apresentacao_texto(j, acao[k], 70,y,25,RAYWHITE);
        apresentacao_texto(j, j1[k], 420,y,25,RAYWHITE);
        apresentacao_texto(j, j2[k], 650,y,25,RAYWHITE);
        apresentacao_texto(j, pad[k], 890,y,25,RAYWHITE);
    }
    apresentacao_centro(j, "Energia recarrega; dano aumenta sua vulnerabilidade e cura com o tempo.   ·   Esc: voltar", 644, 23, LIGHTGRAY);
}

static void desenhar(Jogo *j)
{
    BeginDrawing();
    renderizar_previas(j);
    if (j->quadro.id)
        BeginTextureMode(j->quadro);
    projetar_interface();
    ClearBackground((Color){ 32, 28, 36, 255 });

    switch (j->tela) {
    case TELA_MENU: apresentacao_menu_desenhar(j); break;
    case TELA_CONFIGURACOES: apresentacao_config_desenhar(j); break;
    case TELA_SELECAO: apresentacao_selecao_desenhar(j); break;
    case TELA_ARENA: apresentacao_arena_desenhar(j); break;
    case TELA_MONTAGEM: apresentacao_montagem_desenhar(j); break;
    case TELA_LUTA: {
        InfoHud info;
        render_cena_arena(&j->partida, &j->recursos, &j->camera, j->depuracao, j->arena);
        partida_preencher_hud(&j->partida, &info);
        apresentacao_hud_desenhar(j, &info);
    } break;
    case TELA_RESULTADO: apresentacao_resultado_desenhar(j); break;
    case TELA_ENTRADA: apresentacao_entrada_desenhar(j); break;
    case TELA_INSCRICAO: apresentacao_inscricao_desenhar(j); break;
    case TELA_CHAVE: apresentacao_chave_desenhar(j); break;
    case TELA_VERSUS: apresentacao_versus_desenhar(j); break;
    case TELA_AJUDA: apresentacao_ajuda_desenhar(j); break;
    }

    if (j->depuracao)
        DrawFPS(10, JANELA_ALTURA - 30);

    if (j->quadro.id) {
        EndTextureMode();
        ClearBackground(BLACK);
        desenhar_textura(j->quadro, (Rectangle){ 0, 0, GetScreenWidth(), GetScreenHeight() });
    }
    EndDrawing();
}

int main(void)
{
    /* static: a struct é grande (modelos e sons) para ficar na pilha. */
    static Jogo jogo;
    Jogo *j = &jogo;

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(JANELA_LARGURA, JANELA_ALTURA, JANELA_TITULO);
    InitAudioDevice();
    SetExitKey(KEY_NULL);                 /* ESC não fecha o jogo */
    SetTargetFPS(JANELA_FPS);

    fisica_iniciar_biblioteca();
    recursos_carregar(&j->recursos);
    int caracteres[224];
    for (int i = 0; i < 224; i++) caracteres[i] = i + 32;
    j->fonte_ui = LoadFontEx(TextFormat("%sassets/fontes/Barlow-Regular.ttf", GetApplicationDirectory()), 96, caracteres, 224);
    if (!IsFontValid(j->fonte_ui)) j->fonte_ui = GetFontDefault();
    SetTextureFilter(j->fonte_ui.texture, TEXTURE_FILTER_BILINEAR);
    j->volume_musica = 3;
    j->volume_efeitos = 8;
    FILE *config = fopen(TextFormat("%sconfiguracoes-locais.txt", GetApplicationDirectory()), "r");
    if (config) {
        int musica, efeitos, cheia;
        if (fscanf(config, "%d %d %d", &musica, &efeitos, &cheia) == 3 &&
            musica >= 0 && musica <= 10 && efeitos >= 0 && efeitos <= 10 && (cheia == 0 || cheia == 1)) {
            j->volume_musica = musica; j->volume_efeitos = efeitos;
            if (cheia) ToggleFullscreen();
        }
        fclose(config);
    }
    const char *caminho = TextFormat("%sassets/sons/arena_musica.wav", GetApplicationDirectory());
    if (IsAudioDeviceReady() && FileExists(caminho)) {
        j->musica = LoadMusicStream(caminho);
        j->tem_musica = IsMusicValid(j->musica);
        if (j->tem_musica) { j->musica.looping = true; PlayMusicStream(j->musica); }
    }
    caminho = TextFormat("%sassets/sons/arena_torcida.wav", GetApplicationDirectory());
    if (IsAudioDeviceReady() && FileExists(caminho)) {
        j->torcida = LoadSound(caminho);
        j->tem_torcida = IsSoundValid(j->torcida);
    }
    apresentacao_volumes(j);
    for (int i = 0; i < 2; i++) j->dispositivo[i] = -2;
    filtrar_modelos(&j->recursos);
    render_iniciar_camera(&j->camera);
    for (int i = 0; i < 2; i++)
        previa_criar(&j->previa[i], i);
    atualizar_resolucao(j);
    for (int i = 0; i < 2; i++) {
        j->jogadores[i].personagem = 0;
        combo_iniciar(&j->jogadores[i].combo);
    }
    menu_iniciar(&j->menu);
    j->tela = TELA_MENU;

    while (!WindowShouldClose() && !j->sair) {
        float dt = GetFrameTime();
        if (dt > 0.1f) dt = 0.1f;        /* quadro travado (ex.: arrastar a janela) não vira salto */
        atualizar(j, dt);
        atualizar_resolucao(j);
        desenhar(j);
    }

    encerrar_luta(j);
    for (int i = 0; i < 2; i++)
        combo_liberar(&j->jogadores[i].combo);
    for (int i = 0; i < 2; i++)
        previa_liberar(&j->previa[i]);
    limpar_previas_golpes(j);
    for (int i = 0; i < 2; i++)
        if (j->previa_golpe[i].imagem.id != 0)
            UnloadRenderTexture(j->previa_golpe[i].imagem);
    recursos_liberar(&j->recursos);
    if (j->tem_musica) UnloadMusicStream(j->musica);
    if (j->tem_torcida) UnloadSound(j->torcida);
    if (j->fonte_ui.texture.id != GetFontDefault().texture.id) UnloadFont(j->fonte_ui);
    if (j->quadro.id)
        UnloadRenderTexture(j->quadro);
    if (j->imagem_arena.id)
        UnloadRenderTexture(j->imagem_arena);
    CloseAudioDevice();
    CloseWindow();
    fisica_finalizar_biblioteca();
    return 0;
}
