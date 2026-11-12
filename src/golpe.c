/*
 * golpe.c — catálogo de golpes (tabela constante).
 * Especificação e fases do golpe: golpe.h. Sinais dos ângulos: esqueleto.h.
 *
 * Dono: Integrante B (tabela, nomes e descrições).
 *       Principal (poses, tempos e forças, ajustados nos testes de física).
 *
 * Como ler as chaves: CHAVE(tempo, cotovelo E, cotovelo D, articulações...).
 * Tudo é DIFERENÇA para a guarda e a base de sumô do personagem (rad):
 *   cintura / pescoço  { + frente,   + esquerda }      (pescoço + = queixo no peito)
 *   ombro              { + levantar, + para a frente }
 *   PERNAS(q, j)       quadris e joelhos juntos: (+x, +x) afunda o corpo sem
 *                      inclinar a bacia; (-x, -x) empurra o chão e estica
 *   PERNA_E / PERNA_D  uma perna só (quadril + = coxa à frente: o pé avança)
 *   cotovelo           + dobra (ART_COTOVELO_E/D, junta física)
 *   PES(x)             tornozelos: - = empurra o chão com a ponta do pé
 * A guarda média é ombro (-0,95, 0,5), cintura 0,14 e cotovelo 0,55: as
 * diferenças abaixo foram pensadas a partir dela.
 *
 * Os movimentos seguem o sumô de verdade (docs/ANIMACOES.md, referências):
 *   - os cotovelos ficam FECHADOS, perto do corpo (como no suriashi): o braço
 *     empurra por baixo e à frente, não abre para o lado;
 *   - o quadril fica BAIXO o golpe inteiro: a força vem das pernas empurrando
 *     o chão para a frente, não de ficar de pé;
 *   - mão e pé do mesmo lado vão juntos (como no treino de teppo);
 *   - ninguém recua o tronco para "carregar": a antecipação é afundar.
 */
#include <stddef.h>
#include "golpe.h"

#define CHAVE(t, cot_e, cot_d, ...)                                           \
    { .tempo = (t), .pose = { .alvo = { [ART_COTOVELO_E] = { (cot_e), 0.0f },  \
                                        [ART_COTOVELO_D] = { (cot_d), 0.0f },  \
                                        __VA_ARGS__ } } }
#define PERNAS(q, j)                                                          \
    [ART_QUADRIL_E] = { (q), 0.0f }, [ART_QUADRIL_D] = { (q), 0.0f },         \
    [ART_JOELHO_E] = { (j), 0.0f }, [ART_JOELHO_D] = { (j), 0.0f }
#define PERNA_E(q, j) [ART_QUADRIL_E] = { (q), 0.0f }, [ART_JOELHO_E] = { (j), 0.0f }
#define PERNA_D(q, j) [ART_QUADRIL_D] = { (q), 0.0f }, [ART_JOELHO_D] = { (j), 0.0f }
#define BRACOS(levantar, frente) \
    [ART_OMBRO_E] = { (levantar), (frente) }, [ART_OMBRO_D] = { (levantar), (frente) }
/* Tornozelos, só com o pé apoiado: - = ponta do pé aperta o chão (empurra). */
#define PES(x) [ART_TORNOZELO_E] = { (x), 0.0f }, [ART_TORNOZELO_D] = { (x), 0.0f }
#define QTD(v) ((int)(sizeof (v) / sizeof (v)[0]))

/* Empurrão = tsuppari: estocadas de palma, uma mão de cada vez.
 *   0,09  afunda, as duas mãos vêm ao peito com os cotovelos fechados;
 *   0,15  estocada da DIREITA na altura do peito, o cotovelo levemente
 *         dobrado para a palma ir ao CENTRO do peito do outro; o pé direito
 *         desliza à frente
 *         enquanto a perna esquerda empurra o chão; 0,20 segura esticada
 *         (o braço físico leva ~50 ms para chegar lá);
 *   0,24  a direita volta ao peito enquanto a esquerda sai;
 *   0,28  estocada da ESQUERDA (o pé esquerdo avança); 0,32 segura;
 *   0,40  as duas recolhem, ainda baixo; 0,50 de volta à guarda. */
