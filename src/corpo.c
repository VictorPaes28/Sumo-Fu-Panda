/*
 * corpo.c — corpo articulado: peças, articulações, motores e equilíbrio.
 * Especificação: corpo.h. Sinais dos ângulos: esqueleto.h.
 *
 * Como o corpo fica de pé: as pernas seguram o peso com os motores dos
 * quadris e joelhos (base de sumô), e uma "mola de equilíbrio" aplica um
 * torque no tronco e na bacia puxando-os para a vertical. É uma força de
 * jogo (não existe no corpo humano), a mesma ideia usada em Gang Beasts.
 * Com dano, a mola enfraquece e o lutador cambaleia.
 *
 * Dono: Principal.
 */
#include <math.h>
#include "corpo.h"
#include "config.h"
#include "raymath.h"
#include "animacao.h"

/* ------------------------------------------------------------ Tabelas */

/* Limites (rad) de cada eixo, fator de força do músculo (x CORPO_FORCA_MOTOR)
 * e maciez (x MUSCULO_ERRO_CHEIO; maior = cede mais antes da força máxima).
 * Juntas universais do ODE ficam instáveis perto de ±90°: limites < 1,45. */
typedef struct {
    float min[2];
    float max[2];
    float forca;
    float maciez;
} LimiteArticulacao;

static const LimiteArticulacao LIMITES[ART_QTD] = {
    [ART_CINTURA]   = { { -0.50f, -0.40f }, { 0.80f, 0.40f }, 2.5f, 1.5f },
    [ART_PESCOCO]   = { { -0.50f, -0.40f }, { 0.50f, 0.40f }, 0.3f, 1.8f },
    [ART_OMBRO_E]   = { { -1.40f, -0.80f }, { 1.20f, 1.45f }, 0.5f, 1.0f },
    [ART_OMBRO_D]   = { { -1.40f, -0.80f }, { 1.20f, 1.45f }, 0.5f, 1.0f },
    [ART_QUADRIL_E] = { { -0.80f, -0.30f }, { 1.40f, 0.70f }, 2.0f, 0.3f },
    [ART_QUADRIL_D] = { { -0.80f, -0.30f }, { 1.40f, 0.70f }, 2.0f, 0.3f },
    [ART_JOELHO_E]  = { {  0.00f,  0.00f }, { 2.20f, 0.00f }, 2.0f, 0.3f },
    [ART_JOELHO_D]  = { {  0.00f,  0.00f }, { 2.20f, 0.00f }, 2.0f, 0.3f },
    [ART_COTOVELO_E]  = { { 0.00f, 0.00f }, { 2.40f, 0.00f }, 0.35f, 1.0f },
    [ART_COTOVELO_D]  = { { 0.00f, 0.00f }, { 2.40f, 0.00f }, 0.35f, 1.0f },
    [ART_TORNOZELO_E] = { { -0.60f, -0.35f }, { 0.60f, 0.35f }, 1.2f, 1.0f },
    [ART_TORNOZELO_D] = { { -0.60f, -0.35f }, { 0.60f, 0.35f }, 1.2f, 1.0f },
};

/* Braços e pernas são cápsulas: escorregam uns nos outros em vez de
 * prender pela quina, e o corpo rola ao cair. Tronco, bacia, cabeça e pés
 * (sola reta no chão) continuam caixas. */
static bool eh_capsula(int parte)
{
    switch (parte) {
    case PARTE_BRACO_E: case PARTE_BRACO_D: case PARTE_ANTEBRACO_E: case PARTE_ANTEBRACO_D:
    case PARTE_COXA_E: case PARTE_COXA_D: case PARTE_CANELA_E: case PARTE_CANELA_D:
        return true;
    default:
        return false;
    }
}

/* Eixo (0 = X, 1 = Y, 2 = Z) da maior medida da caixa. */
static int eixo_comprido(const float t[3])
{
    int e = 0;
    for (int k = 1; k < 3; k++)
        if (t[k] > t[e])
            e = k;
    return e;
}

/* Cápsula do tamanho da caixa: raio pela média das duas medidas finas e o
 * comprimento total igual ao da caixa. */
static void medidas_da_capsula(const float t[3], float *raio, float *comprimento)
{
    int e = eixo_comprido(t);
    *raio = 0.25f * (t[(e + 1) % 3] + t[(e + 2) % 3]);
    *comprimento = fmaxf(t[e] - 2.0f * *raio, 0.01f);
}

/* Reação de braços, cintura e cabeça à queda (reflexo_dos_bracos), em rad
 * por unidade de reflexo. Caindo para trás os braços vão à frente e sobem;
 * para a frente, vão à frente para se proteger; de lado, abrem. */
static const struct {
    float ombro_tras[2];      /* { levantar, frente } caindo para trás          */
    float ombro_frente[2];    /* { levantar, frente } caindo para a frente      */
    float giro;               /* amplitude do "moinho" dos braços, para trás    */
    float abre_queda;         /* braço do lado da queda abre para se segurar    */
    float abre_contrapeso;    /* braço do outro lado sobe mais, de contrapeso   */
    float cintura[3];         /* { para trás, para a frente, de lado }          */
    float pescoco[3];         /* { para trás, para a frente, de lado }          */
} REACAO_BRACOS = {
    .ombro_tras = { 0.70f, 1.10f },
    .ombro_frente = { 0.35f, 1.00f },
    .giro = 0.45f,
    .abre_queda = 0.90f,
    .abre_contrapeso = 1.10f,
    .cintura = { 0.35f, 0.15f, 0.30f },
    .pescoco = { 0.35f, 0.25f, 0.35f },
};

/* Articulações da parte de cima (seguem a pose e a rigidez). */
static bool eh_parte_de_cima(int a)
{
    return a == ART_CINTURA || a == ART_PESCOCO || a == ART_OMBRO_E || a == ART_OMBRO_D
        || a == ART_COTOVELO_E || a == ART_COTOVELO_D;
}

/* Articulações das pernas (o choque nelas cede menos: REACAO_PERNAS). */
static bool eh_perna(int a)
{
    return a == ART_QUADRIL_E || a == ART_QUADRIL_D || a == ART_JOELHO_E || a == ART_JOELHO_D
        || a == ART_TORNOZELO_E || a == ART_TORNOZELO_D;
}

/* ------------------------------------------------------------ Vetores */

static Vector3 girar_y(const float v[3], float angulo)
{
    float c = cosf(angulo), s = sinf(angulo);
    return (Vector3){ c * v[0] + s * v[2], v[1], -s * v[0] + c * v[2] };
}

static Vector3 vetor_ode(const dReal *v)
{
    return (Vector3){ (float)v[0], (float)v[1], (float)v[2] };
}

/* Eixo local da peça (0 = X, 1 = Y, 2 = Z) no sistema do mundo. A matriz do
 * ODE é 3x4 por linhas: a coluna k é a imagem do eixo local k. */
static Vector3 eixo_da_peca(dBodyID b, int k)
{
    const dReal *R = dBodyGetRotation(b);
    return (Vector3){ (float)R[k], (float)R[4 + k], (float)R[8 + k] };
}

static void somar_torque(dBodyID b, Vector3 t)
{
    dBodyAddTorque(b, t.x, t.y, t.z);
}

/* ------------------------------------------------------------ Criação */

