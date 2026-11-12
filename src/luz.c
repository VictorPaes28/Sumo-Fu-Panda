/*
 * luz.c — shader de iluminação simples (GLSL 330, OpenGL 3.3 da raylib).
 * Especificação: luz.h
 *
 * Dono: Principal.
 */
#include <stddef.h>
#include "luz.h"
#include "config.h"
#include "raymath.h"

static const char *const VERTICE =
    "#version 330\n"
    "in vec3 vertexPosition;\n"
    "in vec2 vertexTexCoord;\n"
    "in vec3 vertexNormal;\n"
    "in vec4 vertexColor;\n"
    "uniform mat4 mvp;\n"
    "uniform mat4 matModel;\n"
    "uniform mat4 matNormal;\n"
    "out vec3 posicao;\n"
    "out vec3 normal;\n"
    "out vec2 coord;\n"
    "out vec4 cor;\n"
    "void main() {\n"
    "    posicao = vec3(matModel * vec4(vertexPosition, 1.0));\n"
    "    normal = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));\n"
    "    coord = vertexTexCoord;\n"
    "    cor = vertexColor;\n"
    "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
    "}\n";

static const char *const FRAGMENTO =
    "#version 330\n"
    "in vec3 posicao;\n"
    "in vec3 normal;\n"
    "in vec2 coord;\n"
    "in vec4 cor;\n"
    "uniform sampler2D texture0;\n"
    "uniform vec4 colDiffuse;\n"
    "uniform vec3 direcaoLuz;\n"
    "uniform vec3 posicaoCamera;\n"
    "uniform float intensidadeLuz;\n"
    "uniform float intensidadeAmbiente;\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    vec4 base = texture(texture0, coord) * colDiffuse * cor;\n"
    "    vec3 n = normalize(normal);\n"
    "    float difusa = max(dot(n, -direcaoLuz), 0.0) * intensidadeLuz;\n"
    "    float ceu = 0.5 + 0.5 * n.y;\n"
    "    float ambiente = intensidadeAmbiente * mix(0.55, 1.0, ceu);\n"
    "    vec3 olhar = normalize(posicaoCamera - posicao);\n"
    "    float contorno = pow(1.0 - max(dot(n, olhar), 0.0), 3.0) * 0.25;\n"
    "    vec3 rgb = base.rgb * (ambiente + difusa) + contorno;\n"
    "    finalColor = vec4(rgb, base.a);\n"
    "}\n";

Shader luz_criar(void)
{
    Shader s = LoadShaderFromMemory(VERTICE, FRAGMENTO);
    if (!IsShaderValid(s)) {
        TraceLog(LOG_WARNING, "LUZ: shader não compilou; modelos sem iluminação");
        return LoadShaderFromMemory(NULL, NULL);
    }
    Vector3 direcao = Vector3Normalize((Vector3){ LUZ_DIRECAO_X, LUZ_DIRECAO_Y, LUZ_DIRECAO_Z });
    float intensidade = LUZ_INTENSIDADE, ambiente = LUZ_AMBIENTE;
    SetShaderValue(s, GetShaderLocation(s, "direcaoLuz"), &direcao, SHADER_UNIFORM_VEC3);
    SetShaderValue(s, GetShaderLocation(s, "intensidadeLuz"), &intensidade, SHADER_UNIFORM_FLOAT);
    SetShaderValue(s, GetShaderLocation(s, "intensidadeAmbiente"), &ambiente, SHADER_UNIFORM_FLOAT);
    return s;
}

void luz_atualizar(Shader luz, Vector3 posicao_camera)
{
    SetShaderValue(luz, GetShaderLocation(luz, "posicaoCamera"), &posicao_camera,
                   SHADER_UNIFORM_VEC3);
}

void luz_liberar(Shader luz)
{
    UnloadShader(luz);
}
