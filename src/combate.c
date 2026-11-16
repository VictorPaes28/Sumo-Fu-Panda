/*
 * combate.c — contatos físicos viram acertos.
 * Especificação: combate.h
 *
 * Dono: Principal.
 */
#include <math.h>
#include "combate.h"
#include "config.h"
#include "raymath.h"

static bool eh_peca_de_ataque(const Lutador *l, ParteCorpo parte)
{
    const Golpe *g = l->golpe_atual;
    return g != NULL && (parte == g->parte_ataque || parte == g->parte_ataque_2);
}

void combate_processar(Lutador lut[2], const Fisica *f, double relogio, EventosCombate *ev)
{
    *ev = (EventosCombate){0};

    for (int i = 0; i < f->qtd_contatos; i++) {
        const Contato *c = &f->contatos[i];
        /* O toque pode ser um acerto de a em b ou de b em a: testa os dois sentidos. */
        for (int sentido = 0; sentido < 2; sentido++) {
            RefPeca atk = sentido ? c->b : c->a;
            RefPeca alvo = sentido ? c->a : c->b;
            if (atk.lutador < 0 || alvo.lutador < 0 || atk.lutador == alvo.lutador)
                continue;

            Lutador *atacante = &lut[atk.lutador];
            Lutador *defensor = &lut[alvo.lutador];
            /* Só a fase ativa acerta: o braço indo para trás no preparo não conta. */
            if (lutador_fase_golpe(atacante) != FASE_ATIVO || atacante->golpe_acertou)
                continue;
            if (!eh_peca_de_ataque(atacante, atk.parte))
                continue;
            if (defensor->estado == LUTADOR_CAIDO || defensor->estado == LUTADOR_FORA)
                continue;

            bool forte = atacante->golpe_atual->forte;
            Vector3 ponto = { c->ponto[0], c->ponto[1], c->ponto[2] };
            combate_aplicar_golpe(atacante, defensor, atk.parte, alvo.parte, ponto,
                                  c->intensidade, relogio);
            ev->acerto[atacante->indice] = true;
            ev->forte[atacante->indice] = forte;
        }
    }
}

/* Peça pesada que recebe a força quando o toque é numa peça leve (braço,
 * cabeça, perna). A força continua no ponto do toque, então o giro é o
 * mesmo de "transmitir pela articulação", sem arremessar a peça leve
 * (o que faria a física tremer). */
static ParteCorpo peca_que_recebe(ParteCorpo parte)
{
    switch (parte) {
    case PARTE_BRACO_E: case PARTE_BRACO_D: case PARTE_ANTEBRACO_E: case PARTE_ANTEBRACO_D:
    case PARTE_CABECA:
        return PARTE_TRONCO;
    case PARTE_COXA_E: case PARTE_COXA_D: case PARTE_CANELA_E: case PARTE_CANELA_D:
    case PARTE_PE_E: case PARTE_PE_D:
        return PARTE_BACIA;
    default:
        return parte;
    }
}

/* Alto do tronco do alvo (a altura dos ombros), no meio. */
static Vector3 alto_do_tronco(const Lutador *alvo)
{
    Matrix m = corpo_transformacao(&alvo->corpo, PARTE_TRONCO);
    const float *t = alvo->corpo.personagem->tamanho[PARTE_TRONCO];
    return Vector3Add((Vector3){ m.m12, m.m13, m.m14 },
                      Vector3Scale((Vector3){ m.m4, m.m5, m.m6 }, t[1] * 0.5f));
}

/* Centro da frente do peito do alvo, do lado de quem bate, na altura do
 * toque (limitada à parte de cima do tronco). */
static Vector3 centro_do_peito(const Lutador *alvo, Vector3 ponto, Vector3 de_onde)
{
    Matrix m = corpo_transformacao(&alvo->corpo, PARTE_TRONCO);
    Vector3 centro = { m.m12, m.m13, m.m14 };
    Vector3 cima = { m.m4, m.m5, m.m6 }, frente = { m.m8, m.m9, m.m10 };
    const float *t = alvo->corpo.personagem->tamanho[PARTE_TRONCO];
    float altura = Clamp(Vector3DotProduct(Vector3Subtract(ponto, centro), cima),
                         -t[1] * 0.2f, t[1] * 0.4f);
    float lado = (Vector3DotProduct(Vector3Subtract(de_onde, centro), frente) >= 0.0f) ? 1.0f : -1.0f;
    return Vector3Add(Vector3Add(centro, Vector3Scale(cima, altura)),
                      Vector3Scale(frente, lado * t[2] * 0.5f));
}