bool corpo_criar(Corpo *c, Fisica *f, const Personagem *p, int indice_lutador,
                 Vector3 posicao, float angulo_y)
{
    *c = (Corpo){0};
    c->personagem = p;
    c->fisica = f;
    c->apoio = 1.0f;
    c->fator_velocidade = 1.0f;
    c->equilibrio = 1.0f;
    c->passo_perna = -1;
    c->forca_muscular = 1.0f;
    c->alvo_olhar = Vector3Add(posicao, girar_y((float[3]){ 0, 0, 1 }, angulo_y));

    dMatrix3 R;
    dRFromAxisAndAngle(R, 0, 1, 0, angulo_y);

    for (int i = 0; i < PARTE_QTD; i++) {
        /* O ODE quer o centro de massa na origem da peça: a peça fica no
         * centro de massa e a forma é deslocada para trás dele. */
        const float *t = p->tamanho[i];
        const float *cm = p->centro_massa[i];
        float centro_massa[3] = { p->centro[i][0] + cm[0], p->centro[i][1] + cm[1],
                                  p->centro[i][2] + cm[2] };
        Vector3 centro = Vector3Add(posicao, girar_y(centro_massa, angulo_y));

        c->pecas[i] = dBodyCreate(f->mundo);
        dBodySetPosition(c->pecas[i], centro.x, centro.y, centro.z);
        dBodySetRotation(c->pecas[i], R);

        dMass m;
        int eixo = eixo_comprido(t);
        if (eh_capsula(i)) {
            float raio, comprimento;
            medidas_da_capsula(t, &raio, &comprimento);
            dMassSetCapsuleTotal(&m, p->massa[i], eixo + 1, raio, comprimento);
            c->formas[i] = dCreateCapsule(f->espaco, raio, comprimento);
        } else {
            dMassSetBoxTotal(&m, p->massa[i], t[0], t[1], t[2]);
            c->formas[i] = dCreateBox(f->espaco, t[0], t[1], t[2]);
        }
        dBodySetMass(c->pecas[i], &m);
        dGeomSetBody(c->formas[i], c->pecas[i]);
        dGeomSetOffsetPosition(c->formas[i], -cm[0], -cm[1], -cm[2]);
        if (eh_capsula(i) && eixo != 2) {
            /* A cápsula do ODE fica deitada em Z: gira para o eixo comprido. */
            dMatrix3 Rc;
            if (eixo == 0)
                dRFromAxisAndAngle(Rc, 0, 1, 0, PI / 2);
            else
                dRFromAxisAndAngle(Rc, 1, 0, 0, PI / 2);
            dGeomSetOffsetRotation(c->formas[i], Rc);
        }
        c->refs[i].lutador = indice_lutador;
        c->refs[i].parte = (ParteCorpo)i;
        dGeomSetData(c->formas[i], &c->refs[i]);
    }

    for (int a = 0; a < ART_QTD; a++) {
        ParteCorpo pai, filha;
        esqueleto_pecas_da_articulacao((Articulacao)a, &pai, &filha);
        Vector3 ancora = Vector3Add(posicao, girar_y(p->ancora[a], angulo_y));
        /* O ODE mede o ângulo das juntas no sentido contrário ao do eixo dado
         * (regra da mão direita invertida). Passando os eixos de esqueleto.h
         * com o sinal trocado, o ângulo lido pelo ODE fica com o significado
         * "+" de esqueleto.h. Conferido por demo_fisica --teste. */
        float eixo1[3], eixo2[3];
        esqueleto_eixos((Articulacao)a, eixo1, eixo2);
        for (int k = 0; k < 3; k++) {
            eixo1[k] = -eixo1[k];
            eixo2[k] = -eixo2[k];
        }
        Vector3 e1 = girar_y(eixo1, angulo_y);
        const LimiteArticulacao *lim = &LIMITES[a];
        dJointID j;

        if (esqueleto_tipo_articulacao((Articulacao)a) == ARTIC_DOBRADICA) {
            j = dJointCreateHinge(f->mundo, 0);
            dJointAttach(j, c->pecas[pai], c->pecas[filha]);
            dJointSetHingeAnchor(j, ancora.x, ancora.y, ancora.z);
            dJointSetHingeAxis(j, e1.x, e1.y, e1.z);
            dJointSetHingeParam(j, dParamLoStop, lim->min[0]);
            dJointSetHingeParam(j, dParamHiStop, lim->max[0]);
            dJointSetHingeParam(j, dParamStopERP, JUNTA_LIMITE_ERP);
            dJointSetHingeParam(j, dParamStopCFM, JUNTA_LIMITE_CFM);
        } else {
            Vector3 e2 = girar_y(eixo2, angulo_y);
            j = dJointCreateUniversal(f->mundo, 0);
            dJointAttach(j, c->pecas[pai], c->pecas[filha]);
            dJointSetUniversalAnchor(j, ancora.x, ancora.y, ancora.z);
            dJointSetUniversalAxis1(j, e1.x, e1.y, e1.z);
            dJointSetUniversalAxis2(j, e2.x, e2.y, e2.z);
            dJointSetUniversalParam(j, dParamLoStop, lim->min[0]);
            dJointSetUniversalParam(j, dParamHiStop, lim->max[0]);
            dJointSetUniversalParam(j, dParamLoStop2, lim->min[1]);
            dJointSetUniversalParam(j, dParamHiStop2, lim->max[1]);
            dJointSetUniversalParam(j, dParamStopERP, JUNTA_LIMITE_ERP);
            dJointSetUniversalParam(j, dParamStopCFM, JUNTA_LIMITE_CFM);
            dJointSetUniversalParam(j, dParamStopERP2, JUNTA_LIMITE_ERP);
            dJointSetUniversalParam(j, dParamStopCFM2, JUNTA_LIMITE_CFM);
        }
        c->articulacoes[a] = j;
    }

    corpo_pose_neutra(c);
    c->pose_suave = c->pose_alvo;
    c->rigidez = RIGIDEZ_REPOUSO;
    return true;
}

/* ------------------------------------------------------------ Intenções */

void corpo_definir_pose(Corpo *c, const Pose *pose)
{
    c->pose_alvo = *pose;
}

/* A guarda de cada personagem (personagem.h). O braço físico é reto, sem
 * cotovelo: levantado à frente ele fica "apontando", por isso fica baixo. */
void corpo_pose_neutra(Corpo *c)
{
    const EstiloMovimento *e = &c->personagem->estilo;
    pose_zerar(&c->pose_alvo);
    c->pose_alvo.alvo[ART_CINTURA][0] = e->guarda_cintura;
    c->pose_alvo.alvo[ART_OMBRO_E][0] = e->guarda_ombro;
    c->pose_alvo.alvo[ART_OMBRO_E][1] = e->guarda_frente;
    c->pose_alvo.alvo[ART_OMBRO_D][0] = e->guarda_ombro;
    c->pose_alvo.alvo[ART_OMBRO_D][1] = e->guarda_frente;
    c->pose_alvo.alvo[ART_COTOVELO_E][0] = e->guarda_cotovelo;
    c->pose_alvo.alvo[ART_COTOVELO_D][0] = e->guarda_cotovelo;
}

void corpo_definir_movimento(Corpo *c, Vector2 direcao, float fator_velocidade)
{
    c->direcao_andar = direcao;
    c->fator_velocidade = fator_velocidade;
}

void corpo_definir_alvo_olhar(Corpo *c, Vector3 ponto)
{
    c->alvo_olhar = ponto;
}

void corpo_definir_equilibrio(Corpo *c, float equilibrio)
{
    c->equilibrio = (equilibrio < 0.0f) ? 0.0f : equilibrio;
}

void corpo_definir_rigidez(Corpo *c, float rigidez)
{
    c->rigidez = (rigidez < 0.1f) ? 0.1f : rigidez;
}

