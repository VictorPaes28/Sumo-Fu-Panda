/*
 * pele.c — modelo 3D com esqueleto que segue o corpo físico.
 * Especificação: pele.h
 *
 * Convenção das matrizes da raylib: MatrixMultiply(a, b) = aplica a, depois b.
 *
 * Dono: Principal.
 */
#include <string.h>
#include "pele.h"
#include "raymath.h"

/* Matriz de um Transform (escala, depois rotação, depois translação). */
static Matrix matriz_de(Transform t)
{
    return MatrixMultiply(MatrixMultiply(MatrixScale(t.scale.x, t.scale.y, t.scale.z),
                                         QuaternionToMatrix(t.rotation)),
                          MatrixTranslate(t.translation.x, t.translation.y, t.translation.z));
}

static int achar_osso(const Model *m, const char *nome)
{
    for (int i = 0; i < m->skeleton.boneCount; i++)
        if (strcmp(m->skeleton.bones[i].name, nome) == 0)
            return i;
    return -1;
}

bool pele_carregar(Pele *p, const char *caminho, const Personagem *per, Shader luz)
{
    *p = (Pele){0};
    if (per->modelo == NULL || !FileExists(caminho))
        return false;

    p->modelo = LoadModel(caminho);
    int n = p->modelo.skeleton.boneCount;
    if (n == 0 || p->modelo.meshCount == 0) {
        TraceLog(LOG_WARNING, "PELE: %s não tem esqueleto; usando caixas", caminho);
        UnloadModel(p->modelo);
        *p = (Pele){0};
        return false;
    }

    p->peca_do_osso = MemAlloc((unsigned)n * sizeof(int));
    p->ajuste = MemAlloc((unsigned)n * sizeof(Matrix));
    for (int i = 0; i < n; i++)
        p->peca_do_osso[i] = -1;

    int ligados = 0;
    for (int k = 0; k < per->qtd_ossos; k++) {
        int i = achar_osso(&p->modelo, per->ossos[k].osso);
        if (i < 0) {
            TraceLog(LOG_WARNING, "PELE: osso \"%s\" não existe em %s", per->ossos[k].osso, caminho);
            continue;
        }
        p->peca_do_osso[i] = per->ossos[k].parte;
        ligados++;
    }
    if (ligados == 0) {
        TraceLog(LOG_WARNING, "PELE: nenhum osso de %s ligado às peças; usando caixas", caminho);
        pele_liberar(p);
        return false;
    }

    /* Ajuste de cada osso, com modelo e corpo na pose de criação. */
    Matrix escala = MatrixScale(per->escala_modelo, per->escala_modelo, per->escala_modelo);
    for (int i = 0; i < n; i++) {
        Matrix osso = MatrixMultiply(matriz_de(p->modelo.skeleton.bindPose[i]), escala);
        int pai = p->modelo.skeleton.bones[i].parent;
        if (p->peca_do_osso[i] < 0 && pai < 0)
            p->peca_do_osso[i] = PARTE_BACIA;              /* raiz sem peça: segue a bacia */

        int peca = p->peca_do_osso[i];
        if (peca >= 0) {
            const float *c = per->centro[peca];
            p->ajuste[i] = MatrixMultiply(osso, MatrixInvert(MatrixTranslate(c[0], c[1], c[2])));
        } else {
            Matrix osso_pai = MatrixMultiply(matriz_de(p->modelo.skeleton.bindPose[pai]), escala);
            p->ajuste[i] = MatrixMultiply(osso, MatrixInvert(osso_pai));
        }
    }

    for (int i = 0; i < p->modelo.materialCount; i++)
        p->modelo.materials[i].shader = luz;

    p->pose.boneCount = n;
    p->pose.keyframeCount = 1;
    p->pose.keyframePoses = MemAlloc(sizeof(ModelAnimPose));
    p->pose.keyframePoses[0] = MemAlloc((unsigned)n * sizeof(Transform));
    p->carregada = true;
    return true;
}

/* Osso i no mundo agora: segue a peça ligada ou, se não tiver, o osso pai. */
static Matrix osso_no_mundo(const Pele *p, const Matrix pecas[PARTE_QTD], int i)
{
    int peca = p->peca_do_osso[i];
    if (peca >= 0)
        return MatrixMultiply(p->ajuste[i], pecas[peca]);
    int pai = p->modelo.skeleton.bones[i].parent;
    return MatrixMultiply(p->ajuste[i], osso_no_mundo(p, pecas, pai));
}

void pele_desenhar(Pele *p, const Matrix pecas[PARTE_QTD], Color tom)
{
    if (!p->carregada)
        return;

    Transform *pose = p->pose.keyframePoses[0];
    for (int i = 0; i < p->pose.boneCount; i++)
        MatrixDecompose(osso_no_mundo(p, pecas, i), &pose[i].translation, &pose[i].rotation,
                        &pose[i].scale);

    /* A pose já está no mundo: o modelo é desenhado sem transformação extra. */
    UpdateModelAnimation(p->modelo, p->pose, 0.0f);
    p->modelo.transform = MatrixIdentity();
    DrawModel(p->modelo, (Vector3){ 0 }, 1.0f, tom);
}

void pele_liberar(Pele *p)
{
    if (p->modelo.meshCount > 0)
        UnloadModel(p->modelo);
    if (p->pose.keyframePoses != NULL) {
        MemFree(p->pose.keyframePoses[0]);
        MemFree(p->pose.keyframePoses);
    }
    MemFree(p->peca_do_osso);
    MemFree(p->ajuste);
    *p = (Pele){0};
}
