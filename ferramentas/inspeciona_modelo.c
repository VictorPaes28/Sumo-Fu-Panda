/*
 * inspeciona_modelo.c — mostra o esqueleto de um .glb e mede as caixas
 * físicas pelo modelo, para preencher personagem.c.
 *
 *   inspeciona_modelo <arquivo.glb> [altura_desejada_em_metros]
 *       altura, escala sugerida (padrão 1,7 m), pés e posição de cada osso
 *       (já multiplicada pela escala, em metros): são as âncoras das
 *       articulações (ver docs/ASSETS.md).
 *
 *   inspeciona_modelo <arquivo.glb> --caixas <indice_do_personagem>
 *       mede a caixa física de cada peça pela MALHA (e não pelos ossos): junta
 *       os vértices que se movem com cada peça (pela ligação Personagem.ossos),
 *       descarta as pontas soltas (orelhas, cauda: CORTE_PONTAS de cada lado)
 *       e imprime .centro e .tamanho prontos para colar em personagem.c, além
 *       de quanto da malha fica fora das caixas atuais e das novas.
 *
 * Dono: Principal (ferramenta da equipe de arte).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "raylib.h"
#include "personagem.h"

#define CORTE_PONTAS  0.05f   /* fração dos vértices ignorada em cada ponta, por eixo */
#define TOLERANCIA    0.02f   /* vértice a menos disto da caixa conta como dentro (m) */

static const char *const NOMES_PECAS[PARTE_QTD] = {
    "PARTE_BACIA", "PARTE_TRONCO", "PARTE_CABECA", "PARTE_BRACO_E", "PARTE_BRACO_D",
    "PARTE_COXA_E", "PARTE_COXA_D", "PARTE_CANELA_E", "PARTE_CANELA_D",
    "PARTE_ANTEBRACO_E", "PARTE_ANTEBRACO_D", "PARTE_PE_E", "PARTE_PE_D",
};

static int comparar(const void *a, const void *b)
{
    float x = *(const float *)a, y = *(const float *)b;
    return (x > y) - (x < y);
}

/* Peça que move cada osso: a da tabela do personagem, ou a do osso pai. */
static void pecas_dos_ossos(const Model *m, const Personagem *p, int *peca)
{
    int n = m->skeleton.boneCount;
    for (int i = 0; i < n; i++) {
        peca[i] = -1;
        for (int k = 0; k < p->qtd_ossos; k++)
            if (strcmp(m->skeleton.bones[i].name, p->ossos[k].osso) == 0)
                peca[i] = p->ossos[k].parte;
    }
    for (int volta = 0; volta < n; volta++)        /* propaga dos pais para os filhos */
        for (int i = 0; i < n; i++)
            if (peca[i] < 0) {
                int pai = m->skeleton.bones[i].parent;
                peca[i] = (pai < 0) ? PARTE_BACIA : peca[pai];
            }
}

/* Distância (m) do ponto até a caixa; 0 se dentro. */
static float fora_da_caixa(const float pt[3], const float centro[3], const float tamanho[3])
{
    float d2 = 0.0f;
    for (int e = 0; e < 3; e++) {
        float o = fabsf(pt[e] - centro[e]) - tamanho[e] * 0.5f;
        if (o > 0.0f) d2 += o * o;
    }
    return sqrtf(d2);
}