void corpo_definir_inclinacao(Corpo *c, Vector2 inclinacao)
{
    c->inclinacao = inclinacao;
}

void corpo_definir_movimento_extra(Corpo *c, const Pose *delta)
{
    c->tem_extra = (delta != NULL);
    if (delta != NULL)
        c->extra = *delta;
    else
        pose_zerar(&c->extra);
}

void corpo_definir_forca_muscular(Corpo *c, float forca)
{
    c->forca_muscular = Clamp(forca, 0.0f, 1.0f);
}

/* ------------------------------------------------------------ Um passo */

/* A pose seguida pelos motores anda até a pose pedida aos poucos. Com rigidez
 * alta (golpe saindo) ela chega quase na hora; com rigidez baixa, devagar. */
static void suavizar_pose(Corpo *c)
{
    float taxa = CORPO_SUAVIZACAO_POSE * c->rigidez * (c->rigidez > 1.0f ? 2.5f : 1.0f);
    float k = 1.0f - expf(-taxa * FISICA_DT);
    for (int a = 0; a < ART_QTD; a++) {
        if (!eh_parte_de_cima(a))
            continue;
        for (int e = 0; e < 2; e++)
            c->pose_suave.alvo[a][e] += (c->pose_alvo.alvo[a][e] - c->pose_suave.alvo[a][e]) * k;
    }
}

/* Soma a variação da animação 'a' (no tempo t) à pose, com o peso dado,
 * nas articulações de 'de' a 'ate' (inclusive). */
static void somar_animacao(Pose *alvo, TipoAnimacao tipo, float t, float peso,
                           Articulacao de, Articulacao ate)
{
    Pose var;
    animacao_variacao(animacao_dados(tipo), t, &var);
    for (int a = (int)de; a <= (int)ate; a++)
        for (int e = 0; e < 2; e++)
            alvo->alvo[a][e] += var.alvo[a][e] * peso;
}

/* Animações de verdade por cima da postura (animacao.h):
 *   - PARADO, sempre: o balanço natural de quem está de pé (respiração,
 *     peso mudando de perna);
 *   - ANDAR, quando há movimento: o ciclo de passos. O ritmo segue a
 *     velocidade REAL da bacia, e não só o comando: se o lutador desliza
 *     depois de um empurrão, os pés tropeçam acompanhando. O passo vai na
 *     direção do movimento em relação ao corpo: para a frente/trás ao avançar
 *     ou recuar, abrindo e fechando as pernas ao ir de lado (passo lateral). */
static void animar(Corpo *c, Pose *alvo)
{
    const EstiloMovimento *e = &c->personagem->estilo;
    /* Parado, quem respira e mexe é a parte de cima; nas pernas a animação
     * entra fraca: inteira, ela fazia o corpo todo ir e voltar ~13 cm. */
    somar_animacao(alvo, ANIM_PARADO, c->tempo_vida, e->balanco_parado, ART_CINTURA, ART_OMBRO_D);
    somar_animacao(alvo, ANIM_PARADO, c->tempo_vida, e->balanco_parado * PARADO_PERNAS,
                   ART_QUADRIL_E, ART_JOELHO_D);

    const dReal *v = dBodyGetLinearVel(c->pecas[PARTE_BACIA]);
    Vector2 vel = { (float)v[0], (float)v[2] };
    float ritmo = Clamp(Vector2Length(vel) / fmaxf(c->personagem->velocidade_max, 0.1f), 0.0f, 1.3f);
    float comando = Vector2Length(c->direcao_andar);
    /* Deslizando de verdade (ritmo > 0.25) ou andando: dá passos. Parado, não. */
    float intensidade = fmaxf(ritmo > 0.25f ? ritmo : 0.0f, comando > 0.1f ? fmaxf(ritmo, 0.35f) : 0.0f);
    if (intensidade < 0.08f)
        return;

    /* Tempo dentro do ciclo de andar: na velocidade máxima, um ciclo dura
     * periodo_passo do personagem, seja qual for a duração da animação. */
    const Animacao *andar = animacao_dados(ANIM_ANDAR);
    float duracao = animacao_duracao(andar);
    c->tempo_passo += FISICA_DT * fmaxf(intensidade, 0.5f) * duracao / e->periodo_passo;
    float peso = fminf(intensidade, 1.0f);

    Pose var;
    animacao_variacao(andar, c->tempo_passo, &var);

    /* Direção do movimento no sistema do corpo: quanto é para a frente e quanto é de lado. */
    Vector2 mov = (comando > 0.1f) ? c->direcao_andar : vel;
    Vector2 dir = (Vector2Length(mov) > 0.01f) ? Vector2Normalize(mov) : (Vector2){ 0, 0 };
    Vector3 f3 = corpo_frente(c);
    Vector2 frente = { f3.x, f3.z };
    Vector2 esquerda = { f3.z, -f3.x };
    float para_frente = Vector2DotProduct(dir, frente);     /* -1 (recua) a 1 (avança) */
    float de_lado = fabsf(Vector2DotProduct(dir, esquerda)); /* 0 a 1 */

    float pernas = peso * e->amplitude_passo;
    for (int lado = 0; lado < 2; lado++) {
        Articulacao quadril = lado ? ART_QUADRIL_D : ART_QUADRIL_E;
        Articulacao joelho = lado ? ART_JOELHO_D : ART_JOELHO_E;
        float passo = var.alvo[quadril][0];
        alvo->alvo[quadril][0] += passo * para_frente * pernas;
        alvo->alvo[quadril][1] += passo * de_lado * 0.8f * pernas;   /* passo lateral */
        alvo->alvo[joelho][0] += var.alvo[joelho][0] * pernas;
    }

    float cima = peso * e->balanco_bracos;                      /* balanço do tronco e braços */
    for (int a = ART_CINTURA; a <= ART_OMBRO_D; a++)
        for (int k = 0; k < 2; k++)
            alvo->alvo[a][k] += var.alvo[a][k] * cima;

    /* Gingado: o tronco passa o peso de uma perna para a outra a cada passo
     * (um ciclo = dois passos); a cabeça compensa pela metade, para o olhar
     * continuar firme. */
    float ginga = e->gingado * peso * sinf(2.0f * PI * c->tempo_passo / duracao);
    alvo->alvo[ART_CINTURA][1] += ginga;
    alvo->alvo[ART_PESCOCO][1] -= ginga * 0.5f;
}

/* Postura das pernas: a base de sumô do personagem (as animações somam por
 * cima). O pé fica plano no chão, então a canela fica quase vertical e a
 * inclinação da bacia ~ quadril - joelho (ver o aviso em personagem.h). */
static void base_das_pernas(const Corpo *c, Pose *alvo)
{
    const EstiloMovimento *e = &c->personagem->estilo;
    for (int lado = 0; lado < 2; lado++) {
        Articulacao quadril = lado ? ART_QUADRIL_D : ART_QUADRIL_E;
        Articulacao joelho = lado ? ART_JOELHO_D : ART_JOELHO_E;
        alvo->alvo[quadril][0] = e->base_quadril;
        alvo->alvo[quadril][1] = e->base_abertura;
        alvo->alvo[joelho][0] = e->base_joelho;
    }
}