static const ChaveGolpe CHAVES_EMPURRAO[] = {
    CHAVE(0.00f, 0.0f, 0.0f, [ART_CINTURA] = { 0.0f, 0.0f }),
    CHAVE(0.09f, 0.95f, 0.95f, [ART_CINTURA] = { 0.08f, 0.0f }, BRACOS(0.10f, 0.45f),
          PERNAS(0.25f, 0.25f)),
    CHAVE(0.15f, 0.95f, -0.50f, [ART_CINTURA] = { 0.18f, 0.06f },
          [ART_OMBRO_E] = { 0.10f, 0.45f }, [ART_OMBRO_D] = { 0.95f, 0.90f },
          PERNA_E(0.00f, 0.05f), PERNA_D(0.45f, 0.25f), PES(-0.15f)),
    CHAVE(0.20f, 0.90f, -0.45f, [ART_CINTURA] = { 0.18f, 0.05f },
          [ART_OMBRO_E] = { 0.10f, 0.45f }, [ART_OMBRO_D] = { 0.90f, 0.88f },
          PERNA_E(0.02f, 0.06f), PERNA_D(0.42f, 0.25f)),
    CHAVE(0.24f, 0.30f, 0.60f, [ART_CINTURA] = { 0.16f, 0.0f },
          [ART_OMBRO_E] = { 0.40f, 0.65f }, [ART_OMBRO_D] = { 0.25f, 0.55f },
          PERNAS(0.25f, 0.22f)),
    CHAVE(0.28f, -0.50f, 0.95f, [ART_CINTURA] = { 0.18f, -0.06f },
          [ART_OMBRO_E] = { 0.95f, 0.90f }, [ART_OMBRO_D] = { 0.10f, 0.45f },
          PERNA_E(0.45f, 0.25f), PERNA_D(0.00f, 0.05f), PES(-0.15f)),
    CHAVE(0.32f, -0.45f, 0.90f, [ART_CINTURA] = { 0.18f, -0.05f },
          [ART_OMBRO_E] = { 0.90f, 0.88f }, [ART_OMBRO_D] = { 0.10f, 0.45f },
          PERNA_E(0.42f, 0.25f), PERNA_D(0.02f, 0.06f)),
    CHAVE(0.40f, 0.30f, 0.30f, [ART_CINTURA] = { 0.12f, 0.0f }, BRACOS(0.40f, 0.60f),
          PERNAS(0.15f, 0.15f)),
    CHAVE(0.50f, 0.0f, 0.0f, [ART_CINTURA] = { 0.0f, 0.0f }),
};

/* Levantador = hazu-oshi: a mão em "forquilha" (entre o polegar e o
 * indicador) por baixo das axilas do outro, empurrando PARA CIMA.
 *   0,14  afunda fundo, tronco curvado, mãos baixas com os cotovelos
 *         dobrados e fechados (as mãos sobem até a altura das axilas dele);
 *   0,24  as pernas esticam e os braços sobem e avançam: o tronco fica de
 *         pé, sem jogar para trás (quem ergue é a perna), queixo no peito;
 *   0,36  passa do ponto, braços quase retos; 0,62 guarda. */
static const ChaveGolpe CHAVES_LEVANTADOR[] = {
    CHAVE(0.00f, 0.0f, 0.0f, [ART_CINTURA] = { 0.0f, 0.0f }),
    CHAVE(0.14f, 0.95f, 0.95f, [ART_CINTURA] = { 0.26f, 0.0f }, [ART_PESCOCO] = { 0.15f, 0.0f },
          BRACOS(-0.05f, 0.25f), PERNAS(0.35f, 0.35f)),
    CHAVE(0.24f, 0.05f, 0.05f, [ART_CINTURA] = { -0.04f, 0.0f }, [ART_PESCOCO] = { 0.10f, 0.0f },
          BRACOS(0.60f, 0.75f), PERNAS(-0.10f, -0.20f), PES(-0.20f)),
    CHAVE(0.36f, -0.15f, -0.15f, [ART_CINTURA] = { -0.06f, 0.0f }, [ART_PESCOCO] = { 0.05f, 0.0f },
          BRACOS(0.70f, 0.80f), PERNAS(-0.05f, -0.15f)),
    CHAVE(0.62f, 0.0f, 0.0f, [ART_CINTURA] = { 0.0f, 0.0f }),
};

/* Tapa lateral = harite: tapa de mão aberta na lateral do rosto, curto e
 * em chicote, com a mão ESQUERDA. A direita fica fechada na frente do peito,
 * protegendo.
 *   0,06  arma: o braço esquerdo vai para o lado e para trás, dobrado, e o
 *         peso carrega a perna esquerda;
 *   0,12  o tapa: o braço passa à frente na altura do rosto, quase reto, e o
 *         corpo passa o peso para a direita;
 *   0,19  continua cruzando à frente do corpo; 0,32 guarda. */
