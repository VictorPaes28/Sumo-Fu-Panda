/*
 * lutador.c — um lutador durante um round.
 * Especificação: lutador.h. Fases do golpe: golpe.h.
 *
 * Dono: Principal.
 */
#include <math.h>
#include "lutador.h"
#include "config.h"
#include "raymath.h"

/* Converte a direção do analógico (tela) em direção no chão (mundo, X e Z),
 * de acordo com a câmera: para cima no analógico = para longe da câmera.
 * A câmera só se desloca (não gira), então a conversão é sempre a mesma. */
static Vector2 tela_para_chao(Vector2 mover)
{
    Vector2 frente = { CAMERA_ALVO_X - CAMERA_POS_X, CAMERA_ALVO_Z - CAMERA_POS_Z };
    frente = Vector2Normalize(frente);
    Vector2 direita = { -frente.y, frente.x };      /* frente x cima, no plano XZ */
    Vector2 d = Vector2Add(Vector2Scale(direita, mover.x), Vector2Scale(frente, mover.y));
    float tam = Vector2Length(d);
    return (tam > 1.0f) ? Vector2Scale(d, 1.0f / tam) : d;
}

/* Movimento do golpe agora: a curva de chaves (golpe.h) por cima da guarda. */
static void animar_golpe(Lutador *l)
{
    Pose delta;
    golpe_amostrar(l->golpe_atual, l->tempo_golpe, &delta);
    if (l->golpe_atual == golpe_dados(GOLPE_LEVANTADOR)) {
        /* A curva sai da guarda de cada personagem e converge para a
         * mesma pose absoluta no alto. */
        const EstiloMovimento *e = &l->corpo.personagem->estilo;
        float peso = Clamp(fminf(l->tempo_golpe, l->golpe_atual->duracao - l->tempo_golpe) / 0.16f, 0, 1);
        for (int lado = 0; lado < 2; lado++) {
            Articulacao a = lado ? ART_OMBRO_D : ART_OMBRO_E;
            delta.alvo[a][0] += peso * (-1.0f - e->guarda_ombro);
            delta.alvo[a][1] += peso * (0.50f - e->guarda_frente);
            /* Compensa a inclinacao do tronco e o peso durante a sustentacao. */
            if (l->tempo_golpe >= 0.50f && l->tempo_golpe <= 2.50f) {
                Matrix braco = corpo_transformacao(&l->corpo, lado ? PARTE_BRACO_D : PARTE_BRACO_E);
                float altura = asinf(Clamp(lado ? -braco.m1 : braco.m1, -1, 1));
                delta.alvo[a][1] += Clamp(1.2f * (30.0f * DEG2RAD - altura),-0.16f, 0.16f);
            }
        }
    }
    corpo_definir_movimento_extra(&l->corpo, &delta);
}

static bool fora_de_combate(const Lutador *l)
{
    return l->estado == LUTADOR_CAIDO || l->estado == LUTADOR_FORA;
}

bool lutador_criar(Lutador *l, Fisica *f, int indice, const Personagem *p,
                   Combo *combo, Vector3 posicao, float angulo_y)
{
    *l = (Lutador){0};
    l->indice = indice;
    l->personagem = p;
    l->combo = combo;
    hematomas_iniciar(&l->hematomas);
    l->stamina = STAMINA_MAX;
    l->tempo_desde_golpe = STAMINA_ESPERA;
    l->estado = LUTADOR_NORMAL;
    combo_reiniciar(combo);
    return corpo_criar(&l->corpo, f, p, indice, posicao, angulo_y);
}

bool lutador_pode_golpear(const Lutador *l)
{
    /* Encostando no chão (sentado, de joelhos, apoiado na mão) não golpeia:
     * no sumô essa luta já acabou. */
    if (l->estado != LUTADOR_NORMAL || corpo_caido(&l->corpo))
        return false;
    /* Desequilibrado, primeiro se recupera: ninguém ataca caindo. */
    if (l->corpo.reflexo > GOLPE_REFLEXO_MAX)
        return false;
    const Golpe *g = golpe_dados(combo_golpe_atual(l->combo));
    return g != NULL && l->stamina >= g->custo_stamina;
}

FaseGolpe lutador_fase_golpe(const Lutador *l)
{
    const Golpe *g = l->golpe_atual;
    if (l->estado != LUTADOR_GOLPEANDO || g == NULL)
        return FASE_NENHUMA;
    if (l->tempo_golpe < g->tempo_preparo)
        return FASE_PREPARO;
    if (l->tempo_golpe < g->tempo_preparo + g->tempo_ativo)
        return FASE_ATIVO;
    return FASE_VOLTA;
}

