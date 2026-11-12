/*
 * fisica.h — o mundo físico do ODE.
 *
 * Só fisica.c e corpo.c incluem o ODE. O resto do jogo fala com a física
 * por estas funções e pelas de corpo.h.
 *
 * Passo fixo: fisica_passo avança SEMPRE FISICA_DT segundos. Quem controla
 * quantos passos dar por quadro é a partida (acumulador). Motivo: no ODE
 * as forças aplicadas com dBodyAddForce/Torque são zeradas depois de cada
 * passo, então as forças dos corpos precisam ser reaplicadas ANTES de cada
 * passo (corpo_passo), e não uma vez por quadro.
 *
 * Contatos: a cada passo, fisica_passo zera a lista de contatos e registra
 * os toques entre peças de lutadores DIFERENTES. A partida processa esses
 * contatos (combate_processar) logo depois de cada passo.
 *
 * Colisões ignoradas: peças do mesmo lutador não colidem entre si
 * (evita tremedeira nas articulações).
 *
 * Chão: a cada passo, no_chao marca quais peças encostam no chão. O corpo
 * usa isso para saber se está apoiado nos pés. Os pés deslizam na areia
 * (FISICA_ATRITO_PES; a ponta do pé firma mais, FISICA_ATRITO_PONTA); o resto
 * do corpo arrasta (FISICA_ATRITO_CORPO_CHAO).
 *
 * Dono: Principal.
 */
#ifndef FISICA_H
#define FISICA_H

#include <stdbool.h>
#include <ode/ode.h>
#include "config.h"
#include "esqueleto.h"

/* Identifica uma forma do ODE. Guardada com dGeomSetData em cada forma,
 * para saber de quem é a peça quando duas se tocam. */
typedef struct {
    int lutador;          /* 0 ou 1; -1 para o chão */
    ParteCorpo parte;     /* ignorado no chão       */
} RefPeca;

/* Toque entre peças de lutadores diferentes, registrado no passo. */
typedef struct {
    RefPeca a, b;
    float ponto[3];       /* onde as peças se tocam: média dos pontos de
                             contato do par, no mundo                  (m)   */
    float intensidade;    /* velocidade relativa DAS PEÇAS NESSE PONTO (m/s):
                             um soco rápido dá muito, um encostar dá pouco   */
} Contato;

typedef struct {
    dWorldID      mundo;
    dSpaceID      espaco;
    dJointGroupID grupo_contatos;   /* juntas de contato temporárias do ODE */
    dGeomID       chao;             /* plano y = 0 */
    RefPeca       ref_chao;
    Contato       contatos[FISICA_MAX_CONTATOS];
    int           qtd_contatos;     /* contatos do último passo */
    bool          no_chao[2][PARTE_QTD]; /* peças de cada lutador encostando no
                                            chão no último passo (pés de apoio) */
} Fisica;

/* Inicializa a biblioteca ODE (dInitODE2). Uma vez, no início do main. */
void fisica_iniciar_biblioteca(void);
/* Finaliza a biblioteca ODE (dCloseODE). Uma vez, no fim do main. */
void fisica_finalizar_biblioteca(void);

/* Cria mundo, espaço, gravidade e chão. Retorna false se falhar. */
bool fisica_iniciar(Fisica *f);

/* Um passo de FISICA_DT: zera os contatos, detecta colisões (registrando os
 * toques entre lutadores), avança a simulação e esvazia as juntas de contato. */
void fisica_passo(Fisica *f);

/* Destrói tudo o que fisica_iniciar criou. Os corpos devem ter sido
 * destruídos antes (corpo_destruir). */
void fisica_liberar(Fisica *f);

#endif /* FISICA_H */