static const ChaveGolpe CHAVES_TAPA_LATERAL[] = {
    CHAVE(0.00f, 0.0f, 0.0f, [ART_CINTURA] = { 0.0f, 0.0f }),
    CHAVE(0.06f, 1.10f, 0.95f, [ART_CINTURA] = { 0.04f, 0.10f },
          [ART_OMBRO_E] = { 1.10f, -0.95f }, [ART_OMBRO_D] = { 0.25f, 0.40f },
          PERNAS(0.10f, 0.10f)),
    CHAVE(0.12f, -0.55f, 0.95f, [ART_CINTURA] = { 0.10f, -0.15f },
          [ART_OMBRO_E] = { 1.20f, 0.80f }, [ART_OMBRO_D] = { 0.25f, 0.40f },
          PERNA_E(0.05f, 0.00f), PERNA_D(0.12f, 0.12f)),
    CHAVE(0.19f, -0.05f, 0.80f, [ART_CINTURA] = { 0.08f, -0.18f },
          [ART_OMBRO_E] = { 0.95f, 1.05f }, [ART_OMBRO_D] = { 0.20f, 0.35f },
          PERNAS(0.06f, 0.06f)),
    CHAVE(0.32f, 0.0f, 0.0f, [ART_CINTURA] = { 0.0f, 0.0f }),
};

/* Barrigada = gaburi-yori: segura o outro pela cintura e o empurra com a
 * barriga, em dois "pulos" do quadril.
 *   0,12  afunda e abraça baixo (mãos na faixa dele, cotovelos dobrados),
 *         tronco curvado sobre ele;
 *   0,24  1º pulo: o quadril vai à frente e o tronco para trás (a barriga
 *         bate), os braços puxam o outro contra a barriga;
 *   0,34  recarrega afundando; 0,44 2º pulo, mais forte; 0,75 guarda. */
static const ChaveGolpe CHAVES_BARRIGADA[] = {
    CHAVE(0.00f, 0.0f, 0.0f, [ART_CINTURA] = { 0.0f, 0.0f }),
    CHAVE(0.12f, 0.85f, 0.85f, [ART_CINTURA] = { 0.16f, 0.0f }, BRACOS(0.30f, 0.45f),
          PERNAS(0.25f, 0.25f)),
    CHAVE(0.24f, 1.25f, 1.25f, [ART_CINTURA] = { -0.44f, 0.0f }, [ART_PESCOCO] = { 0.10f, 0.0f },
          BRACOS(0.25f, 0.35f), PERNAS(-0.20f, -0.05f), PES(-0.15f)),
    CHAVE(0.34f, 0.95f, 0.95f, [ART_CINTURA] = { -0.05f, 0.0f }, BRACOS(0.28f, 0.40f),
          PERNAS(0.15f, 0.15f)),
    CHAVE(0.44f, 1.25f, 1.25f, [ART_CINTURA] = { -0.50f, 0.0f }, [ART_PESCOCO] = { 0.12f, 0.0f },
          BRACOS(0.25f, 0.35f), PERNAS(-0.25f, -0.10f), PES(-0.15f)),
    CHAVE(0.75f, 0.0f, 0.0f, [ART_CINTURA] = { 0.0f, 0.0f }),
};

/* Investida = tachiai: a saída agachada, explodindo para a frente.
 *   0,16  agacha fundo com o tronco à frente, cabeça baixa (queixo no peito,
 *         olhos no outro), mãos baixas perto dos joelhos;
 *   0,26  as pernas esticam empurrando para a FRENTE (não para cima): o
 *         tronco continua baixo, o peito e o ombro batem primeiro, e os
 *         antebraços sobem contra o peito do outro;
 *   0,46  segue empurrando; 0,90 guarda.
 * A cabeça não bate: com ela o choque parecia dois bonecos batendo a
 * cabeça (parte_ataque é só o tronco). */
static const ChaveGolpe CHAVES_INVESTIDA[] = {
    CHAVE(0.00f, 0.0f, 0.0f, [ART_CINTURA] = { 0.0f, 0.0f }),
    CHAVE(0.16f, -0.25f, -0.25f, [ART_CINTURA] = { 0.41f, 0.0f }, [ART_PESCOCO] = { 0.20f, 0.0f },
          BRACOS(-0.30f, -0.15f), PERNAS(0.45f, 0.45f), PES(0.15f)),
    CHAVE(0.26f, 0.65f, 0.65f, [ART_CINTURA] = { 0.41f, 0.0f }, [ART_PESCOCO] = { 0.25f, 0.0f },
          BRACOS(0.50f, 0.70f), PERNAS(-0.10f, -0.30f), PES(-0.30f)),
    CHAVE(0.46f, 0.50f, 0.50f, [ART_CINTURA] = { 0.35f, 0.0f }, [ART_PESCOCO] = { 0.20f, 0.0f },
          BRACOS(0.45f, 0.65f), PERNAS(0.00f, -0.15f)),
    CHAVE(0.90f, 0.0f, 0.0f, [ART_CINTURA] = { 0.0f, 0.0f }),
};

