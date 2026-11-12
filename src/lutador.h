/*
 * lutador.h — um lutador durante UM round.
 *
 * Junta o corpo físico, a stamina, os hematomas e o golpe em andamento.
 * O combo NÃO pertence ao lutador: ele é do Jogador (partida.h), dura a
 * partida inteira e o lutador só guarda um ponteiro para ele.
 * A cada round, os lutadores são destruídos e criados de novo (stamina
 * cheia, sem hematomas, corpo de pé).
 *
 * ATENÇÃO: nunca copie um Lutador (nem Corpo, Fisica ou Partida) por valor.
 * As formas do ODE guardam ponteiros para dentro do Corpo (refs); uma cópia
 * deixa esses ponteiros apontando para o lugar errado. Use sempre ponteiros.
 *
 * Dono: Principal.
 */
#ifndef LUTADOR_H
#define LUTADOR_H

#include <stdbool.h>
#include "corpo.h"
#include "combo.h"
#include "hematoma.h"
#include "golpe.h"
#include "entrada.h"

typedef enum {
    LUTADOR_NORMAL,      /* de pé, livre                              */
    LUTADOR_GOLPEANDO,   /* executando golpe_atual                    */
    LUTADOR_CAIDO,       /* tronco caiu: perde o round                */
    LUTADOR_FORA         /* saiu do ringue: perde o round             */
} EstadoLutador;

typedef struct {
    int indice;                   /* 0 ou 1                                    */
    const Personagem *personagem;
    Corpo corpo;
    Combo *combo;                 /* do Jogador; NÃO é liberado aqui           */
    ListaHematomas hematomas;     /* criada e liberada pelo lutador            */
    float stamina;                /* 0 a STAMINA_MAX                     (pts) */
    float tempo_desde_golpe;      /* para regenerar stamina e reiniciar combo (s) */
    EstadoLutador estado;
    bool  firmando;
    float golpe_pedido;           /* aperto do golpe guardado: quanto tempo ele
                                     ainda vale (s); 0 = nenhum (GOLPE_BUFFER) */
    const Golpe *golpe_atual;     /* NULL se não está golpeando                */
    float tempo_golpe;            /* há quanto tempo o golpe começou     (s)   */
    bool  golpe_acertou;          /* cada golpe acerta no máximo uma vez       */
    bool  avancou;                /* já entrou na fase ativa (pose e avanço)   */
    bool  voltou;                 /* já entrou na fase de volta                */
    Vector2 direcao_suave;        /* direção de andar, seguindo o analógico
                                     aos poucos (ENTRADA_SUAVIZACAO)           */
    float atordoado;              /* tempo cambaleando depois de levar golpe (s) */
    Vector2 direcao_impacto;      /* de onde veio o último golpe (no chão)     */
    float impacto;                /* 1 ao levar golpe, cai até 0: efeito no HUD */
    float tempo_inclinado;        /* há quanto tempo corpo_caido está true, ou
                                     seja, encostando no chão (s);
                                     vira LUTADOR_CAIDO após CORPO_TEMPO_QUEDA */
    int    golpes_seguidos;       /* golpes recebidos em sequência (sem pausa de
                                     DANO_JANELA_SEGUIDOS): reduzem o dano     */
    double ultimo_golpe_recebido; /* relógio da partida no último golpe   (s)  */
} Lutador;

/* Cria o corpo e a lista de hematomas, stamina cheia, combo reiniciado. */
bool  lutador_criar(Lutador *l, Fisica *f, int indice, const Personagem *p,
                    Combo *combo, Vector3 posicao, float angulo_y);

/* UMA vez por quadro, com a intenção do jogador: movimento, firmar, olhar
 * para o adversário e o golpe. O aperto do botão A fica guardado por
 * GOLPE_BUFFER segundos: assim que lutador_pode_golpear, inicia o golpe
 * atual do combo (gasta stamina, aplica a pose e o avanço do golpe,
 * combo_avancar). Quem aperta um instante antes de poder (terminando outro
 * golpe, se equilibrando) não perde o aperto.
 * Retorna true se iniciou um golpe. */
bool  lutador_comandar(Lutador *l, const Intencao *in, Vector3 posicao_adversario, float dt);

/* A CADA passo de física: calcula o equilíbrio pelo dano e chama corpo_passo. */
void  lutador_passo_fisica(Lutador *l);

/* UMA vez por quadro, depois da física: stamina, hematomas_curar(relogio),
 * fim do golpe, reinício do combo parado (COMBO_TEMPO_REINICIO), queda. */
void  lutador_atualizar(Lutador *l, float dt, double relogio);

bool  lutador_pode_golpear(const Lutador *l);  /* estado, stamina e não estar
                                                 encostando no chão (corpo_caido) */

/* Fase do golpe em andamento (golpe.h). FASE_NENHUMA se não está golpeando. */
typedef enum { FASE_NENHUMA, FASE_PREPARO, FASE_ATIVO, FASE_VOLTA } FaseGolpe;
FaseGolpe lutador_fase_golpe(const Lutador *l);

/* Chamado pelo combate quando o lutador leva um golpe: cambaleia por
 * 'duracao' segundos, tombando na direção do impacto (no chão, unitária). */
void  lutador_receber_impacto(Lutador *l, Vector2 direcao, float duracao);
float lutador_dano(const Lutador *l);          /* hematomas_dano_total */

/* Destrói o corpo e libera os hematomas (o combo continua do Jogador). */
void  lutador_destruir(Lutador *l);

#endif /* LUTADOR_H */
