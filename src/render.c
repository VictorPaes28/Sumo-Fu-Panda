/*
 * render.c — câmera, ringue e corpos em 3D.
 * Especificação: render.h
 *
 * Dono: Principal.
 */
#include <math.h>
#include "render.h"
#include "config.h"
#include "raymath.h"
#include "rlgl.h"
#include "luz.h"

/* Medidas só de enfeite: a física continua com chão em y = 0 e o limite em
 * RINGUE_RAIO. Por isso o topo da plataforma fica em y = 0 e ela "desce" até
 * o chão do ginásio, e é larga o bastante para quem é jogado para fora não
 * aparecer flutuando durante a pausa do fim do round. */
#define QTD_FARDOS        36       /* fardos de palha (tawara) no círculo */
#define FARDO_RAIO        0.06f    /* raio de um fardo deitado (m) */
#define FARDO_FORA        0.10f    /* os 4 fardos dos pontos cardeais (tokudawara)
                                      ficam um pouco para fora do círculo (m) */
#define PLATAFORMA_TOPO   (RINGUE_RAIO + 1.7f) /* meia largura do topo da plataforma (m) */
#define PLATAFORMA_BASE   (PLATAFORMA_TOPO + 0.5f) /* meia largura da base, no chão (m) */
#define PLATAFORMA_ALTURA 0.20f    /* altura da plataforma (m) */
#define AREIA_LARGURA     0.35f    /* faixa de areia lisa (janome) fora do círculo (m) */
#define LADOS_CIRCULO     72       /* segmentos dos círculos no chão */
#define PLATEIA_INICIO    (PLATAFORMA_BASE + 1.0f) /* distância do centro à 1ª fila (m) */
#define PLATEIA_FILAS     6        /* filas de camarotes (masu-seki) de cada lado */
#define CAMAROTE_LADO     1.3f     /* lado de um camarote quadrado (m) */
#define DEGRAU            0.30f    /* cada fila fica esta altura acima da anterior (m) */
#define OCUPACAO          78       /* % das almofadas com alguém sentado */
#define TORCIDA_DURACAO   1.8f     /* tempo que a plateia pula depois de um round (s) */

static const Color COR_CHAO      = { 64, 46, 36, 255 };
static const Color COR_ARGILA    = { 196, 158, 110, 255 };
static const Color COR_AREIA     = { 218, 190, 142, 255 };
static const Color COR_FARDO     = { 186, 164, 98, 255 };
static const Color COR_FARDO_ESC = { 160, 140, 80, 255 };
static const Color COR_LINHA     = { 240, 236, 226, 255 };
/* Plateia em tons apagados: é fundo, não pode disputar atenção com a luta e o HUD. */
static const Color COR_MADEIRA   = { 84, 60, 42, 255 };
static const Color COR_ALMOFADA[3] = { { 104, 42, 46, 255 }, { 84, 38, 56, 255 },
                                       { 112, 68, 40, 255 } };
static const Color COR_ROUPA[6] = { { 46, 58, 92, 255 }, { 70, 70, 76, 255 },
                                    { 196, 192, 180, 255 }, { 110, 52, 48, 255 },
                                    { 52, 84, 70, 255 }, { 150, 120, 70, 255 } };
static const Color COR_PELE[3]  = { { 224, 186, 150, 255 }, { 190, 146, 108, 255 },
                                    { 140, 98, 70, 255 } };
static const Color COR_CABELO[3] = { { 24, 20, 20, 255 }, { 70, 48, 32, 255 },
                                     { 150, 146, 140, 255 } };
/* Centro do ringue um pouco mais claro: parece um holofote sobre a luta. */
static const Color COR_ARGILA_LUZ = { 222, 186, 136, 255 };
static const Color COR_JOGADOR[2] = { { 235, 120, 40, 255 }, { 60, 150, 235, 255 } };
/* Tom aplicado aos modelos: diferencia os jogadores mesmo com o mesmo personagem. */
static const Color TOM_MODELO[2]  = { { 255, 205, 165, 255 }, { 175, 205, 255, 255 } };
/* Modelo pintado (com textura): só um toque do tom do jogador, para a
 * pintura ficar com as cores dela; o lado de cada um já está no painel. */