/* Empurrar para fora = oshi-dashi: as duas palmas no peito do outro, ao
 * mesmo tempo, e passos curtos para a frente (suriashi) a cada empurrão.
 *   0,12  afunda, cotovelos bem dobrados e colados ao corpo;
 *   0,22  1º empurrão: os braços esticam juntos, o pé direito avança e a
 *         ponta do pé de trás empurra o chão;
 *   0,32  recolhe um pouco sem subir o quadril;
 *   0,42  2º empurrão, com o pé esquerdo; 0,70 guarda. */
static const ChaveGolpe CHAVES_EMPURRAR_FORA[] = {
    CHAVE(0.00f, 0.0f, 0.0f, [ART_CINTURA] = { 0.0f, 0.0f }),
    CHAVE(0.12f, 0.90f, 0.90f, [ART_CINTURA] = { 0.12f, 0.0f }, BRACOS(0.05f, 0.50f),
          PERNAS(0.30f, 0.30f)),
    CHAVE(0.22f, -0.45f, -0.45f, [ART_CINTURA] = { 0.22f, 0.0f }, BRACOS(0.85f, 0.95f),
          PERNA_E(0.10f, 0.05f), PERNA_D(0.40f, 0.25f), PES(-0.20f)),
    CHAVE(0.32f, 0.25f, 0.25f, [ART_CINTURA] = { 0.20f, 0.0f }, BRACOS(0.60f, 0.80f),
          PERNAS(0.25f, 0.25f)),
    CHAVE(0.42f, -0.45f, -0.45f, [ART_CINTURA] = { 0.24f, 0.0f }, BRACOS(0.90f, 1.00f),
          PERNA_E(0.40f, 0.25f), PERNA_D(0.10f, 0.05f), PES(-0.20f)),
    CHAVE(0.56f, 0.25f, 0.25f, [ART_CINTURA] = { 0.12f, 0.0f }, BRACOS(0.40f, 0.60f),
          PERNAS(0.15f, 0.15f)),
    CHAVE(0.70f, 0.0f, 0.0f, [ART_CINTURA] = { 0.0f, 0.0f }),
};

/* Forçar para fora = yori-kiri: segura a faixa do outro com as duas mãos
 * (cotovelos dobrados, puxando-o contra o peito) e anda com ele para fora,
 * em passos curtos alternados, quadril baixo.
 *   0,12  mãos baixas na faixa; 0,25 / 0,40 / 0,55 três passos; 0,95 guarda. */
static const ChaveGolpe CHAVES_FORCAR_FORA[] = {
    CHAVE(0.00f, 0.0f, 0.0f, [ART_CINTURA] = { 0.0f, 0.0f }),
    CHAVE(0.12f, 1.10f, 1.10f, [ART_CINTURA] = { 0.20f, 0.0f }, BRACOS(0.15f, 0.55f),
          PERNAS(0.30f, 0.30f)),
    CHAVE(0.25f, 1.25f, 1.25f, [ART_CINTURA] = { 0.18f, 0.0f }, BRACOS(0.05f, 0.40f),
          PERNA_E(0.10f, 0.15f), PERNA_D(0.40f, 0.30f), PES(-0.15f)),
    CHAVE(0.40f, 1.25f, 1.25f, [ART_CINTURA] = { 0.18f, 0.0f }, BRACOS(0.05f, 0.40f),
          PERNA_E(0.40f, 0.30f), PERNA_D(0.10f, 0.15f), PES(-0.15f)),
    CHAVE(0.55f, 1.25f, 1.25f, [ART_CINTURA] = { 0.20f, 0.0f }, BRACOS(0.05f, 0.40f),
          PERNA_E(0.10f, 0.15f), PERNA_D(0.40f, 0.30f), PES(-0.15f)),
    CHAVE(0.72f, 0.60f, 0.60f, [ART_CINTURA] = { 0.10f, 0.0f }, BRACOS(0.10f, 0.45f),
          PERNAS(0.15f, 0.15f)),
    CHAVE(0.95f, 0.0f, 0.0f, [ART_CINTURA] = { 0.0f, 0.0f }),
};

/* Projeção por cima = uwate-nage: a mão ESQUERDA pega a faixa por cima do
 * braço do outro; o corpo gira para a esquerda (tronco tomba de lado, perna
 * esquerda recua abrindo caminho) e o braço esquerdo puxa para baixo e para
 * trás, enquanto o direito empurra. O outro passa pelo lado e cai.
 *   0,12  o braço esquerdo passa por cima; 0,20 pega;
 *   0,34  gira e puxa; 0,48 termina o giro; 0,80 guarda. */
