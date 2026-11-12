/*
 * demo_partida.c — partida completa contra um "robô", sem as telas de menu.
 *
 *   demo_partida             janela: você (controle 1 ou teclado J1) contra o robô
 *   demo_partida --teste     sem janela: 20 partidas robô contra robô, confere as
 *                            regras (rounds, vencedor, hematomas, física estável)
 *   demo_partida --capturas  janela: robô contra robô, salva capturas PNG em
 *                            momentos da luta e fecha sozinho
 *
 * Nos modos com janela, dois números no fim escolhem os personagens
 * (índices do catálogo). Ex.: "demo_partida --capturas 1 1" = Tai Lung contra Tai Lung.
 *
 * O robô anda até o adversário e golpeia quando está perto. Serve para testar
 * a luta sem precisar de duas pessoas. Não faz parte do jogo final.
 *
 * Dono: Principal.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "raylib.h"
#include "raymath.h"
#include "config.h"
#include "partida.h"
#include "personagem.h"
#include "render.h"
#include "recursos.h"
#include "hud.h"

/* ------------------------------------------------------------ Robô */

typedef struct {
    float relogio;       /* tempo próprio do robô (s)          */
    float proximo_golpe; /* quando tentar o próximo golpe (s)  */
    unsigned semente;    /* para variar o comportamento         */
} Robo;

static float aleatorio(Robo *r)
{
    r->semente = r->semente * 1103515245u + 12345u;
    return (float)((r->semente >> 16) & 0x7fff) / 32767.0f;
}

/* Converte uma direção no chão (mundo) para o analógico (tela). */
static Vector2 chao_para_tela(Vector2 d)
{
    Vector2 frente = Vector2Normalize((Vector2){ CAMERA_ALVO_X - CAMERA_POS_X,
                                                 CAMERA_ALVO_Z - CAMERA_POS_Z });
    Vector2 direita = { -frente.y, frente.x };
    return (Vector2){ Vector2DotProduct(d, direita), Vector2DotProduct(d, frente) };
}

static Intencao robo_pensar(Robo *r, const Lutador *eu, const Lutador *outro, float dt)
{
    Intencao in = entrada_vazia();
    r->relogio += dt;

    Vector3 a = corpo_posicao(&eu->corpo), b = corpo_posicao(&outro->corpo);
    Vector2 para_ele = { b.x - a.x, b.z - a.z };
    float dist = Vector2Length(para_ele);
    Vector2 dir = (dist > 0.01f) ? Vector2Scale(para_ele, 1.0f / dist) : (Vector2){ 0 };

    /* Anda sempre na direção dele (empurra). Perto da borda, também puxa para
     * o centro para não sair sozinho. */
    Vector2 desejo = dir;
    float raio = Vector2Length((Vector2){ a.x, a.z });
    if (raio > RINGUE_RAIO * 0.75f)
        desejo = Vector2Add(desejo, Vector2Scale((Vector2){ -a.x / raio, -a.z / raio }, 0.7f));
    if (Vector2Length(desejo) > 1.0f) desejo = Vector2Normalize(desejo);
    in.mover = chao_para_tela(desejo);

    /* Só golpeia com stamina sobrando para um golpe forte de vez em quando. */
    if (dist < 1.0f && r->relogio >= r->proximo_golpe && eu->stamina > 30.0f) {
        in.a = true;
        r->proximo_golpe = r->relogio + 0.35f + 0.5f * aleatorio(r);
    }
    in.firmar = (dist < 0.8f && aleatorio(r) < 0.02f);
    return in;
}

static void montar_combo(Combo *c, int variante)
{
    static const TipoGolpe combos[6][COMBO_MAX] = {
        { GOLPE_EMPURRAO, GOLPE_EMPURRAO, GOLPE_TAPA_LATERAL, GOLPE_INVESTIDA },
        { GOLPE_LEVANTADOR, GOLPE_EMPURRAO, GOLPE_BARRIGADA, GOLPE_TAPA_LATERAL },
        { GOLPE_TAPA_LATERAL, GOLPE_BARRIGADA, GOLPE_EMPURRAO, GOLPE_INVESTIDA },
        { GOLPE_EMPURRAR_FORA, GOLPE_FORCAR_FORA, GOLPE_TAPA_BAIXO, GOLPE_EMPURRAO },
        { GOLPE_FORCAR_FORA, GOLPE_PROJECAO_CIMA, GOLPE_EMPURRAO, GOLPE_PUXAR_BAIXO },
        { GOLPE_EMPURRAR_FORA, GOLPE_PROJECAO_BAIXO, GOLPE_TAPA_LATERAL, GOLPE_TAPA_BAIXO },
    };
    combo_liberar(c);
    for (int k = 0; k < COMBO_MAX; k++)
        combo_inserir(c, k, combos[variante % 6][k]);
}

