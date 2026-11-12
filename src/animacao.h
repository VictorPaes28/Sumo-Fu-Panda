/*
 * animacao.h — animações de verdade convertidas em ângulos das articulações.
 *
 * As animações (parado, andar) vieram de um modelo animado e foram
 * convertidas, quadro a quadro, para os ângulos das articulações do corpo
 * físico (as dos cotovelos e tornozelos ficam em zero: o golpe e os pés cuidam
 * delas) (ferramentas/extrai_animacoes.c). Como são só ângulos, servem para
 * qualquer personagem. O corpo usa a VARIAÇÃO da animação (quanto cada quadro
 * se afasta da média) somada à guarda e à base de sumô: o movimento natural
 * entra sem trocar a postura de sumô.
 *
 * Os dados ficam em animacoes_dados.c, GERADO pela ferramenta (não edite).
 * C puro: sem raylib e sem ODE.
 *
 * Dono: Principal.
 */
#ifndef ANIMACAO_H
#define ANIMACAO_H

#include "esqueleto.h"

typedef enum {
    ANIM_PARADO,      /* respiração e balanço de quem está parado      */
    ANIM_ANDAR,       /* um ciclo de passos (pé esquerdo e direito)    */
    ANIM_QTD
} TipoAnimacao;

typedef struct {
    const char *nome;
    float fps;            /* quadros por segundo dos dados          */
    int   quadros;
    const Pose *poses;    /* um Pose por quadro (ângulos em rad)    */
    Pose  media;          /* média de todos os quadros              */
} Animacao;

/* Dados da animação (animacoes_dados.c). NULL se o tipo for inválido. */
const Animacao *animacao_dados(TipoAnimacao tipo);

/* Duração de um ciclo (s). */
float animacao_duracao(const Animacao *a);

/* Variação da animação no tempo t (s), em laço: pose(t) - média, com
 * interpolação entre quadros. Somada a outra pose, acrescenta o movimento
 * da animação sem mudar a postura base. */
void animacao_variacao(const Animacao *a, float t, Pose *saida);

#endif /* ANIMACAO_H */
