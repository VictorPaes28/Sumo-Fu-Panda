/*
 * extrai_animacoes.c — converte animações de um modelo com esqueleto em
 * ângulos das 8 articulações do corpo físico ("retarget").
 *
 *   extrai_animacoes <modelo_animado.glb> <saida.c>
 *       grava src/animacoes_dados.c (parado e andar);
 *   extrai_animacoes --golpe <animacao.glb> <duracao_s> [trecho do nome]
 *       imprime as CHAVES de um golpe (golpe.h), prontas para colar em
 *       golpe.c: cada articulação e os cotovelos como diferença para o
 *       primeiro quadro, com o tempo esticado para durar duracao_s. No fim
 *       da animação o movimento volta suave a zero (o golpe volta à guarda).
 *       Sugere também o tempo do acerto (mãos mais rápidas). Ver
 *       docs/ANIMACOES.md.
 *
 * Para cada quadro (a 30 por segundo) e cada articulação:
 *   1. giro de cada peça desde a T-pose = rotação do osso agora x inversa da
 *      rotação do osso na T-pose (no mundo);
 *   2. giro do filho em relação ao pai = inversa(giro do pai) x giro do filho;
 *   3. esse giro é decomposto nos eixos de esqueleto_eixos(): primeiro em
 *      torno do eixo 1, depois do eixo 2 (como numa junta universal).
 * O modelo precisa estar em T-pose na pose de descanso (bind pose).
 *
 * Esqueletos reconhecidos: o do robô de Quaternius (exemplos da raylib) e o
 * do Mixamo (o mesmo do Meshy). Animações usadas: a que tem "Idle" no nome
 * (parado) e a que tem "Walk" (andar, sem "Jump").
 *
 * Dono: Principal.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "raylib.h"
#include "raymath.h"
#include "esqueleto.h"
#include "animacao.h"

#define FPS_SAIDA 30.0f

static const char *const OSSOS_ROBO[PARTE_QTD] = {
    [PARTE_BACIA] = "Hips", [PARTE_TRONCO] = "Torso", [PARTE_CABECA] = "Head",
    [PARTE_BRACO_E] = "UpperArm.L", [PARTE_BRACO_D] = "UpperArm.R",
    [PARTE_COXA_E] = "UpperLeg.L", [PARTE_COXA_D] = "UpperLeg.R",
    [PARTE_CANELA_E] = "LowerLeg.L", [PARTE_CANELA_D] = "LowerLeg.R",
    [PARTE_ANTEBRACO_E] = "LowerArm.L", [PARTE_ANTEBRACO_D] = "LowerArm.R",
    [PARTE_PE_E] = "Foot.L", [PARTE_PE_D] = "Foot.R",
};

static const char *const OSSOS_MIXAMO[PARTE_QTD] = {
    [PARTE_BACIA] = "mixamorig:Hips", [PARTE_TRONCO] = "mixamorig:Spine2",
    [PARTE_CABECA] = "mixamorig:Head",
    [PARTE_BRACO_E] = "mixamorig:LeftArm", [PARTE_BRACO_D] = "mixamorig:RightArm",
    [PARTE_COXA_E] = "mixamorig:LeftUpLeg", [PARTE_COXA_D] = "mixamorig:RightUpLeg",
    [PARTE_CANELA_E] = "mixamorig:LeftLeg", [PARTE_CANELA_D] = "mixamorig:RightLeg",
    [PARTE_ANTEBRACO_E] = "mixamorig:LeftForeArm", [PARTE_ANTEBRACO_D] = "mixamorig:RightForeArm",
    [PARTE_PE_E] = "mixamorig:LeftFoot", [PARTE_PE_D] = "mixamorig:RightFoot",
};

/* Braço, antebraço e mão (esquerdo, depois direito): a dobra do cotovelo é
 * o ângulo entre braço->antebraço e antebraço->mão. */
static const char *const COTOVELO_ROBO[2][3] = {
    { "UpperArm.L", "LowerArm.L", "Palm2.L" }, { "UpperArm.R", "LowerArm.R", "Palm2.R" },
};
static const char *const COTOVELO_MIXAMO[2][3] = {
    { "mixamorig:LeftArm", "mixamorig:LeftForeArm", "mixamorig:LeftHand" },
    { "mixamorig:RightArm", "mixamorig:RightForeArm", "mixamorig:RightHand" },
};

static int achar_osso(const Model *m, const char *nome)
{
    for (int i = 0; i < m->skeleton.boneCount; i++)
        if (strcmp(m->skeleton.bones[i].name, nome) == 0)
            return i;
    return -1;
}

static Vector3 vetor(const float v[3])
{
    return (Vector3){ v[0], v[1], v[2] };
}