void lutador_receber_impacto(Lutador *l, Vector2 direcao, float duracao)
{
    if (duracao > l->atordoado)
        l->atordoado = duracao;
    l->direcao_impacto = direcao;
    l->impacto = 1.0f;
}

/* Fila do golpe ("buffer de entrada", como nos jogos de luta): o aperto fica
 * guardado por GOLPE_BUFFER segundos e sai assim que o lutador puder golpear.
 * Retorna true se é hora de começar o golpe. */
static bool golpe_pedido_agora(Lutador *l, bool apertou, float dt)
{
    if (apertou)
        l->golpe_pedido = GOLPE_BUFFER;                       /* 1. lembra do aperto */
    else
        l->golpe_pedido = fmaxf(0.0f, l->golpe_pedido - dt);  /* 2. esquece aos poucos */

    if (l->golpe_pedido <= 0.0f || !lutador_pode_golpear(l))
        return false;                                         /* sem pedido, ou ocupado */
    l->golpe_pedido = 0.0f;                                   /* 3. usou: não repete */
    return true;
}

bool lutador_comandar(Lutador *l, const Intencao *in, Vector3 posicao_adversario, float dt)
{
    corpo_definir_alvo_olhar(&l->corpo, posicao_adversario);

    if (fora_de_combate(l)) {
        l->golpe_pedido = 0.0f;
        l->firmando = false;
        l->direcao_suave = (Vector2){ 0, 0 };
        corpo_definir_movimento(&l->corpo, l->direcao_suave, 1.0f);
        return false;
    }

    /* Movimento com peso: a direção acompanha o analógico aos poucos.
     * Durante o golpe e cambaleando, anda bem menos. */
    l->firmando = in->firmar;
    Vector2 desejada = tela_para_chao(in->mover);
    if (l->estado == LUTADOR_GOLPEANDO) {
        bool carregando = l->golpe_atual == golpe_dados(GOLPE_LEVANTADOR)
                           && l->golpe_acertou && l->tempo_golpe <= 2.50f;
        desejada = Vector2Scale(desejada, carregando ? 0.85f : 0.3f);
    }
    if (l->atordoado > 0.0f)            desejada = Vector2Scale(desejada, 0.4f);
    float k = 1.0f - expf(-ENTRADA_SUAVIZACAO * dt);
    l->direcao_suave = Vector2Add(l->direcao_suave,
                                  Vector2Scale(Vector2Subtract(desejada, l->direcao_suave), k));
    corpo_definir_movimento(&l->corpo, l->direcao_suave,
                            l->firmando ? FIRMADO_VELOCIDADE : 1.0f);

    if (!golpe_pedido_agora(l, in->a, dt))
        return false;

    const Golpe *g = golpe_dados(combo_golpe_atual(l->combo));
    l->stamina -= g->custo_stamina;
    l->golpe_atual = g;
    l->tempo_golpe = 0.0f;
    l->golpe_acertou = false;
    l->avancou = false;
    l->voltou = false;
    l->tempo_desde_golpe = 0.0f;
    l->estado = LUTADOR_GOLPEANDO;
    corpo_pose_neutra(&l->corpo);
    animar_golpe(l);
    corpo_definir_rigidez(&l->corpo, RIGIDEZ_PREPARO);
    combo_avancar(l->combo);
    return true;
}

void lutador_passo_fisica(Lutador *l)
{
    float equilibrio = 1.0f - lutador_dano(l) / DANO_MAX;
    if (equilibrio < EQUILIBRIO_MINIMO)
        equilibrio = EQUILIBRIO_MINIMO;
    if (l->firmando)
        equilibrio *= EQUILIBRIO_FIRMADO;
    if (l->atordoado > 0.0f)
        equilibrio *= ATORDOADO_EQUILIBRIO;
    /* Quem perdeu o round vira um boneco mole: a queda é a parte engraçada. */
    if (fora_de_combate(l))
        equilibrio = 0.0f;
    corpo_definir_equilibrio(&l->corpo, equilibrio);

    /* Para onde o corpo "quer" inclinar: para a frente ao andar e para trás
     * (na direção do golpe) ao cambalear. */
    Vector2 incl = Vector2Scale(l->direcao_suave, INCLINACAO_ANDAR);
    if (l->atordoado > 0.0f) {
        float forca = fminf(l->atordoado / ATORDOADO_FORTE, 1.0f);
        incl = Vector2Add(incl, Vector2Scale(l->direcao_impacto, INCLINACAO_IMPACTO * forca));
    }
    corpo_definir_inclinacao(&l->corpo, incl);

    corpo_passo(&l->corpo);
}

/* Golpe em andamento: a curva do movimento a cada quadro; na troca de fase,
 * a rigidez e o avanço no começo da fase ativa. */
