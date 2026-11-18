/*
 * previa.c — o lutador escolhido, ao vivo, na tela de seleção.
 * Especificação: previa.h
 *
 * Dono: Victor.
 */
#include <math.h>
#include "previa.h"
#include "config.h"
#include "personagem.h"
#include "render.h"

bool previa_criar(Previa *pv, int jogador)
{
    *pv = (Previa){ 0 };
    pv->personagem = -1;
    pv->jogador = jogador;
    if (!fisica_iniciar(&pv->fisica))
        return false;
    pv->imagem = LoadRenderTexture(PREVIA_LARGURA, PREVIA_ALTURA);
    return true;
}

/* Um passo de física com o corpo parado na guarda, olhando para a câmera. */
static void simular_passo(Previa *pv)
{
    corpo_pose_neutra(&pv->corpo);
    corpo_definir_movimento(&pv->corpo, (Vector2){ 0, 0 }, 1.0f);
    corpo_definir_alvo_olhar(&pv->corpo, (Vector3){ 0.0f, 1.0f, 5.0f });
    corpo_passo(&pv->corpo);
    fisica_passo(&pv->fisica);
}

/* Troca o corpo pelo do personagem novo. Simula um pouco antes de mostrar:
 * o corpo nasce na T-pose e leva um instante para assentar na guarda. */
static void trocar_corpo(Previa *pv, int personagem)
{
    if (pv->personagem >= 0)
        corpo_destruir(&pv->corpo);
    pv->personagem = -1;
    const Personagem *p = personagem_dados(personagem);
    if (p == NULL || !corpo_criar(&pv->corpo, &pv->fisica, p, pv->jogador, (Vector3){ 0 }, 0.0f))
        return;
    pv->personagem = personagem;
    for (float t = 0.0f; t < PREVIA_ASSENTAR; t += FISICA_DT)
        simular_passo(pv);
}

void previa_atualizar(Previa *pv, int personagem, float dt)
{
    if (personagem != pv->personagem)
        trocar_corpo(pv, personagem);
    if (pv->personagem < 0)
        return;
    pv->tempo += dt;
    /* Passo fixo, como na luta; um quadro lento não vira câmera lenta. */
    pv->acumulador += dt;
    for (int n = 0; pv->acumulador >= FISICA_DT && n < FISICA_MAX_PASSOS_POR_QUADRO; n++) {
        simular_passo(pv);
        pv->acumulador -= FISICA_DT;
    }
    if (pv->acumulador > FISICA_DT)
        pv->acumulador = 0.0f;
}

void previa_liberar(Previa *pv)
{
    if (pv->personagem >= 0)
        corpo_destruir(&pv->corpo);
    fisica_liberar(&pv->fisica);
    if (pv->imagem.id != 0)
        UnloadRenderTexture(pv->imagem);
    *pv = (Previa){ 0 };
    pv->personagem = -1;
}
