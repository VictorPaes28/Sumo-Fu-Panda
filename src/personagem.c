/*
 * personagem.c — catálogo de personagens (tabela constante).
 * Especificação: personagem.h e docs/ASSETS.md
 *
 * Coordenadas: T-pose, pés na origem, olhando para +Z, Y para cima,
 * esquerda do personagem = X positivo. Metros e quilogramas.
 *
 * Dono: Integrante B (tabela com as medidas da arte).
 *       Principal (ajuste de massas e forças).
 */
#include <stddef.h>
#include "personagem.h"
#include "config.h"

/*
 * PROVISÓRIO: corpo genérico de 1,72 m, igual para todos, até a arte entregar
 * as medidas de cada modelo (tabelas em docs/ASSETS.md). As diferenças de
 * jogo entre os personagens vêm por enquanto das massas e das forças.
 */
#define CORPO_GENERICO                                                        \
    .centro = {                                                               \
        [PARTE_BACIA]       = {  0.00f, 0.93f,  0.0f },                       \
        [PARTE_TRONCO]      = {  0.00f, 1.25f,  0.0f },                       \
        [PARTE_CABECA]      = {  0.00f, 1.59f,  0.0f },                       \
        [PARTE_BRACO_E]     = {  0.37f, 1.38f,  0.0f },                       \
        [PARTE_BRACO_D]     = { -0.37f, 1.38f,  0.0f },                       \
        [PARTE_ANTEBRACO_E] = {  0.67f, 1.38f,  0.0f },                       \
        [PARTE_ANTEBRACO_D] = { -0.67f, 1.38f,  0.0f },                       \
        [PARTE_COXA_E]      = {  0.12f, 0.62f,  0.0f },                       \
        [PARTE_COXA_D]      = { -0.12f, 0.62f,  0.0f },                       \
        [PARTE_CANELA_E]    = {  0.12f, 0.255f, 0.0f },                       \
        [PARTE_CANELA_D]    = { -0.12f, 0.255f, 0.0f },                       \
        [PARTE_PE_E]        = {  0.12f, 0.045f, 0.06f },                      \
        [PARTE_PE_D]        = { -0.12f, 0.045f, 0.06f },                      \
    },                                                                        \
    .ancora = {                                                               \
        [ART_CINTURA]     = {  0.00f, 1.04f, 0.0f },                          \
        [ART_PESCOCO]     = {  0.00f, 1.46f, 0.0f },                          \
        [ART_OMBRO_E]     = {  0.24f, 1.38f, 0.0f },                          \
        [ART_OMBRO_D]     = { -0.24f, 1.38f, 0.0f },                          \
        [ART_QUADRIL_E]   = {  0.12f, 0.82f, 0.0f },                          \
        [ART_QUADRIL_D]   = { -0.12f, 0.82f, 0.0f },                          \
        [ART_JOELHO_E]    = {  0.12f, 0.42f, 0.0f },                          \
        [ART_JOELHO_D]    = { -0.12f, 0.42f, 0.0f },                          \
        [ART_COTOVELO_E]  = {  0.50f, 1.38f, 0.0f },                          \
        [ART_COTOVELO_D]  = { -0.50f, 1.38f, 0.0f },                          \
        [ART_TORNOZELO_E] = {  0.12f, 0.09f, 0.0f },                          \
        [ART_TORNOZELO_D] = { -0.12f, 0.09f, 0.0f },                          \
    }

