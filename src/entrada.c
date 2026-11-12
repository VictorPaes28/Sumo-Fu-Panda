/*
 * entrada.c — controle/teclado -> Intencao.
 * Especificação: entrada.h (tabela de botões e teclas).
 *
 * Dono: Integrante B.
 */
#include <math.h>
#include "entrada.h"
#include "config.h"

typedef struct {
    int cima, baixo, esquerda, direita;
    int a, b, y, firmar, start;
} TeclasJogador;

static const TeclasJogador TECLAS[2] = {
    { KEY_W, KEY_S, KEY_A, KEY_D, KEY_C, KEY_X, KEY_Z, KEY_V, KEY_TAB },
    { KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_K, KEY_J, KEY_U, KEY_L, KEY_ENTER },
};

Intencao entrada_vazia(void)
{
    Intencao in = {0};
    return in;
}

static Intencao ler_controle(int g)
{
    Intencao in = entrada_vazia();
    in.usando_controle = true;

    /* Analógico: zona morta pelo tamanho do vetor; Y invertido (para cima = +). */
    Vector2 v = { GetGamepadAxisMovement(g, GAMEPAD_AXIS_LEFT_X),
                  -GetGamepadAxisMovement(g, GAMEPAD_AXIS_LEFT_Y) };
    float tam = sqrtf(v.x * v.x + v.y * v.y);
    if (tam >= ENTRADA_ZONA_MORTA) {
        if (tam > 1.0f) { v.x /= tam; v.y /= tam; }
        in.mover = v;
    }

    in.direcional_x = IsGamepadButtonPressed(g, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)
                    - IsGamepadButtonPressed(g, GAMEPAD_BUTTON_LEFT_FACE_LEFT);
    in.direcional_y = IsGamepadButtonPressed(g, GAMEPAD_BUTTON_LEFT_FACE_DOWN)
                    - IsGamepadButtonPressed(g, GAMEPAD_BUTTON_LEFT_FACE_UP);
    in.a = IsGamepadButtonPressed(g, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    in.b = IsGamepadButtonPressed(g, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
    in.y = IsGamepadButtonPressed(g, GAMEPAD_BUTTON_RIGHT_FACE_UP);
    in.start = IsGamepadButtonPressed(g, GAMEPAD_BUTTON_MIDDLE_RIGHT);

    /* Gatilho: a raylib vai de -1 (solto) a 1 (apertado); converte para 0 a 1.
     * Alguns controles só informam o gatilho como botão: aceita os dois. */
    float gatilho = (GetGamepadAxisMovement(g, GAMEPAD_AXIS_RIGHT_TRIGGER) + 1.0f) * 0.5f;
    in.firmar = gatilho > ENTRADA_LIMIAR_GATILHO ||
                IsGamepadButtonDown(g, GAMEPAD_BUTTON_RIGHT_TRIGGER_2);
    return in;
}

static Intencao ler_teclado(int jogador)
{
    const TeclasJogador *t = &TECLAS[jogador];
    Intencao in = entrada_vazia();

    Vector2 v = { (float)(IsKeyDown(t->direita) - IsKeyDown(t->esquerda)),
                  (float)(IsKeyDown(t->cima) - IsKeyDown(t->baixo)) };
    float tam = sqrtf(v.x * v.x + v.y * v.y);
    if (tam > 0.0f) { v.x /= tam; v.y /= tam; }    /* diagonal com tamanho 1 */
    in.mover = v;

    in.direcional_x = IsKeyPressed(t->direita) - IsKeyPressed(t->esquerda);
    in.direcional_y = IsKeyPressed(t->baixo) - IsKeyPressed(t->cima);
    in.a = IsKeyPressed(t->a);
    in.b = IsKeyPressed(t->b);
    in.y = IsKeyPressed(t->y);
    in.start = IsKeyPressed(t->start);
    in.firmar = IsKeyDown(t->firmar);
    return in;
}

Intencao entrada_ler(int jogador)
{
    if (jogador < 0 || jogador > 1)
        return entrada_vazia();
    if (IsGamepadAvailable(jogador))
        return ler_controle(jogador);
    return ler_teclado(jogador);
}
