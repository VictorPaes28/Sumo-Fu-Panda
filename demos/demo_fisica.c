/*
 * demo_fisica.c — demonstração e verificação do corpo articulado.
 *
 *   demo_fisica           janela com dois corpos e teclas de teste
 *   demo_fisica --teste   verificação sem janela (imprime os resultados;
 *                         código de saída 0 se tudo passar)
 *
 * Teclas na janela:
 *   WASD   anda o corpo 0          ESPACO  empurrão forte no corpo 1
 *   1..5   pose de golpe no corpo 0 (solta: volta à pose neutra)
 *   F      firmar (mola mais forte) R       recomeça
 *
 * Usa só fisica, corpo, personagem, esqueleto e golpe (ver docs/tarefas/TAREFA-PRINCIPAL.md).
 * Dono: Principal.
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "config.h"
#include "fisica.h"
#include "corpo.h"
#include "golpe.h"
#include "personagem.h"

/* ------------------------------------------------------------ Cena */

typedef struct {
    Fisica fisica;
    Corpo corpos[2];
} Cena;

static void cena_criar(Cena *c)
{
    fisica_iniciar(&c->fisica);
    corpo_criar(&c->corpos[0], &c->fisica, personagem_dados(0), 0,
                (Vector3){ -0.7f, 0, 0 }, PI / 2);
    corpo_criar(&c->corpos[1], &c->fisica, personagem_dados(1), 1,
                (Vector3){ 0.7f, 0, 0 }, -PI / 2);
}

static void cena_destruir(Cena *c)
{
    corpo_destruir(&c->corpos[0]);
    corpo_destruir(&c->corpos[1]);
    fisica_liberar(&c->fisica);
}

/* Um passo de física: forças dos dois corpos e passo do mundo. */
static void cena_passo(Cena *c)
{
    corpo_definir_alvo_olhar(&c->corpos[0], corpo_posicao(&c->corpos[1]));
    corpo_definir_alvo_olhar(&c->corpos[1], corpo_posicao(&c->corpos[0]));
    corpo_passo(&c->corpos[0]);
    corpo_passo(&c->corpos[1]);
    fisica_passo(&c->fisica);
}

static void cena_simular(Cena *c, float segundos)
{
    int passos = (int)(segundos / FISICA_DT);
    for (int i = 0; i < passos; i++)
        cena_passo(c);
}

static Vector3 eixo(const Corpo *c, ParteCorpo parte, int k)
{
    Matrix m = corpo_transformacao(c, parte);
    if (k == 0) return (Vector3){ m.m0, m.m1, m.m2 };
    if (k == 1) return (Vector3){ m.m4, m.m5, m.m6 };
    return (Vector3){ m.m8, m.m9, m.m10 };
}

/* ------------------------------------------------------------ Verificação */

static int falhas = 0;

static void confere(int ok, const char *nome, const char *detalhe)
{
    printf("[%s] %s  (%s)\n", ok ? " OK " : "FALHOU", nome, detalhe);
    if (!ok) falhas++;
}

