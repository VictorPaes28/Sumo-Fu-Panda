/*
 * partida.c — rounds, placar e o laço de passos fixos da física.
 * Especificação: partida.h. Ordem de um quadro: docs/ARQUITETURA.md, seção 7.
 *
 * Dono: Principal.
 */
#include <math.h>
#include "partida.h"
#include "config.h"
#include "personagem.h"
#include "raymath.h"

/* ------------------------------------------------------------ Round */

static bool iniciar_round(Partida *p)
{
    if (!fisica_iniciar(&p->fisica))
        return false;

    for (int i = 0; i < 2; i++) {
        const Personagem *per = personagem_dados(p->jogadores[i].personagem);
        if (per == NULL)
            per = personagem_dados(0);
        float x = (i == 0 ? -0.5f : 0.5f) * RINGUE_DISTANCIA_INICIAL;
        float angulo = (i == 0) ? PI / 2.0f : -PI / 2.0f;    /* um de frente para o outro */
        lutador_criar(&p->lutadores[i], &p->fisica, i, per, &p->jogadores[i].combo,
                      (Vector3){ x, 0.0f, 0.0f }, angulo);
    }

    p->round_ativo = true;
    p->estado = PARTIDA_CONTAGEM;
    p->tempo_no_estado = 0.0f;
    p->acumulador = 0.0f;
    p->mensagem = NULL;
    p->perdedor_round = -1;
    p->contagem_anterior = 0;
    p->congelado = 0.0f;
    return true;
}

static void encerrar_round(Partida *p)
{
    if (!p->round_ativo)
        return;
    lutador_destruir(&p->lutadores[0]);
    lutador_destruir(&p->lutadores[1]);
    fisica_liberar(&p->fisica);
    p->round_ativo = false;
}

bool partida_iniciar(Partida *p, Jogador jogadores[2])
{
    *p = (Partida){0};
    p->jogadores = jogadores;
    p->round = 1;
    return iniciar_round(p);
}

bool partida_fora_do_ringue(const Lutador *l)
{
    Vector3 b = corpo_posicao(&l->corpo);
    return Vector2Length((Vector2){ b.x, b.z }) > RINGUE_RAIO;
}

/* ------------------------------------------------------------ Quadro */

/* Passos fixos de física até consumir o tempo do quadro. */
static void simular(Partida *p, float dt)
{
    bool lutando = (p->estado == PARTIDA_LUTA);
    p->acumulador += dt;

    int passos = 0;
    while (p->acumulador >= FISICA_DT && passos < FISICA_MAX_PASSOS_POR_QUADRO) {
        lutador_passo_fisica(&p->lutadores[0]);     /* forças reaplicadas a cada passo */
        lutador_passo_fisica(&p->lutadores[1]);
        fisica_passo(&p->fisica);
        for (int i = 0; i < 2; i++) {
            const Lutador *l = &p->lutadores[i];
            bool quer = lutando && l->estado == LUTADOR_GOLPEANDO && l->golpe_atual->agarra;
            corpo_agarrar(&p->lutadores[i].corpo, &p->lutadores[1 - i].corpo, &p->fisica, quer);
        }

        /* Todo choque entre os lutadores (golpe ou encontrão) faz a região
         * atingida ceder um pouco, dos dois lados. */
        for (int i = 0; i < p->fisica.qtd_contatos; i++) {
            const Contato *c = &p->fisica.contatos[i];
            corpo_receber_contato(&p->lutadores[c->a.lutador].corpo, c->a.parte, c->intensidade);
            corpo_receber_contato(&p->lutadores[c->b.lutador].corpo, c->b.parte, c->intensidade);
        }

        if (lutando) {
            EventosCombate ev;
            combate_processar(p->lutadores, &p->fisica, p->relogio, &ev);
            bool acertou = false, forte = false;
            for (int i = 0; i < 2; i++) {
                if (ev.acerto[i] && ev.forte[i])  p->eventos.acerto_forte = true;
                if (ev.acerto[i] && !ev.forte[i]) p->eventos.acerto_leve = true;
                acertou |= ev.acerto[i];
                forte |= ev.acerto[i] && ev.forte[i];
            }
            p->relogio += FISICA_DT;

            if (acertou) {
                /* Sensação de impacto: a câmera treme e, no golpe forte, o jogo
                 * congela um instante (só aí os passos deste quadro param; parar
                 * em todo acerto dava um soluço). */
                p->congelado = forte ? PAUSA_IMPACTO_FORTE : PAUSA_IMPACTO_LEVE;
                p->tremor = fmaxf(p->tremor, forte ? TREMOR_FORTE : TREMOR_LEVE);
                if (p->congelado > 0.0f) {
                    p->acumulador = 0.0f;
                    break;
                }
            }
        }
        p->acumulador -= FISICA_DT;
        passos++;
    }
    /* Quadro muito lento: descarta o atraso em vez de acumular "câmera lenta". */
    if (passos == FISICA_MAX_PASSOS_POR_QUADRO)
        p->acumulador = 0.0f;
}