/* Entrega um pedaço de cada força de golpe pendente, no ponto da peça. */
static void entregar_forcas_pendentes(Corpo *c)
{
    for (int i = 0; i < CORPO_MAX_PENDENTES; i++) {
        ForcaPendente *f = &c->pendentes[i];
        if (f->passos <= 0)
            continue;
        dBodyID b = c->pecas[f->parte];
        dVector3 ponto;
        dBodyGetRelPointPos(b, f->ponto_local.x, f->ponto_local.y, f->ponto_local.z, ponto);
        dBodyAddForceAtPos(b, f->forca.x, f->forca.y, f->forca.z, ponto[0], ponto[1], ponto[2]);
        f->passos--;
    }
}

/* Músculo de um eixo: mola (k, N.m/rad) + amortecedor (d, N.m.s/rad) em
 * direção ao ângulo alvo, resolvidos pelo próprio ODE (implícito, estável
 * mesmo com mola dura): o motor de velocidade com CFM vira mola-amortecedor
 * quando vel = k.erro / (h.k + d) e CFM = 1 / (h.k + d) (manual do ODE,
 * "ERP e CFM"). Ao contrário do motor antigo, que corria atrás do alvo com
 * força máxima, o músculo cede proporcional ao empurrão e volta sozinho. */
typedef struct {
    float vel;      /* velocidade pedida ao motor (rad/s) */
    float cfm;      /* "maciez" do motor                  */
} Musculo;

static Musculo musculo(float alvo, float atual, float k, float d)
{
    float mix = FISICA_DT * k + d;
    if (mix < 1e-4f)
        return (Musculo){ 0.0f, 1e4f };
    /* Força-velocidade (Hill): o músculo não puxa mais rápido que
     * MUSCULO_VEL_MAX. Sem isso a perna esticava de uma vez e o lutador
     * pulava no próprio golpe. */
    float vel = Clamp(k * (alvo - atual) / mix, -MUSCULO_VEL_MAX, MUSCULO_VEL_MAX);
    return (Musculo){ vel, 1.0f / mix };
}

static void aplicar_motores(Corpo *c, const Pose *alvo)
{
    float tonus = Lerp(TONUS_MINIMO, 1.0f,
                       Clamp(c->equilibrio / TONUS_EQUILIBRIO_CHEIO, 0.0f, 1.0f))
                * c->forca_muscular;
    for (int a = 0; a < ART_QTD; a++) {
        dJointID j = c->articulacoes[a];
        float forca = CORPO_FORCA_MOTOR * LIMITES[a].forca * tonus;
        /* A parte de cima fica solta em repouso e firme no golpe. A cintura
         * nunca fica muito mole: é ela que segura o tronco em cima da bacia. */
        if (eh_parte_de_cima(a)) {
            /* No reflexo, braços e cabeça ganham força para reagir. */
            float r = fmaxf(c->rigidez, RIGIDEZ_REPOUSO + 0.9f * c->reflexo);
            if (a == ART_CINTURA)
                r = fmaxf(r, 0.9f);
            forca *= r;
        }
        /* Reação a impacto: a articulação que levou um choque cede. */
        forca *= 1.0f - REACAO_QUEDA_FORCA * c->amolecido[a];
        /* Mola: a força máxima sai com MUSCULO_ERRO_CHEIO de erro. */
        float k = forca / (MUSCULO_ERRO_CHEIO * LIMITES[a].maciez);
        float d = k * MUSCULO_AMORTECIMENTO;
        /* Atrito interno (passivo): existe mesmo sem tônus. */
        float atrito = JUNTA_AMORTECIMENTO * LIMITES[a].forca;
        if (esqueleto_tipo_articulacao((Articulacao)a) == ARTIC_DOBRADICA) {
            Musculo m = musculo(alvo->alvo[a][0], (float)dJointGetHingeAngle(j), k, d);
            dJointSetHingeParam(j, dParamVel, m.vel);
            dJointSetHingeParam(j, dParamCFM, m.cfm);
            dJointSetHingeParam(j, dParamFMax, forca);
            dJointAddHingeTorque(j, -atrito * (float)dJointGetHingeAngleRate(j));
        } else {
            Musculo m1 = musculo(alvo->alvo[a][0], (float)dJointGetUniversalAngle1(j), k, d);
            Musculo m2 = musculo(alvo->alvo[a][1], (float)dJointGetUniversalAngle2(j), k, d);
            dJointSetUniversalParam(j, dParamVel, m1.vel);
            dJointSetUniversalParam(j, dParamCFM, m1.cfm);
            dJointSetUniversalParam(j, dParamFMax, forca);
            dJointSetUniversalParam(j, dParamVel2, m2.vel);
            dJointSetUniversalParam(j, dParamCFM2, m2.cfm);
            dJointSetUniversalParam(j, dParamFMax2, forca);
            dJointAddUniversalTorques(j, -atrito * (float)dJointGetUniversalAngle1Rate(j),
                                      -atrito * (float)dJointGetUniversalAngle2Rate(j));
        }
    }
}

/* Torque que leva o eixo "para cima" da peça até a direção alvo (a vertical,
 * inclinada por c->inclinacao), com amortecimento. */
static Vector3 mola_de_equilibrio(dBodyID b, float forca, Vector2 inclinacao)
{
    Vector3 cima = eixo_da_peca(b, 1);
    Vector3 alvo = Vector3Normalize((Vector3){ inclinacao.x, 1.0f, inclinacao.y });
    Vector3 correcao = Vector3CrossProduct(cima, alvo);   /* tamanho = sen(ângulo até o alvo) */
    Vector3 w = vetor_ode(dBodyGetAngularVel(b));
    w.y = 0.0f;                                   /* o giro em Y é tratado à parte */
    Vector3 t = Vector3Subtract(Vector3Scale(correcao, forca),
                                Vector3Scale(w, forca * CORPO_AMORTECIMENTO_EQUILIBRIO));
    somar_torque(b, t);
    return t;
}

/* Torque em Y que vira a bacia para o ponto alvo_olhar. */
static void girar_para_o_alvo(Corpo *c)
{
    dBodyID bacia = c->pecas[PARTE_BACIA];
    Vector3 frente = corpo_frente(c);
    Vector3 para_alvo = Vector3Subtract(c->alvo_olhar, corpo_posicao(c));
    para_alvo.y = 0.0f;
    if (Vector3Length(para_alvo) < 0.05f)
        return;
    para_alvo = Vector3Normalize(para_alvo);

    float sen = frente.z * para_alvo.x - frente.x * para_alvo.z;   /* (frente x alvo).y */
    float cos_ = Vector3DotProduct(frente, para_alvo);
    float erro = atan2f(sen, cos_);
    float wy = (float)dBodyGetAngularVel(bacia)[1];
    float t = CORPO_FORCA_GIRO * c->apoio * (erro - CORPO_AMORTECIMENTO_GIRO * wy);
    dBodyAddTorque(bacia, 0, t, 0);
}

/* Centro de massa do corpo inteiro e a velocidade dele. */
static void centro_de_massa(const Corpo *c, Vector3 *posicao, Vector3 *velocidade)
{
    Vector3 cm = { 0 }, v = { 0 };
    float massa = 0.0f;
    for (int i = 0; i < PARTE_QTD; i++) {
        dMass m;
        dBodyGetMass(c->pecas[i], &m);
        float k = (float)m.mass;
        cm = Vector3Add(cm, Vector3Scale(vetor_ode(dBodyGetPosition(c->pecas[i])), k));
        v = Vector3Add(v, Vector3Scale(vetor_ode(dBodyGetLinearVel(c->pecas[i])), k));
        massa += k;
    }
    *posicao = Vector3Scale(cm, 1.0f / massa);
    *velocidade = Vector3Scale(v, 1.0f / massa);
}

static Vector2 no_chao(Vector3 v)
{
    return (Vector2){ v.x, v.z };
}