static const ChaveGolpe CHAVES_PROJECAO_CIMA[] = {
    CHAVE(0.00f, 0.0f, 0.0f, [ART_CINTURA] = { 0.0f, 0.0f }),
    CHAVE(0.12f, 0.60f, 0.90f, [ART_CINTURA] = { 0.15f, 0.0f },
          [ART_OMBRO_E] = { 0.55f, 0.85f }, [ART_OMBRO_D] = { 0.10f, 0.45f },
          PERNAS(0.25f, 0.25f)),
    CHAVE(0.20f, 1.00f, 1.00f, [ART_CINTURA] = { 0.18f, 0.05f },
          [ART_OMBRO_E] = { 0.40f, 0.70f }, [ART_OMBRO_D] = { 0.10f, 0.50f },
          PERNAS(0.30f, 0.30f)),
    CHAVE(0.34f, 1.30f, 0.50f, [ART_CINTURA] = { 0.25f, 0.30f },
          [ART_OMBRO_E] = { -0.20f, 0.10f }, [ART_OMBRO_D] = { 0.40f, 0.80f },
          PERNA_E(-0.15f, 0.10f), PERNA_D(0.35f, 0.30f), PES(-0.10f)),
    CHAVE(0.48f, 1.20f, 0.30f, [ART_CINTURA] = { 0.22f, 0.35f },
          [ART_OMBRO_E] = { -0.30f, -0.05f }, [ART_OMBRO_D] = { 0.45f, 0.85f },
          PERNA_E(-0.20f, 0.10f), PERNA_D(0.30f, 0.25f)),
    CHAVE(0.80f, 0.0f, 0.0f, [ART_CINTURA] = { 0.0f, 0.0f }),
};

/* Projeção por baixo = shitate-nage: a mão DIREITA pega a faixa por baixo
 * do braço do outro (o braço entra baixo, pela axila); o corpo gira para a
 * direita, a perna direita recua e o braço direito puxa para baixo, por
 * baixo do centro dele: ele perde o apoio e tomba de lado.
 *   0,12  o braço direito entra por baixo; 0,20 pega;
 *   0,34  gira e puxa; 0,48 termina; 0,80 guarda. */
static const ChaveGolpe CHAVES_PROJECAO_BAIXO[] = {
    CHAVE(0.00f, 0.0f, 0.0f, [ART_CINTURA] = { 0.0f, 0.0f }),
    CHAVE(0.12f, 0.90f, 0.70f, [ART_CINTURA] = { 0.20f, 0.0f },
          [ART_OMBRO_E] = { 0.15f, 0.45f }, [ART_OMBRO_D] = { 0.05f, 0.75f },
          PERNAS(0.30f, 0.30f)),
    CHAVE(0.20f, 1.00f, 1.10f, [ART_CINTURA] = { 0.22f, -0.05f },
          [ART_OMBRO_E] = { 0.15f, 0.50f }, [ART_OMBRO_D] = { 0.00f, 0.65f },
          PERNAS(0.32f, 0.32f)),
    CHAVE(0.34f, 0.50f, 1.30f, [ART_CINTURA] = { 0.25f, -0.30f },
          [ART_OMBRO_E] = { 0.40f, 0.80f }, [ART_OMBRO_D] = { -0.35f, 0.10f },
          PERNA_E(0.35f, 0.30f), PERNA_D(-0.15f, 0.10f), PES(-0.10f)),
    CHAVE(0.48f, 0.30f, 1.20f, [ART_CINTURA] = { 0.22f, -0.35f },
          [ART_OMBRO_E] = { 0.45f, 0.85f }, [ART_OMBRO_D] = { -0.40f, -0.05f },
          PERNA_E(0.30f, 0.25f), PERNA_D(-0.20f, 0.10f)),
    CHAVE(0.80f, 0.0f, 0.0f, [ART_CINTURA] = { 0.0f, 0.0f }),
};

/* Tapa para baixo = hataki-komi: o contra-ataque de quem vê o outro vir
 * baixo demais. Recua um passo saindo da frente, levanta as mãos e bate de
 * cima para baixo nas costas e nos ombros dele, que vai de cara no chão.
 *   0,10  recua a perna esquerda e levanta as mãos;
 *   0,18  bate para baixo, o tronco acompanha; 0,30 segue; 0,55 guarda. */
static const ChaveGolpe CHAVES_TAPA_BAIXO[] = {
    CHAVE(0.00f, 0.0f, 0.0f, [ART_CINTURA] = { 0.0f, 0.0f }),
    CHAVE(0.10f, 0.80f, 0.80f, [ART_CINTURA] = { -0.05f, 0.0f }, BRACOS(0.75f, 0.60f),
          PERNA_E(-0.25f, 0.10f), PERNA_D(0.10f, 0.15f)),
    CHAVE(0.18f, 0.10f, 0.10f, [ART_CINTURA] = { 0.35f, 0.10f }, [ART_PESCOCO] = { 0.15f, 0.0f },
          BRACOS(-0.10f, 0.95f), PERNAS(0.25f, 0.25f)),
    CHAVE(0.30f, 0.30f, 0.30f, [ART_CINTURA] = { 0.30f, 0.10f }, BRACOS(-0.20f, 0.80f),
          PERNAS(0.25f, 0.25f)),
    CHAVE(0.55f, 0.0f, 0.0f, [ART_CINTURA] = { 0.0f, 0.0f }),
};

