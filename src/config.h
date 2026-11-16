/*
 * config.h — todas as constantes ajustáveis do jogo.
 *
 * Regra: nenhum número "mágico" no código. Se um valor pode precisar de
 * ajuste (força, tempo, tamanho), ele mora aqui, com a unidade no comentário.
 * Cada integrante mexe só nos valores da sua área; mudar o significado de uma
 * constante exige avisar o grupo e atualizar docs/ARQUITETURA.md.
 *
 * Unidades: metros (m), quilogramas (kg), segundos (s), radianos (rad).
 * C puro: este arquivo não inclui raylib nem ODE.
 */
#ifndef CONFIG_H
#define CONFIG_H

/* ---------------------------------------------------------------- Janela */
#define JANELA_LARGURA      1280
#define JANELA_ALTURA       720
#define JANELA_TITULO       "Jogo de Sumo Kung Fu Panda"
#define NOME_JOGO           "SUMÔ-FU PANDA"  /* nome escolhido pelo grupo */
#define JANELA_FPS          60

/* ---------------------------------------------------------------- Física */
#define FISICA_DT                     (1.0f / 120.0f) /* passo fixo da simulação (s) */
#define FISICA_MAX_PASSOS_POR_QUADRO  8        /* limite para o jogo não travar se um quadro demorar */
#define FISICA_GRAVIDADE              (-9.81f) /* (m/s²) */
#define FISICA_ITERACOES              50       /* iterações do dWorldQuickStep */
#define FISICA_ATRITO_PONTA           0.45f    /* atrito da metade da frente do pé: quem empurra
                                                  com a ponta do pé (calcanhar no ar) firma mais */
#define FISICA_ATRITO_PES             0.28f    /* atrito do resto do pé x chão: baixo de
                                                  propósito, para os lutadores deslizarem.
                                                  Com 0,45 os pés prendiam, o corpo girava em
                                                  volta deles e o lutador sentava */
#define FISICA_ATRITO_CORPO_CHAO      1.0f     /* atrito do resto do corpo x chão: quem cai
                                                  arrasta em vez de escorregar */
#define FISICA_ATRITO_CORPOS          0.35f    /* coeficiente de atrito peça x peça; alto (0,6)
                                                  prendia um lutador no outro */
#define FISICA_MAX_CONTATOS           64       /* contatos entre lutadores guardados por passo */
#define FISICA_PONTOS_POR_PAR         8        /* pontos de contato calculados por par de formas */

/* ---------------------------------------------------------------- Corpo */
#define CORPO_TEMPO_QUEDA         0.1f   /* tempo encostando no chão (parte além dos pés) para
                                            valer como queda (s); só filtra um contato de um
                                            passo de física, como no sumô a queda é imediata */
#define CORPO_FORCA_MOTOR         150.0f /* torque máximo base dos motores das articulações (N.m);
                                            cada articulação multiplica por um fator (corpo.c) */
#define MUSCULO_ERRO_CHEIO        0.35f  /* erro da pose (rad) em que o músculo chega à força
                                            máxima: maior = músculo mais macio, cede mais */
#define MUSCULO_VEL_MAX           8.0f   /* velocidade máxima com que um músculo puxa a
                                            junta (rad/s): músculo rápido é mais fraco */
#define MUSCULO_AMORTECIMENTO     0.06f  /* amortecedor do músculo, em segundos de mola:
                                            d = k x isto (s) */
#define CORPO_AMORTECIMENTO_EQUILIBRIO 0.30f /* amortecimento da mola de equilíbrio,
                                            como fração da força por rad/s (s) */
#define CORPO_FORCA_GIRO          260.0f /* torque para virar a bacia na direção do adversário (N.m) */
#define CORPO_AMORTECIMENTO_GIRO  0.25f  /* amortecimento do giro, fração da força por rad/s (s) */

#define PARADO_PERNAS             0.25f  /* fração da animação de parado nas pernas: quem
                                            respira e se mexe parado é a parte de cima */
#define CORPO_SUAVIZACAO_POSE     7.0f   /* rapidez com que a pose muda, com rigidez 1 (1/s);
                                            evita o "teleporte" de uma pose para outra */