#define TAMANHOS(larg_tronco, larg_bacia)                                     \
    .tamanho = {                                                              \
        [PARTE_BACIA]       = { (larg_bacia), 0.22f, 0.30f },                 \
        [PARTE_TRONCO]      = { (larg_tronco), 0.42f, 0.34f },                \
        [PARTE_CABECA]      = { 0.26f, 0.26f, 0.26f },                        \
        [PARTE_BRACO_E]     = { 0.26f, 0.14f, 0.14f },                        \
        [PARTE_BRACO_D]     = { 0.26f, 0.14f, 0.14f },                        \
        [PARTE_ANTEBRACO_E] = { 0.34f, 0.12f, 0.12f },                        \
        [PARTE_ANTEBRACO_D] = { 0.34f, 0.12f, 0.12f },                        \
        [PARTE_COXA_E]      = { 0.18f, 0.40f, 0.20f },                        \
        [PARTE_COXA_D]      = { 0.18f, 0.40f, 0.20f },                        \
        [PARTE_CANELA_E]    = { 0.14f, 0.33f, 0.16f },                        \
        [PARTE_CANELA_D]    = { 0.14f, 0.33f, 0.16f },                        \
        [PARTE_PE_E]        = { 0.15f, 0.09f, 0.26f },                        \
        [PARTE_PE_D]        = { 0.15f, 0.09f, 0.26f },                        \
    }

/* Ossos do rig do Mixamo (o Meshy usa o mesmo padrão) ligados às peças.
 * Mãos, dedos dos pés e o topo da cabeça seguem o osso pai. */
static const OssoPeca OSSOS_MIXAMO[] = {
    { "mixamorig:Hips",          PARTE_BACIA },
    { "mixamorig:Spine",         PARTE_TRONCO },
    { "mixamorig:Spine1",        PARTE_TRONCO },
    { "mixamorig:Spine2",        PARTE_TRONCO },
    { "mixamorig:Neck",          PARTE_CABECA },
    { "mixamorig:Head",          PARTE_CABECA },
    { "mixamorig:LeftShoulder",  PARTE_TRONCO },
    { "mixamorig:LeftArm",       PARTE_BRACO_E },
    { "mixamorig:LeftForeArm",   PARTE_ANTEBRACO_E },
    { "mixamorig:RightShoulder", PARTE_TRONCO },
    { "mixamorig:RightArm",      PARTE_BRACO_D },
    { "mixamorig:RightForeArm",  PARTE_ANTEBRACO_D },
    { "mixamorig:LeftUpLeg",     PARTE_COXA_E },
    { "mixamorig:LeftLeg",       PARTE_CANELA_E },
    { "mixamorig:LeftFoot",      PARTE_PE_E },
    { "mixamorig:RightUpLeg",    PARTE_COXA_D },
    { "mixamorig:RightLeg",      PARTE_CANELA_D },
    { "mixamorig:RightFoot",     PARTE_PE_D },
};