/* Puxar para baixo = hiki-otoshi: segura o outro pelos braços/ombros quando
 * ele empurra, recua de uma vez e o puxa para a frente e para baixo.
 *   0,10  as mãos vão à frente; 0,20 agarra e recua as duas pernas, puxando
 *   os cotovelos para o corpo; 0,34 abaixa o tronco levando-o ao chão;
 *   0,65 guarda. */
static const ChaveGolpe CHAVES_PUXAR_BAIXO[] = {
    CHAVE(0.00f, 0.0f, 0.0f, [ART_CINTURA] = { 0.0f, 0.0f }),
    CHAVE(0.10f, 0.50f, 0.50f, [ART_CINTURA] = { 0.10f, 0.0f }, BRACOS(0.45f, 0.85f),
          PERNAS(0.15f, 0.15f)),
    CHAVE(0.20f, 1.30f, 1.30f, [ART_CINTURA] = { -0.10f, 0.0f }, BRACOS(-0.10f, 0.40f),
          PERNA_E(-0.30f, 0.20f), PERNA_D(-0.20f, 0.25f)),
    CHAVE(0.34f, 1.20f, 1.20f, [ART_CINTURA] = { 0.15f, 0.0f }, BRACOS(-0.35f, 0.25f),
          PERNAS(0.25f, 0.30f)),
    CHAVE(0.65f, 0.0f, 0.0f, [ART_CINTURA] = { 0.0f, 0.0f }),
};