/* Rigidez da parte de cima do corpo (multiplica força e rapidez dos motores). */
#define RIGIDEZ_REPOUSO           0.45f  /* parado: braços soltos, corpo "mole" */
#define RIGIDEZ_PREPARO           0.95f
#define RIGIDEZ_ATIVO             1.80f  /* golpe saindo: rápido e firme */
#define RIGIDEZ_VOLTA             0.65f

/* Apoio: a mola de equilíbrio, andar e virar só têm força total com os dois
 * pés no chão e o peso em cima deles, como numa pessoa de verdade. */
#define APOIO_UM_PE               0.85f  /* fração da força com um pé só no chão */
#define APOIO_SEM_PES             0.05f  /* fração da força no ar (sem pés no chão) */
#define APOIO_BASE                0.42f  /* centro de massa até esta distância do meio dos pés:
                                            equilíbrio total (m) */
#define APOIO_LIMITE              1.00f  /* a partir desta distância não há mais como se
                                            salvar: ponto sem volta (m) */

/* Inclinação do corpo (rad, aproximado): para onde a mola de equilíbrio puxa. */
#define INCLINACAO_ANDAR          0.14f  /* inclina para a frente ao andar */
#define INCLINACAO_IMPACTO        0.0f   /* quanto a mola tomba o corpo na direção do golpe.
                                            Era 0,35: junto com a mola fraca, quase todo
                                            golpe terminava com o lutador sentado */

/* ---------------------------------------------------------------- Câmera */
/* Fixa, elevada, olhando o ringue de frente e de cima. Os jogadores ficam
 * à esquerda e à direita da tela. lutador.c usa estes valores para converter
 * o analógico (tela) em direção no chão (mundo). */
#define CAMERA_POS_X              0.0f
#define CAMERA_POS_Y              4.0f
#define CAMERA_POS_Z              5.8f
#define CAMERA_ALVO_X             0.0f
#define CAMERA_ALVO_Y             0.7f
#define CAMERA_ALVO_Z             0.0f
#define CAMERA_FOV                45.0f  /* (graus) */
#define CAMERA_SEGUIR             2.5f   /* rapidez com que a câmera acompanha a luta (1/s) */
#define CAMERA_SEGUIR_MAX         1.6f   /* quanto a câmera pode se afastar do centro (m) */
/* Zoom: a posição CAMERA_POS é a câmera aberta. Com os lutadores perto um do
 * outro, ela chega mais perto (fração da distância); abre aos poucos quando
 * eles se afastam. Só muda a distância, não a direção: o analógico continua
 * valendo o mesmo. */
#define CAMERA_ZOOM_PERTO         0.72f  /* fração da distância com os dois colados (0,55
                                            ficava perto demais) */
#define CAMERA_ZOOM_DIST_PERTO    1.5f   /* lutadores até esta distância: zoom máximo (m) */
#define CAMERA_ZOOM_DIST_LONGE    3.5f   /* a partir desta distância: câmera aberta (m) */
#define CAMERA_ZOOM_RAPIDEZ       0.8f   /* rapidez do zoom (1/s): devagar, para não ficar
                                            "respirando" a cada empurrão */
#define CAMERA_ALVO_Y_PERTO       1.0f   /* com o zoom máximo, mira no peito (não na bacia):
                                            os lutadores descem na tela, longe do HUD (m) */

/* ---------------------------------------------------------------- Luz */
/* Iluminação dos modelos (luz.h). A direção vai da luz para a cena: de cima,
 * vindo da frente (lado da câmera) e da esquerda da tela. */
#define LUZ_DIRECAO_X             0.45f
#define LUZ_DIRECAO_Y             (-1.0f)
#define LUZ_DIRECAO_Z             (-0.55f)
#define LUZ_INTENSIDADE           0.75f  /* força da luz direcional (0 a 1) */
#define LUZ_AMBIENTE              0.45f  /* luz que chega em todo lugar (0 a 1) */