static const Personagem CATALOGO[] = {
    {
        /* Po: pesado e lento, difícil de derrubar. */
        .nome = "Po",
        .pasta = "po",
        CORPO_GENERICO,
        TAMANHOS(0.48f, 0.42f),
        /* 98 kg. Proporções de De Leva (1996), com mais peso no tronco e na
         * bacia (barriga) e um pouco menos nos braços e nas coxas. */
        .massa = {
            [PARTE_BACIA] = 13.7f, [PARTE_TRONCO] = 34.3f, [PARTE_CABECA] = 5.9f,
            [PARTE_BRACO_E] = 2.45f, [PARTE_BRACO_D] = 2.45f,
            [PARTE_ANTEBRACO_E] = 1.96f, [PARTE_ANTEBRACO_D] = 1.96f,
            [PARTE_COXA_E] = 12.5f, [PARTE_COXA_D] = 12.5f,
            [PARTE_CANELA_E] = 3.9f, [PARTE_CANELA_D] = 3.9f,
            [PARTE_PE_E] = 1.2f, [PARTE_PE_D] = 1.2f,
        },
        /* A barriga: o peso do tronco e da bacia fica à frente do meio. */
        .centro_massa = {
            [PARTE_BACIA]  = { 0.0f,  0.00f, 0.04f },
            [PARTE_TRONCO] = { 0.0f, -0.03f, 0.05f },
        },
        .forca_equilibrio = 900.0f,
        .forca_andar = 900.0f,
        .velocidade_max = 1.3f,
        /* Panda pesado: base larga, passos lentos e curtos, bamboleia o
         * corpo todo ao andar e se desespera com os braços quando perde o
         * equilíbrio. */
        .estilo = {
            .base_quadril = 0.35f, .base_abertura = 0.28f, .base_joelho = 0.40f,
            .inclinacao = 0.08f,
            .guarda_ombro = -1.10f, .guarda_frente = 0.40f, .guarda_cintura = 0.10f,
            .guarda_cotovelo = 0.45f,
            .balanco_parado = 1.2f,
            .periodo_passo = 0.75f, .amplitude_passo = 0.45f,
            .balanco_bracos = 0.75f, .gingado = 0.12f,
            .reflexo_bracos = 1.2f, .passo_reacao = 0.85f,
        },
    },
    {
        /* Tai Lung: mais leve e rápido, porém mais fácil de empurrar.
         * Âncoras pelos ossos e caixas pela MALHA do modelo
         * (inspeciona_modelo --caixas 1; esquerda e direita igualadas).
         * A cabeça usa uma caixa medida à mão: no rig do Meshy boa parte da
         * cabeça está presa ao osso do tronco, e a medição sai pequena. */
        .nome = "Tai Lung",
        .pasta = "tai_lung",
        .centro = {
            [PARTE_BACIA]    = {  0.000f, 0.714f,  0.013f },
            [PARTE_TRONCO]   = {  0.000f, 1.139f, -0.026f },
            [PARTE_CABECA]   = {  0.000f, 1.490f,  0.000f },
            [PARTE_BRACO_E]     = {  0.395f, 1.173f, -0.073f },
            [PARTE_BRACO_D]     = { -0.395f, 1.173f, -0.073f },
            [PARTE_ANTEBRACO_E] = {  0.737f, 1.182f,  0.012f },
            [PARTE_ANTEBRACO_D] = { -0.737f, 1.182f,  0.012f },
            [PARTE_COXA_E]      = {  0.161f, 0.484f,  0.027f },
            [PARTE_COXA_D]      = { -0.161f, 0.484f,  0.027f },
            [PARTE_CANELA_E]    = {  0.180f, 0.225f, -0.020f },
            [PARTE_CANELA_D]    = { -0.180f, 0.225f, -0.020f },
            [PARTE_PE_E]        = {  0.220f, 0.057f,  0.047f },
            [PARTE_PE_D]        = { -0.220f, 0.057f,  0.047f },
        },
        .tamanho = {
            [PARTE_BACIA]       = { 0.625f, 0.288f, 0.553f },
            [PARTE_TRONCO]      = { 0.534f, 0.598f, 0.506f },
            [PARTE_CABECA]      = { 0.220f, 0.220f, 0.240f },
            [PARTE_BRACO_E]     = { 0.205f, 0.288f, 0.276f },
            [PARTE_BRACO_D]     = { 0.205f, 0.288f, 0.276f },
            [PARTE_ANTEBRACO_E] = { 0.398f, 0.186f, 0.284f },
            [PARTE_ANTEBRACO_D] = { 0.398f, 0.186f, 0.284f },
            [PARTE_COXA_E]      = { 0.300f, 0.233f, 0.478f },
            [PARTE_COXA_D]      = { 0.300f, 0.233f, 0.478f },
            [PARTE_CANELA_E]    = { 0.248f, 0.188f, 0.258f },
            [PARTE_CANELA_D]    = { 0.248f, 0.188f, 0.258f },
            [PARTE_PE_E]        = { 0.223f, 0.113f, 0.312f },
            [PARTE_PE_D]        = { 0.223f, 0.113f, 0.312f },
        },
        .ancora = {
            [ART_CINTURA]   = {  0.000f, 0.860f, 0.0f },
            [ART_PESCOCO]   = {  0.000f, 1.390f, 0.0f },
            [ART_OMBRO_E]   = {  0.307f, 1.213f, 0.0f },
            [ART_OMBRO_D]   = { -0.308f, 1.213f, 0.0f },
            [ART_QUADRIL_E] = {  0.154f, 0.648f, 0.0f },
            [ART_QUADRIL_D] = { -0.155f, 0.648f, 0.0f },
            [ART_JOELHO_E]  = {  0.161f, 0.371f, 0.0f },
            [ART_JOELHO_D]  = { -0.162f, 0.371f, 0.0f },
            [ART_COTOVELO_E]  = {  0.480f, 1.187f, 0.0f },
            [ART_COTOVELO_D]  = { -0.481f, 1.187f, 0.0f },
            [ART_TORNOZELO_E] = {  0.192f, 0.114f, 0.0f },
            [ART_TORNOZELO_D] = { -0.193f, 0.114f, 0.0f },
        },
        /* 75 kg. Proporções de De Leva (1996), com braços e ombros mais
         * pesados (empurra forte) e bacia mais leve (desequilibra fácil). */
        .massa = {
            [PARTE_BACIA] = 9.5f, [PARTE_TRONCO] = 24.2f, [PARTE_CABECA] = 4.5f,
            [PARTE_BRACO_E] = 2.25f, [PARTE_BRACO_D] = 2.25f,
            [PARTE_ANTEBRACO_E] = 1.8f, [PARTE_ANTEBRACO_D] = 1.8f,
            [PARTE_COXA_E] = 10.1f, [PARTE_COXA_D] = 10.1f,
            [PARTE_CANELA_E] = 3.25f, [PARTE_CANELA_D] = 3.25f,
            [PARTE_PE_E] = 1.0f, [PARTE_PE_D] = 1.0f,
        },
        /* Peito e ombros pesados: o peso do tronco fica mais para cima. */
        .centro_massa = {
            [PARTE_TRONCO] = { 0.0f, 0.04f, 0.0f },
        },
        .modelo = "tai_lung.glb",
        .escala_modelo = 1.0f,
        .ossos = OSSOS_MIXAMO,
        .qtd_ossos = (int)(sizeof OSSOS_MIXAMO / sizeof OSSOS_MIXAMO[0]),
        .forca_equilibrio = 700.0f,
        .forca_andar = 750.0f,
        .velocidade_max = 1.7f,
        /* Leopardo: postura de caçador (tronco à frente, braços prontos),
         * passos rápidos e firmes, quase sem gingado; recupera o equilíbrio
         * com passos largos. */
        .estilo = {
            .base_quadril = 0.35f, .base_abertura = 0.20f, .base_joelho = 0.40f,
            .inclinacao = 0.10f,
            .guarda_ombro = -0.85f, .guarda_frente = 0.60f, .guarda_cintura = 0.18f,
            .guarda_cotovelo = 0.70f,
            .balanco_parado = 0.8f,
            .periodo_passo = 0.50f, .amplitude_passo = 0.60f,
            .balanco_bracos = 0.45f, .gingado = 0.03f,
            .reflexo_bracos = 0.9f, .passo_reacao = 1.2f,
        },
    },
    {
        /* Oogway: velho mestre, o mais pesado; tão firme quanto os outros
         * (mesmo equilíbrio por kg), mas o mais lento e o que menos força faz
         * para andar e empurrar (idade). Âncoras pelos ossos (o rig foi montado no Blender com
         * os nomes do Mixamo) e caixas pela MALHA (inspeciona_modelo
         * --caixas 2; esquerda e direita igualadas), menos bacia e coxas: os
         * pesos automáticos deram às coxas parte da faixa, então a bacia é
         * larga como a do Tai Lung e as coxas vão do quadril ao joelho. */
        .nome = "Oogway",
        .pasta = "oogway",
        .centro = {
            [PARTE_BACIA]       = {  0.000f, 0.700f, -0.080f },
            [PARTE_TRONCO]      = {  0.000f, 1.172f, -0.127f },
            [PARTE_CABECA]      = {  0.000f, 1.491f,  0.110f },
            [PARTE_BRACO_E]     = {  0.521f, 1.101f, -0.085f },
            [PARTE_BRACO_D]     = { -0.521f, 1.101f, -0.085f },
            [PARTE_ANTEBRACO_E] = {  0.906f, 1.122f, -0.141f },
            [PARTE_ANTEBRACO_D] = { -0.906f, 1.122f, -0.141f },
            [PARTE_COXA_E]      = {  0.235f, 0.630f, -0.060f },
            [PARTE_COXA_D]      = { -0.235f, 0.630f, -0.060f },
            [PARTE_CANELA_E]    = {  0.205f, 0.335f, -0.068f },
            [PARTE_CANELA_D]    = { -0.205f, 0.335f, -0.068f },
            [PARTE_PE_E]        = {  0.238f, 0.070f, -0.020f },
            [PARTE_PE_D]        = { -0.238f, 0.070f, -0.020f },
        },
        .tamanho = {
            [PARTE_BACIA]       = { 0.550f, 0.300f, 0.550f },
            [PARTE_TRONCO]      = { 0.667f, 0.422f, 0.509f },
            [PARTE_CABECA]      = { 0.190f, 0.416f, 0.252f },
            [PARTE_BRACO_E]     = { 0.305f, 0.205f, 0.265f },
            [PARTE_BRACO_D]     = { 0.305f, 0.205f, 0.265f },
            [PARTE_ANTEBRACO_E] = { 0.395f, 0.144f, 0.191f },
            [PARTE_ANTEBRACO_D] = { 0.395f, 0.144f, 0.191f },
            [PARTE_COXA_E]      = { 0.240f, 0.220f, 0.300f },
            [PARTE_COXA_D]      = { 0.240f, 0.220f, 0.300f },
            [PARTE_CANELA_E]    = { 0.275f, 0.370f, 0.288f },
            [PARTE_CANELA_D]    = { 0.275f, 0.370f, 0.288f },
            [PARTE_PE_E]        = { 0.234f, 0.139f, 0.322f },
            [PARTE_PE_D]        = { 0.234f, 0.139f, 0.322f },
        },
        .ancora = {
            [ART_CINTURA]     = {  0.000f, 0.869f, 0.0f },
            [ART_PESCOCO]     = {  0.000f, 1.269f, 0.0f },
            [ART_OMBRO_E]     = {  0.338f, 1.116f, -0.153f },
            [ART_OMBRO_D]     = { -0.338f, 1.116f, -0.153f },
            [ART_QUADRIL_E]   = {  0.251f, 0.720f, 0.0f },
            [ART_QUADRIL_D]   = { -0.251f, 0.720f, 0.0f },
            [ART_JOELHO_E]    = {  0.211f, 0.537f, 0.0f },
            [ART_JOELHO_D]    = { -0.211f, 0.537f, 0.0f },
            [ART_COTOVELO_E]  = {  0.689f, 1.135f, -0.136f },
            [ART_COTOVELO_D]  = { -0.689f, 1.135f, -0.136f },
            [ART_TORNOZELO_E] = {  0.247f, 0.148f, 0.0f },
            [ART_TORNOZELO_D] = { -0.247f, 0.148f, 0.0f },
        },
        /* ~102 kg. Proporções de De Leva (1996) com o casco: tronco bem mais
         * pesado e cabeça leve. */
        .massa = {
            [PARTE_BACIA] = 12.0f, [PARTE_TRONCO] = 40.0f, [PARTE_CABECA] = 5.5f,
            [PARTE_BRACO_E] = 2.6f, [PARTE_BRACO_D] = 2.6f,
            [PARTE_ANTEBRACO_E] = 2.0f, [PARTE_ANTEBRACO_D] = 2.0f,
            [PARTE_COXA_E] = 12.5f, [PARTE_COXA_D] = 12.5f,
            [PARTE_CANELA_E] = 4.0f, [PARTE_CANELA_D] = 4.0f,
            [PARTE_PE_E] = 1.3f, [PARTE_PE_D] = 1.3f,
        },
        /* O casco: o peso do tronco fica atrás do meio. */
        .centro_massa = {
            [PARTE_TRONCO] = { 0.0f, -0.02f, -0.08f },
        },
        .modelo = "oogway.glb",
        .escala_modelo = 1.0f,
        .ossos = OSSOS_MIXAMO,
        .qtd_ossos = (int)(sizeof OSSOS_MIXAMO / sizeof OSSOS_MIXAMO[0]),
        .forca_equilibrio = 950.0f,
        .forca_andar = 560.0f,
        .velocidade_max = 0.8f,
        /* Tartaruga mestre: base baixa e larga, braços calmos e abertos,
         * passos curtos e lentos; quase não se assusta (reflexos contidos)
         * e balança pouco parado (sereno). */
        .estilo = {
            .base_quadril = 0.40f, .base_abertura = 0.30f, .base_joelho = 0.48f,
            .inclinacao = 0.06f,
            .guarda_ombro = -1.00f, .guarda_frente = 0.35f, .guarda_cintura = 0.06f,
            .guarda_cotovelo = 0.55f,
            .balanco_parado = 0.6f,
            .periodo_passo = 0.90f, .amplitude_passo = 0.35f,
            .balanco_bracos = 0.35f, .gingado = 0.06f,
            .reflexo_bracos = 0.7f, .passo_reacao = 0.8f,
        },
    },
    {
        /* Minotauro: o mais forte e o mais alto (1,78 m); empurra muito, mas
         * é grande e anda devagar. Malha pintada do Meshy sem rig: esqueleto
         * com os nomes do Mixamo montado no Blender (juntas pelas fatias da
         * malha, pesos automáticos). Âncoras pelos ossos e caixas pela MALHA
         * (inspeciona_modelo --caixas 3; esquerda e direita igualadas), menos
         * bacia, tronco e coxas: como no Oogway, as coxas levaram parte da
         * barriga e da faixa, então a bacia é larga, o tronco vai da cintura
         * ao pescoço e as coxas vão do quadril ao joelho. O pé físico é do
         * tamanho do dos outros (os cascos são pequenos: com eles o touro,
         * pesado em cima, tombava); braço e canela têm a medida comprida no
         * sentido do membro (a cápsula segue a maior medida da caixa). */
        .nome = "Minotauro",
        .pasta = "minotauro",
        .centro = {
            [PARTE_BACIA]       = {  0.000f, 0.700f, -0.022f },
            [PARTE_TRONCO]      = {  0.000f, 1.100f, -0.026f },
            [PARTE_CABECA]      = {  0.000f, 1.469f,  0.097f },
            [PARTE_BRACO_E]     = {  0.424f, 1.192f, -0.090f },
            [PARTE_BRACO_D]     = { -0.424f, 1.192f, -0.090f },
            [PARTE_ANTEBRACO_E] = {  0.758f, 1.205f, -0.045f },
            [PARTE_ANTEBRACO_D] = { -0.758f, 1.205f, -0.045f },
            [PARTE_COXA_E]      = {  0.186f, 0.480f, -0.022f },
            [PARTE_COXA_D]      = { -0.186f, 0.480f, -0.022f },
            [PARTE_CANELA_E]    = {  0.213f, 0.205f, -0.076f },
            [PARTE_CANELA_D]    = { -0.213f, 0.205f, -0.076f },
            [PARTE_PE_E]        = {  0.236f, 0.065f,  0.020f },
            [PARTE_PE_D]        = { -0.236f, 0.065f,  0.020f },
        },
        .tamanho = {
            [PARTE_BACIA]       = { 0.550f, 0.300f, 0.508f },
            [PARTE_TRONCO]      = { 0.552f, 0.500f, 0.454f },
            [PARTE_CABECA]      = { 0.323f, 0.340f, 0.261f },
            [PARTE_BRACO_E]     = { 0.280f, 0.240f, 0.240f },
            [PARTE_BRACO_D]     = { 0.280f, 0.240f, 0.240f },
            [PARTE_ANTEBRACO_E] = { 0.347f, 0.143f, 0.194f },
            [PARTE_ANTEBRACO_D] = { 0.347f, 0.143f, 0.194f },
            [PARTE_COXA_E]      = { 0.254f, 0.360f, 0.355f },
            [PARTE_COXA_D]      = { 0.254f, 0.360f, 0.355f },
            [PARTE_CANELA_E]    = { 0.170f, 0.200f, 0.170f },
            [PARTE_CANELA_D]    = { 0.170f, 0.200f, 0.170f },
            [PARTE_PE_E]        = { 0.200f, 0.128f, 0.300f },
            [PARTE_PE_D]        = { 0.200f, 0.128f, 0.300f },
        },
        .ancora = {
            [ART_CINTURA]     = {  0.000f, 0.850f, 0.0f },
            [ART_PESCOCO]     = {  0.000f, 1.300f, 0.0f },
            [ART_OMBRO_E]     = {  0.280f, 1.200f, 0.0f },
            [ART_OMBRO_D]     = { -0.280f, 1.200f, 0.0f },
            [ART_QUADRIL_E]   = {  0.180f, 0.660f, 0.0f },
            [ART_QUADRIL_D]   = { -0.180f, 0.660f, 0.0f },
            [ART_JOELHO_E]    = {  0.200f, 0.300f, 0.0f },
            [ART_JOELHO_D]    = { -0.200f, 0.300f, 0.0f },
            [ART_COTOVELO_E]  = {  0.560f, 1.195f, 0.0f },
            [ART_COTOVELO_D]  = { -0.560f, 1.195f, 0.0f },
            [ART_TORNOZELO_E] = {  0.220f, 0.120f, 0.0f },
            [ART_TORNOZELO_D] = { -0.220f, 0.120f, 0.0f },
        },
        /* ~115 kg. Proporções de De Leva (1996) com tronco, braços e cabeça
         * (chifres) mais pesados. */
        .massa = {
            [PARTE_BACIA] = 14.0f, [PARTE_TRONCO] = 42.0f, [PARTE_CABECA] = 7.0f,
            [PARTE_BRACO_E] = 3.2f, [PARTE_BRACO_D] = 3.2f,
            [PARTE_ANTEBRACO_E] = 2.4f, [PARTE_ANTEBRACO_D] = 2.4f,
            [PARTE_COXA_E] = 13.5f, [PARTE_COXA_D] = 13.5f,
            [PARTE_CANELA_E] = 4.3f, [PARTE_CANELA_D] = 4.3f,
            [PARTE_PE_E] = 1.4f, [PARTE_PE_D] = 1.4f,
        },
        /* Barriga e peito: o peso do tronco fica à frente do meio. */
        .centro_massa = {
            [PARTE_TRONCO] = { 0.0f, 0.02f, 0.04f },
        },
        .modelo = "minotauro.glb",
        .escala_modelo = 1.0f,
        .ossos = OSSOS_MIXAMO,
        .qtd_ossos = (int)(sizeof OSSOS_MIXAMO / sizeof OSSOS_MIXAMO[0]),
        .forca_equilibrio = 1050.0f,
        .forca_andar = 750.0f,
        .velocidade_max = 0.9f,
        /* Touro: base larga, tronco à frente como quem vai dar uma chifrada,
         * passos pesados; reflexos medianos. */
        .estilo = {
            .base_quadril = 0.38f, .base_abertura = 0.28f, .base_joelho = 0.44f,
            .inclinacao = 0.10f,
            .guarda_ombro = -1.00f, .guarda_frente = 0.45f, .guarda_cintura = 0.12f,
            .guarda_cotovelo = 0.55f,
            .balanco_parado = 0.9f,
            .periodo_passo = 0.80f, .amplitude_passo = 0.45f,
            .balanco_bracos = 0.55f, .gingado = 0.08f,
            .reflexo_bracos = 1.0f, .passo_reacao = 0.9f,
        },
    },
};

#define QTD_PERSONAGENS ((int)(sizeof CATALOGO / sizeof CATALOGO[0]))

/* O catálogo não pode passar do tamanho que os outros módulos reservam. */
typedef char verifica_max_personagens[(QTD_PERSONAGENS <= MAX_PERSONAGENS) ? 1 : -1];

int personagem_quantidade(void)
{
    return QTD_PERSONAGENS;
}

const Personagem *personagem_dados(int indice)
{
    if (indice < 0 || indice >= QTD_PERSONAGENS)
        return NULL;
    return &CATALOGO[indice];
}
