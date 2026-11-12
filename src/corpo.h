/*
 * corpo.h — corpo articulado de um lutador (como no Sumotori Dreams).
 *
 * 13 peças rígidas (caixas e cápsulas) ligadas por 12 articulações com
 * músculos (mola + amortecedor em cada junta).
 * O corpo é criado na pose de criação (T-pose), com as medidas do personagem.
 *
 * Divisão do trabalho por quadro e por passo:
 *   - UMA vez por quadro, o lutador define as INTENÇÕES do corpo:
 *       corpo_definir_pose, corpo_definir_movimento, corpo_definir_alvo_olhar,
 *       corpo_definir_equilibrio
 *   - A CADA passo de física (antes de fisica_passo), corpo_passo aplica as
 *     forças: motores em direção à pose, mola de equilíbrio no tronco
 *     (proporcional a equilibrio), força de andar na bacia, giro da bacia
 *     para o alvo e o ciclo das pernas ao andar.
 *
 * Só fisica.c e corpo.c incluem o ODE.
 * Dono: Principal.
 */
#ifndef CORPO_H
#define CORPO_H

#include <stdbool.h>
#include "raylib.h"
#include "fisica.h"
#include "personagem.h"

/* Força de um golpe sendo entregue aos poucos (ACERTO_PASSOS passos). */
typedef struct {
    ParteCorpo parte;
    Vector3  forca;                   /* força por passo (N)                     */
    Vector3  ponto_local;             /* ponto na peça, no sistema dela (m)      */
    int      passos;                  /* passos que faltam (0 = livre)           */
} ForcaPendente;

#define CORPO_MAX_PENDENTES 4

typedef struct {
    dBodyID  pecas[PARTE_QTD];        /* corpos rígidos                          */
    dGeomID  formas[PARTE_QTD];       /* formas de colisão (caixas)              */
    dJointID articulacoes[ART_QTD];   /* articulações com motor                  */
    RefPeca  refs[PARTE_QTD];         /* dGeomSetData de cada forma aponta aqui  */
    const Personagem *personagem;
    const Fisica *fisica;             /* para ler quais pés estão no chão       */
    float    apoio;                   /* 0 a 1: quanto o corpo está apoiado (pés
                                         no chão e peso em cima deles). Calculado
                                         em corpo_passo                          */
    Pose     pose_alvo;               /* pose pedida pelo lutador                */
    Pose     pose_suave;              /* pose que os motores seguem: anda até
                                         pose_alvo aos poucos (sem "teleporte")  */
    float    rigidez;                 /* força e rapidez dos motores da parte de
                                         cima: < 1 solto, > 1 golpe firme         */
    Vector2  inclinacao;              /* para onde a mola puxa o tronco, em vez da
                                         vertical pura: x = mundo X, y = mundo Z  */
    float    tempo_vida;              /* desde a criação: tempo da animação parado (s) */
    Vector2  direcao_andar;           /* desejada no chão: x = mundo X, y = mundo Z, -1 a 1 */
    float    fator_velocidade;        /* 1 = normal; FIRMADO_VELOCIDADE ao firmar */
    Vector3  alvo_olhar;              /* ponto que a bacia tenta encarar         */
    float    equilibrio;              /* multiplicador da mola de equilíbrio, >= 0 */
    float    tempo_passo;             /* tempo dentro da animação de andar (s)   */
    float    amolecido[ART_QTD];      /* 0 a 1: quanto cada articulação cedeu
                                         por um choque recente (reação)          */
    ForcaPendente pendentes[CORPO_MAX_PENDENTES];

    /* Reflexos de equilíbrio (calculados em corpo_passo). */
    Vector2  desequilibrio;           /* capture point - meio dos pés, no chão:
                                         x = mundo X, y = mundo Z (m)            */
    float    reflexo;                 /* 0 a 1: quanto o corpo está reagindo     */
    int      passo_perna;             /* perna dando o passo de recuperação:
                                         0 = esquerda, 1 = direita, -1 = nenhuma */
    float    passo_tempo;             /* tempo desde o começo do passo (s)       */
    Vector2  passo_alvo;              /* onde o pé vai pisar: mundo X, Z (m)     */
    float    passo_espera;            /* tempo até poder dar outro passo (s)     */

    /* Golpe animado (golpe.h). */
    Pose     extra;                   /* somado a todas as articulações (rad)    */
    bool     tem_extra;               /* há golpe animado agora                  */

    float    forca_muscular;          /* 0 a 1: multiplica a força de todos os
                                         músculos (cansaço); 1 = descansado      */

    /* Pegada na faixa (yotsu-zumo): a mão esquerda/direita presa no outro. */
    dJointID pegada[2];               /* junta mão-outro lutador; NULL = solta   */
    dJointFeedback pegada_forca[2];   /* força que a pegada está segurando        */
    float    pegada_tempo[2];         /* há quanto tempo segura (s)              */
} Corpo;

/* Cria as peças e articulações na pose de criação, com os pés em
 * (posicao.x, 0, posicao.z), virado angulo_y rad em torno do eixo Y.
 * indice_lutador (0 ou 1) vai para os RefPeca. Retorna false se falhar. */
