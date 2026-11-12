/*
 * esqueleto.h — peças do corpo, articulações e pose.
 *
 * Define o "vocabulário" do corpo articulado, usado pela física (corpo.c),
 * pelos golpes (golpe.c), pelos personagens (personagem.c) e pelos modelos
 * (recursos.c). Só tipos e funções simples: C puro, sem raylib e sem ODE.
 *
 * Dono: Integrante B (esqueleto.c). Mudanças nos enums: combinar com o Principal.
 *
 * Pose de criação: o corpo é criado na mesma pose em que os modelos foram
 * feitos no Meshy (T-pose: braços abertos na horizontal, pernas retas).
 * Todos os ângulos de uma Pose são medidos A PARTIR dessa pose de criação.
 */
#ifndef ESQUELETO_H
#define ESQUELETO_H

/* As 13 peças do corpo. A ordem é usada como índice em vetores. */
typedef enum {
    PARTE_BACIA,
    PARTE_TRONCO,
    PARTE_CABECA,
    PARTE_BRACO_E,       /* do ombro ao cotovelo */
    PARTE_BRACO_D,
    PARTE_COXA_E,
    PARTE_COXA_D,
    PARTE_CANELA_E,      /* do joelho ao tornozelo */
    PARTE_CANELA_D,
    PARTE_ANTEBRACO_E,   /* do cotovelo à ponta da mão */
    PARTE_ANTEBRACO_D,
    PARTE_PE_E,
    PARTE_PE_D,
    PARTE_QTD            /* quantidade de peças; sempre o último */
} ParteCorpo;

/* As 12 articulações, cada uma ligando uma peça "pai" a uma peça "filha". */
typedef enum {
    ART_CINTURA,         /* bacia  -> tronco */
    ART_PESCOCO,         /* tronco -> cabeça */
    ART_OMBRO_E,         /* tronco -> braço esquerdo */
    ART_OMBRO_D,         /* tronco -> braço direito */
    ART_QUADRIL_E,       /* bacia  -> coxa esquerda */
    ART_QUADRIL_D,       /* bacia  -> coxa direita */
    ART_JOELHO_E,        /* coxa   -> canela esquerda */
    ART_JOELHO_D,        /* coxa   -> canela direita */
    ART_COTOVELO_E,      /* braço  -> antebraço esquerdo */
    ART_COTOVELO_D,      /* braço  -> antebraço direito */
    ART_TORNOZELO_E,     /* canela -> pé esquerdo */
    ART_TORNOZELO_D,     /* canela -> pé direito */
    ART_QTD              /* quantidade de articulações; sempre o último */
} Articulacao;

/* Tipo de cada articulação no ODE. */
typedef enum {
    ARTIC_DOBRADICA,     /* 1 eixo (dJointCreateHinge): joelhos e cotovelos */
    ARTIC_UNIVERSAL      /* 2 eixos (dJointCreateUniversal): cintura, pescoço, ombros,
                            quadris e tornozelos */
} TipoArticulacao;

/*
 * Pose: ângulos que os motores das articulações tentam alcançar (rad),
 * medidos a partir da pose de criação. alvo[a][0] é o 1º eixo e alvo[a][1]
 * o 2º eixo da articulação a. Nas dobradiças, alvo[a][1] é ignorado.
 *
 * Significado do sinal (o mesmo para os lados esquerdo e direito):
 *   ART_CINTURA, ART_PESCOCO  [0] + = inclinar para a frente
 *                             [1] + = inclinar para a esquerda do personagem
 *   ART_OMBRO_E, ART_OMBRO_D  [0] + = levantar o braço (0 = horizontal, como
 *                                   na T-pose; cerca de -1.2 = braço para baixo)
 *                             [1] + = levar o braço para a frente
 *   ART_QUADRIL_E/D           [0] + = levar a coxa para a frente
 *                             [1] + = abrir a perna para fora
 *   ART_JOELHO_E/D            [0] + = dobrar o joelho (canela para trás)
 *   ART_COTOVELO_E/D          [0] + = dobrar o cotovelo (antebraço para a
 *                                   frente na T-pose; 0 = braço reto)
 *   ART_TORNOZELO_E/D         [0] + = ponta do pé para cima (canela vem à
 *                                   frente do pé)
 *                             [1] + = sola virada para fora
 * Os limites de cada articulação ficam em corpo.c.
 */
typedef struct {
    float alvo[ART_QTD][2];
} Pose;

/* Nome da peça em minúsculas e sem acento: "bacia", "tronco", "cabeca",
 * "braco_e", "braco_d", "coxa_e", "coxa_d", "canela_e", "canela_d",
 * "antebraco_e", "antebraco_d", "pe_e", "pe_d". Devolve "desconhecida" se
 * inválida. */
const char *esqueleto_nome_parte(ParteCorpo parte);

/* Tipo da articulação (dobradiça ou universal). */
TipoArticulacao esqueleto_tipo_articulacao(Articulacao art);

/* Peças ligadas pela articulação: *pai e *filha recebem as peças.
 * Ex.: ART_JOELHO_E -> pai = PARTE_COXA_E, filha = PARTE_CANELA_E. */
void esqueleto_pecas_da_articulacao(Articulacao art, ParteCorpo *pai, ParteCorpo *filha);

/* Eixos de cada articulação na pose de criação (personagem olhando para +Z,
 * esquerda em +X): girar o filho em torno do eixo, pela regra da mão direita,
 * com ângulo positivo produz o significado "+" da tabela acima.
 * eixo2 só vale para juntas universais (dobradiças devolvem zeros). */
void esqueleto_eixos(Articulacao art, float eixo1[3], float eixo2[3]);

/* Zera todos os ângulos da pose (volta à pose de criação). */
void pose_zerar(Pose *pose);

#endif /* ESQUELETO_H */