static const Color TOM_PINTADO[2] = { { 255, 240, 228, 255 }, { 230, 240, 255, 255 } };

/* O modelo tem textura própria (e não só a textura branca padrão)? */
static bool modelo_pintado(const Pele *pele)
{
    for (int i = 0; i < pele->modelo.materialCount; i++)
        if (pele->modelo.materials[i].maps[MATERIAL_MAP_ALBEDO].texture.id != rlGetTextureIdDefault())
            return true;
    return false;
}

void render_iniciar_camera(CameraLuta *c)
{
    *c = (CameraLuta){0};
    c->camera.position = (Vector3){ CAMERA_POS_X, CAMERA_POS_Y, CAMERA_POS_Z };
    c->camera.target = (Vector3){ CAMERA_ALVO_X, CAMERA_ALVO_Y, CAMERA_ALVO_Z };
    c->camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    c->camera.fovy = CAMERA_FOV;
    c->camera.projection = CAMERA_PERSPECTIVE;
    c->zoom = 1.0f;                 /* começa aberta e se aproxima na contagem */
}

void render_atualizar_camera(CameraLuta *c, const Partida *p, float dt)
{
    /* Foco = meio dos dois lutadores no chão, limitado perto do centro. */
    Vector3 alvo = { 0 };
    if (p->round_ativo) {
        Vector3 a = corpo_posicao(&p->lutadores[0].corpo);
        Vector3 b = corpo_posicao(&p->lutadores[1].corpo);
        alvo = (Vector3){ (a.x + b.x) * 0.5f, 0.0f, (a.z + b.z) * 0.5f };
        float d = Vector3Length(alvo);
        if (d > CAMERA_SEGUIR_MAX)
            alvo = Vector3Scale(alvo, CAMERA_SEGUIR_MAX / d);
    }
    float k = 1.0f - expf(-CAMERA_SEGUIR * dt);
    c->foco = Vector3Lerp(c->foco, alvo, k);

    /* Zoom pela distância entre os lutadores: colados, perto; separados, abre. */
    float zoom_alvo = 1.0f;
    if (p->round_ativo) {
        Vector3 a = corpo_posicao(&p->lutadores[0].corpo);
        Vector3 b = corpo_posicao(&p->lutadores[1].corpo);
        float d = Vector2Distance((Vector2){ a.x, a.z }, (Vector2){ b.x, b.z });
        float t = Clamp((d - CAMERA_ZOOM_DIST_PERTO) / (CAMERA_ZOOM_DIST_LONGE - CAMERA_ZOOM_DIST_PERTO),
                        0.0f, 1.0f);
        zoom_alvo = Lerp(CAMERA_ZOOM_PERTO, 1.0f, t * t * (3.0f - 2.0f * t));
    }
    c->zoom += (zoom_alvo - c->zoom) * (1.0f - expf(-CAMERA_ZOOM_RAPIDEZ * dt));

    /* Tremor: deslocamento aleatório pequeno, proporcional a Partida.tremor. */
    Vector3 tremor = { 0 };
    if (p->tremor > 0.001f) {
        tremor.x = p->tremor * ((float)GetRandomValue(-100, 100) / 100.0f);
        tremor.y = p->tremor * ((float)GetRandomValue(-100, 100) / 100.0f);
    }

    Vector3 base_pos = { CAMERA_POS_X, CAMERA_POS_Y, CAMERA_POS_Z };
    Vector3 base_alvo = { CAMERA_ALVO_X, CAMERA_ALVO_Y, CAMERA_ALVO_Z };
    Vector3 deslocamento = Vector3Add(c->foco, tremor);
    Vector3 de_alvo_a_camera = Vector3Scale(Vector3Subtract(base_pos, base_alvo), c->zoom);
    float perto = Clamp((1.0f - c->zoom) / (1.0f - CAMERA_ZOOM_PERTO), 0.0f, 1.0f);
    base_alvo.y = Lerp(CAMERA_ALVO_Y, CAMERA_ALVO_Y_PERTO, perto);
    c->camera.target = Vector3Add(base_alvo, deslocamento);
    c->camera.position = Vector3Add(c->camera.target, de_alvo_a_camera);
}

