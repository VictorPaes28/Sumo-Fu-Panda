/*
 * main.c — janela, laço principal e troca de telas.
 *
 * Fluxo de telas:
 *   MENU -> SELECAO -> MONTAGEM -> LUTA -> RESULTADO
 *                ^          ^                 |
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
#include <stdbool.h>
#include "raylib.h"
#include "config.h"
#include "entrada.h"
#include "recursos.h"
#include "telas.h"
#include "montagem.h"
#include "hud.h"
#include "partida.h"
#include "personagem.h"
#include "render.h"

typedef enum {
    TELA_MENU, TELA_SELECAO, TELA_MONTAGEM, TELA_LUTA, TELA_RESULTADO
} Tela;

typedef struct {
    Tela tela;
    bool sair;
    bool depuracao;
    Jogador jogadores[2];        /* personagem e combo: duram o jogo inteiro */
    TelaMenu menu;
    TelaSelecao selecao;
    TelaMontagem montagem;
    TelaResultado resultado;
    Partida partida;
    bool partida_ativa;          /* true entre partida_iniciar e partida_liberar */
    bool controle_antes[2];      /* para pausar quando um controle desconecta */
    Recursos recursos;
    CameraLuta camera;           /* câmera da luta (acompanha e treme) */
} Jogo;

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
        j->partida_ativa = true;
        render_iniciar_camera(&j->camera);
        j->tela = TELA_LUTA;
    } else {
        TraceLog(LOG_ERROR, "Falha ao iniciar a partida");
        menu_iniciar(&j->menu);
        j->tela = TELA_MENU;
    }
}

static void atualizar_luta(Jogo *j, const Intencao in[2], float dt)
{
    Partida *p = &j->partida;

    /* Controle desconectou no meio da luta: pausa. */
    for (int i = 0; i < 2; i++) {
        if (j->controle_antes[i] && !in[i].usando_controle)
            partida_pausar(p, true);
        j->controle_antes[i] = in[i].usando_controle;
    }

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

    partida_atualizar(p, in, dt);
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

static void atualizar(Jogo *j, float dt)
{
    Intencao in[2] = { entrada_ler(0), entrada_ler(1) };

    if (IsKeyPressed(KEY_F1))
        j->depuracao = !j->depuracao;

    if (j->tela != TELA_LUTA)
        tocar_sons_de_menu(j, in);

    switch (j->tela) {
    case TELA_MENU: {
        AcaoMenu a = menu_atualizar(&j->menu, in);
        if (a == MENU_JOGAR) {
            selecao_iniciar(&j->selecao);
            j->tela = TELA_SELECAO;
        } else if (a == MENU_SAIR) {
            j->sair = true;
        }
    } break;

    case TELA_SELECAO: {
        AcaoSelecao a = selecao_atualizar(&j->selecao, in);
        if (a == SELECAO_CONCLUIDA) {
            for (int i = 0; i < 2; i++)
                j->jogadores[i].personagem = j->selecao.escolha[i];
            montagem_iniciar(&j->montagem);
            j->tela = TELA_MONTAGEM;
        } else if (a == SELECAO_VOLTAR) {
            menu_iniciar(&j->menu);
            j->tela = TELA_MENU;
        }
    } break;

    case TELA_MONTAGEM: {
        AcaoMontagem a = montagem_atualizar(&j->montagem, &j->jogadores[0].combo,
                                            &j->jogadores[1].combo, in);
        if (a == MONTAGEM_CONCLUIDA) {
            for (int i = 0; i < 2; i++) j->controle_antes[i] = in[i].usando_controle;
            iniciar_luta(j);
        } else if (a == MONTAGEM_VOLTAR) {
            selecao_iniciar(&j->selecao);
            j->tela = TELA_SELECAO;
        }
    } break;

    case TELA_LUTA:
        atualizar_luta(j, in, dt);
        break;

    case TELA_RESULTADO: {
        AcaoResultado a = resultado_atualizar(&j->resultado, in);
        if (a == RESULTADO_REVANCHE) {
            iniciar_luta(j);
        } else if (a == RESULTADO_REMONTAR) {
            montagem_iniciar(&j->montagem);
            j->tela = TELA_MONTAGEM;
        } else if (a == RESULTADO_MENU) {
            menu_iniciar(&j->menu);
            j->tela = TELA_MENU;
        }
    } break;
    }
}

/* Não muda o estado do jogo: só desenha (e posiciona os ossos dos modelos). */
static void desenhar(Jogo *j)
{
    BeginDrawing();
    ClearBackground((Color){ 32, 28, 36, 255 });

    switch (j->tela) {
    case TELA_MENU:      menu_desenhar(&j->menu);           break;
    case TELA_SELECAO:   selecao_desenhar(&j->selecao);     break;
    case TELA_MONTAGEM:
        montagem_desenhar(&j->montagem, &j->jogadores[0].combo, &j->jogadores[1].combo);
        break;
    case TELA_LUTA: {
        InfoHud info;
        render_cena(&j->partida, &j->recursos, &j->camera, j->depuracao);
        partida_preencher_hud(&j->partida, &info);
        hud_desenhar(&info);
    } break;
    case TELA_RESULTADO: resultado_desenhar(&j->resultado); break;
    }

    if (j->depuracao)
        DrawFPS(10, JANELA_ALTURA - 30);

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
    render_iniciar_camera(&j->camera);
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
        desenhar(j);
    }

    encerrar_luta(j);
    for (int i = 0; i < 2; i++)
        combo_liberar(&j->jogadores[i].combo);
    recursos_liberar(&j->recursos);
    CloseAudioDevice();
    CloseWindow();
    fisica_finalizar_biblioteca();
    return 0;
}