static Vector2 posicao_no_chao(dBodyID b)
{
    return no_chao(vetor_ode(dBodyGetPosition(b)));
}

static Vector2 meio_dos_pes(const Corpo *c)
{
    return Vector2Lerp(posicao_no_chao(c->pecas[PARTE_PE_E]),
                       posicao_no_chao(c->pecas[PARTE_PE_D]), 0.5f);
}

/* Quanto o corpo está apoiado, de 0 a 1:
 *   pés no chão (os dois = 1, um = APOIO_UM_PE, nenhum = APOIO_SEM_PES)
 *   x peso em cima dos pés (centro de massa perto do meio dos pés; passando
 *     de APOIO_LIMITE, não há como se salvar). */
static float calcular_apoio(const Corpo *c)
{
    const bool *chao = c->fisica->no_chao[c->refs[0].lutador];
    int pes = (int)chao[PARTE_PE_E] + (int)chao[PARTE_PE_D];
    /* Andando (ou dando um passo para se salvar), ter um pé no ar é o
     * próprio passo: não conta como perder apoio. */
    bool andando = Vector2Length(c->direcao_andar) > 0.1f || c->passo_perna >= 0;
    float pelos_pes = (pes == 2 || (pes == 1 && andando)) ? 1.0f
                    : (pes == 1) ? APOIO_UM_PE : APOIO_SEM_PES;

    Vector3 cm, v;
    centro_de_massa(c, &cm, &v);
    float d = Vector2Distance(no_chao(cm), meio_dos_pes(c));

    float t = Clamp((d - APOIO_BASE) / (APOIO_LIMITE - APOIO_BASE), 0.0f, 1.0f);
    float pelo_peso = 1.0f - t * t * (3.0f - 2.0f * t);   /* cai suave até 0 */
    return pelos_pes * pelo_peso;
}

/* ------------------------------------------------------------ Reflexos */

/* Mede o desequilíbrio pelo "capture point" (config.h, REFLEXO_*) e atualiza
 * a intensidade do reflexo. Da velocidade do corpo, desconta a parte que o
 * jogador pediu: quem anda para a frente não está "caindo para a frente". */
static void medir_desequilibrio(Corpo *c)
{
    Vector3 cm, v;
    centro_de_massa(c, &cm, &v);
    Vector2 vel = no_chao(v);
    float comando = Vector2Length(c->direcao_andar);
    if (comando > 0.1f) {
        Vector2 dir = Vector2Scale(c->direcao_andar, 1.0f / comando);
        float vmax = c->personagem->velocidade_max * c->fator_velocidade * fminf(comando, 1.0f);
        float pedida = Clamp(Vector2DotProduct(vel, dir), 0.0f, vmax);
        vel = Vector2Subtract(vel, Vector2Scale(dir, pedida));
    }
    float tempo = sqrtf(fmaxf(cm.y, 0.3f) / -FISICA_GRAVIDADE);
    Vector2 capture = Vector2Add(no_chao(cm), Vector2Scale(vel, tempo));
    Vector2 medido = Vector2Subtract(capture, meio_dos_pes(c));

    float k = 1.0f - expf(-REFLEXO_SUAVIZACAO * FISICA_DT);
    c->desequilibrio = Vector2Add(c->desequilibrio,
                                  Vector2Scale(Vector2Subtract(medido, c->desequilibrio), k));
    float alvo = Clamp((Vector2Length(c->desequilibrio) - REFLEXO_INICIO) / REFLEXO_FAIXA, 0.0f, 1.0f);
    /* Machucado (equilíbrio baixo), reage pior: o dano continua derrubando.
     * Quem já perdeu o round (equilíbrio 0) não reage: desmonta. */
    alvo *= Clamp(c->equilibrio, 0.0f, 1.0f);
    c->reflexo += (alvo - c->reflexo) * k;
}

/* Desequilíbrio no sistema do corpo, unitário: x = para a frente, y = para a
 * esquerda. (0, 0) se não há desequilíbrio. */
static Vector2 direcao_da_queda(const Corpo *c)
{
    float tam = Vector2Length(c->desequilibrio);
    if (tam < 1e-3f)
        return (Vector2){ 0, 0 };
    Vector2 u = Vector2Scale(c->desequilibrio, 1.0f / tam);
    Vector3 f = corpo_frente(c);
    return (Vector2){ u.x * f.x + u.y * f.z, u.x * f.z - u.y * f.x };
}

/* Braços, cintura e cabeça reagindo à queda, como numa pessoa:
 *   - caindo para trás: braços para a frente girando ("moinho"), cintura e
 *     queixo para a frente (joga o peso de volta);
 *   - caindo para a frente: braços à frente para se proteger, cabeça para cima;
 *   - caindo de lado: os dois braços se abrem (o do outro lado faz contrapeso)
 *     e o tronco inclina para o lado contrário. */
static void reflexo_dos_bracos(const Corpo *c, Pose *alvo)
{
    float r = c->reflexo * c->personagem->estilo.reflexo_bracos;
    if (r <= 0.01f || c->tem_extra)            /* golpeando: os braços são do golpe */
        return;
    Vector2 q = direcao_da_queda(c);
    float tras = r * fmaxf(-q.x, 0.0f);
    float frente = r * fmaxf(q.x, 0.0f);
    float lado = r * q.y;                       /* + = caindo para a esquerda */
    float giro = 2.0f * PI * REFLEXO_GIRO_BRACOS * c->tempo_vida;

    for (int b = 0; b < 2; b++) {
        Articulacao ombro = b ? ART_OMBRO_D : ART_OMBRO_E;
        /* O braço do lado da queda se abre para "segurar"; o outro sobe mais,
         * de contrapeso. */
        float do_lado = b ? -lado : lado;
        const float *t = REACAO_BRACOS.ombro_tras, *f = REACAO_BRACOS.ombro_frente;
        float abre = fabsf(lado) * (do_lado > 0.0f ? REACAO_BRACOS.abre_queda
                                                   : REACAO_BRACOS.abre_contrapeso);
        float moinho = REACAO_BRACOS.giro * tras;
        alvo->alvo[ombro][0] += t[0] * tras + f[0] * frente + abre + moinho * cosf(giro);
        alvo->alvo[ombro][1] += t[1] * tras + f[1] * frente + moinho * sinf(giro);
        /* Os braços se esticam para se equilibrar. */
        Articulacao cotovelo = b ? ART_COTOVELO_D : ART_COTOVELO_E;
        alvo->alvo[cotovelo][0] -= COTOVELO_REFLEXO * fminf(r, 1.0f)
                                 * c->personagem->estilo.guarda_cotovelo;
    }
    const float *ci = REACAO_BRACOS.cintura, *pe = REACAO_BRACOS.pescoco;
    alvo->alvo[ART_CINTURA][0] += ci[0] * tras - ci[1] * frente;
    alvo->alvo[ART_CINTURA][1] -= ci[2] * lado;
    alvo->alvo[ART_PESCOCO][0] += pe[0] * tras - pe[1] * frente;
    alvo->alvo[ART_PESCOCO][1] -= pe[2] * lado;
}

/* Pés: a sola procura ficar reta no chão (o tornozelo compensa a inclinação
 * da canela), como o pé de quem está de pé. O alvo é o ângulo atual menos a
 * inclinação do pé no mundo; no chão, o próprio chão já o deixa reto. */
