/*
 * recursos.h — carrega e libera tudo o que vem de arquivo: modelos e sons.
 *
 * Arquivos (caminhos relativos à pasta do executável; o CMake copia assets/
 * para lá depois de compilar — use GetApplicationDirectory()):
 *   assets/modelos/<pasta do personagem>/<Personagem.modelo>
 *       ex.: assets/modelos/tai_lung/tai_lung.glb  (um .glb com esqueleto; ver pele.h)
 *   assets/sons/<nome>.wav  (nomes na tabela de SomJogo, em recursos.c)
 *
 * Arquivo faltando NÃO é erro: o jogo precisa rodar antes da arte ficar
 * pronta. Faltou um modelo -> a pele fica vazia (o render desenha caixas).
 * Faltou um som -> tem_som = false (recursos_tocar não faz nada).
 * Cada arquivo faltando gera um aviso com TraceLog(LOG_WARNING, ...).
 *
 * Dono: Integrante B.
 */
#ifndef RECURSOS_H
#define RECURSOS_H

#include <stdbool.h>
#include "raylib.h"
#include "config.h"
#include "pele.h"

typedef enum {
    SOM_GOLPE_LEVE,      /* "golpe_leve.wav"     */
    SOM_GOLPE_FORTE,     /* "golpe_forte.wav"    */
    SOM_QUEDA,           /* "queda.wav"          */
    SOM_CONTAGEM,        /* "contagem.wav"       */
    SOM_LUTE,            /* "lute.wav"           */
    SOM_VITORIA,         /* "vitoria.wav"        */
    SOM_MENU_MOVER,      /* "menu_mover.wav"     */
    SOM_MENU_CONFIRMAR,  /* "menu_confirmar.wav" */
    SOM_QTD
} SomJogo;

typedef struct {
    Shader luz;                      /* iluminação dos modelos (luz.h)       */
    Pele  peles[MAX_PERSONAGENS];    /* modelo de cada personagem (pode estar vazia) */
    Sound sons[SOM_QTD];
    bool  tem_som[SOM_QTD];
} Recursos;

/* Carrega os modelos de todos os personagens do catálogo e todos os sons.
 * Chamar DEPOIS de InitWindow() e InitAudioDevice(). Nunca falha:
 * o que não existir fica marcado como ausente. */
void recursos_carregar(Recursos *r);

/* Toca o som, se ele foi carregado. */
void recursos_tocar(const Recursos *r, SomJogo som);

/* Libera tudo o que foi carregado. Chamar ANTES de CloseAudioDevice()
 * e CloseWindow(). */
void recursos_liberar(Recursos *r);

#endif /* RECURSOS_H */