/* Ângulo que leva o vetor 'de' até 'para' girando em torno de 'eixo'
 * (os dois perpendiculares ao eixo). */
static float angulo_em_torno(Vector3 eixo, Vector3 de, Vector3 para)
{
    Vector3 terceiro = Vector3CrossProduct(eixo, de);
    return atan2f(Vector3DotProduct(para, terceiro), Vector3DotProduct(para, de));
}

/* Decompõe o giro q (filho em relação ao pai) nos eixos da articulação. */
static void decompor(Articulacao art, Quaternion q, float angulos[2])
{
    float e1[3], e2[3];
    esqueleto_eixos(art, e1, e2);
    Vector3 u = vetor(e1);

    if (esqueleto_tipo_articulacao(art) == ARTIC_DOBRADICA) {
        Vector3 v = (fabsf(u.y) < 0.9f) ? (Vector3){ 0, 1, 0 } : (Vector3){ 0, 0, 1 };
        v = Vector3Normalize(Vector3Subtract(v, Vector3Scale(u, Vector3DotProduct(u, v))));
        angulos[0] = angulo_em_torno(u, v, Vector3RotateByQuaternion(v, q));
        angulos[1] = 0.0f;
        return;
    }

    /* q = giro(u, a1) x giro(v, a2). O eixo v não muda com o giro em v, então
     * q leva v aonde só o giro em u levaria: daí sai a1. Tirando a1, sobra a2. */
    Vector3 v = vetor(e2);
    angulos[0] = angulo_em_torno(u, v, Vector3RotateByQuaternion(v, q));
    Quaternion sem_a1 = QuaternionMultiply(QuaternionFromAxisAngle(u, -angulos[0]), q);
    angulos[1] = angulo_em_torno(v, u, Vector3RotateByQuaternion(u, sem_a1));
}

/* Ângulos das articulações num quadro da animação. */
static void angulos_do_quadro(const Model *m, const Transform *pose,
                              const int osso_da_peca[PARTE_QTD], float saida[ART_QTD][2])
{
    Quaternion giro[PARTE_QTD];
    for (int p = 0; p < PARTE_QTD; p++) {
        int b = osso_da_peca[p];
        giro[p] = QuaternionMultiply(pose[b].rotation,
                                     QuaternionInvert(m->skeleton.bindPose[b].rotation));
    }
    for (int a = 0; a < ART_QTD; a++) {
        ParteCorpo pai, filha;
        esqueleto_pecas_da_articulacao((Articulacao)a, &pai, &filha);
        Quaternion rel = QuaternionMultiply(QuaternionInvert(giro[pai]), giro[filha]);
        decompor((Articulacao)a, rel, saida[a]);
    }
}

static void escrever_animacao(FILE *f, const char *ident, const char *nome,
                              const Model *m, const ModelAnimation *anim,
                              const int osso_da_peca[PARTE_QTD])
{
    int passo = (int)roundf(60.0f / FPS_SAIDA);              /* a raylib lê glTF a 60 qps */
    int quadros = anim->keyframeCount / passo;
    double soma[ART_QTD][2] = { { 0 } };
    float menor[ART_QTD][2], maior[ART_QTD][2];
    for (int a = 0; a < ART_QTD; a++)
        for (int e = 0; e < 2; e++) {
            menor[a][e] = 1e9f;
            maior[a][e] = -1e9f;
        }

    fprintf(f, "static const Pose QUADROS_%s[%d] = {\n", ident, quadros);
    for (int q = 0; q < quadros; q++) {
        float angulos[ART_QTD][2];
        angulos_do_quadro(m, anim->keyframePoses[q * passo], osso_da_peca, angulos);
        fprintf(f, "    { .alvo = {");
        for (int a = 0; a < ART_QTD; a++) {
            const float *ang = angulos[a];
            for (int e = 0; e < 2; e++) {
                soma[a][e] += ang[e];
                menor[a][e] = fminf(menor[a][e], ang[e]);
                maior[a][e] = fmaxf(maior[a][e], ang[e]);
            }
            fprintf(f, " { %.4ff, %.4ff },", ang[0], ang[1]);
        }
        fprintf(f, " } },\n");
    }
    fprintf(f, "};\n\n");

    fprintf(f, "static const Animacao DADOS_%s = {\n", ident);
    fprintf(f, "    .nome = \"%s\", .fps = %.1ff, .quadros = %d, .poses = QUADROS_%s,\n",
            nome, FPS_SAIDA, quadros, ident);
    fprintf(f, "    .media = { .alvo = {");
    for (int a = 0; a < ART_QTD; a++)
        fprintf(f, " { %.4ff, %.4ff },", soma[a][0] / quadros, soma[a][1] / quadros);
    fprintf(f, " } },\n};\n\n");

    /* Resumo para conferir sinais: faixa de cada ângulo, em rad. */
    static const char *const NOMES_ART[ART_QTD] = {
        "cintura", "pescoco", "ombro_e", "ombro_d", "quadril_e", "quadril_d", "joelho_e", "joelho_d"
    };
    printf("%s: %d quadros (%.2f s)\n", nome, quadros, quadros / FPS_SAIDA);
    for (int a = 0; a < ART_QTD; a++)
        printf("  %-10s eixo1 de %6.2f a %6.2f   eixo2 de %6.2f a %6.2f\n", NOMES_ART[a],
               menor[a][0], maior[a][0], menor[a][1], maior[a][1]);
}

