/*
 * personagem.h — catálogo de personagens (dados fixos).
 *
 * Cada personagem descreve o próprio corpo físico (onde fica e qual o
 * tamanho de cada peça, a massa, onde ficam as articulações e as forças) e,
 * opcionalmente, o modelo 3D que o veste: um .glb com esqueleto, cujos
 * ossos seguem as peças físicas (ver pele.h). Sem modelo, o personagem é
 * desenhado com caixas.
 *
 * As medidas das peças devem bater com o modelo: use a ferramenta
 * inspeciona_modelo para ler as posições dos ossos (docs/ASSETS.md).
 * Adicionar um personagem = uma entrada nova na tabela + o .glb em
 * assets/modelos/<pasta>/. Nenhum outro módulo muda.
 *
 * Coordenadas: metros, Y para cima, personagem olhando para +Z, pés na
 * origem (0, 0, 0), na pose de criação (T-pose).
 * C puro: sem raylib e sem ODE.
 *
 * Dono: Integrante B cria a tabela com as medidas da arte.
 *       O Principal ajusta massas e forças depois dos testes de física.
 */
#ifndef PERSONAGEM_H
#define PERSONAGEM_H

#include "esqueleto.h"

/* Liga um osso do modelo (nome exato no .glb) a uma peça física: o osso
 * acompanha a peça. Ossos que não aparecem na lista seguem o osso pai
 * (dedos, mãos...); ossos sem pai seguem a bacia. */
typedef struct {
    const char *osso;
    ParteCorpo parte;
} OssoPeca;

/* Jeito de se mexer do personagem: postura, andar e reflexos.
 * Ângulos no sentido de esqueleto.h ("+" = frente / abrir / dobrar).
 * Ideias de valores:
 *   pesado (panda, touro): base larga, passo longo e lento, gingado grande;
 *   ágil (macaco, leopardo): base baixa, passo curto e rápido, reflexo alto.
 * Cuidado com a base: quadril maior que o joelho joga o peso para a frente e
 * joelho bem maior que o quadril senta o lutador para trás. Teste com
 * demo_fisica --teste depois de mudar. */
typedef struct {
    /* Postura parada (base de sumô). */
    float base_quadril;     /* coxa para a frente                          (rad) */
    float base_abertura;    /* pernas abertas para os lados                (rad) */
    float base_joelho;      /* joelho dobrado                              (rad) */
    float inclinacao;       /* tronco para a frente (alvo da mola)         (rad) */
    float guarda_ombro;     /* braços: negativo = baixos ao lado do corpo  (rad) */
    float guarda_frente;    /* braços para a frente                        (rad) */
    float guarda_cintura;   /* cintura dobrada para a frente               (rad) */
    float guarda_cotovelo;  /* cotovelos dobrados na guarda (0 = reto)     (rad) */
    float balanco_parado;   /* força do balanço de quem está parado (1 = animação) */

    /* Andar. */
    float periodo_passo;    /* duração de um ciclo de passos, na velocidade máx. (s) */
    float amplitude_passo;  /* tamanho dos passos (1 = animação original)        */
    float balanco_bracos;   /* braços e tronco ao andar (1 = animação original)  */
    float gingado;          /* tronco balançando de um lado para o outro    (rad) */

    /* Reflexos de equilíbrio (corpo.c). */
    float reflexo_bracos;   /* força dos braços ao se desequilibrar (1 = normal) */
    float passo_reacao;     /* tamanho do passo de recuperação (1 = normal)      */
} EstiloMovimento;

typedef struct {
    const char *nome;               /* nome na tela, ex.: "Po"                         */
    const char *pasta;              /* pasta em assets/modelos/, ex.: "po"             */
    float centro[PARTE_QTD][3];     /* centro de cada peça (x, y, z)            (m)    */
    float tamanho[PARTE_QTD][3];    /* largura x, altura y, profundidade z      (m)    */
    float massa[PARTE_QTD];         /* massa de cada peça                       (kg)   */
    float centro_massa[PARTE_QTD][3]; /* centro de massa de cada peça em relação
                                       ao centro da caixa; zero = no meio (a
                                       barriga do Po puxa o tronco à frente) (m) */
    float ancora[ART_QTD][3];       /* ponto de cada articulação (x, y, z)      (m)    */
    float forca_equilibrio;         /* torque máximo da mola de equilíbrio      (N.m)  */
    float forca_andar;              /* força aplicada na bacia ao andar         (N)    */
    float velocidade_max;           /* velocidade máxima andando                (m/s)  */
    EstiloMovimento estilo;         /* jeito de se mexer                               */

    /* Modelo 3D (opcional). */
    const char *modelo;             /* arquivo em assets/modelos/<pasta>/, ex.:
                                       "po.glb"; NULL = desenhar com caixas        */
    float escala_modelo;            /* multiplica o modelo para ficar em metros     */
    const OssoPeca *ossos;          /* ossos ligados às peças                       */
    int qtd_ossos;
} Personagem;

/* Quantidade de personagens no catálogo (no máximo MAX_PERSONAGENS). */
int personagem_quantidade(void);

/* Dados do personagem. Devolve NULL se o índice for inválido. */
const Personagem *personagem_dados(int indice);

#endif /* PERSONAGEM_H */
