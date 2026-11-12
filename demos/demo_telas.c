/*
 * demo_telas.c — todas as telas 2D, sem física.
 *
 *   demo_telas             janela: navegue com controle ou teclado
 *                          MENU -> SELEÇÃO -> MONTAGEM -> luta falsa -> RESULTADO
 *   demo_telas --capturas  passa sozinho pelas telas, salva capturas PNG e fecha
 *
 * A "luta falsa" preenche um InfoHud à mão, com valores que mudam com o tempo,
 * para ver o HUD sem rodar a física. Aperte A (ou C/K) para ir ao resultado.
 *
 * Dono: Integrante B.
 */
#include <string.h>
#include <math.h>
#include "raylib.h"
#include "config.h"
#include "entrada.h"
#include "telas.h"
#include "montagem.h"
#include "hud.h"
#include "personagem.h"

typedef enum { D_MENU, D_SELECAO, D_MONTAGEM, D_LUTA, D_RESULTADO } TelaDemo;

/* No modo de capturas, um "roteiro" de apertos simula os jogadores. */
static Intencao roteiro(int quadro, int jogador)
{
    Intencao in = entrada_vazia();
    switch (quadro) {
    case 30:  in.a = (jogador == 0); break;                         /* menu: Jogar      */
    case 70:  in.direcional_x = (jogador == 0) ? 1 : 0; break;      /* seleção          */
    case 100: in.a = true; break;                                   /* os dois prontos  */
    case 130: case 145: case 160: in.a = true; break;               /* montagem: 3 golpes */
    case 175: in.direcional_y = 1; break;
    case 190: in.a = (jogador == 0); break;                         /* J1 completa 4    */
    default: break;
    }
    return in;
}

int main(int argc, char **argv)
{
    bool capturas = (argc > 1 && strcmp(argv[1], "--capturas") == 0);
    InitWindow(JANELA_LARGURA, JANELA_ALTURA, "demo_telas");
    SetExitKey(KEY_NULL);
    SetTargetFPS(JANELA_FPS);

    TelaDemo tela = D_MENU;
    TelaMenu menu;
    TelaSelecao selecao;
    TelaMontagem montagem;
    TelaResultado resultado;
    Combo combos[2];
    combo_iniciar(&combos[0]);
    combo_iniciar(&combos[1]);
    menu_iniciar(&menu);
    float t_luta = 0.0f;
    int quadro = 0;
    bool sair = false;

    while (!WindowShouldClose() && !sair) {
        Intencao in[2];
        for (int i = 0; i < 2; i++)
            in[i] = capturas ? roteiro(quadro, i) : entrada_ler(i);

        switch (tela) {
        case D_MENU: {
            AcaoMenu a = menu_atualizar(&menu, in);
            if (a == MENU_JOGAR) { selecao_iniciar(&selecao); tela = D_SELECAO; }
            if (a == MENU_SAIR) sair = true;
        } break;
        case D_SELECAO: {
            AcaoSelecao a = selecao_atualizar(&selecao, in);
            if (a == SELECAO_CONCLUIDA) { montagem_iniciar(&montagem); tela = D_MONTAGEM; }
            if (a == SELECAO_VOLTAR) { menu_iniciar(&menu); tela = D_MENU; }
        } break;
        case D_MONTAGEM: {
            AcaoMontagem a = montagem_atualizar(&montagem, &combos[0], &combos[1], in);
            if (a == MONTAGEM_CONCLUIDA) { t_luta = 0.0f; tela = D_LUTA; }
            if (a == MONTAGEM_VOLTAR) { selecao_iniciar(&selecao); tela = D_SELECAO; }
        } break;
        case D_LUTA:
            t_luta += GetFrameTime();
            if (in[0].a || in[1].a) { resultado_iniciar(&resultado, 1, "Tai Lung"); tela = D_RESULTADO; }
            break;
        case D_RESULTADO: {
            AcaoResultado a = resultado_atualizar(&resultado, in);
            if (a == RESULTADO_REVANCHE) { t_luta = 0.0f; tela = D_LUTA; }
            if (a == RESULTADO_REMONTAR) { montagem_iniciar(&montagem); tela = D_MONTAGEM; }
            if (a == RESULTADO_MENU) { menu_iniciar(&menu); tela = D_MENU; }
        } break;
        }

        BeginDrawing();
        ClearBackground((Color){ 32, 28, 36, 255 });
        switch (tela) {
        case D_MENU:      menu_desenhar(&menu); break;
        case D_SELECAO:   selecao_desenhar(&selecao); break;
        case D_MONTAGEM:  montagem_desenhar(&montagem, &combos[0], &combos[1]); break;
        case D_RESULTADO: resultado_desenhar(&resultado); break;
        case D_LUTA: {
            InfoHud h = {0};
            h.nome[0] = "Po";
            h.nome[1] = "Tai Lung";
            h.dano[0] = fmodf(t_luta * 12.0f, 120.0f);
            h.dano[1] = fmodf(t_luta * 7.0f, 120.0f);
            h.stamina[0] = 0.5f + 0.5f * sinf(t_luta);
            h.stamina[1] = 0.2f;
            h.vitorias[1] = 1;
            h.rounds_para_vencer = PARTIDA_ROUNDS_PARA_VENCER;
            h.round = 2;
            for (int i = 0; i < 2; i++) {
                h.combo_tamanho[i] = combos[i].tamanho;
                for (int k = 0; k < combos[i].tamanho; k++)
                    h.combo[i][k] = combo_golpe_em(&combos[i], k);
                h.combo_atual[i] = ((int)t_luta) % (combos[i].tamanho > 0 ? combos[i].tamanho : 1);
            }
            h.contagem = (t_luta < 3.0f) ? 3 - (int)t_luta : (t_luta < 3.8f ? 0 : -1);
            h.mensagem = (t_luta > 5.0f && t_luta < 7.0f) ? "FORA!" : NULL;
            hud_desenhar(&h);
        } break;
        }
        EndDrawing();

        if (capturas) {
            if (quadro == 20)  TakeScreenshot("captura_menu.png");
            if (quadro == 90)  TakeScreenshot("captura_selecao.png");
            if (quadro == 200) TakeScreenshot("captura_montagem.png");
            if (quadro == 200) { tela = D_LUTA; t_luta = 1.2f; }
            if (quadro == 230) TakeScreenshot("captura_hud.png");
            if (quadro == 240) { resultado_iniciar(&resultado, 1, "Tai Lung"); tela = D_RESULTADO; }
            if (quadro == 260) { TakeScreenshot("captura_resultado.png"); sair = true; }
        }
        quadro++;
    }

    combo_liberar(&combos[0]);
    combo_liberar(&combos[1]);
    CloseWindow();
    return 0;
}
