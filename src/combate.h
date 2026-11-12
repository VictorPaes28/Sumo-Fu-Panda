/*
 * combate.h — transforma contatos físicos em acertos.
 *
 * Um golpe acerta quando, durante o golpe, uma das peças de ataque do golpe
 * (golpe_atual->parte_ataque ou parte_ataque_2) toca qualquer peça do
 * adversário. Encostar fora de um golpe não é acerto (só empurra fisicamente).
 * Cada golpe acerta no máximo uma vez (golpe_acertou).
 *
 * Como há vários passos de física por quadro, a partida junta (OU lógico)
 * os EventosCombate de todos os passos do quadro em Partida.eventos.
 *
 * Dono: Principal.
 */
#ifndef COMBATE_H
#define COMBATE_H

#include <stdbool.h>
#include "lutador.h"
#include "fisica.h"

/* O que aconteceu no passo, para a partida tocar sons e efeitos. */
typedef struct {
    bool acerto[2];        /* acerto[i] = o lutador i acertou alguém */
    bool forte[2];         /* o acerto foi de um golpe forte         */
} EventosCombate;

/* Percorre os contatos do último passo (f->contatos) e aplica os acertos.
 * Chamar logo depois de cada fisica_passo. Preenche *ev (zera antes). */
void combate_processar(Lutador lut[2], const Fisica *f, double relogio, EventosCombate *ev);

/* Aplica o golpe atual do atacante no alvo, como física de verdade:
 *   - hematoma_inserir(&alvo->hematomas, dano x escala, relogio + tempo_cura),
 *     com escala = DANO_ESCALA_SEGUIDOS ^ golpes recebidos em sequência
 *     (a sequência zera após DANO_JANELA_SEGUIDOS sem levar golpe);
 *   - força do golpe = impulso do golpe x (1 + dano_do_alvo * IMPULSO_POR_DANO)
 *     x fator da velocidade do toque (ACERTO_VEL_REFERENCIA, entre
 *     ACERTO_FATOR_MIN e ACERTO_FATOR_MAX) x FIRMADO_IMPULSO se firmado;
 *   - parte da força entra NO PONTO do toque, na peça atingida (gira o corpo:
 *     na cabeça tomba, na bacia desliza), ou no centro do peito se o golpe
 *     é no_peito (golpe.h); o resto, no centro da bacia;
 *   - quem bate recebe ACERTO_RECUO da força ao contrário, na peça que bateu;
 *   - o alvo cambaleia (lutador_receber_impacto). */
void combate_aplicar_golpe(Lutador *atacante, Lutador *alvo,
                           ParteCorpo parte_ataque, ParteCorpo parte_atingida,
                           Vector3 ponto, float intensidade, double relogio);

#endif /* COMBATE_H */