/* ---------------------------------------------------------------- Ringue */
#define RINGUE_RAIO               2.3f   /* (m) — o dohyô real tem 4,55 m de diâmetro. Com
                                            2,7 só 14% dos rounds acabavam fora do ringue */
#define RINGUE_DISTANCIA_INICIAL  1.4f   /* distância entre os lutadores no início do round (m) */

/* ---------------------------------------------------------------- Combo */
#define COMBO_MAX                 4      /* espaços no combo */
#define COMBO_TEMPO_REINICIO      1.5f   /* tempo sem golpear para voltar ao 1º golpe (s) */

/* ---------------------------------------------------------------- Stamina */
#define STAMINA_MAX               100.0f /* (pts) */
#define STAMINA_REGENERACAO       25.0f  /* (pts/s) */
#define AGARRAR_TEMPO_MIN         0.25f  /* a pegada dura pelo menos isto (s) */
#define AGARRAR_TEMPO_MAX         1.2f   /* e no máximo isto: a mão cansa e solta (s) */
#define AGARRAR_FORCA_MAX         900.0f /* acima desta força a mão escapa (N) */
#define CANSACO_FORCA_MINIMA      0.75f  /* força dos músculos com a stamina zerada (0 a 1) */
#define STAMINA_ESPERA            0.6f   /* tempo após um golpe até voltar a regenerar (s) */

/* ---------------------------------------------------------------- Dano */
#define HEMATOMAS_MAX             32     /* hematomas ativos por lutador */
#define DANO_MAX                  100.0f /* dano (%) em que o equilíbrio chega ao mínimo */
#define EQUILIBRIO_MINIMO         0.25f  /* multiplicador da mola com dano máximo (0 a 1) */
#define EQUILIBRIO_FIRMADO        1.6f   /* multiplicador extra da mola ao firmar */
#define FIRMADO_VELOCIDADE        0.35f  /* fração da velocidade ao andar firmado */
#define IMPULSO_POR_DANO          0.015f /* +1,5% de impulso recebido por ponto de dano */
#define DANO_ESCALA_SEGUIDOS      0.6f   /* cada golpe seguido vale isto do anterior
                                            (100%, 60%, 36%, 22%...) */
#define DANO_JANELA_SEGUIDOS      1.5f   /* sem levar golpe por este tempo, a sequência
                                            zera e o próximo vale 100% (s) */
#define FIRMADO_IMPULSO           0.55f  /* fração do impulso recebido quando firmado */

/* ---------------------------------------------------------------- Acerto físico */
#define ACERTO_FORCA              0.95f /* multiplicador geral da força de todos os golpes:
                                            o botão de "dificuldade de derrubar". Era 0,80:
                                            com pouco dano os lutadores ficavam grudados; 0,95
                                            é a força que antes só vinha com ~12% de dano */
#define ACERTO_VEL_REFERENCIA     2.5f   /* velocidade da peça no contato que dá 100% do golpe (m/s).
                                            Medido com robôs: mediana de 1,4 a 4 m/s por golpe */
#define ACERTO_FATOR_MIN          0.45f  /* golpe que mal encosta ainda dá esta fração */
#define ACERTO_FATOR_MAX          1.5f   /* golpe muito rápido dá no máximo esta fração */
#define ACERTO_FRACAO_NO_PONTO    0.15f  /* parte do impulso aplicada no ponto do toque (gira o
                                            corpo); o resto vai no centro da bacia (empurra).
                                            Era 0,6: força alta tombava em vez de empurrar */
#define ACERTO_RECUO              0.35f  /* fração do impulso devolvida a quem bate (3ª lei) */
#define ACERTO_PASSOS             4      /* passos de física em que a força do golpe é
                                            entregue: um tranco num passo só faz as peças
                                            afundarem umas nas outras */

/* ---------------------------------------------------------------- Reação a impacto */
/* Ao levar um choque, as articulações da peça atingida "cedem" (motores mais
 * fracos) e voltam aos poucos: o corpo reage como carne, não como plástico. */