static Vector3 posicao(const Transform *pose, int osso)
{
    return pose[osso].translation;
}

/* Dobra dos cotovelos num quadro (rad, 0 = reto). */
static void cotovelos_do_quadro(const Transform *pose, int ossos[2][3], float saida[2])
{
    for (int lado = 0; lado < 2; lado++) {
        Vector3 braco = Vector3Subtract(posicao(pose, ossos[lado][1]), posicao(pose, ossos[lado][0]));
        Vector3 antebraco = Vector3Subtract(posicao(pose, ossos[lado][2]), posicao(pose, ossos[lado][1]));
        saida[lado] = Vector3Angle(braco, antebraco);
    }
}

/* Cotovelos (dobra medida pelos ossos) e tornozelos (fora da animação: o
 * corpo cuida dos pés) nas juntas novas da pose. */
static void juntas_novas(float ang[ART_QTD][2], const float cot[2])
{
    ang[ART_COTOVELO_E][0] = cot[0];
    ang[ART_COTOVELO_D][0] = cot[1];
    ang[ART_COTOVELO_E][1] = ang[ART_COTOVELO_D][1] = 0.0f;
    for (int a = ART_TORNOZELO_E; a <= ART_TORNOZELO_D; a++)
        ang[a][0] = ang[a][1] = 0.0f;
}

/* Modo --golpe: imprime as chaves do golpe (golpe.h) a ~30 por segundo. */
static int extrair_golpe(const Model *m, const ModelAnimation *anim, float duracao,
                         const int osso_da_peca[PARTE_QTD], int cotovelo[2][3])
{
    int n = anim->keyframeCount;                     /* a raylib lê glTF a 60 qps */
    float original = n / 60.0f;
    int chaves = (int)ceilf(duracao * FPS_SAIDA) + 1;
    float base[ART_QTD][2], base_cot[2];
    angulos_do_quadro(m, anim->keyframePoses[0], osso_da_peca, base);
    cotovelos_do_quadro(anim->keyframePoses[0], cotovelo, base_cot);
    juntas_novas(base, base_cot);

    /* Acerto sugerido: quadro em que a mão mais rápida é mais rápida. */
    float maior = 0.0f, t_acerto = 0.0f;
    for (int q = 1; q < n; q++)
        for (int lado = 0; lado < 2; lado++) {
            float v = Vector3Distance(posicao(anim->keyframePoses[q], cotovelo[lado][2]),
                                      posicao(anim->keyframePoses[q - 1], cotovelo[lado][2])) * 60.0f;
            if (v > maior) { maior = v; t_acerto = q / 60.0f; }
        }

    printf("/* Gerado por extrai_animacoes --golpe a partir de \"%s\"\n", anim->name);
    printf(" * (%.2f s esticados para %.2f s). Acerto sugerido: t = %.2f s\n", original, duracao,
           t_acerto * duracao / original);
    printf(" * (tempo_preparo um pouco antes disso). */\n");
    printf("static const ChaveGolpe CHAVES_NOVO[] = {\n");
    for (int k = 0; k < chaves; k++) {
        float u = (float)k / (float)(chaves - 1);    /* 0 a 1 ao longo da animação */
        int q = (int)roundf(u * (float)(n - 1));
        float ang[ART_QTD][2], cot[2];
        angulos_do_quadro(m, anim->keyframePoses[q], osso_da_peca, ang);
        cotovelos_do_quadro(anim->keyframePoses[q], cotovelo, cot);
        juntas_novas(ang, cot);
        /* Nos últimos 20% volta suave a zero: o golpe termina na guarda. */
        float fim = Clamp((1.0f - u) / 0.2f, 0.0f, 1.0f);
        float peso = fim * fim * (3.0f - 2.0f * fim);
        printf("    { .tempo = %.3ff, .pose = { .alvo = {", u * duracao);
        for (int a = 0; a < ART_QTD; a++)
            printf(" { %.3ff, %.3ff },", (ang[a][0] - base[a][0]) * peso, (ang[a][1] - base[a][1]) * peso);
        printf(" } } },\n");
    }
    printf("};\n");
    return 0;
}