static int verificar(void)
{
    char d[160];
    Cena cena;
    fisica_iniciar_biblioteca();

    /* 1. De pé sozinho por 30 s. */
    cena_criar(&cena);
    float alt_min[2] = { 10, 10 }, alt_max[2] = { 0, 0 };   /* por corpo: os personagens têm alturas diferentes */
    bool caiu = false, invalido = false;
    for (int i = 0; i < (int)(30.0f / FISICA_DT); i++) {
        cena_passo(&cena);
        for (int k = 0; k < 2; k++) {
            float y = corpo_posicao(&cena.corpos[k]).y;
            if (i > 240) { alt_min[k] = fminf(alt_min[k], y); alt_max[k] = fmaxf(alt_max[k], y); }
            caiu |= corpo_caido(&cena.corpos[k]);
            invalido |= !corpo_valido(&cena.corpos[k]);
        }
    }
    snprintf(d, sizeof d, "bacia: corpo 0 entre %.2f e %.2f m, corpo 1 entre %.2f e %.2f m",
             alt_min[0], alt_max[0], alt_min[1], alt_max[1]);
    confere(!caiu && !invalido && alt_min[0] > 0.5f && alt_min[1] > 0.5f &&
            alt_max[0] - alt_min[0] < 0.15f && alt_max[1] - alt_min[1] < 0.15f,
            "fica de pe 30 s sem cair nem tremer", d);
    Vector3 p0 = corpo_posicao(&cena.corpos[0]);
    snprintf(d, sizeof d, "deslocou %.2f m parado", Vector2Length((Vector2){ p0.x + 0.7f, p0.z }));
    confere(Vector2Length((Vector2){ p0.x + 0.7f, p0.z }) < 0.3f, "parado nao escorrega", d);
    Vector3 f0 = corpo_frente(&cena.corpos[0]);
    snprintf(d, sizeof d, "frente = (%.2f, %.2f)", f0.x, f0.z);
    confere(f0.x > 0.9f, "corpo 0 olha para o corpo 1 (+X)", d);
    cena_destruir(&cena);

    /* 2. Anda. */
    cena_criar(&cena);
    cena_simular(&cena, 1.0f);
    Vector3 antes = corpo_posicao(&cena.corpos[0]);
    corpo_definir_movimento(&cena.corpos[0], (Vector2){ 0, 1 }, 1.0f);   /* para +Z */
    cena_simular(&cena, 2.0f);
    Vector3 depois = corpo_posicao(&cena.corpos[0]);
    snprintf(d, sizeof d, "andou %.2f m em Z em 2 s; caido=%d", depois.z - antes.z,
             corpo_caido(&cena.corpos[0]));
    confere(depois.z - antes.z > 1.0f && !corpo_caido(&cena.corpos[0]), "anda para onde mandam", d);
    cena_destruir(&cena);

    /* 3. Cai com um empurrão forte, e com dano (mola fraca) cai mais fácil. */
    cena_criar(&cena);
    cena_simular(&cena, 1.0f);
    corpo_impulso(&cena.corpos[1], (Vector3){ 220, 0, 0 });
    bool derrubou = false;
    Vector3 ini = corpo_posicao(&cena.corpos[1]);
    for (int i = 0; i < (int)(3.0f / FISICA_DT); i++) {
        cena_passo(&cena);
        derrubou |= corpo_caido(&cena.corpos[1]);
    }
    float desloc = corpo_posicao(&cena.corpos[1]).x - ini.x;
    snprintf(d, sizeof d, "impulso 220 N.s: deslocou %.2f m, caiu=%d", desloc, derrubou);
    confere(derrubou || desloc > 0.4f, "empurrao forte derruba ou joga longe", d);
    cena_destruir(&cena);

    cena_criar(&cena);
    cena_simular(&cena, 1.0f);
    corpo_impulso(&cena.corpos[1], (Vector3){ 50, 0, 0 });
    bool caiu_leve = false;
    for (int i = 0; i < (int)(3.0f / FISICA_DT); i++) {
        cena_passo(&cena);
        caiu_leve |= corpo_caido(&cena.corpos[1]);
    }
    confere(!caiu_leve, "empurrao leve sem dano nao derruba", "impulso 50 N.s, equilibrio 1.0");
    cena_destruir(&cena);

    cena_criar(&cena);
    cena_simular(&cena, 1.0f);
    corpo_definir_equilibrio(&cena.corpos[1], EQUILIBRIO_MINIMO);
    cena_simular(&cena, 0.5f);
    corpo_impulso(&cena.corpos[1], (Vector3){ 50, 0, 0 });
    bool caiu_fraco = false;
    for (int i = 0; i < (int)(3.0f / FISICA_DT); i++) {
        cena_passo(&cena);
        caiu_fraco |= corpo_caido(&cena.corpos[1]);
    }
    snprintf(d, sizeof d, "impulso 50 N.s, equilibrio %.2f", EQUILIBRIO_MINIMO);
    confere(caiu_fraco || !caiu_fraco, "(informativo) empurrao leve com dano maximo", d);
    printf("        -> com dano maximo o empurrao leve %s\n", caiu_fraco ? "DERRUBOU" : "nao derrubou");
    cena_destruir(&cena);

    /* 3b. Força no ponto: o mesmo impulso tomba mais quando pega no alto
     *     (perto da cabeça) do que na altura da bacia. */
    float inclinacao_max[2] = { 0, 0 };
    for (int caso = 0; caso < 2; caso++) {
        cena_criar(&cena);
        cena_simular(&cena, 1.0f);
        Vector3 b = corpo_posicao(&cena.corpos[1]);
        Vector3 ponto = { b.x - 0.15f, caso == 0 ? 1.55f : b.y, b.z };
        /* Só a inclinação NA DIREÇÃO do golpe (+X), a partir da postura de
         * antes: o reflexo de dobrar a cintura contra a queda não é tombar. */
        Vector3 cima0 = eixo(&cena.corpos[1], PARTE_TRONCO, 1);
        float incl0 = atan2f(cima0.x, cima0.y);
        corpo_impulso_no_ponto(&cena.corpos[1], PARTE_TRONCO, (Vector3){ 70, 0, 0 }, ponto);
        for (int i = 0; i < (int)(1.5f / FISICA_DT); i++) {
            cena_passo(&cena);
            Vector3 cima = eixo(&cena.corpos[1], PARTE_TRONCO, 1);
            float incl = atan2f(cima.x, cima.y) - incl0;
            inclinacao_max[caso] = fmaxf(inclinacao_max[caso], incl);
        }
        cena_destruir(&cena);
    }
    snprintf(d, sizeof d, "inclinacao maxima: alto %.2f rad, baixo %.2f rad",
             inclinacao_max[0], inclinacao_max[1]);
    confere(inclinacao_max[0] > inclinacao_max[1] * 1.2f, "golpe alto tomba mais que golpe baixo", d);

    /* 3c. Apoio: erguido do chão, o corpo perde o equilíbrio (não se salva no ar). */
    cena_criar(&cena);
    cena_simular(&cena, 1.0f);
    float apoio_parado = cena.corpos[1].apoio;
    corpo_impulso(&cena.corpos[1], (Vector3){ 0, 220, 0 });
    float apoio_min = 1.0f;
    for (int i = 0; i < (int)(0.6f / FISICA_DT); i++) {
        cena_passo(&cena);
        apoio_min = fminf(apoio_min, cena.corpos[1].apoio);
    }
    snprintf(d, sizeof d, "apoio parado %.2f, menor apoio no ar %.2f", apoio_parado, apoio_min);
    confere(apoio_parado > 0.9f && apoio_min < 0.2f, "no ar o corpo perde o apoio", d);
    cena_destruir(&cena);

    /* 4. Sinais das articulações (esqueleto.h). Corpo 0 olha para +X, então
     *    "frente" = +X e "esquerda" = -Z para ele. */
    cena_criar(&cena);
    cena_simular(&cena, 0.5f);
    Pose pose;
    struct { Articulacao art; int eixo_pose; float valor; ParteCorpo parte; int eixo_peca;
             int coord; float sinal; const char *nome; } casos[] = {
        { ART_OMBRO_E,  0,  0.6f, PARTE_BRACO_E, 0, 1,  1, "ombro_e[0]+ levanta o braco esquerdo" },
        { ART_OMBRO_D,  0,  0.6f, PARTE_BRACO_D, 0, 1, -1, "ombro_d[0]+ levanta o braco direito" },
        { ART_OMBRO_E,  1,  1.2f, PARTE_BRACO_E, 0, 0,  1, "ombro_e[1]+ leva o braco esquerdo a frente" },
        { ART_OMBRO_D,  1,  1.2f, PARTE_BRACO_D, 0, 0, -1, "ombro_d[1]+ leva o braco direito a frente" },
        { ART_CINTURA,  0,  0.6f, PARTE_TRONCO,  1, 0,  1, "cintura[0]+ inclina para a frente" },
        { ART_CINTURA,  1,  0.35f, PARTE_TRONCO, 1, 2, -1, "cintura[1]+ inclina para a esquerda" },
        /* Pernas: o corpo ignora a pose e usa a base de sumô (quadril + para a
         * frente e aberto, joelho dobrado). Confere o sinal pela própria base. */
        { ART_QUADRIL_E, 0, 0.0f, PARTE_COXA_E,  1, 0, -1, "quadril_e[0]+ leva a coxa esquerda a frente" },
        { ART_QUADRIL_D, 0, 0.0f, PARTE_COXA_D,  1, 0, -1, "quadril_d[0]+ leva a coxa direita a frente" },
        { ART_QUADRIL_E, 1, 0.0f, PARTE_COXA_E,  1, 2,  1, "quadril_e[1]+ abre a perna esquerda" },
        { ART_QUADRIL_D, 1, 0.0f, PARTE_COXA_D,  1, 2, -1, "quadril_d[1]+ abre a perna direita" },
        /* Joelho: compara a canela com a coxa (canela mais "para trás" que a coxa). */
        { ART_JOELHO_E,  0, 0.0f, PARTE_CANELA_E, 1, 0, 1, "joelho_e[0]+ dobra (canela para tras da coxa)" },
        /* Cotovelo, com o braço na horizontal: o antebraço vai à frente. */
        { ART_COTOVELO_E, 0, 1.2f, PARTE_ANTEBRACO_E, 0, 0,  1, "cotovelo_e[0]+ dobra o antebraco esquerdo a frente" },
        { ART_COTOVELO_D, 0, 1.2f, PARTE_ANTEBRACO_D, 0, 0, -1, "cotovelo_d[0]+ dobra o antebraco direito a frente" },
    };
    corpo_definir_rigidez(&cena.corpos[0], 1.0f);     /* motores na força normal */
    for (size_t k = 0; k < sizeof casos / sizeof casos[0]; k++) {
        corpo_pose_neutra(&cena.corpos[0]);
        pose = cena.corpos[0].pose_alvo;
        pose.alvo[ART_OMBRO_E][0] = pose.alvo[ART_OMBRO_D][0] = 0.0f;   /* braços na horizontal */
        pose.alvo[ART_OMBRO_E][1] = pose.alvo[ART_OMBRO_D][1] = 0.0f;
        pose.alvo[ART_CINTURA][0] = 0.0f;
        pose.alvo[casos[k].art][casos[k].eixo_pose] = casos[k].valor;
        corpo_definir_pose(&cena.corpos[0], &pose);
        cena_simular(&cena, 0.8f);
        Vector3 e = eixo(&cena.corpos[0], casos[k].parte, casos[k].eixo_peca);
        float comp = (casos[k].coord == 0) ? e.x : (casos[k].coord == 1) ? e.y : e.z;
        if (casos[k].art == ART_JOELHO_E)
            comp -= eixo(&cena.corpos[0], PARTE_COXA_E, 1).x;
        comp *= casos[k].sinal;
        snprintf(d, sizeof d, "componente = %.2f", comp);
        confere(comp > (casos[k].art >= ART_QUADRIL_E || casos[k].art == ART_CINTURA ? 0.08f : 0.25f), casos[k].nome, d);
    }
    /* Pés: a sola fica reta no chão (o corpo controla os tornozelos). */
    for (int lado = 0; lado < 2; lado++) {
        float reto = eixo(&cena.corpos[0], lado ? PARTE_PE_D : PARTE_PE_E, 1).y;
        snprintf(d, sizeof d, "eixo de cima do pe: y = %.3f", reto);
        confere(reto > 0.97f, lado ? "pe direito reto no chao" : "pe esquerdo reto no chao", d);
    }
    cena_destruir(&cena);

    /* 5. Contato entre os dois corpos é registrado. */
    cena_criar(&cena);
    cena_simular(&cena, 0.5f);
    corpo_definir_movimento(&cena.corpos[0], (Vector2){ 1, 0 }, 1.0f);
    int com_contato = 0;
    for (int i = 0; i < (int)(2.0f / FISICA_DT); i++) {
        cena_passo(&cena);
        if (cena.fisica.qtd_contatos > 0) com_contato++;
    }
    snprintf(d, sizeof d, "%d passos com contato", com_contato);
    confere(com_contato > 0, "contato entre lutadores registrado", d);
    cena_destruir(&cena);

    fisica_finalizar_biblioteca();
    printf("\n%s (%d falhas)\n", falhas == 0 ? "FISICA OK" : "FISICA COM FALHAS", falhas);
    return falhas == 0 ? 0 : 1;
}

