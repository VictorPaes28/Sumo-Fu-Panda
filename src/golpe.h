/*
 * golpe.h — catálogo de golpes (dados fixos).
 *
 * O catálogo é uma tabela constante: ninguém altera os golpes durante o jogo.
 * O combo (combo.h) guarda só o TipoGolpe; os dados vêm daqui.
 * C puro: sem raylib e sem ODE.
 *
 * Dono: Integrante B cria a tabela com nomes, descrições e valores provisórios.
 *       O Principal ajusta poses e forças depois dos testes de física.
 */
#ifndef GOLPE_H
#define GOLPE_H

#include <stdbool.h>
#include "esqueleto.h"

typedef enum {
    GOLPE_EMPURRAO,
    GOLPE_LEVANTADOR,
    GOLPE_TAPA_LATERAL,
    GOLPE_BARRIGADA,
    GOLPE_INVESTIDA,
    GOLPE_EMPURRAR_FORA,     /* oshi-dashi: empurra o peito e avança         */
    GOLPE_FORCAR_FORA,       /* yori-kiri: segura a faixa e anda para fora   */
    GOLPE_PROJECAO_CIMA,     /* uwate-nage: pegada por cima, joga de lado    */
    GOLPE_PROJECAO_BAIXO,    /* shitate-nage: pegada por baixo, joga de lado */
    GOLPE_TAPA_BAIXO,        /* hataki-komi: sai da frente e bate nas costas */
    GOLPE_PUXAR_BAIXO,       /* hiki-otoshi: recua puxando para o chão       */
    GOLPE_QTD            /* quantidade de golpes; também usado como "nenhum golpe" */
} TipoGolpe;

/*
 * Todo golpe tem 3 fases, como num jogo de luta:
 *
 *   PREPARO  [0, tempo_preparo)               puxa o braço, abaixa, toma impulso
 *   ATIVO    [tempo_preparo, + tempo_ativo)   o avanço é aplicado e SÓ AQUI o
 *                                             golpe acerta
 *   VOLTA    até duracao                      volta à guarda (vulnerável)
 *
 * O MOVIMENTO é uma curva de chaves (como numa animação): em cada chave, o
 * quanto cada articulação se afasta da guarda e da base de sumô do
 * personagem, pernas inclusive (o corpo inteiro golpeia: afunda no preparo e
 * empurra o chão no ataque) e cotovelos. Entre as chaves a
 * curva é suave (golpe_amostrar). A primeira chave é em 0 s e a última em
 * duracao, as duas com tudo zero (sai da guarda e volta para ela).
 * As chaves podem ser escritas à mão ou extraídas de uma animação do Mixamo
 * (ferramentas/extrai_animacoes --golpe, docs/ANIMACOES.md).
 */
typedef struct {
    float tempo;              /* desde o começo do golpe                 (s)    */
    Pose  pose;               /* diferença para a guarda e a base        (rad)  */
} ChaveGolpe;

typedef struct {
    const char *nome;         /* nome curto para a tela, ex.: "Empurrão"        */
    const char *descricao;    /* uma frase para a tela de montagem              */
    const ChaveGolpe *chaves; /* curva do movimento, em ordem de tempo          */
    int   qtd_chaves;         /* pelo menos 2                                   */
    float tempo_preparo;      /* duração do preparo                      (s)    */
    float tempo_ativo;        /* janela em que o golpe pode acertar      (s)    */
    ParteCorpo parte_ataque;  /* peça que acerta (ex.: PARTE_ANTEBRACO_D)       */
    ParteCorpo parte_ataque_2;/* 2ª peça que também acerta; PARTE_QTD = nenhuma */
    float avanco;             /* impulso no PRÓPRIO corpo, para a frente, no
                                 início da fase ativa (investida...);
                                 negativo = recua                        (N.s)  */
    float impulso;            /* impulso horizontal no alvo ao acertar, para
                                 longe do atacante; negativo = puxa      (N.s)  */
    float impulso_vertical;   /* impulso para cima (levantador); negativo =
                                 para baixo (tapa para baixo)            (N.s)  */
    float impulso_lateral;    /* impulso para a direita do atacante (tapa) (N.s) */
    float dano;               /* porcentagem somada ao alvo              (%)    */
    float tempo_cura;         /* quanto o hematoma dura                  (s)    */
    float custo_stamina;      /* stamina gasta ao usar                   (pts)  */
    float duracao;            /* duração total: preparo + ativo + volta  (s)    */
    bool  forte;              /* golpe forte: som e efeito maiores              */
    bool  agarra;             /* as mãos prendem na faixa/tronco do outro ao
                                 encostar (yotsu-zumo, corpo_agarrar)           */
    float derruba;            /* 0 a 1: quanto do impulso entra no ALTO do
                                 tronco do alvo (alavanca: gira e derruba, como
                                 nas projeções e puxões) em vez de no corpo
                                 inteiro (só empurra). 0 = ACERTO_FRACAO_NO_PONTO
                                 no ponto do toque                              */
    bool  no_peito;           /* empurra pelo CENTRO DO PEITO do alvo: a força
                                 entra no meio da frente do tronco, mesmo que o
                                 contato tenha sido no braço da guarda (ela
                                 passa pelos braços até o tronco). false = no
                                 ponto do toque (tapa de lado, levantador nas
                                 axilas)                                        */
} Golpe;

/* Dados do golpe. Devolve NULL se tipo for inválido (inclusive GOLPE_QTD). */
const Golpe *golpe_dados(TipoGolpe tipo);

/* Movimento do golpe no tempo t (s desde o começo): diferença para a guarda
 * em cada articulação (delta). Curva suave (Hermite cúbica) entre as chaves;
 * antes da primeira e depois da última, vale a chave da ponta. */
void golpe_amostrar(const Golpe *g, float t, Pose *delta);

#endif /* GOLPE_H */