static const Golpe CATALOGO[GOLPE_QTD] = {
    [GOLPE_EMPURRAO] = {
        .nome = "Empurrão",
        .descricao = "Empurra com as duas mãos.",
        .chaves = CHAVES_EMPURRAO, .qtd_chaves = QTD(CHAVES_EMPURRAO),
        .tempo_preparo = 0.12f,
        .tempo_ativo = 0.20f,           /* cobre as duas estocadas */
        .duracao = 0.50f,
        .parte_ataque = PARTE_ANTEBRACO_D,
        .parte_ataque_2 = PARTE_ANTEBRACO_E,
        .avanco = 60.0f,                /* acompanha o empurrão (suriashi) */
        .impulso = 380.0f,              /* medido: com 170 o alvo recuava só 11 cm */
        .impulso_vertical = 0.0f,
        .impulso_lateral = 0.0f,
        .dano = 6.0f,
        .tempo_cura = 3.0f,
        .custo_stamina = 15.0f,
        .forte = false,
        .no_peito = true,
    },
    [GOLPE_LEVANTADOR] = {
        .nome = "Levantador",
        .descricao = "Ergue o adversário por baixo.",
        .chaves = CHAVES_LEVANTADOR, .qtd_chaves = QTD(CHAVES_LEVANTADOR),
        .tempo_preparo = 0.16f,
        .tempo_ativo = 0.20f,
        .duracao = 0.62f,
        .parte_ataque = PARTE_ANTEBRACO_D,
        .parte_ataque_2 = PARTE_ANTEBRACO_E,
        .avanco = 20.0f,
        .impulso = 80.0f,
        .impulso_vertical = 150.0f,
        .impulso_lateral = 0.0f,
        .dano = 8.0f,
        .tempo_cura = 4.0f,
        .custo_stamina = 20.0f,
        .forte = false,
        .no_peito = false,
    },
    [GOLPE_TAPA_LATERAL] = {
        .nome = "Tapa lateral",
        .descricao = "Tapa rápido que desvia para o lado.",
        .chaves = CHAVES_TAPA_LATERAL, .qtd_chaves = QTD(CHAVES_TAPA_LATERAL),
        .tempo_preparo = 0.08f,
        .tempo_ativo = 0.16f,
        .duracao = 0.32f,
        .parte_ataque = PARTE_ANTEBRACO_E,
        .parte_ataque_2 = PARTE_BRACO_E,   /* o braço todo chicoteia */
        .avanco = 0.0f,
        .impulso = 60.0f,
        .impulso_vertical = 0.0f,
        .impulso_lateral = 130.0f,
        .dano = 4.0f,
        .tempo_cura = 2.5f,
        .custo_stamina = 10.0f,
        .forte = false,
        .no_peito = false,
    },
    [GOLPE_BARRIGADA] = {
        .nome = "Barrigada",
        .descricao = "Golpe de barriga, curto e pesado.",
        .chaves = CHAVES_BARRIGADA, .qtd_chaves = QTD(CHAVES_BARRIGADA),
        .tempo_preparo = 0.20f,
        .tempo_ativo = 0.28f,           /* cobre os dois pulos do quadril */
        .duracao = 0.75f,
        .parte_ataque = PARTE_TRONCO,
        .parte_ataque_2 = PARTE_BACIA,
        .avanco = 70.0f,
        .impulso = 310.0f,
        .impulso_vertical = 30.0f,
        .agarra = true,                 /* segura a faixa enquanto empurra */
        .impulso_lateral = 0.0f,
        .dano = 15.0f,
        .tempo_cura = 8.0f,
        .custo_stamina = 30.0f,
        .forte = true,
        .no_peito = true,
    },
    [GOLPE_INVESTIDA] = {
        .nome = "Investida",
        .descricao = "Avança com o corpo todo.",
        .chaves = CHAVES_INVESTIDA, .qtd_chaves = QTD(CHAVES_INVESTIDA),
        .tempo_preparo = 0.22f,
        .tempo_ativo = 0.30f,
        .duracao = 0.90f,
        .parte_ataque = PARTE_TRONCO,
        .parte_ataque_2 = PARTE_ANTEBRACO_D, /* antebraços sobem contra o peito */
        .avanco = 120.0f,
        .impulso = 360.0f,
        .impulso_vertical = 0.0f,
        .impulso_lateral = 0.0f,
        .dano = 12.0f,
        .tempo_cura = 6.0f,
        .custo_stamina = 35.0f,
        .forte = true,
        .no_peito = true,
    },
    [GOLPE_EMPURRAR_FORA] = {
        .nome = "Empurrar fora",
        .descricao = "Duas palmas no peito, avançando.",
        .chaves = CHAVES_EMPURRAR_FORA, .qtd_chaves = QTD(CHAVES_EMPURRAR_FORA),
        .tempo_preparo = 0.16f,
        .tempo_ativo = 0.32f,           /* cobre os dois empurrões */
        .duracao = 0.70f,
        .parte_ataque = PARTE_ANTEBRACO_D,
        .parte_ataque_2 = PARTE_ANTEBRACO_E,
        .avanco = 120.0f,
        .impulso = 380.0f,
        .impulso_vertical = 0.0f,
        .impulso_lateral = 0.0f,
        .dano = 7.0f,
        .tempo_cura = 3.5f,
        .custo_stamina = 22.0f,
        .forte = false,
        .no_peito = true,
    },
    [GOLPE_FORCAR_FORA] = {
        .nome = "Forçar fora",
        .descricao = "Segura a faixa e anda com ele para fora.",
        .chaves = CHAVES_FORCAR_FORA, .qtd_chaves = QTD(CHAVES_FORCAR_FORA),
        .tempo_preparo = 0.12f,
        .tempo_ativo = 0.50f,           /* acerta em qualquer dos passos */
        .duracao = 0.95f,
        .parte_ataque = PARTE_ANTEBRACO_D,
        .parte_ataque_2 = PARTE_ANTEBRACO_E,
        .avanco = 110.0f,
        .impulso = 260.0f,
        .impulso_vertical = 20.0f,
        .impulso_lateral = 0.0f,
        .dano = 9.0f,
        .tempo_cura = 5.0f,
        .custo_stamina = 28.0f,
        .forte = true,
        .no_peito = true,
        .agarra = true,
    },
    [GOLPE_PROJECAO_CIMA] = {
        .nome = "Projeção cima",
        .descricao = "Pega a faixa por cima e joga de lado.",
        .chaves = CHAVES_PROJECAO_CIMA, .qtd_chaves = QTD(CHAVES_PROJECAO_CIMA),
        .tempo_preparo = 0.16f,
        .tempo_ativo = 0.30f,
        .duracao = 0.80f,
        .parte_ataque = PARTE_ANTEBRACO_E,
        .parte_ataque_2 = PARTE_ANTEBRACO_D,
        .avanco = 0.0f,
        .impulso = 60.0f,
        .impulso_vertical = -40.0f,
        .impulso_lateral = -340.0f,     /* para a esquerda de quem joga */
        .derruba = 0.8f,
        .dano = 13.0f,
        .tempo_cura = 6.0f,
        .custo_stamina = 32.0f,
        .forte = true,
        .no_peito = false,
        .agarra = true,
    },
    [GOLPE_PROJECAO_BAIXO] = {
        .nome = "Projeção baixo",
        .descricao = "Pega por baixo do braço e derruba de lado.",
        .chaves = CHAVES_PROJECAO_BAIXO, .qtd_chaves = QTD(CHAVES_PROJECAO_BAIXO),
        .tempo_preparo = 0.16f,
        .tempo_ativo = 0.30f,
        .duracao = 0.80f,
        .parte_ataque = PARTE_ANTEBRACO_D,
        .parte_ataque_2 = PARTE_ANTEBRACO_E,
        .avanco = 0.0f,
        .impulso = 40.0f,
        .impulso_vertical = -20.0f,
        .impulso_lateral = 320.0f,      /* para a direita de quem joga */
        .derruba = 0.8f,
        .dano = 12.0f,
        .tempo_cura = 6.0f,
        .custo_stamina = 30.0f,
        .forte = true,
        .no_peito = false,
        .agarra = true,
    },
    [GOLPE_TAPA_BAIXO] = {
        .nome = "Tapa p/ baixo",
        .descricao = "Contra-ataque: use quando ele vier para cima.",
        .chaves = CHAVES_TAPA_BAIXO, .qtd_chaves = QTD(CHAVES_TAPA_BAIXO),
        .tempo_preparo = 0.12f,
        .tempo_ativo = 0.16f,
        .duracao = 0.55f,
        .parte_ataque = PARTE_ANTEBRACO_D,
        .parte_ataque_2 = PARTE_ANTEBRACO_E,
        .avanco = -40.0f,               /* recua saindo da frente */
        .impulso = -220.0f,             /* puxa para a frente: ele passa direto */
        .impulso_vertical = -320.0f,
        .impulso_lateral = -180.0f,     /* de lado: ele passa pela esquerda */
        .derruba = 0.8f,
        .dano = 8.0f,
        .tempo_cura = 4.0f,
        .custo_stamina = 18.0f,
        .forte = false,
        .no_peito = false,
    },
    [GOLPE_PUXAR_BAIXO] = {
        .nome = "Puxar p/ baixo",
        .descricao = "Contra-ataque: puxa quem empurra para o chão.",
        .chaves = CHAVES_PUXAR_BAIXO, .qtd_chaves = QTD(CHAVES_PUXAR_BAIXO),
        .tempo_preparo = 0.10f,
        .tempo_ativo = 0.22f,
        .duracao = 0.65f,
        .parte_ataque = PARTE_ANTEBRACO_D,
        .parte_ataque_2 = PARTE_ANTEBRACO_E,
        .avanco = -60.0f,               /* recua puxando */
        .impulso = -380.0f,             /* para perto de quem puxa */
        .impulso_vertical = -200.0f,
        .impulso_lateral = 240.0f,      /* de lado: ele passa pela direita */
        .derruba = 0.7f,
        .dano = 9.0f,
        .tempo_cura = 4.5f,
        .custo_stamina = 20.0f,
        .forte = false,
        .no_peito = false,
    },
};