/* Caixa (ou arame) do tamanho t, na posição/rotação da matriz m. */
static void desenhar_caixa(Matrix m, const float t[3], Vector3 deslocamento, Color cor, bool arame)
{
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(m));
    if (arame)
        DrawCubeWires(deslocamento, t[0], t[1], t[2], cor);
    else
        DrawCube(deslocamento, t[0], t[1], t[2], cor);
    rlPopMatrix();
}

/* O ringue não usa o shader de luz (luz.h), então o tom de cada face inclinada
 * vem desta conta: a mesma luz difusa dos modelos, relativa ao topo (que fica
 * com a cor original). Assim os lados da plataforma ganham volume. */
static Color tom_da_face(Vector3 normal, Color cor)
{
    Vector3 para_luz = Vector3Negate(Vector3Normalize(
        (Vector3){ LUZ_DIRECAO_X, LUZ_DIRECAO_Y, LUZ_DIRECAO_Z }));
    float topo = LUZ_AMBIENTE + LUZ_INTENSIDADE * fmaxf(para_luz.y, 0.0f);
    float face = LUZ_AMBIENTE + LUZ_INTENSIDADE *
                 fmaxf(Vector3DotProduct(Vector3Normalize(normal), para_luz), 0.0f);
    float f = 0.85f * face / topo;   /* lado de argila batida: um pouco mais escuro */
    return (Color){ (unsigned char)fminf(cor.r * f, 255.0f),
                    (unsigned char)fminf(cor.g * f, 255.0f),
                    (unsigned char)fminf(cor.b * f, 255.0f), cor.a };
}

/* Quadrilátero a-b-c-d (vértices em ordem ao redor). A raylib descarta a face
 * de trás, então a ordem é trocada se for preciso para a face olhar para "fora". */
static void desenhar_face(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Vector3 fora, Color cor)
{
    Vector3 n = Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a));
    if (Vector3DotProduct(n, fora) < 0.0f) {
        Vector3 t = b;
        b = d;
        d = t;
    }
    DrawTriangle3D(a, b, c, cor);
    DrawTriangle3D(a, c, d, cor);
}

/* Faixa circular plana no chão, entre dois raios, na altura y. */
static void desenhar_anel(float raio_dentro, float raio_fora, float y, Color cor)
{
    for (int k = 0; k < LADOS_CIRCULO; k++) {
        float a0 = 2.0f * PI * (float)k / LADOS_CIRCULO;
        float a1 = 2.0f * PI * (float)(k + 1) / LADOS_CIRCULO;
        Vector3 d0 = { cosf(a0), 0.0f, sinf(a0) }, d1 = { cosf(a1), 0.0f, sinf(a1) };
        desenhar_face((Vector3){ d0.x * raio_dentro, y, d0.z * raio_dentro },
                      (Vector3){ d0.x * raio_fora,   y, d0.z * raio_fora },
                      (Vector3){ d1.x * raio_fora,   y, d1.z * raio_fora },
                      (Vector3){ d1.x * raio_dentro, y, d1.z * raio_dentro },
                      (Vector3){ 0.0f, 1.0f, 0.0f }, cor);
    }
}

/* Fardo de palha deitado, meio enterrado, com o centro em (x, z) e
 * comprimento ao longo da direção dir (no chão). */
static void desenhar_fardo(float x, float z, Vector3 dir, float comprimento, Color cor)
{
    Vector3 meio = Vector3Scale(dir, comprimento * 0.5f);
    Vector3 centro = { x, FARDO_RAIO * 0.55f, z };
    Vector3 ini = Vector3Subtract(centro, meio), fim = Vector3Add(centro, meio);
    DrawCylinderEx(ini, fim, FARDO_RAIO, FARDO_RAIO, 8, cor);
}

/* Plataforma de argila elevada (dohyô): topo quadrado em y = 0 e quatro lados
 * inclinados até o chão do ginásio. */
