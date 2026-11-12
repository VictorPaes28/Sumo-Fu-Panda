/*
 * recursos.c — carrega e libera modelos e sons.
 * Especificação: recursos.h e docs/ASSETS.md
 *
 * Dono: Integrante B.
 */
#include <stddef.h>
#include "recursos.h"
#include "luz.h"
#include "personagem.h"

static const char *const ARQUIVOS_SONS[SOM_QTD] = {
    [SOM_GOLPE_LEVE]     = "golpe_leve.wav",
    [SOM_GOLPE_FORTE]    = "golpe_forte.wav",
    [SOM_QUEDA]          = "queda.wav",
    [SOM_CONTAGEM]       = "contagem.wav",
    [SOM_LUTE]           = "lute.wav",
    [SOM_VITORIA]        = "vitoria.wav",
    [SOM_MENU_MOVER]     = "menu_mover.wav",
    [SOM_MENU_CONFIRMAR] = "menu_confirmar.wav",
};

void recursos_carregar(Recursos *r)
{
    *r = (Recursos){0};
    const char *base = GetApplicationDirectory();
    int faltando = 0;

    r->luz = luz_criar();

    int qtd = personagem_quantidade();
    for (int p = 0; p < qtd && p < MAX_PERSONAGENS; p++) {
        const Personagem *per = personagem_dados(p);
        if (per->modelo == NULL)
            continue;                     /* personagem ainda sem modelo: caixas */
        const char *caminho = TextFormat("%sassets/modelos/%s/%s", base, per->pasta, per->modelo);
        if (!pele_carregar(&r->peles[p], caminho, per, r->luz))
            faltando++;
    }

    for (int s = 0; s < SOM_QTD; s++) {
        const char *caminho = TextFormat("%sassets/sons/%s", base, ARQUIVOS_SONS[s]);
        if (FileExists(caminho)) {
            r->sons[s] = LoadSound(caminho);
            r->tem_som[s] = IsSoundValid(r->sons[s]);
        }
        if (!r->tem_som[s])
            faltando++;
    }

    if (faltando > 0)
        TraceLog(LOG_WARNING, "RECURSOS: %d arquivos faltando em assets/ "
                 "(o jogo usa caixas no lugar dos modelos e fica sem esses sons)", faltando);
}

void recursos_tocar(const Recursos *r, SomJogo som)
{
    if ((int)som >= 0 && som < SOM_QTD && r->tem_som[som])
        PlaySound(r->sons[som]);
}

void recursos_liberar(Recursos *r)
{
    for (int p = 0; p < MAX_PERSONAGENS; p++)
        pele_liberar(&r->peles[p]);
    luz_liberar(r->luz);
    for (int s = 0; s < SOM_QTD; s++)
        if (r->tem_som[s])
            UnloadSound(r->sons[s]);
    *r = (Recursos){0};
}