const Golpe *golpe_dados(TipoGolpe tipo)
{
    if ((int)tipo < 0 || tipo >= GOLPE_QTD)
        return NULL;
    return &CATALOGO[tipo];
}

/* Hermite cúbica: passa pelas chaves com inclinação tirada das vizinhas
 * (Catmull-Rom para tempos desiguais). Sai e chega com velocidade zero nas
 * pontas: o golpe começa e termina parado na guarda. */
static float hermite(float p0, float p1, float m0, float m1, float u, float dt)
{
    float u2 = u * u, u3 = u2 * u;
    return (2 * u3 - 3 * u2 + 1) * p0 + (u3 - 2 * u2 + u) * m0 * dt
         + (-2 * u3 + 3 * u2) * p1 + (u3 - u2) * m1 * dt;
}

/* Valor número k de uma chave: articulação k / 2, eixo k % 2. */
static float valor(const ChaveGolpe *c, int k)
{
    return c->pose.alvo[k / 2][k % 2];
}

/* Inclinação (rad/s) do valor k na chave i, pelas vizinhas; zero nas pontas. */
static float inclinacao(const ChaveGolpe *c, int n, int i, int k)
{
    if (i <= 0 || i >= n - 1)
        return 0.0f;
    return (valor(&c[i + 1], k) - valor(&c[i - 1], k)) / (c[i + 1].tempo - c[i - 1].tempo);
}

void golpe_amostrar(const Golpe *g, float t, Pose *delta)
{
    const ChaveGolpe *c = g->chaves;
    int n = g->qtd_chaves;
    int i = 0;                                   /* segmento [i, i+1] que contém t */
    while (i < n - 2 && t > c[i + 1].tempo)
        i++;
    float dt = c[i + 1].tempo - c[i].tempo;
    float u = (dt > 0.0f) ? (t - c[i].tempo) / dt : 1.0f;
    if (u < 0.0f) u = 0.0f;
    if (u > 1.0f) u = 1.0f;

    for (int k = 0; k < 2 * ART_QTD; k++)
        delta->alvo[k / 2][k % 2] = hermite(valor(&c[i], k), valor(&c[i + 1], k),
                                            inclinacao(c, n, i, k), inclinacao(c, n, i + 1, k),
                                            u, dt);
}