static void desenhar_plataforma(void)
{
    const float t = PLATAFORMA_TOPO, b = PLATAFORMA_BASE, h = -PLATAFORMA_ALTURA;
    const float y = -0.006f;   /* abaixo das faixas do chão, para não piscarem */
    desenhar_face((Vector3){ -t, y, -t }, (Vector3){ t, y, -t }, (Vector3){ t, y, t },
                  (Vector3){ -t, y, t }, (Vector3){ 0.0f, 1.0f, 0.0f }, COR_ARGILA);

    /* Cada lado: (sx, sz) aponta para fora; (px, pz) corre ao longo do lado. */
    static const float LADOS[4][4] = {
        { 0.0f, 1.0f, 1.0f, 0.0f }, { 0.0f, -1.0f, 1.0f, 0.0f },
        { 1.0f, 0.0f, 0.0f, 1.0f }, { -1.0f, 0.0f, 0.0f, 1.0f },
    };
    for (int i = 0; i < 4; i++) {
        float sx = LADOS[i][0], sz = LADOS[i][1], px = LADOS[i][2], pz = LADOS[i][3];
        Vector3 fora = { sx * -h, b - t, sz * -h };   /* normal do lado inclinado */
        desenhar_face((Vector3){ sx * t - px * t, y, sz * t - pz * t },
                      (Vector3){ sx * t + px * t, y, sz * t + pz * t },
                      (Vector3){ sx * b + px * b, h, sz * b + pz * b },
                      (Vector3){ sx * b - px * b, h, sz * b - pz * b },
                      fora, tom_da_face(fora, COR_ARGILA));
    }
}

static Color escurecer(Color c, float f)
{
    return (Color){ (unsigned char)(c.r * f), (unsigned char)(c.g * f),
                    (unsigned char)(c.b * f), c.a };
}

static void vertice(float x, float y, float z) { rlVertex3f(x, y, z); }

/* Caixa barata para a plateia (milhares por quadro). Chamar entre
 * rlBegin(RL_QUADS) e rlEnd(). Desenha só as 3 faces que a câmera pode ver
 * (topo, frente e o lado virado para o centro), sem empilhar matriz, com
 * um tom por face para dar volume. DrawCube custaria o dobro de vértices. */
static void caixa_plateia_topo(Vector3 c, float lx, float ly, float lz, Color cor, Color topo)
{
    float x0 = c.x - lx * 0.5f, x1 = c.x + lx * 0.5f;
    float y0 = c.y - ly * 0.5f, y1 = c.y + ly * 0.5f;
    float z0 = c.z - lz * 0.5f, z1 = c.z + lz * 0.5f;
    rlCheckRenderBatchLimit(12);

    rlColor4ub(topo.r, topo.g, topo.b, topo.a);                      /* topo   */
    vertice(x0, y1, z0); vertice(x0, y1, z1); vertice(x1, y1, z1); vertice(x1, y1, z0);

    Color f = escurecer(cor, 0.78f);                                 /* frente */
    rlColor4ub(f.r, f.g, f.b, f.a);
    vertice(x0, y0, z1); vertice(x1, y0, z1); vertice(x1, y1, z1); vertice(x0, y1, z1);

    Color l = escurecer(cor, 0.62f);                                 /* lado   */
    rlColor4ub(l.r, l.g, l.b, l.a);
    if (c.x > 0.0f) {
        vertice(x0, y0, z0); vertice(x0, y0, z1); vertice(x0, y1, z1); vertice(x0, y1, z0);
    } else {
        vertice(x1, y0, z1); vertice(x1, y0, z0); vertice(x1, y1, z0); vertice(x1, y1, z1);
    }
}

static void caixa_plateia(Vector3 c, float lx, float ly, float lz, Color cor)
{
    caixa_plateia_topo(c, lx, ly, lz, cor, cor);
}

