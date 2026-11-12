/*
 * fisica.c — mundo do ODE, passo fixo e registro de contatos.
 * Especificação: fisica.h
 *
 * Dono: Principal.
 */
#include <math.h>
#include <string.h>
#include "fisica.h"

/* Parâmetros de "maciez" dos contatos e das juntas. Contatos firmes (ERP alto,
 * CFM baixo) para as peças não afundarem umas nas outras; a camada de contato
 * e o limite da velocidade de correção evitam tremedeira e "estouros" quando
 * duas peças já estão afundadas. Mexer só se a física tremer. */
#define ERP_MUNDO        0.2f
#define CFM_MUNDO        1e-5f
#define ERP_CONTATO      0.45f
#define ERP_LUTADORES    0.80f   /* entre os dois lutadores: desafunda mais rápido
                                    (motores e passos empurram peça contra peça) */
#define CFM_CONTATO      1e-5f
#define CAMADA_CONTATO   0.002f  /* afundamento tolerado antes de empurrar (m) */
#define CORRECAO_MAX     3.0f    /* velocidade máxima para desafundar (m/s) */
#define VEL_ANGULAR_MAX  40.0f   /* rad/s: trava contra explosões numéricas */

void fisica_iniciar_biblioteca(void)
{
    dInitODE2(0);
    dAllocateODEDataForThread(dAllocateMaskAll);   /* dados da thread principal */
}

void fisica_finalizar_biblioteca(void)
{
    dCloseODE();
}

/* Velocidade relativa entre as duas peças NO PONTO p (m/s). Usa a velocidade
 * do ponto, e não do centro: a mão de um braço girando anda bem mais rápido
 * que o centro do braço. */
static float velocidade_relativa(dBodyID a, dBodyID b, const dReal *p)
{
    dVector3 va, vb;
    dBodyGetPointVel(a, p[0], p[1], p[2], va);
    dBodyGetPointVel(b, p[0], p[1], p[2], vb);
    float dx = (float)(va[0] - vb[0]);
    float dy = (float)(va[1] - vb[1]);
    float dz = (float)(va[2] - vb[2]);
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

/* Chamada pelo ODE para cada par de formas que podem estar se tocando. */
static void ao_aproximar(void *dados, dGeomID g1, dGeomID g2)
{
    Fisica *f = dados;
    const RefPeca *r1 = dGeomGetData(g1);
    const RefPeca *r2 = dGeomGetData(g2);
    if (r1 == NULL || r2 == NULL)
        return;

    /* Peças do mesmo lutador não colidem: as articulações já as ligam,
     * e colidir faria o corpo tremer. */
    if (r1->lutador == r2->lutador)
        return;

    dContact pontos[FISICA_PONTOS_POR_PAR];
    int n = dCollide(g1, g2, FISICA_PONTOS_POR_PAR, &pontos[0].geom, sizeof(dContact));
    if (n <= 0)
        return;

    dBodyID b1 = dGeomGetBody(g1);
    dBodyID b2 = dGeomGetBody(g2);
    bool entre_lutadores = (r1->lutador >= 0 && r2->lutador >= 0);

    /* Peça de lutador no chão: marca o apoio e escolhe o atrito. */
    float atrito = FISICA_ATRITO_CORPOS;
    dBodyID pe_no_chao = NULL;
    if (!entre_lutadores) {
        const RefPeca *peca = (r1->lutador >= 0) ? r1 : r2;
        f->no_chao[peca->lutador][peca->parte] = true;
        bool pe = (peca->parte == PARTE_PE_E || peca->parte == PARTE_PE_D);
        atrito = pe ? FISICA_ATRITO_PES : FISICA_ATRITO_CORPO_CHAO;
        if (pe)
            pe_no_chao = (r1->lutador >= 0) ? b1 : b2;
    }

    for (int i = 0; i < n; i++) {
        /* Approx1: mu passa a ser o coeficiente de atrito (sem ele, mu é força). */
        pontos[i].surface.mode = dContactApprox1 | dContactSoftERP | dContactSoftCFM;
        pontos[i].surface.mu = atrito;
        if (pe_no_chao != NULL) {
            /* Ponta do pé: o toque fica na metade da frente do pé. */
            dVector3 local;
            dBodyGetPosRelPoint(pe_no_chao, pontos[i].geom.pos[0], pontos[i].geom.pos[1],
                                pontos[i].geom.pos[2], local);
            if (local[2] > 0.0)
                pontos[i].surface.mu = FISICA_ATRITO_PONTA;
        }
        pontos[i].surface.soft_erp = entre_lutadores ? ERP_LUTADORES : ERP_CONTATO;
        pontos[i].surface.soft_cfm = CFM_CONTATO;
        dJointID j = dJointCreateContact(f->mundo, f->grupo_contatos, &pontos[i]);
        dJointAttach(j, b1, b2);
    }

    if (entre_lutadores && f->qtd_contatos < FISICA_MAX_CONTATOS) {
        Contato *c = &f->contatos[f->qtd_contatos++];
        c->a = *r1;
        c->b = *r2;
        dVector3 media = { 0, 0, 0 };
        for (int i = 0; i < n; i++)
            for (int k = 0; k < 3; k++)
                media[k] += pontos[i].geom.pos[k] / n;
        for (int k = 0; k < 3; k++)
            c->ponto[k] = (float)media[k];
        c->intensidade = velocidade_relativa(b1, b2, media);
    }
}

bool fisica_iniciar(Fisica *f)
{
    *f = (Fisica){0};

    f->mundo = dWorldCreate();
    f->espaco = dHashSpaceCreate(0);
    f->grupo_contatos = dJointGroupCreate(0);
    if (f->mundo == NULL || f->espaco == NULL || f->grupo_contatos == NULL) {
        fisica_liberar(f);
        return false;
    }

    dWorldSetGravity(f->mundo, 0, FISICA_GRAVIDADE, 0);
    dWorldSetERP(f->mundo, ERP_MUNDO);
    dWorldSetCFM(f->mundo, CFM_MUNDO);
    dWorldSetQuickStepNumIterations(f->mundo, FISICA_ITERACOES);
    dWorldSetAutoDisableFlag(f->mundo, 0);          /* corpos nunca "dormem" */
    dWorldSetMaxAngularSpeed(f->mundo, VEL_ANGULAR_MAX);
    dWorldSetContactSurfaceLayer(f->mundo, CAMADA_CONTATO);
    dWorldSetContactMaxCorrectingVel(f->mundo, CORRECAO_MAX);
    dWorldSetAngularDamping(f->mundo, 0.02f);
    dWorldSetLinearDamping(f->mundo, 0.005f);

    f->chao = dCreatePlane(f->espaco, 0, 1, 0, 0);  /* plano y = 0 */
    f->ref_chao.lutador = -1;
    f->ref_chao.parte = PARTE_QTD;
    dGeomSetData(f->chao, &f->ref_chao);
    return true;
}

void fisica_passo(Fisica *f)
{
    f->qtd_contatos = 0;
    memset(f->no_chao, 0, sizeof f->no_chao);
    dSpaceCollide(f->espaco, f, ao_aproximar);
    dWorldQuickStep(f->mundo, FISICA_DT);
    dJointGroupEmpty(f->grupo_contatos);
}

void fisica_liberar(Fisica *f)
{
    if (f->grupo_contatos != NULL)
        dJointGroupDestroy(f->grupo_contatos);
    if (f->espaco != NULL)
        dSpaceDestroy(f->espaco);    /* destrói junto as formas que restarem (o chão) */
    if (f->mundo != NULL)
        dWorldDestroy(f->mundo);
    *f = (Fisica){0};
}
