/*
 * esqueleto.c — nomes e tabelas das peças e articulações.
 * Especificação: esqueleto.h
 *
 * Dono: Integrante B.
 */
#include "esqueleto.h"

static const char *const NOMES_PARTES[PARTE_QTD] = {
    [PARTE_BACIA]    = "bacia",
    [PARTE_TRONCO]   = "tronco",
    [PARTE_CABECA]   = "cabeca",
    [PARTE_BRACO_E]  = "braco_e",
    [PARTE_BRACO_D]  = "braco_d",
    [PARTE_COXA_E]   = "coxa_e",
    [PARTE_COXA_D]   = "coxa_d",
    [PARTE_CANELA_E] = "canela_e",
    [PARTE_CANELA_D] = "canela_d",
    [PARTE_ANTEBRACO_E] = "antebraco_e",
    [PARTE_ANTEBRACO_D] = "antebraco_d",
    [PARTE_PE_E]     = "pe_e",
    [PARTE_PE_D]     = "pe_d",
};

typedef struct {
    ParteCorpo pai;
    ParteCorpo filha;
    TipoArticulacao tipo;
} DadosArticulacao;

static const DadosArticulacao ARTICULACOES[ART_QTD] = {
    [ART_CINTURA]   = { PARTE_BACIA,  PARTE_TRONCO,   ARTIC_UNIVERSAL },
    [ART_PESCOCO]   = { PARTE_TRONCO, PARTE_CABECA,   ARTIC_UNIVERSAL },
    [ART_OMBRO_E]   = { PARTE_TRONCO, PARTE_BRACO_E,  ARTIC_UNIVERSAL },
    [ART_OMBRO_D]   = { PARTE_TRONCO, PARTE_BRACO_D,  ARTIC_UNIVERSAL },
    [ART_QUADRIL_E] = { PARTE_BACIA,  PARTE_COXA_E,   ARTIC_UNIVERSAL },
    [ART_QUADRIL_D] = { PARTE_BACIA,  PARTE_COXA_D,   ARTIC_UNIVERSAL },
    [ART_JOELHO_E]  = { PARTE_COXA_E, PARTE_CANELA_E, ARTIC_DOBRADICA },
    [ART_JOELHO_D]  = { PARTE_COXA_D, PARTE_CANELA_D, ARTIC_DOBRADICA },
    [ART_COTOVELO_E]  = { PARTE_BRACO_E,  PARTE_ANTEBRACO_E, ARTIC_DOBRADICA },
    [ART_COTOVELO_D]  = { PARTE_BRACO_D,  PARTE_ANTEBRACO_D, ARTIC_DOBRADICA },
    [ART_TORNOZELO_E] = { PARTE_CANELA_E, PARTE_PE_E,        ARTIC_UNIVERSAL },
    [ART_TORNOZELO_D] = { PARTE_CANELA_D, PARTE_PE_D,        ARTIC_UNIVERSAL },
};

/* Eixos com o significado de esqueleto.h (ver a tabela de sinais lá). */
static const float EIXOS[ART_QTD][2][3] = {
    [ART_CINTURA]   = { {  1, 0,  0 }, { 0,  0, -1 } },   /* frente, esquerda   */
    [ART_PESCOCO]   = { {  1, 0,  0 }, { 0,  0, -1 } },
    [ART_OMBRO_E]   = { {  0, 0,  1 }, { 0, -1,  0 } },   /* levantar, à frente */
    [ART_OMBRO_D]   = { {  0, 0, -1 }, { 0,  1,  0 } },
    [ART_QUADRIL_E] = { { -1, 0,  0 }, { 0,  0,  1 } },   /* à frente, abrir    */
    [ART_QUADRIL_D] = { { -1, 0,  0 }, { 0,  0, -1 } },
    [ART_JOELHO_E]  = { {  1, 0,  0 }, { 0,  0,  0 } },   /* dobrar             */
    [ART_JOELHO_D]  = { {  1, 0,  0 }, { 0,  0,  0 } },
    [ART_COTOVELO_E]  = { { 0, -1, 0 }, { 0, 0,  0 } },   /* dobrar             */
    [ART_COTOVELO_D]  = { { 0,  1, 0 }, { 0, 0,  0 } },
    [ART_TORNOZELO_E] = { { -1, 0, 0 }, { 0, 0,  1 } },   /* ponta sobe, sola p/ fora */
    [ART_TORNOZELO_D] = { { -1, 0, 0 }, { 0, 0, -1 } },
};

void esqueleto_eixos(Articulacao art, float eixo1[3], float eixo2[3])
{
    for (int k = 0; k < 3; k++) {
        eixo1[k] = ((int)art >= 0 && art < ART_QTD) ? EIXOS[art][0][k] : 0.0f;
        eixo2[k] = ((int)art >= 0 && art < ART_QTD) ? EIXOS[art][1][k] : 0.0f;
    }
}

const char *esqueleto_nome_parte(ParteCorpo parte)
{
    if ((int)parte < 0 || parte >= PARTE_QTD)
        return "desconhecida";
    return NOMES_PARTES[parte];
}

TipoArticulacao esqueleto_tipo_articulacao(Articulacao art)
{
    if ((int)art < 0 || art >= ART_QTD)
        return ARTIC_UNIVERSAL;
    return ARTICULACOES[art].tipo;
}

void esqueleto_pecas_da_articulacao(Articulacao art, ParteCorpo *pai, ParteCorpo *filha)
{
    if ((int)art < 0 || art >= ART_QTD) {
        *pai = PARTE_BACIA;
        *filha = PARTE_BACIA;
        return;
    }
    *pai = ARTICULACOES[art].pai;
    *filha = ARTICULACOES[art].filha;
}

void pose_zerar(Pose *pose)
{
    for (int a = 0; a < ART_QTD; a++) {
        pose->alvo[a][0] = 0.0f;
        pose->alvo[a][1] = 0.0f;
    }
}