/* Disco no chão com a cor indo de cor_centro (no meio) até cor_borda. */
static void desenhar_disco_gradiente(float raio, float y, Color cor_centro, Color cor_borda)
{
    rlBegin(RL_TRIANGLES);
    for (int k = 0; k < LADOS_CIRCULO; k++) {
        float a0 = 2.0f * PI * (float)k / LADOS_CIRCULO;
        float a1 = 2.0f * PI * (float)(k + 1) / LADOS_CIRCULO;
        /* Ordem centro, a1, a0: face virada para cima (a de baixo é descartada). */
        rlColor4ub(cor_centro.r, cor_centro.g, cor_centro.b, cor_centro.a);
        rlVertex3f(0.0f, y, 0.0f);
        rlColor4ub(cor_borda.r, cor_borda.g, cor_borda.b, cor_borda.a);
        rlVertex3f(cosf(a1) * raio, y, sinf(a1) * raio);
        rlVertex3f(cosf(a0) * raio, y, sinf(a0) * raio);
    }
    rlEnd();
}

/* Espectador sentado numa almofada, com a base em (x, y, z). Tudo nele (cor,
 * se pula, quando) vem do número "h" do lugar, e não de sorteio: o desenho não
 * pode mudar estado, nem o do gerador aleatório. */
static void desenhar_espectador(float x, float y, float z, unsigned h, float animo,
                                double t, float luz)
{
    float fase = (float)(h % 628) / 100.0f;
    /* Balança um pouco o tempo todo; na torcida, pula com os braços para cima. */
    float sobe = 0.012f * sinf((float)t * 2.0f + fase);
    float pulo = animo * 0.10f * fabsf(sinf((float)t * 8.0f + fase));
    y += sobe + pulo;

    Color roupa = escurecer(COR_ROUPA[(h >> 3) % 6], luz);
    Color pele = escurecer(COR_PELE[(h >> 7) % 3], luz);
    Color cabelo = escurecer(COR_CABELO[(h >> 11) % 3], luz);
    caixa_plateia((Vector3){ x, y + 0.20f, z }, 0.32f, 0.40f, 0.24f, roupa);
    /* O cabelo é só o topo da cabeça: de longe não dá para ver a diferença
     * e economiza uma caixa por pessoa. */
    caixa_plateia_topo((Vector3){ x, y + 0.51f, z }, 0.19f, 0.22f, 0.19f, pele, cabelo);
    if (animo > 0.05f) {
        float braco = 0.30f * animo;
        caixa_plateia((Vector3){ x - 0.19f, y + 0.40f + braco * 0.5f, z }, 0.07f, braco, 0.07f, roupa);
        caixa_plateia((Vector3){ x + 0.19f, y + 0.40f + braco * 0.5f, z }, 0.07f, braco, 0.07f, roupa);
    }
}

/* Um camarote aparece na tela? Os de fora do quadro (atrás ou dos lados da
 * câmera) são pulados: são centenas de caixas, e assim a plateia funciona com
 * qualquer posição ou zoom da câmera sem pesar à toa. */
static bool camarote_na_tela(const Camera3D *cam, Vector3 centro)
{
    const float margem = 160.0f;   /* (px) o camarote tem largura; não cortar na borda */
    Vector3 frente = Vector3Subtract(cam->target, cam->position);
    if (Vector3DotProduct(Vector3Subtract(centro, cam->position), frente) <= 0.0f)
        return false;
    Vector2 t = GetWorldToScreen(centro, *cam);
    return t.x > -margem && t.x < GetScreenWidth() + margem &&
           t.y > -margem && t.y < GetScreenHeight() + margem;
}

/* Camarotes da plateia (masu-seki) em degraus, cada um com 4 almofadas e
 * gente sentada. Quanto mais longe, mais escuro: o olho fica no ringue.
 * Só os camarotes que aparecem na tela são desenhados. */