static void pes_retos(const Corpo *c, Pose *alvo)
{
    /* Equilíbrio pelo tornozelo (o 1º nível, antes do quadril e do passo):
     * com o peso indo para a frente, a canela vai para trás sobre o pé e a
     * ponta do pé aperta o chão, empurrando o corpo de volta; para trás, o
     * contrário. Proporcional ao desequilíbrio para a frente (m), limitado. */
    Vector3 f = corpo_frente(c);
    float para_frente = c->desequilibrio.x * f.x + c->desequilibrio.y * f.z;
    float pe_compensa = Clamp(-TORNOZELO_EQUILIBRIO * para_frente,
                              -TORNOZELO_EQUILIBRIO_MAX, TORNOZELO_EQUILIBRIO_MAX)
                      * Clamp(c->equilibrio, 0.0f, 1.0f);
    for (int lado = 0; lado < 2; lado++) {
        dBodyID pe = c->pecas[lado ? PARTE_PE_D : PARTE_PE_E];
        dJointID j = c->articulacoes[lado ? ART_TORNOZELO_D : ART_TORNOZELO_E];
        Vector3 frente = eixo_da_peca(pe, 2);
        Vector3 lado_x = eixo_da_peca(pe, 0);
        float ponta = asinf(Clamp(frente.y, -1.0f, 1.0f));     /* + = ponta para cima */
        /* A borda de fora fica em +X no pé esquerdo e em -X no direito. */
        float fora = asinf(Clamp(lado_x.y, -1.0f, 1.0f)) * (lado ? -1.0f : 1.0f);
        Articulacao a = lado ? ART_TORNOZELO_D : ART_TORNOZELO_E;
        /* Apoiado, o tornozelo segura a canela na postura (equilíbrio pelo pé:
         * solto, o corpo balançava sobre ele como um pêndulo); no ar, deixa a
         * sola reta para pisar. De lado ele sempre acompanha o chão. */
        bool apoiado = c->fisica->no_chao[c->refs[0].lutador][lado ? PARTE_PE_D : PARTE_PE_E];
        /* O golpe também empurra com a ponta do pé (extra). */
        alvo->alvo[a][0] = apoiado ? TORNOZELO_POSTURA + pe_compensa + c->extra.alvo[a][0]
                                   : (float)dJointGetUniversalAngle1(j) - ponta;
        alvo->alvo[a][1] = (float)dJointGetUniversalAngle2(j) - fora;
    }
}

/* Pernas no reflexo: afunda o quadril e abre a base para se firmar, como o
 * lutador de sumô empurrado. Sem isso as pernas ficavam duras e o corpo
 * escorregava inteiro, como um bloco. A perna que dá o passo é do passo. */
static void reflexo_das_pernas(const Corpo *c, Pose *alvo)
{
    float r = c->reflexo;
    if (r <= 0.01f)
        return;
    float afunda = REFLEXO_AFUNDAR * r;
    for (int perna = 0; perna < 2; perna++) {
        if (perna == c->passo_perna)
            continue;
        Articulacao quadril = perna ? ART_QUADRIL_D : ART_QUADRIL_E;
        Articulacao joelho = perna ? ART_JOELHO_D : ART_JOELHO_E;
        alvo->alvo[quadril][0] += afunda;
        alvo->alvo[joelho][0] += afunda;
        alvo->alvo[quadril][1] += REFLEXO_ABRIR * r;
    }
}

static Vector3 ancora_do_quadril(const Corpo *c, int perna)
{
    dVector3 a;
    dJointGetUniversalAnchor(c->articulacoes[perna ? ART_QUADRIL_D : ART_QUADRIL_E], a);
    return vetor_ode(a);
}

/* Começa um passo de recuperação se o desequilíbrio passou do limite. A
 * perna escolhida é a do lado para onde o corpo cai; caindo para a frente ou
 * para trás, a do pé mais longe do ponto de parada (o pé com menos peso).
 * O pé vai para o capture point: pisando ali, o corpo para. */
static void comecar_passo(Corpo *c)
{
    if (c->passo_perna >= 0 || c->passo_espera > 0.0f || c->reflexo < PASSO_LIMIAR
        || c->apoio < PASSO_APOIO_MINIMO || c->equilibrio <= 0.0f)
        return;
    Vector2 capture = Vector2Add(meio_dos_pes(c), c->desequilibrio);
    Vector2 q = direcao_da_queda(c);
    int perna;
    if (fabsf(q.y) > 0.5f) {
        perna = (q.y > 0.0f) ? 0 : 1;
    } else {
        float de = Vector2Distance(posicao_no_chao(c->pecas[PARTE_PE_E]), capture);
        float dd = Vector2Distance(posicao_no_chao(c->pecas[PARTE_PE_D]), capture);
        perna = (de > dd) ? 0 : 1;
    }
    /* O pé fica um pouco para fora do ponto, do lado da própria perna: a base
     * continua larga depois do passo. */
    Vector3 f = corpo_frente(c);
    Vector2 esquerda = { f.z, -f.x };
    Vector2 alvo = Vector2Add(capture, Vector2Scale(esquerda, perna ? -PASSO_PE_ABERTO : PASSO_PE_ABERTO));
    /* A perna tem tamanho: o passo não passa de PASSO_MAX a partir do quadril. */
    Vector2 quadril = no_chao(ancora_do_quadril(c, perna));
    Vector2 d = Vector2Subtract(alvo, quadril);
    float maximo = PASSO_MAX * c->personagem->estilo.passo_reacao;
    if (Vector2Length(d) > maximo)
        alvo = Vector2Add(quadril, Vector2Scale(Vector2Normalize(d), maximo));
    /* Não pisa dentro do adversário (alvo_olhar é a bacia dele): a mola do
     * passo empurraria o pé contra as pernas do outro, e elas afundavam até
     * 11 cm umas nas outras. */
    Vector2 outro = no_chao(c->alvo_olhar);
    Vector2 dele = Vector2Subtract(alvo, outro);
    if (Vector2Length(dele) < PASSO_DISTANCIA_ADVERSARIO)
        alvo = Vector2Add(outro, Vector2Scale(Vector2Length(dele) > 1e-3f ? Vector2Normalize(dele)
                                              : Vector2Normalize(Vector2Subtract(quadril, outro)),
                                              PASSO_DISTANCIA_ADVERSARIO));

    c->passo_perna = perna;
    c->passo_tempo = 0.0f;
    c->passo_alvo = alvo;
}

/* Pose da perna que dá o passo: aponta a coxa para o lugar do passo e dobra o
 * joelho no meio do caminho (o pé sobe e desce). */
static void pose_do_passo(const Corpo *c, Pose *alvo)
{
    if (c->passo_perna < 0)
        return;
    int perna = c->passo_perna;
    Vector3 q3 = ancora_do_quadril(c, perna);
    Vector2 d = Vector2Subtract(c->passo_alvo, no_chao(q3));
    Vector3 f = corpo_frente(c);
    float para_frente = d.x * f.x + d.y * f.z;
    float para_fora = (d.x * f.z - d.y * f.x) * (perna ? -1.0f : 1.0f);
    float altura = fmaxf(q3.y, 0.3f);
    float sobe = sinf(PI * Clamp(c->passo_tempo / PASSO_DURACAO, 0.0f, 1.0f));

    Articulacao quadril = perna ? ART_QUADRIL_D : ART_QUADRIL_E;
    Articulacao joelho = perna ? ART_JOELHO_D : ART_JOELHO_E;
    alvo->alvo[quadril][0] = atan2f(para_frente, altura) + PASSO_SOBE_QUADRIL * sobe;
    alvo->alvo[quadril][1] = atan2f(para_fora, altura);
    alvo->alvo[joelho][0] = c->personagem->estilo.base_joelho + PASSO_SOBE_JOELHO * sobe;
}

