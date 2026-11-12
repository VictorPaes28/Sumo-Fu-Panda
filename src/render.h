/*
 * render.h — desenho 3D da luta: câmera, ringue e corpos.
 *
 * Só lê o estado do jogo (Partida const). Cada lutador é desenhado com a
 * pele do personagem (modelo com esqueleto seguindo as peças, pele.h) ou,
 * se o personagem não tiver modelo, com uma caixa por peça física.
 * Modo de depuração (F1 no main.c): desenha também as caixas físicas
 * em arame por cima dos modelos.
 *
 * Câmera: a posição base vem de config.h (CAMERA_*). Ela acompanha o meio
 * da luta devagar (só desliza, nunca gira, para o analógico continuar
 * valendo o mesmo), chega mais perto quando os lutadores estão perto um do
 * outro (CAMERA_ZOOM_*) e treme com Partida.tremor.
 *
 * Dono: Principal.
 */
#ifndef RENDER_H
#define RENDER_H

#include <stdbool.h>
#include "raylib.h"
#include "partida.h"
#include "recursos.h"

typedef struct {
    Camera3D camera;      /* a câmera usada no desenho (com tremor)        */
    Vector3  foco;        /* ponto que a câmera acompanha, sem tremor (m)  */
    float    zoom;        /* fração da distância base (CAMERA_ZOOM_*)      */
} CameraLuta;

/* Câmera na posição base, olhando o centro do ringue. */
void render_iniciar_camera(CameraLuta *c);

/* Uma vez por quadro, antes de desenhar: acompanha a luta e aplica o tremor. */
void render_atualizar_camera(CameraLuta *c, const Partida *p, float dt);

/* Desenha ringue e lutadores. Chamar entre BeginDrawing e EndDrawing;
 * a função chama BeginMode3D/EndMode3D. Recursos não é const porque a pele
 * de cada personagem recalcula a pose dos ossos a cada desenho; o estado do
 * jogo (Partida) continua só sendo lido. */
void render_cena(const Partida *p, Recursos *r, const CameraLuta *c, bool depuracao);

#endif /* RENDER_H */