static void atualizar_golpe(Lutador *l, float dt)
{
    const Golpe *g = l->golpe_atual;
    /* No segundo apoio do Empurrao a perna de tras impulsiona o corpo. */
    if (g == golpe_dados(GOLPE_EMPURRAO) && l->tempo_golpe < 0.43f &&
        l->tempo_golpe + dt >= 0.43f && l->corpo.apoio > PASSO_APOIO_MINIMO)
        corpo_impulso(&l->corpo, Vector3Scale(corpo_frente(&l->corpo), 0.60f * g->avanco));
    /* Cada mao do Harite pode acertar uma vez. */
    if (g == golpe_dados(GOLPE_FORCAR_FORA) && l->tempo_golpe < 0.30f &&
        l->tempo_golpe + dt >= 0.30f)
        l->golpe_acertou = false;
    l->tempo_golpe += dt;
    animar_golpe(l);

    switch (lutador_fase_golpe(l)) {
    case FASE_PREPARO:
        break;
    case FASE_ATIVO:
        if (!l->avancou) {
            l->avancou = true;
            corpo_definir_rigidez(&l->corpo, RIGIDEZ_ATIVO);
            if (g->avanco != 0.0f) {
                Vector3 frente = corpo_frente(&l->corpo);
                Vector3 impulso = Vector3Scale(frente, g->avanco);
                if (g == golpe_dados(GOLPE_BARRIGADA)) {
                    const bool *chao = l->corpo.fisica->no_chao[l->indice];
                    if (!chao[PARTE_PE_E] && !chao[PARTE_PE_D])
                        break;
                    /* Mesma velocidade vertical para lutadores de massas diferentes. */
                    float massa = 0;
                    for (int i = 0; i < PARTE_QTD; i++) massa += l->personagem->massa[i];
                    impulso.y = massa * 2.05f;
                }
                corpo_impulso(&l->corpo, impulso);
            }
        }
        break;
    case FASE_VOLTA:
        if (g == golpe_dados(GOLPE_LEVANTADOR) && l->tempo_golpe < 2.52f)
            break; /* sustenta a carga com os ombros firmes ate iniciar a descida */
        if (!l->voltou) {
            l->voltou = true;
            corpo_definir_rigidez(&l->corpo, RIGIDEZ_VOLTA);
        }
        if (l->tempo_golpe >= g->duracao) {
            l->golpe_atual = NULL;
            l->estado = LUTADOR_NORMAL;
            corpo_definir_movimento_extra(&l->corpo, NULL);
            corpo_definir_rigidez(&l->corpo, RIGIDEZ_REPOUSO);
        }
        break;
    case FASE_NENHUMA:
        break;
    }
}

void lutador_atualizar(Lutador *l, float dt, double relogio)
{
    hematomas_curar(&l->hematomas, relogio);
    l->tempo_desde_golpe += dt;
    if (l->atordoado > 0.0f) l->atordoado = fmaxf(0.0f, l->atordoado - dt);
    if (l->impacto > 0.0f)   l->impacto = fmaxf(0.0f, l->impacto - dt * 3.0f);

    if (l->estado == LUTADOR_GOLPEANDO)
        atualizar_golpe(l, dt);
    else if (fora_de_combate(l))               /* saiu do ringue no meio do golpe */
        corpo_definir_movimento_extra(&l->corpo, NULL);

    if (l->tempo_desde_golpe >= STAMINA_ESPERA) {
        l->stamina += STAMINA_REGENERACAO * dt;
        if (l->stamina > STAMINA_MAX)
            l->stamina = STAMINA_MAX;
    }
    /* Cansaço: com a stamina baixa os músculos perdem força e o corpo fica
     * mais mole e fácil de derrubar. */
    corpo_definir_forca_muscular(&l->corpo,
                                 Lerp(CANSACO_FORCA_MINIMA, 1.0f, l->stamina / STAMINA_MAX));

    if (!fora_de_combate(l)) {
        if (corpo_caido(&l->corpo)) {
            l->tempo_inclinado += dt;
            if (l->tempo_inclinado >= CORPO_TEMPO_QUEDA) {
                l->estado = LUTADOR_CAIDO;
                l->golpe_atual = NULL;
                corpo_definir_movimento_extra(&l->corpo, NULL);
                corpo_definir_rigidez(&l->corpo, 0.2f);   /* braços soltos na queda */
            }
        } else {
            l->tempo_inclinado = 0.0f;
        }
    }
}

float lutador_dano(const Lutador *l)
{
    return hematomas_dano_total(&l->hematomas);
}

void lutador_destruir(Lutador *l)
{
    corpo_destruir(&l->corpo);
    hematomas_liberar(&l->hematomas);
    l->combo = NULL;
}