/* Forças do passo: levam o pé até o lugar do passo e o tiram um pouco do
 * chão. São forças INTERNAS (o contrário vai na bacia), como um músculo: o
 * passo não empurra o corpo inteiro, só reposiciona o pé. */
static void forcas_do_passo(Corpo *c)
{
    if (c->passo_perna < 0)
        return;
    c->passo_tempo += FISICA_DT;
    if (c->passo_tempo >= PASSO_DURACAO || c->equilibrio <= 0.0f) {
        c->passo_perna = -1;
        c->passo_espera = PASSO_ESPERA;
        return;
    }
    /* A força vai na canela (o pé é leve demais para puxar a perna), com a
     * massa da canela e do pé juntas. */
    dBodyID pe = c->pecas[c->passo_perna ? PARTE_CANELA_D : PARTE_CANELA_E];
    dBodyID bacia = c->pecas[PARTE_BACIA];
    const Personagem *p = c->personagem;
    float massa = p->massa[c->passo_perna ? PARTE_CANELA_D : PARTE_CANELA_E]
                + p->massa[c->passo_perna ? PARTE_PE_D : PARTE_PE_E];
    Vector2 erro = Vector2Subtract(c->passo_alvo, posicao_no_chao(pe));
    Vector2 vel = no_chao(vetor_ode(dBodyGetLinearVel(pe)));
    /* Mola com amortecimento crítico para a massa do pé: chega sem passar. */
    Vector2 f = Vector2Subtract(Vector2Scale(erro, PASSO_FORCA),
                                Vector2Scale(vel, 2.0f * sqrtf(PASSO_FORCA * massa)));
    float sobe = sinf(PI * c->passo_tempo / PASSO_DURACAO);
    float levanta = massa * -FISICA_GRAVIDADE * PASSO_LEVANTAR * sobe;
    dBodyAddForce(pe, f.x, levanta, f.y);
    dBodyAddForce(bacia, -f.x, -levanta, -f.y);
}

/* Força horizontal na bacia para chegar à velocidade desejada. */
static void andar(Corpo *c)
{
    const Personagem *p = c->personagem;
    dBodyID bacia = c->pecas[PARTE_BACIA];
    Vector3 v = vetor_ode(dBodyGetLinearVel(bacia));
    float vmax = p->velocidade_max * c->fator_velocidade;
    Vector2 desejada = Vector2Scale(c->direcao_andar, vmax);
    Vector2 erro = { desejada.x - v.x, desejada.y - v.z };

    /* Parado, freia com pouca força: depois de um empurrão o lutador desliza. */
    float forca = p->forca_andar * ((Vector2Length(c->direcao_andar) > 0.1f) ? 1.0f : 0.08f);
    forca *= c->apoio;                    /* sem os pés no chão, não há como andar */
    Vector2 f = Vector2Scale(erro, forca / fmaxf(vmax, 0.1f));
    float tam = Vector2Length(f);
    if (tam > forca)
        f = Vector2Scale(f, forca / tam);
    /* Metade na bacia e metade nos pés: andar é empurrar o chão com os pés.
     * Toda a força na bacia, com os pés presos pelo atrito, tombaria o corpo. */
    dBodyAddForce(bacia, f.x * 0.5f, 0, f.y * 0.5f);
    dBodyAddForce(c->pecas[PARTE_CANELA_E], f.x * 0.25f, 0, f.y * 0.25f);
    dBodyAddForce(c->pecas[PARTE_CANELA_D], f.x * 0.25f, 0, f.y * 0.25f);
}

void corpo_passo(Corpo *c)
{
    c->tempo_vida += FISICA_DT;
    c->apoio = calcular_apoio(c);
    medir_desequilibrio(c);
    c->passo_espera = fmaxf(0.0f, c->passo_espera - FISICA_DT);
    comecar_passo(c);
    suavizar_pose(c);
    entregar_forcas_pendentes(c);
    for (int a = 0; a < ART_QTD; a++)          /* recupera a força depois do choque */
        c->amolecido[a] = fmaxf(0.0f, c->amolecido[a] - FISICA_DT / REACAO_RECUPERACAO);

    Pose alvo = c->pose_suave;
    base_das_pernas(c, &alvo);
    animar(c, &alvo);
    /* O golpe animado já é uma curva suave: entra direto, sem pose_suave.
     * Desequilibrado, as pernas deixam o golpe e cuidam do equilíbrio (os
     * braços continuam): golpeando com o corpo caindo, os pés saíam do chão
     * e metade das quedas era de quem estava golpeando. */
    float pernas_do_golpe = 1.0f - Clamp(c->reflexo / GOLPE_REFLEXO_PERNAS, 0.0f, 1.0f);
    for (int a = 0; a < ART_QTD; a++)
        for (int k = 0; k < 2; k++)
            alvo.alvo[a][k] += c->extra.alvo[a][k] * (eh_perna(a) ? pernas_do_golpe : 1.0f);
    reflexo_dos_bracos(c, &alvo);
    reflexo_das_pernas(c, &alvo);
    pose_do_passo(c, &alvo);
    pes_retos(c, &alvo);
    aplicar_motores(c, &alvo);
    forcas_do_passo(c);

    /* A maior parte da mola vai na bacia: o tronco fica livre para inclinar
     * nas poses (a cintura segura o tronco em relação à bacia). A mola mira
     * um pouco à frente (postura do personagem): mirando a vertical, ela
     * brigava com as pernas dobradas e o lutador "sentava" para trás. */
    Vector3 frente = corpo_frente(c);
    float base = c->personagem->estilo.inclinacao;
    Vector2 inclinacao = Vector2Add(c->inclinacao, (Vector2){ frente.x * base, frente.z * base });
    /* Só MOLA_EQUILIBRIO_FRACAO é força de fora do corpo. O resto é dos
     * músculos das pernas: o mesmo torque, com a reação nos pés apoiados (que
     * o passam ao chão). Sem pé no chão não há onde apoiar, e um torque maior
     * do que o pé aguenta faz o pé tombar, como em gente. */
    const bool *chao = c->fisica->no_chao[c->refs[0].lutador];
    int pes = (int)chao[PARTE_PE_E] + (int)chao[PARTE_PE_D];
    float fracao = (pes > 0) ? 1.0f : MOLA_EQUILIBRIO_FRACAO;
    float mola = c->personagem->forca_equilibrio * fracao * c->equilibrio * c->apoio;
    Vector3 t = Vector3Add(mola_de_equilibrio(c->pecas[PARTE_BACIA], mola * 0.7f, inclinacao),
                           mola_de_equilibrio(c->pecas[PARTE_TRONCO], mola * 0.3f, inclinacao));
    if (pes > 0) {
        Vector3 reacao = Vector3Scale(t, -(1.0f - MOLA_EQUILIBRIO_FRACAO) / (float)pes);
        for (int lado = 0; lado < 2; lado++)
            if (chao[lado ? PARTE_PE_D : PARTE_PE_E])
                somar_torque(c->pecas[lado ? PARTE_PE_D : PARTE_PE_E], reacao);
    }

    girar_para_o_alvo(c);
    andar(c);
}

/* ------------------------------------------------------------ Consultas */