void combate_aplicar_golpe(Lutador *atacante, Lutador *alvo,
                           ParteCorpo parte_ataque, ParteCorpo parte_atingida,
                           Vector3 ponto, float intensidade, double relogio)
{
    const Golpe *g = atacante->golpe_atual;
    atacante->golpe_acertou = true;

    /* Força do golpe: o dano de ANTES deste golpe (quem está machucado voa
     * mais), a velocidade do toque (soco em cheio x encostar) e o firmar. */
    float mult = ACERTO_FORCA * (1.0f + lutador_dano(alvo) * IMPULSO_POR_DANO);
    /* Batida: vale a velocidade da peça. Alavanca (projeção, puxão): a mão
     * segura devagar e a força vem do corpo todo, então não depende dela. */
    if (g->derruba <= 0.0f)
        mult *= Clamp(intensidade / ACERTO_VEL_REFERENCIA, ACERTO_FATOR_MIN, ACERTO_FATOR_MAX);
    if (alvo->firmando)
        mult *= FIRMADO_IMPULSO;
    /* Golpes em sequência valem menos (como nos jogos de luta): uma rajada
     * do adversário não leva o dano às alturas em 2 segundos. A conta zera
     * quando o alvo fica DANO_JANELA_SEGUIDOS sem levar golpe. */
    if (relogio - alvo->ultimo_golpe_recebido > DANO_JANELA_SEGUIDOS)
        alvo->golpes_seguidos = 0;
    float escala = powf(DANO_ESCALA_SEGUIDOS, (float)alvo->golpes_seguidos);
    alvo->golpes_seguidos++;
    alvo->ultimo_golpe_recebido = relogio;
    hematomas_inserir(&alvo->hematomas, g->dano * escala, relogio + g->tempo_cura);

    Vector3 dir = Vector3Subtract(corpo_posicao(&alvo->corpo), corpo_posicao(&atacante->corpo));
    dir.y = 0.0f;
    dir = (Vector3Length(dir) > 0.01f) ? Vector3Normalize(dir) : corpo_frente(&atacante->corpo);
    Vector3 direita = { -dir.z, 0.0f, dir.x };          /* direita de quem bate */

    Vector3 horizontal = Vector3Add(Vector3Scale(dir, g->impulso),
                                    Vector3Scale(direita, g->impulso_lateral));
    horizontal = Vector3Scale(horizontal, mult);
    float vertical = g->impulso_vertical * mult;

    /* No ponto do toque: empurra e gira (na cabeça tomba, na barriga desliza).
     * Golpes de empurrar acertavam 86% das vezes o braço da guarda, 46 cm
     * para o lado: a força girava o alvo em vez de empurrá-lo. Eles empurram
     * pelo centro do peito. */
    Vector3 onde = ponto;
    ParteCorpo peca = peca_que_recebe(parte_atingida);
    if (g->no_peito) {
        onde = centro_do_peito(alvo, ponto, corpo_posicao(&atacante->corpo));
        peca = PARTE_TRONCO;
    }
    /* Projeções e puxões são alavancas: a força entra no alto do tronco
     * (nos ombros) e o corpo gira em volta dos pés, em vez de deslizar. */
    float fracao = ACERTO_FRACAO_NO_PONTO;
    if (g->derruba > 0.0f) {
        fracao = fmaxf(fracao, g->derruba);
        onde = alto_do_tronco(alvo);
        peca = PARTE_TRONCO;
    }
    Vector3 no_ponto = Vector3Scale((Vector3){ horizontal.x, vertical * 0.4f, horizontal.z }, fracao);
    corpo_impulso_no_ponto(&alvo->corpo, peca, no_ponto, onde);
    /* No corpo inteiro: o corpo todo sai do lugar. */
    Vector3 na_bacia = { horizontal.x * (1.0f - fracao), vertical * 0.6f,
                         horizontal.z * (1.0f - fracao) };
    corpo_impulso(&alvo->corpo, na_bacia);

    /* Ação e reação: quem bate também é empurrado para trás, no mesmo ponto. */
    Vector3 recuo = Vector3Scale((Vector3){ horizontal.x, 0.0f, horizontal.z }, -ACERTO_RECUO);
    corpo_impulso_no_ponto(&atacante->corpo, peca_que_recebe(parte_ataque), recuo, ponto);

    /* O alvo cambaleia, tombando para o lado em que foi empurrado. */
    Vector2 para_onde = { horizontal.x, horizontal.z };
    if (Vector2Length(para_onde) > 0.01f)
        para_onde = Vector2Normalize(para_onde);
    float tempo = g->forte ? ATORDOADO_FORTE : ATORDOADO_LEVE;
    lutador_receber_impacto(alvo, para_onde, alvo->firmando ? tempo * 0.5f : tempo);
}