static void desenhar_plateia(const Partida *p, const Camera3D *cam)
{
    const float chao = -PLATAFORMA_ALTURA;
    const float alcance = PLATEIA_INICIO + PLATEIA_FILAS * CAMAROTE_LADO;
    const int n = (int)(2.0f * alcance / CAMAROTE_LADO + 0.5f);
    const double t = GetTime();

    /* A torcida vibra logo depois que alguém perde um round (ou a partida). */
    float animo = 0.0f;
    if (p->estado == PARTIDA_FIM_ROUND || p->estado == PARTIDA_FIM)
        animo = fmaxf(0.0f, 1.0f - p->tempo_no_estado / TORCIDA_DURACAO);

    rlBegin(RL_QUADS);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            float x = -alcance + (i + 0.5f) * CAMAROTE_LADO;
            float z = -alcance + (j + 0.5f) * CAMAROTE_LADO;
            float d = fmaxf(fabsf(x), fabsf(z));
            if (d < PLATEIA_INICIO)
                continue;
            int fila = (int)((d - PLATEIA_INICIO) / CAMAROTE_LADO);
            float topo = chao + fila * DEGRAU + 0.06f;
            if (!camarote_na_tela(cam, (Vector3){ x, topo + 0.3f, z }))
                continue;
            /* Fileiras de trás mais escuras: o olho fica no ringue e o HUD
             * continua legível por cima da plateia. */
            float luz = 1.0f - 0.11f * (float)fila;

            /* Bloco do degrau, do chão até o piso do camarote. */
            float lado = CAMAROTE_LADO * 0.94f;
            caixa_plateia((Vector3){ x, (chao + topo) * 0.5f, z }, lado, topo - chao, lado,
                     escurecer(COR_MADEIRA, luz));
            Color almofada = escurecer(COR_ALMOFADA[(i * 7 + j * 3) % 3], luz);
            for (int q = 0; q < 4; q++) {
                float ox = (q % 2 ? 0.25f : -0.25f) * CAMAROTE_LADO;
                float oz = (q / 2 ? 0.25f : -0.25f) * CAMAROTE_LADO;
                caixa_plateia((Vector3){ x + ox, topo + 0.03f, z + oz }, 0.42f, 0.06f, 0.42f,
                         almofada);
                unsigned h = (unsigned)i * 73856093u ^ (unsigned)j * 19349663u ^
                             (unsigned)q * 83492791u;
                h ^= h >> 13;
                h *= 2654435761u;
                if (h % 100 < OCUPACAO)
                    desenhar_espectador(x + ox, topo + 0.06f, z + oz, h >> 4, animo, t, luz);
            }
        }
    }
    rlEnd();
}

static void desenhar_ringue(const Partida *p, const Camera3D *cam)
{
    DrawPlane((Vector3){ 0.0f, -PLATAFORMA_ALTURA - 0.002f, 0.0f }, (Vector2){ 50.0f, 50.0f },
              COR_CHAO);
    desenhar_plateia(p, cam);
    desenhar_plataforma();
    desenhar_disco_gradiente(PLATAFORMA_TOPO, -0.004f, COR_ARGILA_LUZ, COR_ARGILA);

    /* Areia lisa logo fora do círculo: é onde ficaria a marca do pé de quem saiu. */
    desenhar_anel(RINGUE_RAIO + FARDO_RAIO, RINGUE_RAIO + FARDO_RAIO + AREIA_LARGURA,
                  -0.003f, COR_AREIA);

    /* Círculo de fardos. Os 4 dos pontos cardeais (tokudawara) ficam um pouco
     * para fora, como no dohyô de verdade. */
    float comprimento = 2.0f * PI * RINGUE_RAIO / QTD_FARDOS * 0.94f;
    for (int k = 0; k < QTD_FARDOS; k++) {
        float a = 2.0f * PI * (float)k / QTD_FARDOS;
        bool cardeal = k % (QTD_FARDOS / 4) == 0;
        float r = RINGUE_RAIO + FARDO_RAIO * 0.5f + (cardeal ? FARDO_FORA : 0.0f);
        Vector3 tangente = { -sinf(a), 0.0f, cosf(a) };
        desenhar_fardo(cosf(a) * r, sinf(a) * r, tangente, comprimento,
                       k % 2 ? COR_FARDO : COR_FARDO_ESC);
    }

    /* Quadrado de fardos na beira da plataforma. */
    const float beira = PLATAFORMA_TOPO - 0.15f;
    const int por_lado = 12;
    float pedaco = 2.0f * beira / por_lado;
    for (int k = 0; k < por_lado; k++) {
        float s = -beira + (k + 0.5f) * pedaco;
        Color cor = k % 2 ? COR_FARDO : COR_FARDO_ESC;
        desenhar_fardo(s,  beira, (Vector3){ 1.0f, 0.0f, 0.0f }, pedaco * 0.94f, cor);
        desenhar_fardo(s, -beira, (Vector3){ 1.0f, 0.0f, 0.0f }, pedaco * 0.94f, cor);
        desenhar_fardo( beira, s, (Vector3){ 0.0f, 0.0f, 1.0f }, pedaco * 0.94f, cor);
        desenhar_fardo(-beira, s, (Vector3){ 0.0f, 0.0f, 1.0f }, pedaco * 0.94f, cor);
    }

    /* Linhas de largada (shikiri-sen). */
    DrawCube((Vector3){ -0.35f, 0.002f, 0.0f }, 0.05f, 0.004f, 0.55f, COR_LINHA);
    DrawCube((Vector3){  0.35f, 0.002f, 0.0f }, 0.05f, 0.004f, 0.55f, COR_LINHA);
}