#define REACAO_LIMIAR             0.8f   /* velocidade do choque a partir da qual cede (m/s) */
#define REACAO_FAIXA              3.0f   /* acima do limiar, quanto mais até ceder tudo (m/s) */
#define REACAO_QUEDA_FORCA        0.75f  /* quanto a força do motor cai no máximo (0 a 1) */
#define REACAO_PERNAS             0.15f  /* as pernas cedem menos (era 0,4: os joelhos
                                            dobravam no empurrão e o corpo sentava) */
#define REACAO_RECUPERACAO        0.4f   /* tempo para voltar à força normal (s) */

/* Reflexos de equilíbrio (corpo.c). O desequilíbrio é medido pelo "capture
 * point": onde o corpo pararia se pisasse ali agora = centro de massa +
 * velocidade x sqrt(altura / g). Longe dos pés, o lutador reage como gente:
 * dá um passo para onde está caindo, joga os braços como contrapeso e dobra
 * a cintura para o lado contrário. A velocidade que o jogador pediu é
 * descontada (andar não é cair). */
#define REFLEXO_INICIO            0.12f  /* desequilíbrio a partir do qual reage (m) */
#define REFLEXO_FAIXA             0.30f  /* acima do início, quanto mais até a reação total (m) */
#define REFLEXO_SUAVIZACAO        6.0f  /* rapidez com que a reação aparece e some (1/s) */
#define REFLEXO_GIRO_BRACOS       2.2f   /* braços girando ("moinho") ao cair para trás (voltas/s) */
#define GOLPE_BUFFER              0.25f  /* o aperto do golpe fica guardado este tempo e sai
                                            assim que o lutador puder golpear (s) */
#define GOLPE_REFLEXO_MAX         0.5f   /* com o reflexo de equilíbrio acima disto (0 a 1) o
                                            lutador não começa golpe: recupera primeiro */
#define GOLPE_REFLEXO_PERNAS      0.6f   /* reflexo em que as pernas largam o golpe de vez e
                                            ficam só no equilíbrio (0 a 1) */
#define PASSO_LIMIAR              0.25f  /* reação a partir da qual dá um passo (0 a 1) */
#define REFLEXO_AFUNDAR           0.15f  /* quanto o quadril afunda no reflexo (rad, quadril e
                                            joelho juntos: abaixa sem inclinar a bacia) */
#define REFLEXO_ABRIR             0.15f  /* quanto a base abre no reflexo (rad) */
#define PASSO_DURACAO             0.30f  /* duração do passo de recuperação (s) */
#define PASSO_MAX                 0.60f  /* distância máxima do pé até o quadril no passo (m);
                                            largo: empurrado, recua andando em vez de sentar */
#define PASSO_FORCA               900.0f /* força que leva o pé até o lugar do passo (N/m) */
#define PASSO_LEVANTAR            0.55f  /* fração do peso da perna tirada do chão no passo */
#define PASSO_ESPERA              0.12f  /* intervalo mínimo entre dois passos (s) */
#define PASSO_APOIO_MINIMO        0.3f   /* só dá passo com pelo menos este apoio (0 a 1) */
#define PASSO_PE_ABERTO           0.10f  /* o pé pisa um pouco para fora do ponto, do lado da
                                            própria perna: a base continua larga (m) */
#define PASSO_SOBE_QUADRIL        0.45f  /* no meio do passo, a coxa sobe isto (rad) */
#define PASSO_SOBE_JOELHO         0.8f   /* e o joelho dobra isto: o pé sai do chão (rad) */
#define PASSO_DISTANCIA_ADVERSARIO 0.45f /* o pé não pisa mais perto que isto da bacia
                                            do adversário (m) */

/* Cotovelo (junta física). */
#define COTOVELO_REFLEXO          0.6f   /* no reflexo total os braços abrem: a dobra da
                                            guarda perde esta fração (0 a 1) */

#define TORNOZELO_POSTURA         0.0f   /* tornozelo com o pé apoiado: canela em pé sobre
                                            o pé (rad, + = canela à frente) */
#define TORNOZELO_EQUILIBRIO      1.6f   /* equilíbrio pelo pé: rad de tornozelo por metro de
                                            desequilíbrio para a frente/trás */