/* Decide o fim do round durante a luta. */
static void verificar_round(Partida *p)
{
    bool valido = corpo_valido(&p->lutadores[0].corpo) && corpo_valido(&p->lutadores[1].corpo);
    if (!valido) {
        /* Física explodiu: recomeça o round na hora, sem contar ponto. */
        encerrar_round(p);
        iniciar_round(p);
        p->mensagem = "REPETINDO";
        return;
    }

    bool fora[2], perdeu[2];
    for (int i = 0; i < 2; i++) {
        fora[i] = partida_fora_do_ringue(&p->lutadores[i]);
        perdeu[i] = fora[i] || p->lutadores[i].estado == LUTADOR_CAIDO;
    }
    if (!perdeu[0] && !perdeu[1])
        return;

    if (perdeu[0] && perdeu[1]) {
        p->perdedor_round = -1;
        p->mensagem = "EMPATE!";
    } else {
        int perdedor = perdeu[0] ? 0 : 1;
        p->perdedor_round = perdedor;
        p->vitorias[1 - perdedor]++;
        p->mensagem = fora[perdedor] ? "FORA!" : "CAIU!";
    }
    for (int i = 0; i < 2; i++)
        if (fora[i])
            p->lutadores[i].estado = LUTADOR_FORA;

    p->eventos.queda = true;
    p->tremor = fmaxf(p->tremor, TREMOR_QUEDA);
    p->estado = PARTIDA_FIM_ROUND;
    p->tempo_no_estado = 0.0f;
}

void partida_atualizar(Partida *p, const Intencao in[2], float dt)
{
    p->eventos = (EventosPartida){0};
    if (p->pausada || !p->round_ativo)
        return;

    p->tremor *= expf(-TREMOR_DECAIMENTO * dt);
    if (p->congelado > 0.0f) {          /* pausa no impacto: tudo parado um instante */
        p->congelado -= dt;
        return;
    }
    p->tempo_no_estado += dt;

    /* 1. Comandos (só durante a luta; fora dela, os corpos ficam parados). */
    Intencao vazia = entrada_vazia();
    for (int i = 0; i < 2; i++) {
        const Intencao *cmd = (p->estado == PARTIDA_LUTA) ? &in[i] : &vazia;
        lutador_comandar(&p->lutadores[i], cmd, corpo_posicao(&p->lutadores[1 - i].corpo), dt);
    }

    /* 2. Física em passos fixos (com o combate a cada passo). */
    simular(p, dt);

    /* 3. Lógica dos lutadores. */
    lutador_atualizar(&p->lutadores[0], dt, p->relogio);
    lutador_atualizar(&p->lutadores[1], dt, p->relogio);

    /* 4. Estados da partida. */
    switch (p->estado) {
    case PARTIDA_CONTAGEM: {
        int numero = (int)ceilf(PARTIDA_DURACAO_CONTAGEM - p->tempo_no_estado);
        if (numero >= 1 && numero != p->contagem_anterior) {
            p->contagem_anterior = numero;
            p->eventos.contagem = true;
        }
        if (p->tempo_no_estado >= PARTIDA_DURACAO_CONTAGEM) {
            p->estado = PARTIDA_LUTA;
            p->tempo_no_estado = 0.0f;
            p->eventos.lute = true;
        }
    } break;

    case PARTIDA_LUTA:
        verificar_round(p);
        break;

    case PARTIDA_FIM_ROUND:
        if (p->tempo_no_estado >= PARTIDA_DURACAO_FIM_ROUND) {
            if (p->vitorias[0] >= PARTIDA_ROUNDS_PARA_VENCER ||
                p->vitorias[1] >= PARTIDA_ROUNDS_PARA_VENCER) {
                p->estado = PARTIDA_FIM;
                p->eventos.fim_partida = true;
            } else {
                if (p->perdedor_round != -1)    /* empate repete o mesmo round */
                    p->round++;
                encerrar_round(p);
                iniciar_round(p);
            }
        }
        break;

    case PARTIDA_FIM:
        break;
    }
}

void partida_pausar(Partida *p, bool pausada)
{
    p->pausada = pausada;
}

int partida_vencedor(const Partida *p)
{
    if (p->estado != PARTIDA_FIM)
        return -1;
    return (p->vitorias[0] >= PARTIDA_ROUNDS_PARA_VENCER) ? 0 : 1;
}

void partida_preencher_hud(const Partida *p, InfoHud *info)
{
    *info = (InfoHud){0};
    info->rounds_para_vencer = PARTIDA_ROUNDS_PARA_VENCER;
    info->round = p->round;
    info->pausado = p->pausada;
    info->contagem = -1;

    for (int i = 0; i < 2; i++) {
        const Personagem *per = personagem_dados(p->jogadores[i].personagem);
        const Combo *combo = &p->jogadores[i].combo;
        info->nome[i] = per ? per->nome : "?";
        info->vitorias[i] = p->vitorias[i];
        if (p->round_ativo) {
            info->dano[i] = lutador_dano(&p->lutadores[i]);
            info->stamina[i] = p->lutadores[i].stamina / STAMINA_MAX;
            info->impacto[i] = p->lutadores[i].impacto;
        }
        info->combo_tamanho[i] = combo->tamanho;
        for (int k = 0; k < combo->tamanho && k < COMBO_MAX; k++)
            info->combo[i][k] = combo_golpe_em(combo, k);
        info->combo_atual[i] = combo_indice_atual(combo);
    }

    if (p->estado == PARTIDA_CONTAGEM) {
        int numero = (int)ceilf(PARTIDA_DURACAO_CONTAGEM - p->tempo_no_estado);
        info->contagem = (numero < 1) ? 1 : numero;
    } else if (p->estado == PARTIDA_LUTA && p->tempo_no_estado < PARTIDA_DURACAO_LUTE) {
        info->contagem = 0;
    }

    if (p->estado == PARTIDA_FIM_ROUND || p->estado == PARTIDA_FIM ||
        (p->mensagem != NULL && p->estado == PARTIDA_CONTAGEM))
        info->mensagem = p->mensagem;
}

void partida_liberar(Partida *p)
{
    encerrar_round(p);
}