void corpo_impulso(Corpo *c, Vector3 impulso)
{
    /* Antes o impulso ia inteiro numa peça (a bacia): leve, ela disparava e
     * arrancava os pés do chão (medido: picos de 6 a 40 m/s entre peças). */
    float massa = 0.0f;
    for (int i = 0; i < PARTE_QTD; i++)
        massa += c->personagem->massa[i];
    Vector3 dv = Vector3Scale(impulso, 1.0f / massa);
    for (int i = 0; i < PARTE_QTD; i++) {
        const dReal *v = dBodyGetLinearVel(c->pecas[i]);
        dBodySetLinearVel(c->pecas[i], v[0] + dv.x, v[1] + dv.y, v[2] + dv.z);
    }
}

void corpo_impulso_no_ponto(Corpo *c, ParteCorpo parte, Vector3 impulso, Vector3 ponto)
{
    /* Impulso = força x tempo: J/(N.dt) durante N passos entrega o impulso J,
     * e o ODE calcula sozinho o giro causado pelo ponto. O ponto é guardado no
     * sistema da peça, para continuar no mesmo lugar dela enquanto ela se move. */
    ForcaPendente *f = &c->pendentes[0];
    for (int i = 0; i < CORPO_MAX_PENDENTES; i++)
        if (c->pendentes[i].passos <= 0) {
            f = &c->pendentes[i];
            break;
        }
    dVector3 local;
    dBodyGetPosRelPoint(c->pecas[parte], ponto.x, ponto.y, ponto.z, local);
    f->parte = parte;
    f->forca = Vector3Scale(impulso, 1.0f / (FISICA_DT * ACERTO_PASSOS));
    f->ponto_local = (Vector3){ (float)local[0], (float)local[1], (float)local[2] };
    f->passos = ACERTO_PASSOS;
}

void corpo_receber_contato(Corpo *c, ParteCorpo parte, float intensidade)
{
    float quanto = Clamp((intensidade - REACAO_LIMIAR) / REACAO_FAIXA, 0.0f, 1.0f);
    if (quanto <= 0.0f)
        return;
    for (int a = 0; a < ART_QTD; a++) {
        ParteCorpo pai, filha;
        esqueleto_pecas_da_articulacao((Articulacao)a, &pai, &filha);
        if (pai != parte && filha != parte)
            continue;
        float alvo = eh_perna(a) ? quanto * REACAO_PERNAS : quanto;
        c->amolecido[a] = fmaxf(c->amolecido[a], alvo);
    }
}

Vector3 corpo_posicao(const Corpo *c)
{
    return vetor_ode(dBodyGetPosition(c->pecas[PARTE_BACIA]));
}

Vector3 corpo_frente(const Corpo *c)
{
    Vector3 f = eixo_da_peca(c->pecas[PARTE_BACIA], 2);
    f.y = 0.0f;
    if (Vector3Length(f) < 1e-4f)
        return (Vector3){ 0, 0, 1 };
    return Vector3Normalize(f);
}

/* Regra do sumô: encostar no chão com qualquer parte além das solas dos pés
 * (joelho e canela também perdem). */
bool corpo_caido(const Corpo *c)
{
    const bool *chao = c->fisica->no_chao[c->refs[0].lutador];
    for (int i = 0; i < PARTE_QTD; i++)
        if (chao[i] && i != PARTE_PE_E && i != PARTE_PE_D)
            return true;
    return false;
}

bool corpo_valido(const Corpo *c)
{
    for (int i = 0; i < PARTE_QTD; i++) {
        const dReal *p = dBodyGetPosition(c->pecas[i]);
        const dReal *v = dBodyGetLinearVel(c->pecas[i]);
        for (int k = 0; k < 3; k++)
            if (!isfinite(p[k]) || !isfinite(v[k]))
                return false;
    }
    return true;
}

Matrix corpo_transformacao(const Corpo *c, ParteCorpo parte)
{
    /* A peça fica no centro de massa; o centro da caixa fica deslocado. */
    const dReal *R = dBodyGetRotation(c->pecas[parte]);
    const float *cm = c->personagem->centro_massa[parte];
    dVector3 p;
    dBodyGetRelPointPos(c->pecas[parte], -cm[0], -cm[1], -cm[2], p);
    /* Matrix da raylib: m0..m3 é a 1ª coluna. Rotação do ODE: 3x4 por linhas. */
    Matrix m = {
        (float)R[0], (float)R[1], (float)R[2],  (float)p[0],
        (float)R[4], (float)R[5], (float)R[6],  (float)p[1],
        (float)R[8], (float)R[9], (float)R[10], (float)p[2],
        0.0f,        0.0f,        0.0f,         1.0f,
    };
    return m;
}

/* ------------------------------------------------------------ Pegada */

void corpo_soltar(Corpo *c)
{
    for (int lado = 0; lado < 2; lado++) {
        if (c->pegada[lado] != NULL)
            dJointDestroy(c->pegada[lado]);
        c->pegada[lado] = NULL;
        c->pegada_tempo[lado] = 0.0f;
    }
}

void corpo_agarrar(Corpo *c, Corpo *outro, const Fisica *f, bool quer)
{
    int eu = c->refs[0].lutador;
    bool alguem_caiu = corpo_caido(c) || corpo_caido(outro);
    for (int lado = 0; lado < 2; lado++) {
        ParteCorpo mao = lado ? PARTE_ANTEBRACO_D : PARTE_ANTEBRACO_E;
        dJointID j = c->pegada[lado];
        if (j != NULL) {
            c->pegada_tempo[lado] += FISICA_DT;
            const dJointFeedback *fb = &c->pegada_forca[lado];
            float forca = Vector3Length((Vector3){ (float)fb->f1[0], (float)fb->f1[1], (float)fb->f1[2] });
            bool cansou = c->pegada_tempo[lado] > AGARRAR_TEMPO_MAX;
            bool largou = !quer && c->pegada_tempo[lado] > AGARRAR_TEMPO_MIN;
            if (cansou || largou || alguem_caiu || forca > AGARRAR_FORCA_MAX) {
                dJointDestroy(j);
                c->pegada[lado] = NULL;
                c->pegada_tempo[lado] = 0.0f;
            }
            continue;
        }
        if (!quer || alguem_caiu)
            continue;
        for (int k = 0; k < f->qtd_contatos; k++) {
            const Contato *ct = &f->contatos[k];
            const RefPeca *minha = (ct->a.lutador == eu) ? &ct->a : &ct->b;
            const RefPeca *dele = (ct->a.lutador == eu) ? &ct->b : &ct->a;
            if (minha->lutador != eu || minha->parte != mao)
                continue;
            if (dele->parte != PARTE_BACIA && dele->parte != PARTE_TRONCO)
                continue;
            /* Junta de bola no ponto do toque: a mão segura a faixa, mas o
             * braço continua livre para girar. */
            j = dJointCreateBall(f->mundo, 0);
            dJointAttach(j, c->pecas[mao], outro->pecas[dele->parte]);
            dJointSetBallAnchor(j, ct->ponto[0], ct->ponto[1], ct->ponto[2]);
            dJointSetFeedback(j, &c->pegada_forca[lado]);
            c->pegada[lado] = j;
            c->pegada_tempo[lado] = 0.0f;
            break;
        }
    }
}

void corpo_destruir(Corpo *c)
{
    corpo_soltar(c);
    for (int a = 0; a < ART_QTD; a++)
        if (c->articulacoes[a] != NULL)
            dJointDestroy(c->articulacoes[a]);
    for (int i = 0; i < PARTE_QTD; i++) {
        if (c->formas[i] != NULL)
            dGeomDestroy(c->formas[i]);
        if (c->pecas[i] != NULL)
            dBodyDestroy(c->pecas[i]);
    }
    *c = (Corpo){0};
}