static int achar_animacao(const ModelAnimation *anims, int qtd, const char *com, const char *sem)
{
    for (int i = 0; i < qtd; i++)
        if (strstr(anims[i].name, com) != NULL && (sem == NULL || strstr(anims[i].name, sem) == NULL))
            return i;
    return -1;
}

int main(int argc, char **argv)
{
    bool golpe = (argc > 1 && strcmp(argv[1], "--golpe") == 0);
    if (argc < 3 || (golpe && argc < 4)) {
        printf("uso: extrai_animacoes <modelo_animado.glb> <saida.c>\n"
               "     extrai_animacoes --golpe <animacao.glb> <duracao_s> [trecho do nome]\n");
        return 1;
    }
    const char *arquivo = golpe ? argv[2] : argv[1];
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(100, 100, "extrai_animacoes");

    Model m = LoadModel(arquivo);
    int qtd = 0;
    ModelAnimation *anims = LoadModelAnimations(arquivo, &qtd);

    bool mixamo = achar_osso(&m, "mixamorig:Hips") >= 0;
    const char *const *nomes = mixamo ? OSSOS_MIXAMO : OSSOS_ROBO;
    int osso_da_peca[PARTE_QTD];
    for (int p = 0; p < PARTE_QTD; p++) {
        osso_da_peca[p] = achar_osso(&m, nomes[p]);
        if (osso_da_peca[p] < 0) {
            printf("osso \"%s\" não encontrado: esqueleto desconhecido\n", nomes[p]);
            return 1;
        }
    }

    if (golpe) {
        const char *const (*nomes_cot)[3] = mixamo ? COTOVELO_MIXAMO : COTOVELO_ROBO;
        int cotovelo[2][3];
        for (int lado = 0; lado < 2; lado++)
            for (int k = 0; k < 3; k++)
                if ((cotovelo[lado][k] = achar_osso(&m, nomes_cot[lado][k])) < 0) {
                    printf("osso \"%s\" não encontrado\n", nomes_cot[lado][k]);
                    return 1;
                }
        int qual = (argc > 4) ? achar_animacao(anims, qtd, argv[4], NULL) : (qtd > 0 ? 0 : -1);
        if (qual < 0) {
            printf("animação não encontrada; o arquivo tem:\n");
            for (int i = 0; i < qtd; i++)
                printf("  %s\n", anims[i].name);
            return 1;
        }
        int r = extrair_golpe(&m, &anims[qual], (float)atof(argv[3]), osso_da_peca, cotovelo);
        UnloadModelAnimations(anims, qtd);
        UnloadModel(m);
        CloseWindow();
        return r;
    }

    int parado = achar_animacao(anims, qtd, "Idle", NULL);
    int andar = achar_animacao(anims, qtd, "Walk", "Jump");
    if (parado < 0 || andar < 0) {
        printf("o modelo precisa ter animações com \"Idle\" e \"Walk\" no nome\n");
        return 1;
    }

    FILE *f = fopen(argv[2], "w");
    if (f == NULL) {
        printf("não consegui criar %s\n", argv[2]);
        return 1;
    }
    fprintf(f, "/*\n * animacoes_dados.c — GERADO por ferramentas/extrai_animacoes.c. NÃO EDITE À MÃO.\n");
    fprintf(f, " * Origem: %s (animações \"%s\" e \"%s\").\n", GetFileName(argv[1]),
            anims[parado].name, anims[andar].name);
    fprintf(f, " * Ângulos das articulações (rad) por quadro; significado dos sinais em esqueleto.h.\n */\n");
    fprintf(f, "#include <stddef.h>\n#include \"animacao.h\"\n\n");
    escrever_animacao(f, "PARADO", "parado", &m, &anims[parado], osso_da_peca);
    escrever_animacao(f, "ANDAR", "andar", &m, &anims[andar], osso_da_peca);
    fprintf(f, "const Animacao *animacao_dados(TipoAnimacao tipo)\n{\n");
    fprintf(f, "    switch (tipo) {\n");
    fprintf(f, "    case ANIM_PARADO: return &DADOS_PARADO;\n");
    fprintf(f, "    case ANIM_ANDAR:  return &DADOS_ANDAR;\n");
    fprintf(f, "    default:          return NULL;\n    }\n}\n");
    fclose(f);
    printf("gravado: %s\n", argv[2]);

    UnloadModelAnimations(anims, qtd);
    UnloadModel(m);
    CloseWindow();
    return 0;
}
