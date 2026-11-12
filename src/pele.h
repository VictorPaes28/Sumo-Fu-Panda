/*
 * pele.h — modelo 3D com esqueleto que "veste" o corpo físico.
 *
 * Técnica usada em jogos como Gang Beasts: a física continua com as 13 peças
 * (invisíveis), e cada osso do modelo segue a peça a que está ligado
 * (Personagem.ossos). A malha é uma só e dobra suave nas juntas, sem frestas.
 *
 * Como funciona:
 *   - ao carregar, para cada osso guarda o "ajuste": onde o osso fica em
 *     relação à sua peça, com o modelo e o corpo os dois na pose de criação
 *     (T-pose, pés na origem, olhando para +Z);
 *   - a cada desenho, osso no mundo = ajuste x transformação atual da peça;
 *     ossos não ligados seguem o osso pai mantendo a mesma posição relativa;
 *   - essa pose é passada à raylib (UpdateModelAnimation), que deforma a malha.
 *
 * Por isso o modelo PRECISA estar na mesma pose de criação da física e as
 * medidas das peças (personagem.c) precisam bater com os ossos.
 *
 * Dono: Principal.
 */
#ifndef PELE_H
#define PELE_H

#include <stdbool.h>
#include "raylib.h"
#include "personagem.h"

typedef struct {
    Model  modelo;
    ModelAnimation pose;      /* uma pose só, recalculada a cada desenho        */
    int   *peca_do_osso;      /* peça que move cada osso, ou -1 = segue o pai   */
    Matrix *ajuste;           /* osso em relação à peça (ou ao pai)             */
    bool   carregada;
} Pele;

/* Carrega o .glb e calcula os ajustes dos ossos para o personagem; todos os
 * materiais do modelo passam a usar o shader 'luz' (luz.h).
 * Retorna false (e deixa a pele vazia) se o arquivo não existir, não tiver
 * esqueleto ou se nenhum osso da lista for encontrado. Chamar depois de
 * InitWindow(). */
bool pele_carregar(Pele *p, const char *caminho, const Personagem *per, Shader luz);

/* Coloca os ossos onde estão as peças e desenha a malha. pecas[i] é a
 * transformação atual da peça i (corpo_transformacao). Recebe só números, e
 * não o Corpo, para este módulo não depender do ODE.
 * Chamar dentro de BeginMode3D. */
void pele_desenhar(Pele *p, const Matrix pecas[PARTE_QTD], Color tom);

void pele_liberar(Pele *p);

#endif /* PELE_H */
