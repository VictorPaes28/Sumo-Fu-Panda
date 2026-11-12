/*
 * animacao.c — amostragem das animações convertidas.
 * Especificação: animacao.h. Dados: animacoes_dados.c (gerado).
 *
 * Dono: Principal.
 */
#include <math.h>
#include "animacao.h"

float animacao_duracao(const Animacao *a)
{
    return (float)a->quadros / a->fps;
}

void animacao_variacao(const Animacao *a, float t, Pose *saida)
{
    /* Posição no laço, em quadros, e interpolação entre o quadro e o próximo. */
    float q = fmodf(t * a->fps, (float)a->quadros);
    if (q < 0.0f)
        q += (float)a->quadros;
    int q0 = (int)q;
    int q1 = (q0 + 1) % a->quadros;
    float mistura = q - (float)q0;

    for (int art = 0; art < ART_QTD; art++)
        for (int e = 0; e < 2; e++) {
            float v0 = a->poses[q0].alvo[art][e];
            float v1 = a->poses[q1].alvo[art][e];
            saida->alvo[art][e] = v0 + (v1 - v0) * mistura - a->media.alvo[art][e];
        }
}