/* ------------------------------------------------------------ Verificação */

static int verificar(void)
{
    static Jogador jogadores[2];
    static Partida partida;
    int falhas = 0, vitorias[2] = { 0, 0 };
    float maior_dano = 0.0f, soma_duracao = 0.0f;
    int rounds_total = 0, acertos = 0, curas_ok = 1;

    fisica_iniciar_biblioteca();
    for (int i = 0; i < 2; i++) combo_iniciar(&jogadores[i].combo);

    for (int n = 0; n < 20; n++) {
        jogadores[0].personagem = n % personagem_quantidade();
        jogadores[1].personagem = (n / 2) % personagem_quantidade();
        montar_combo(&jogadores[0].combo, n);
        montar_combo(&jogadores[1].combo, n + 1);
        Robo robos[2] = { { 0, 0, 7u + n }, { 0, 0.2f, 99u + n * 3 } };

        if (!partida_iniciar(&partida, jogadores)) { printf("partida_iniciar falhou\n"); return 1; }
        float t = 0.0f;
        const float dt = 1.0f / 60.0f;
        int ultimo_round = 1;
        while (partida.estado != PARTIDA_FIM && t < 600.0f) {
            Intencao in[2];
            for (int i = 0; i < 2; i++)
                in[i] = robo_pensar(&robos[i], &partida.lutadores[i], &partida.lutadores[1 - i], dt);
            float dano_antes = partida.round_ativo ? lutador_dano(&partida.lutadores[0]) : 0.0f;
            partida_atualizar(&partida, in, dt);
            if (partida.eventos.acerto_leve || partida.eventos.acerto_forte) acertos++;
            if (partida.round_ativo) {
                for (int i = 0; i < 2; i++) {
                    float d = lutador_dano(&partida.lutadores[i]);
                    if (d > maior_dano) maior_dano = d;
                    if (d < 0.0f) curas_ok = 0;
                }
                (void)dano_antes;
            }
            if (partida.round != ultimo_round) { ultimo_round = partida.round; }
            /* Diagnóstico: SUMO_DIAG=<n> imprime a partida n a cada 5 s. */
            if (getenv("SUMO_DIAG") && n == atoi(getenv("SUMO_DIAG")) && partida.round_ativo &&
                fmodf(t, 5.0f) < dt) {
                for (int i = 0; i < 2; i++) {
                    const Lutador *l = &partida.lutadores[i];
                    Vector3 p = corpo_posicao(&l->corpo);
                    printf("t=%5.1f L%d pos=(%5.2f,%5.2f,%5.2f) dano=%3.0f st=%3.0f estado=%d\n",
                           t, i, p.x, p.y, p.z, lutador_dano(l), l->stamina, l->estado);
                }
            }
            t += dt;
        }
        if (partida.estado != PARTIDA_FIM) {
            printf("[FALHOU] partida %d nao terminou em 10 minutos (placar %d x %d)\n",
                   n, partida.vitorias[0], partida.vitorias[1]);
            falhas++;
        } else {
            int v = partida_vencedor(&partida);
            vitorias[v]++;
            rounds_total += partida.vitorias[0] + partida.vitorias[1];
            soma_duracao += t;
            if (partida.vitorias[v] != PARTIDA_ROUNDS_PARA_VENCER) {
                printf("[FALHOU] vencedor com %d rounds\n", partida.vitorias[v]);
                falhas++;
            }
        }
        partida_liberar(&partida);
    }

    printf("partidas: 20 | vitorias J1=%d J2=%d | rounds decididos=%d | duracao media=%.0f s\n",
           vitorias[0], vitorias[1], rounds_total, soma_duracao / 20.0f);
    printf("quadros com acerto=%d | maior dano visto=%.0f%%\n", acertos, maior_dano);
    if (acertos == 0) { printf("[FALHOU] nenhum golpe acertou\n"); falhas++; }
    if (!curas_ok) { printf("[FALHOU] dano negativo\n"); falhas++; }
    if (maior_dano <= 0.0f) { printf("[FALHOU] nenhum hematoma\n"); falhas++; }

    for (int i = 0; i < 2; i++) combo_liberar(&jogadores[i].combo);
    fisica_finalizar_biblioteca();
    printf("%s\n", falhas == 0 ? "PARTIDA OK" : "PARTIDA COM FALHAS");
    return falhas == 0 ? 0 : 1;
}