#define TORNOZELO_EQUILIBRIO_MAX  0.30f  /* o tornozelo compensa no máximo isto (rad) */
#define MOLA_EQUILIBRIO_FRACAO    1.0f  /* parte da "mola de equilíbrio" que é força de jogo
                                            (de fora do corpo); o resto é dos músculos das
                                            pernas, com a reação nos pés apoiados */

/* Juntas: limites macios (ligamentos) e atrito interno. */
#define JUNTA_LIMITE_ERP          0.25f  /* quanto do excesso no limite é corrigido por passo:
                                            menor = freia mais macio (0 a 1) */
#define JUNTA_LIMITE_CFM          0.002f /* "esponja" do limite: maior = mais macio */
#define JUNTA_AMORTECIMENTO       1.5f   /* atrito interno por fator de força da junta: tira
                                            tremidas e dá peso ao membro solto (N.m.s/rad) */

/* Tônus: força de TODOS os motores (pernas inclusive), tirada do equilíbrio.
 * Quem perdeu o round (equilíbrio 0) desmonta como um boneco de pano; quem está
 * atordoado e machucado fica bambo. Sem isso as pernas seguiam duras na queda. */
#define TONUS_MINIMO              0.12f  /* tônus com equilíbrio 0 (fração da força) */
#define TONUS_EQUILIBRIO_CHEIO    0.25f  /* equilíbrio a partir do qual o tônus é total
                                            (era 0,5: cambaleando, as pernas perdiam força) */

/* ---------------------------------------------------------------- Impacto */
#define ATORDOADO_LEVE            0.30f  /* tempo cambaleando após golpe leve (s) */
#define ATORDOADO_FORTE           0.60f  /* tempo cambaleando após golpe forte (s) */
#define ATORDOADO_EQUILIBRIO      0.85f  /* multiplicador da mola enquanto cambaleia (era 0,45) */
#define PAUSA_IMPACTO_LEVE        0.0f   /* "hitstop": o jogo congela no acerto (s). Com
                                            ~1 acerto por segundo, congelar nos leves
                                            parecia o jogo travando */
#define PAUSA_IMPACTO_FORTE       0.035f
#define TREMOR_LEVE               0.035f /* tremor de câmera no acerto (m) */
#define TREMOR_FORTE              0.09f
#define TREMOR_QUEDA              0.12f
#define TREMOR_DECAIMENTO         6.0f   /* quão rápido o tremor some (1/s) */

/* ---------------------------------------------------------------- Partida */
#define PARTIDA_ROUNDS_PARA_VENCER  2    /* melhor de 3 */
#define PARTIDA_DURACAO_CONTAGEM    3.0f /* "3, 2, 1" antes do round (s) */
#define PARTIDA_DURACAO_LUTE        0.8f /* tempo que "LUTE!" fica na tela (s) */
#define PARTIDA_DURACAO_FIM_ROUND   2.5f /* pausa depois de alguém perder o round (s) */

/* ---------------------------------------------------------------- Entrada */
#define ENTRADA_ZONA_MORTA        0.2f   /* analógico abaixo disto conta como parado (0 a 1) */
#define ENTRADA_SUAVIZACAO        10.0f  /* rapidez com que a direção de andar acompanha o
                                            analógico (1/s): dá peso ao começar e parar */
#define ENTRADA_LIMIAR_GATILHO    0.6f   /* gatilho acima disto conta como segurado (0 a 1); acima
                                            de 0.5 porque alguns controles ficam em 0.5 soltos */

/* ---------------------------------------------------------------- Personagens */
#define MAX_PERSONAGENS           8

/* ---------------------------------------------------------------- Telas */
#define SELECAO_CARTOES_POR_LINHA 4      /* cartões de personagem lado a lado na seleção;
                                            com mais personagens, a grade ganha outra linha */
#define MONTAGEM_GOLPES_VISIVEIS  7      /* golpes da lista que aparecem de uma vez na
                                            montagem; o resto aparece rolando */

#endif /* CONFIG_H */