/* ------------------------------------------------------------ Janela */

static void desenhar_corpo(const Corpo *c, Color cor)
{
    for (int i = 0; i < PARTE_QTD; i++) {
        const float *t = c->personagem->tamanho[i];
        Matrix m = corpo_transformacao(c, (ParteCorpo)i);
        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(m));
        DrawCube((Vector3){ 0 }, t[0], t[1], t[2], cor);
        DrawCubeWires((Vector3){ 0 }, t[0], t[1], t[2], BLACK);
        rlPopMatrix();
    }
}

static int janela(void)
{
    InitWindow(JANELA_LARGURA, JANELA_ALTURA, "demo_fisica");
    SetTargetFPS(JANELA_FPS);
    fisica_iniciar_biblioteca();

    Camera3D cam = { { CAMERA_POS_X, CAMERA_POS_Y, CAMERA_POS_Z },
                     { CAMERA_ALVO_X, CAMERA_ALVO_Y, CAMERA_ALVO_Z },
                     { 0, 1, 0 }, CAMERA_FOV, CAMERA_PERSPECTIVE };
    static Cena cena;
    cena_criar(&cena);
    float acumulador = 0.0f;
    int contatos = 0;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_R)) { cena_destruir(&cena); cena_criar(&cena); }

        Vector2 dir = { (float)(IsKeyDown(KEY_D) - IsKeyDown(KEY_A)),
                        (float)(IsKeyDown(KEY_S) - IsKeyDown(KEY_W)) };
        if (Vector2Length(dir) > 0) dir = Vector2Normalize(dir);
        bool firmar = IsKeyDown(KEY_F);
        corpo_definir_movimento(&cena.corpos[0], dir, firmar ? FIRMADO_VELOCIDADE : 1.0f);
        corpo_definir_equilibrio(&cena.corpos[0], firmar ? EQUILIBRIO_FIRMADO : 1.0f);

        int golpe = -1;
        for (int k = 0; k < GOLPE_QTD; k++)
            if (IsKeyDown(KEY_ONE + k)) golpe = k;
        corpo_pose_neutra(&cena.corpos[0]);
        if (golpe >= 0) {
            /* Segura o golpe no momento do acerto (fim do preparo). */
            const Golpe *g = golpe_dados((TipoGolpe)golpe);
            Pose delta;
            golpe_amostrar(g, g->tempo_preparo + g->tempo_ativo * 0.5f, &delta);
            corpo_definir_movimento_extra(&cena.corpos[0], &delta);
        } else {
            corpo_definir_movimento_extra(&cena.corpos[0], NULL);
        }
        if (IsKeyPressed(KEY_SPACE))
            corpo_impulso(&cena.corpos[1], (Vector3){ 120, 0, 0 });

        acumulador += GetFrameTime();
        for (int n = 0; acumulador >= FISICA_DT && n < FISICA_MAX_PASSOS_POR_QUADRO; n++) {
            cena_passo(&cena);
            contatos = cena.fisica.qtd_contatos;
            acumulador -= FISICA_DT;
        }
        if (acumulador > FISICA_DT) acumulador = 0.0f;

        BeginDrawing();
        ClearBackground((Color){ 32, 28, 36, 255 });
        BeginMode3D(cam);
        DrawPlane((Vector3){ 0 }, (Vector2){ 12, 12 }, (Color){ 120, 100, 80, 255 });
        DrawCylinder((Vector3){ 0, 0.0f, 0 }, RINGUE_RAIO, RINGUE_RAIO, 0.01f, 48, (Color){ 210, 180, 130, 255 });
        desenhar_corpo(&cena.corpos[0], ORANGE);
        desenhar_corpo(&cena.corpos[1], SKYBLUE);
        EndMode3D();
        DrawText("WASD anda | 1-5 golpes | F firmar | ESPACO empurra o azul | R recomeca", 10, 10, 20, RAYWHITE);
        DrawText(TextFormat("contatos: %d  caido: %d / %d", contatos,
                            corpo_caido(&cena.corpos[0]), corpo_caido(&cena.corpos[1])), 10, 40, 20, RAYWHITE);
        DrawFPS(10, JANELA_ALTURA - 30);
        EndDrawing();
    }

    cena_destruir(&cena);
    fisica_finalizar_biblioteca();
    CloseWindow();
    return 0;
}

int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "--teste") == 0)
        return verificar();
    return janela();
}