/* ------------------------------------------------------------ Janela */

static int janela(bool capturas, const int personagens[2])
{
    static Jogador jogadores[2];
    static Partida partida;
    static Recursos recursos;
    CameraLuta camera;

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(JANELA_LARGURA, JANELA_ALTURA, "demo_partida");
    InitAudioDevice();
    SetTargetFPS(JANELA_FPS);
    fisica_iniciar_biblioteca();
    recursos_carregar(&recursos);
    render_iniciar_camera(&camera);

    for (int i = 0; i < 2; i++) {
        jogadores[i].personagem = personagens[i];
        combo_iniciar(&jogadores[i].combo);
        montar_combo(&jogadores[i].combo, i);
    }
    partida_iniciar(&partida, jogadores);
    Robo robos[2] = { { 0, 0, 11u }, { 0, 0.3f, 23u } };

    /* Momentos das capturas (s): contagem, luta, golpes, fim de round. */
    const float momentos[] = { 1.5f, 4.5f, 6.0f, 8.0f, 11.0f, 15.0f, 20.0f };
    const int qtd_momentos = (int)(sizeof momentos / sizeof momentos[0]);
    int proxima = 0;
    float t = 0.0f;
    bool depuracao = false;

    while (!WindowShouldClose()) {
        float dt = capturas ? 1.0f / 60.0f : GetFrameTime();
        Intencao in[2];
        if (partida.round_ativo) {
            in[0] = capturas ? robo_pensar(&robos[0], &partida.lutadores[0], &partida.lutadores[1], dt)
                             : entrada_ler(0);
            in[1] = robo_pensar(&robos[1], &partida.lutadores[1], &partida.lutadores[0], dt);
        } else {
            in[0] = in[1] = entrada_vazia();
        }
        if (IsKeyPressed(KEY_F1)) depuracao = !depuracao;
        if (in[0].start) partida_pausar(&partida, !partida.pausada);

        partida_atualizar(&partida, in, dt);
        if (partida.estado == PARTIDA_FIM) {
            partida_liberar(&partida);
            partida_iniciar(&partida, jogadores);
        }

        InfoHud hud;
        BeginDrawing();
        ClearBackground((Color){ 32, 28, 36, 255 });
        render_atualizar_camera(&camera, &partida, dt);
        render_cena(&partida, &recursos, &camera, depuracao);
        partida_preencher_hud(&partida, &hud);
        hud_desenhar(&hud);
        EndDrawing();

        t += dt;
        if (capturas && proxima < qtd_momentos && t >= momentos[proxima]) {
            TakeScreenshot(TextFormat("captura_luta_%d.png", proxima + 1));
            proxima++;
            if (proxima == qtd_momentos) break;
        }
    }

    partida_liberar(&partida);
    for (int i = 0; i < 2; i++) combo_liberar(&jogadores[i].combo);
    recursos_liberar(&recursos);
    CloseAudioDevice();
    CloseWindow();
    fisica_finalizar_biblioteca();
    return 0;
}

int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "--teste") == 0)
        return verificar();

    bool capturas = (argc > 1 && strcmp(argv[1], "--capturas") == 0);
    int primeiro = capturas ? 2 : 1;               /* onde começam os números */
    int personagens[2] = { 0, 1 % personagem_quantidade() };
    for (int i = 0; i < 2 && primeiro + i < argc; i++) {
        int p = atoi(argv[primeiro + i]);
        if (p >= 0 && p < personagem_quantidade())
            personagens[i] = p;
    }
    return janela(capturas, personagens);
}
