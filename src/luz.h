/*
 * luz.h — iluminação simples para os modelos dos personagens.
 *
 * O shader padrão da raylib não tem luz: sem textura, o modelo vira uma
 * silhueta chapada. Este shader acrescenta:
 *   - luz direcional (de cima, da frente-esquerda): lados virados para a luz
 *     ficam claros e os outros escuros;
 *   - luz ambiente de "céu e chão": em cima um pouco mais clara, embaixo mais
 *     escura, para as sombras não ficarem pretas;
 *   - um brilho de contorno fraco, para separar o personagem do fundo.
 * A cor final = textura x tom do material/jogador x luz.
 *
 * Usado só nos modelos (pele). O ringue e as caixas continuam sem luz.
 *
 * Dono: Principal.
 */
#ifndef LUZ_H
#define LUZ_H

#include "raylib.h"

/* Cria o shader (depois de InitWindow). Se falhar, devolve o shader padrão
 * da raylib e o jogo segue sem iluminação. */
Shader luz_criar(void);

/* Atualiza a posição da câmera no shader (para o brilho de contorno).
 * Chamar a cada quadro, antes de desenhar os modelos. */
void luz_atualizar(Shader luz, Vector3 posicao_camera);

void luz_liberar(Shader luz);

#endif /* LUZ_H */