static int medir_caixas(const Model *m, const Personagem *p)
{
    int peca_do_osso[256];
    if (m->skeleton.boneCount > 256 || m->skeleton.boneCount == 0) {
        printf("o modelo precisa ter esqueleto (até 256 ossos)\n");
        return 1;
    }
    pecas_dos_ossos(m, p, peca_do_osso);

    /* Vértices de cada peça (pelo osso de maior peso), já na escala do personagem. */
    int total = 0;
    for (int mi = 0; mi < m->meshCount; mi++)
        total += m->meshes[mi].vertexCount;
    float *pontos = malloc((size_t)total * 3 * sizeof(float));
    int *peca_do_ponto = malloc((size_t)total * sizeof(int));
    int qtd[PARTE_QTD] = { 0 }, n = 0;
    for (int mi = 0; mi < m->meshCount; mi++) {
        const Mesh *me = &m->meshes[mi];
        if (me->boneWeights == NULL || me->boneIndices == NULL)
            continue;
        for (int v = 0; v < me->vertexCount; v++) {
            int osso = 0;
            float peso = -1.0f;
            for (int k = 0; k < 4; k++)
                if (me->boneWeights[v * 4 + k] > peso) {
                    peso = me->boneWeights[v * 4 + k];
                    osso = me->boneIndices[v * 4 + k];
                }
            for (int e = 0; e < 3; e++)
                pontos[n * 3 + e] = me->vertices[v * 3 + e] * p->escala_modelo;
            peca_do_ponto[n] = peca_do_osso[osso];
            qtd[peca_do_osso[osso]]++;
            n++;
        }
    }

    float centro[PARTE_QTD][3] = { { 0 } }, tamanho[PARTE_QTD][3] = { { 0 } };
    float *valores = malloc((size_t)n * sizeof(float));
    for (int pc = 0; pc < PARTE_QTD; pc++) {
        if (qtd[pc] == 0)
            continue;
        for (int e = 0; e < 3; e++) {
            int k = 0;
            for (int i = 0; i < n; i++)
                if (peca_do_ponto[i] == pc)
                    valores[k++] = pontos[i * 3 + e];
            qsort(valores, (size_t)k, sizeof(float), comparar);
            float lo = valores[(int)(k * CORTE_PONTAS)];
            float hi = valores[(int)((k - 1) * (1.0f - CORTE_PONTAS))];
            centro[pc][e] = (lo + hi) * 0.5f;
            tamanho[pc][e] = hi - lo;
        }
    }

    /* Quanto da malha fica fora: caixas atuais x novas. */
    int fora_atual = 0, fora_nova = 0;
    for (int i = 0; i < n; i++) {
        int pc = peca_do_ponto[i];
        if (fora_da_caixa(&pontos[i * 3], p->centro[pc], p->tamanho[pc]) > TOLERANCIA) fora_atual++;
        if (fora_da_caixa(&pontos[i * 3], centro[pc], tamanho[pc]) > TOLERANCIA) fora_nova++;
    }
    printf("personagem: %s   vértices: %d\n", p->nome, n);
    printf("malha fora das caixas (> %.0f cm): atuais %.1f%%   novas %.1f%%\n\n",
           TOLERANCIA * 100.0f, 100.0f * fora_atual / n, 100.0f * fora_nova / n);

    printf("        .centro = {\n");
    for (int pc = 0; pc < PARTE_QTD; pc++)
        printf("            [%-14s] = { %6.3ff, %6.3ff, %6.3ff },\n", NOMES_PECAS[pc],
               centro[pc][0], centro[pc][1], centro[pc][2]);
    printf("        },\n        .tamanho = {\n");
    for (int pc = 0; pc < PARTE_QTD; pc++)
        printf("            [%-14s] = { %5.3ff, %5.3ff, %5.3ff },\n", NOMES_PECAS[pc],
               tamanho[pc][0], tamanho[pc][1], tamanho[pc][2]);
    printf("        },\n");

    free(valores);
    free(peca_do_ponto);
    free(pontos);
    return 0;
}

static void mostrar_esqueleto(const Model *m, const char *arquivo, float altura_desejada)
{
    BoundingBox caixa = GetModelBoundingBox(*m);
    float altura = caixa.max.y - caixa.min.y;
    float escala = (altura > 0.0f) ? altura_desejada / altura : 1.0f;

    printf("arquivo: %s\n", arquivo);
    printf("malhas: %d   ossos: %d\n", m->meshCount, m->skeleton.boneCount);
    printf("altura do modelo: %.3f   escala sugerida (para %.2f m): %.4f\n",
           altura, altura_desejada, escala);
    printf("pés em y = %.3f (deve ser perto de 0)\n", caixa.min.y * escala);
    if (m->skeleton.boneCount == 0)
        printf("ATENÇÃO: o modelo não tem esqueleto (rig). Gere com rig no Meshy.\n");

    printf("\n%-3s %-24s %-4s %8s %8s %8s   (metros, já com a escala)\n",
           "#", "osso", "pai", "x", "y", "z");
    for (int i = 0; i < m->skeleton.boneCount; i++) {
        Vector3 p = m->skeleton.bindPose[i].translation;
        printf("%-3d %-24s %-4d %8.3f %8.3f %8.3f\n", i, m->skeleton.bones[i].name,
               m->skeleton.bones[i].parent, p.x * escala, p.y * escala, p.z * escala);
    }

    int qtd = 0;
    ModelAnimation *anims = LoadModelAnimations(arquivo, &qtd);
    if (qtd > 0) {
        printf("\nanimações:\n");
        for (int i = 0; i < qtd; i++)
            printf("  %-24s %d quadros\n", anims[i].name, anims[i].keyframeCount);
        UnloadModelAnimations(anims, qtd);
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("uso: inspeciona_modelo <arquivo.glb> [altura_desejada_em_metros]\n");
        printf("     inspeciona_modelo <arquivo.glb> --caixas <indice_do_personagem>\n");
        return 1;
    }

    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);       /* carregar modelos exige uma janela */
    InitWindow(100, 100, "inspeciona_modelo");

    Model m = LoadModel(argv[1]);
    int resultado = 0;
    if (m.meshCount == 0) {
        printf("não consegui abrir %s\n", argv[1]);
        resultado = 1;
    } else if (argc > 3 && strcmp(argv[2], "--caixas") == 0) {
        const Personagem *p = personagem_dados(atoi(argv[3]));
        if (p == NULL) {
            printf("personagem %s não existe no catálogo\n", argv[3]);
            resultado = 1;
        } else {
            resultado = medir_caixas(&m, p);
        }
    } else {
        mostrar_esqueleto(&m, argv[1], (argc > 2) ? (float)atof(argv[2]) : 1.7f);
    }

    if (m.meshCount > 0)
        UnloadModel(m);
    CloseWindow();
    return resultado;
}