static void desenhar_lutador(const Lutador *l, int indice_personagem, Recursos *r,
                             bool depuracao)
{
    const Corpo *c = &l->corpo;
    const Personagem *p = c->personagem;
    Color cor = COR_JOGADOR[l->indice];

    /* Sombra simples sob a bacia. */
    Vector3 b = corpo_posicao(c);
    DrawCylinder((Vector3){ b.x, 0.003f, b.z }, 0.38f, 0.38f, 0.002f, 24, Fade(BLACK, 0.35f));

    Matrix pecas[PARTE_QTD];
    for (int i = 0; i < PARTE_QTD; i++)
        pecas[i] = corpo_transformacao(c, (ParteCorpo)i);

    /* Com modelo: a malha com esqueleto segue as peças. Sem modelo: caixas. */
    bool tem_pele = indice_personagem >= 0 && indice_personagem < MAX_PERSONAGENS &&
                    r->peles[indice_personagem].carregada;
    if (tem_pele) {
        const Pele *pele = &r->peles[indice_personagem];
        Color tom = modelo_pintado(pele) ? TOM_PINTADO[l->indice] : TOM_MODELO[l->indice];
        pele_desenhar(&r->peles[indice_personagem], pecas, tom);
    } else {
        for (int i = 0; i < PARTE_QTD; i++) {
            desenhar_caixa(pecas[i], p->tamanho[i], (Vector3){ 0 }, cor, false);
            desenhar_caixa(pecas[i], p->tamanho[i], (Vector3){ 0 }, Fade(BLACK, 0.5f), true);
        }
        /* Faixa (mawashi) na cor do jogador. Com modelo, a cor vem do tom. */
        const float *tb = p->tamanho[PARTE_BACIA];
        float faixa[3] = { tb[0] + 0.03f, tb[1] * 0.45f, tb[2] + 0.03f };
        desenhar_caixa(pecas[PARTE_BACIA], faixa, (Vector3){ 0.0f, -tb[1] * 0.2f, 0.0f }, cor, false);
    }
    if (depuracao)
        for (int i = 0; i < PARTE_QTD; i++)
            desenhar_caixa(pecas[i], p->tamanho[i], (Vector3){ 0 }, GREEN, true);
}

void render_cena(const Partida *p, Recursos *r, const CameraLuta *c, bool depuracao)
{
    luz_atualizar(r->luz, c->camera.position);
    BeginMode3D(c->camera);
    desenhar_ringue(p, &c->camera);
    if (p->round_ativo) {
        for (int i = 0; i < 2; i++)
            desenhar_lutador(&p->lutadores[i], p->jogadores[i].personagem, r, depuracao);
    }
    if (depuracao)
        DrawGrid(12, 0.5f);
    EndMode3D();

    if (depuracao && p->round_ativo) {
        DrawText(TextFormat("contatos no passo: %d  relogio: %.1f s", p->fisica.qtd_contatos,
                            p->relogio), 10, JANELA_ALTURA - 60, 20, GREEN);
    }
}