bool    corpo_criar(Corpo *c, Fisica *f, const Personagem *p, int indice_lutador,
                    Vector3 posicao, float angulo_y);

/* Intenções (uma vez por quadro).
 * A pose só controla cintura, pescoço, ombros e cotovelos: pernas e pés são
 * sempre do corpo (base de sumô, ciclo de passos ao andar, equilíbrio). */
void    corpo_definir_pose(Corpo *c, const Pose *pose);
void    corpo_pose_neutra(Corpo *c);                 /* braços baixos, de pé */
void    corpo_definir_movimento(Corpo *c, Vector2 direcao, float fator_velocidade);
void    corpo_definir_alvo_olhar(Corpo *c, Vector3 ponto);
void    corpo_definir_equilibrio(Corpo *c, float equilibrio);
void    corpo_definir_rigidez(Corpo *c, float rigidez);          /* RIGIDEZ_* */
/* Inclinação desejada do tronco (rad aproximados, no chão). (0, 0) = de pé. */
void    corpo_definir_inclinacao(Corpo *c, Vector2 inclinacao);
/* Movimento de golpe por cima da postura (golpe_amostrar): 'delta' é somado
 * a TODAS as articulações, pernas inclusive (o corpo inteiro participa).
 * NULL = sem movimento extra. Enquanto há movimento extra, os braços não
 * fazem o reflexo de equilíbrio (estão golpeando); o passo de recuperação
 * continua. */
void    corpo_definir_movimento_extra(Corpo *c, const Pose *delta);
/* Força dos músculos, de 0 a 1 (cansaço: a stamina baixa amolece o corpo).
 * Valores fora da faixa são limitados. Começa em 1. */
void    corpo_definir_forca_muscular(Corpo *c, float forca);

/* Forças de um passo (A CADA passo de física, antes de fisica_passo).
 * A mola de equilíbrio, a força de andar e o giro são multiplicados pelo
 * apoio: com os dois pés no chão e o centro de massa em cima deles, força
 * total; com um pé, menos; no ar, quase nada; com o peso longe dos pés
 * (além de APOIO_LIMITE), o corpo não consegue mais se salvar e cai.
 * Também mede o desequilíbrio e reage como gente (REFLEXO_*, PASSO_*):
 * braços de contrapeso e passo de recuperação. */
void    corpo_passo(Corpo *c);

/* Impulso instantâneo no CORPO INTEIRO (N.s), no sistema do mundo: cada peça
 * recebe a parte do impulso proporcional à sua massa, então o corpo todo
 * ganha a mesma velocidade, sem girar e sem arrancar uma peça das outras.
 * Usado no avanço dos golpes e no empurrão de um acerto. */
void    corpo_impulso(Corpo *c, Vector3 impulso);

/* Impulso num PONTO da peça (N.s, ponto no mundo): empurra e também gira a
 * peça, como uma força de verdade (um golpe na cabeça tomba mais que um na
 * bacia). É entregue como força ao longo dos próximos ACERTO_PASSOS passos,
 * no mesmo ponto da peça mesmo que ela se mova. */
void    corpo_impulso_no_ponto(Corpo *c, ParteCorpo parte, Vector3 impulso, Vector3 ponto);

/* Pegada (yotsu-zumo), A CADA passo, depois de fisica_passo: se 'quer' e a
 * mão (antebraço) de c encostou na bacia ou no tronco de 'outro' neste passo
 * (f->contatos), ela prende ali. A pegada solta sozinha depois de
 * AGARRAR_TEMPO_MAX, se a força passar de AGARRAR_FORCA_MAX (escapou), se
 * 'quer' ficar falso depois de AGARRAR_TEMPO_MIN ou se alguém cair. */
void    corpo_agarrar(Corpo *c, Corpo *outro, const Fisica *f, bool quer);
/* Solta as duas mãos (fim do golpe, fim do round). */
void    corpo_soltar(Corpo *c);

/* Reação a um choque de velocidade 'intensidade' (m/s) na peça: as
 * articulações ligadas a ela cedem (REACAO_*) e voltam aos poucos.
 * A partida chama para todo contato entre lutadores, golpe ou encontrão. */
void    corpo_receber_contato(Corpo *c, ParteCorpo parte, float intensidade);

/* Consultas. */
Vector3 corpo_posicao(const Corpo *c);    /* centro da bacia (m)                   */
Vector3 corpo_frente(const Corpo *c);     /* direção para onde a bacia olha (unitária, no chão) */
bool    corpo_caido(const Corpo *c);      /* alguma parte além dos pés encosta no chão
                                             (regra do sumô), no último passo de física */
bool    corpo_valido(const Corpo *c);     /* false se alguma posição virou NaN/infinito */

/* Matriz de transformação da peça (posição + rotação do CENTRO DA CAIXA da
 * peça, mesmo quando o centro de massa está deslocado), para o render
 * desenhar o modelo da peça no lugar da peça física. */
Matrix  corpo_transformacao(const Corpo *c, ParteCorpo parte);

/* Destrói peças, formas e articulações. */
void    corpo_destruir(Corpo *c);

#endif /* CORPO_H */
