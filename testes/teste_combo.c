/*
 * teste_combo.c — testes da LISTA CIRCULAR DUPLAMENTE ENCADEADA (combo.c).
 *
 * Roda no terminal, sem raylib e sem ODE. combo.c está pronto quando todos
 * os testes passam (saída "TODOS OS TESTES PASSARAM" e código de saída 0).
 *
 * NÃO altere este arquivo para fazer um teste passar: corrija combo.c.
 * Se achar que um teste está errado, fale com o grupo antes.
 */
#include <stdio.h>
#include "combo.h"
#include "config.h"

static int verificacoes = 0;
static int falhas = 0;

#define VERIFICA(cond)                                                       \
    do {                                                                     \
        verificacoes++;                                                      \
        if (!(cond)) {                                                       \
            falhas++;                                                        \
            printf("    FALHOU (linha %d): %s\n", __LINE__, #cond);          \
        }                                                                    \
    } while (0)

#define TESTE(nome) printf("- %s\n", nome)

/* Verifica todas as invariantes descritas em combo.h. */
static int combo_consistente(const Combo *c)
{
    if (c->tamanho == 0)
        return c->inicio == NULL && c->atual == NULL;
    if (c->inicio == NULL || c->atual == NULL || c->tamanho < 0 || c->tamanho > COMBO_MAX)
        return 0;

    int achou_atual = 0;
    const NoCombo *n = c->inicio;
    for (int i = 0; i < c->tamanho; i++) {
        if (n == NULL || n->prox == NULL || n->ant == NULL) return 0;
        if (n->prox->ant != n || n->ant->prox != n) return 0;
        if (n == c->atual) achou_atual = 1;
        n = n->prox;
    }
    return n == c->inicio && achou_atual;
}

/* Golpe do nó inicio e do nó antes dele (o último), sem quebrar se vazio. */
static TipoGolpe golpe_do_inicio(const Combo *c)
{
    return c->inicio ? c->inicio->golpe : GOLPE_QTD;
}

static TipoGolpe golpe_do_ultimo(const Combo *c)
{
    return (c->inicio && c->inicio->ant) ? c->inicio->ant->golpe : GOLPE_QTD;
}

/* Monta um combo com os golpes do vetor, inserindo sempre no fim. */
static void montar(Combo *c, const TipoGolpe *golpes, int n)
{
    combo_iniciar(c);
    for (int i = 0; i < n; i++)
        combo_inserir(c, c->tamanho, golpes[i]);
}

/* Confere a sequência de golpes a partir do início. */
static int sequencia_igual(const Combo *c, const TipoGolpe *esperado, int n)
{
    if (c->tamanho != n) return 0;
    for (int i = 0; i < n; i++)
        if (combo_golpe_em(c, i) != esperado[i]) return 0;
    return 1;
}

static const TipoGolpe QUATRO[4] = {
    GOLPE_EMPURRAO, GOLPE_LEVANTADOR, GOLPE_TAPA_LATERAL, GOLPE_BARRIGADA
};

static void teste_iniciar_vazio(void)
{
    TESTE("combo novo fica vazio");
    Combo c;
    combo_iniciar(&c);
    VERIFICA(c.inicio == NULL);
    VERIFICA(c.atual == NULL);
    VERIFICA(c.tamanho == 0);
    VERIFICA(combo_golpe_atual(&c) == GOLPE_QTD);
    VERIFICA(combo_indice_atual(&c) == -1);
    VERIFICA(combo_golpe_em(&c, 0) == GOLPE_QTD);
}

static void teste_um_no_aponta_para_si(void)
{
    TESTE("com 1 golpe, o no aponta para ele mesmo nos dois sentidos");
    Combo c;
    combo_iniciar(&c);
    VERIFICA(combo_inserir(&c, 0, GOLPE_INVESTIDA));
    VERIFICA(c.tamanho == 1);
    VERIFICA(c.inicio != NULL);
    VERIFICA(c.inicio != NULL && c.inicio->prox == c.inicio);
    VERIFICA(c.inicio != NULL && c.inicio->ant == c.inicio);
    VERIFICA(c.atual == c.inicio);
    VERIFICA(combo_golpe_atual(&c) == GOLPE_INVESTIDA);
    VERIFICA(combo_consistente(&c));
    combo_liberar(&c);
}

static void teste_inserir_no_fim(void)
{
    TESTE("inserir 4 golpes no fim mantem a ordem e a circularidade");
    Combo c;
    montar(&c, QUATRO, 4);
    VERIFICA(c.tamanho == 4);
    VERIFICA(sequencia_igual(&c, QUATRO, 4));
    VERIFICA(c.atual == c.inicio);                 /* atual = 1o inserido */
    VERIFICA(golpe_do_ultimo(&c) == GOLPE_BARRIGADA); /* ant do 1o = ultimo */
    VERIFICA(c.inicio != NULL && c.inicio->ant->prox == c.inicio);        /* prox do ultimo = 1o */
    VERIFICA(combo_consistente(&c));
    combo_liberar(&c);
}

static void teste_combo_cheio(void)
{
    TESTE("nao insere alem de COMBO_MAX");
    Combo c;
    montar(&c, QUATRO, 4);
    VERIFICA(!combo_inserir(&c, 4, GOLPE_INVESTIDA));
    VERIFICA(!combo_inserir(&c, 0, GOLPE_INVESTIDA));
    VERIFICA(c.tamanho == COMBO_MAX);
    VERIFICA(sequencia_igual(&c, QUATRO, 4));
    VERIFICA(combo_consistente(&c));
    combo_liberar(&c);
}

static void teste_inserir_invalido(void)
{
    TESTE("posicao ou golpe invalido nao altera o combo");
    Combo c;
    montar(&c, QUATRO, 2);
    VERIFICA(!combo_inserir(&c, -1, GOLPE_INVESTIDA));
    VERIFICA(!combo_inserir(&c, 3, GOLPE_INVESTIDA));   /* tamanho = 2; 3 > 2 */
    VERIFICA(!combo_inserir(&c, 0, GOLPE_QTD));
    VERIFICA(!combo_inserir(&c, 0, (TipoGolpe)-1));
    VERIFICA(c.tamanho == 2);
    VERIFICA(sequencia_igual(&c, QUATRO, 2));
    VERIFICA(combo_consistente(&c));
    combo_liberar(&c);
}

static void teste_inserir_no_inicio_e_meio(void)
{
    TESTE("inserir no inicio e no meio");
    Combo c;
    combo_iniciar(&c);
    combo_inserir(&c, 0, GOLPE_TAPA_LATERAL);         /* [T]       */
    combo_inserir(&c, 0, GOLPE_EMPURRAO);             /* [E T]     */
    combo_inserir(&c, 2, GOLPE_BARRIGADA);            /* [E T B]   */
    combo_inserir(&c, 1, GOLPE_LEVANTADOR);           /* [E L T B] */
    VERIFICA(sequencia_igual(&c, QUATRO, 4));
    VERIFICA(golpe_do_inicio(&c) == GOLPE_EMPURRAO);
    VERIFICA(combo_consistente(&c));
    combo_liberar(&c);
}

static void teste_inserir_nao_move_atual(void)
{
    TESTE("inserir em combo nao vazio mantem o atual no mesmo no");
    Combo c;
    montar(&c, QUATRO, 3);                            /* [E L T] */
    combo_avancar(&c);                                /* atual = L */
    NoCombo *antes = c.atual;
    combo_inserir(&c, 0, GOLPE_BARRIGADA);            /* [B E L T] */
    VERIFICA(c.atual == antes);
    VERIFICA(combo_golpe_atual(&c) == GOLPE_LEVANTADOR);
    VERIFICA(combo_indice_atual(&c) == 2);
    VERIFICA(combo_consistente(&c));
    combo_liberar(&c);
}

static void teste_avancar_da_a_volta(void)
{
    TESTE("avancar 5 vezes em 4 golpes volta ao primeiro (circular)");
    Combo c;
    montar(&c, QUATRO, 4);
    TipoGolpe vistos[6];
    for (int i = 0; i < 6; i++) {
        vistos[i] = combo_golpe_atual(&c);
        combo_avancar(&c);
    }
    VERIFICA(vistos[0] == GOLPE_EMPURRAO);
    VERIFICA(vistos[1] == GOLPE_LEVANTADOR);
    VERIFICA(vistos[2] == GOLPE_TAPA_LATERAL);
    VERIFICA(vistos[3] == GOLPE_BARRIGADA);
    VERIFICA(vistos[4] == GOLPE_EMPURRAO);
    VERIFICA(vistos[5] == GOLPE_LEVANTADOR);
    VERIFICA(combo_consistente(&c));
    combo_liberar(&c);
}

static void teste_indice_e_reiniciar(void)
{
    TESTE("indice do atual e reiniciar");
    Combo c;
    montar(&c, QUATRO, 4);
    VERIFICA(combo_indice_atual(&c) == 0);
    combo_avancar(&c);
    combo_avancar(&c);
    VERIFICA(combo_indice_atual(&c) == 2);
    combo_avancar(&c);
    combo_avancar(&c);
    VERIFICA(combo_indice_atual(&c) == 0);            /* deu a volta */
    combo_avancar(&c);
    combo_reiniciar(&c);
    VERIFICA(c.atual == c.inicio);
    VERIFICA(combo_indice_atual(&c) == 0);
    combo_liberar(&c);
}

static void teste_avancar_e_reiniciar_vazio(void)
{
    TESTE("avancar e reiniciar com combo vazio nao quebram");
    Combo c;
    combo_iniciar(&c);
    combo_avancar(&c);
    combo_reiniciar(&c);
    VERIFICA(c.atual == NULL);
    VERIFICA(combo_consistente(&c));
}

static void teste_golpe_em(void)
{
    TESTE("golpe_em com posicoes validas e invalidas");
    Combo c;
    montar(&c, QUATRO, 4);
    VERIFICA(combo_golpe_em(&c, 0) == GOLPE_EMPURRAO);
    VERIFICA(combo_golpe_em(&c, 3) == GOLPE_BARRIGADA);
    VERIFICA(combo_golpe_em(&c, -1) == GOLPE_QTD);
    VERIFICA(combo_golpe_em(&c, 4) == GOLPE_QTD);
    combo_liberar(&c);
}

static void teste_remover_meio(void)
{
    TESTE("remover do meio religa os vizinhos");
    Combo c;
    montar(&c, QUATRO, 4);
    VERIFICA(combo_remover(&c, 1));                   /* [E T B] */
    TipoGolpe esperado[3] = { GOLPE_EMPURRAO, GOLPE_TAPA_LATERAL, GOLPE_BARRIGADA };
    VERIFICA(sequencia_igual(&c, esperado, 3));
    VERIFICA(combo_consistente(&c));
    combo_liberar(&c);
}

static void teste_remover_inicio(void)
{
    TESTE("remover o inicio passa o inicio para o proximo");
    Combo c;
    montar(&c, QUATRO, 4);
    VERIFICA(combo_remover(&c, 0));                   /* [L T B] */
    VERIFICA(golpe_do_inicio(&c) == GOLPE_LEVANTADOR);
    VERIFICA(golpe_do_ultimo(&c) == GOLPE_BARRIGADA);
    VERIFICA(c.tamanho == 3);
    VERIFICA(combo_consistente(&c));
    combo_liberar(&c);
}

static void teste_remover_ultimo(void)
{
    TESTE("remover o ultimo fecha o circulo no penultimo");
    Combo c;
    montar(&c, QUATRO, 4);
    VERIFICA(combo_remover(&c, 3));                   /* [E L T] */
    VERIFICA(golpe_do_ultimo(&c) == GOLPE_TAPA_LATERAL);
    VERIFICA(sequencia_igual(&c, QUATRO, 3));
    VERIFICA(combo_consistente(&c));
    combo_liberar(&c);
}

static void teste_remover_atual(void)
{
    TESTE("remover o atual passa o atual para o proximo");
    Combo c;
    montar(&c, QUATRO, 4);
    combo_avancar(&c);                                /* atual = L (pos 1) */
    VERIFICA(combo_remover(&c, 1));                   /* [E T B] */
    VERIFICA(combo_golpe_atual(&c) == GOLPE_TAPA_LATERAL);
    VERIFICA(combo_consistente(&c));

    combo_avancar(&c);                                /* atual = B (ultimo) */
    VERIFICA(combo_remover(&c, 2));                   /* [E T], atual da a volta */
    VERIFICA(combo_golpe_atual(&c) == GOLPE_EMPURRAO);
    VERIFICA(combo_consistente(&c));
    combo_liberar(&c);
}

static void teste_remover_unico(void)
{
    TESTE("remover o unico golpe deixa o combo vazio");
    Combo c;
    combo_iniciar(&c);
    combo_inserir(&c, 0, GOLPE_INVESTIDA);
    VERIFICA(combo_remover(&c, 0));
    VERIFICA(c.tamanho == 0);
    VERIFICA(c.inicio == NULL);
    VERIFICA(c.atual == NULL);
    VERIFICA(combo_consistente(&c));
}

static void teste_remover_invalido(void)
{
    TESTE("remover posicao invalida ou de combo vazio retorna false");
    Combo c;
    combo_iniciar(&c);
    VERIFICA(!combo_remover(&c, 0));
    montar(&c, QUATRO, 2);
    VERIFICA(!combo_remover(&c, -1));
    VERIFICA(!combo_remover(&c, 2));
    VERIFICA(c.tamanho == 2);
    VERIFICA(combo_consistente(&c));
    combo_liberar(&c);
}

static void teste_trocar(void)
{
    TESTE("trocar com o proximo, inclusive o ultimo com o primeiro");
    Combo c;
    montar(&c, QUATRO, 4);                            /* [E L T B] */
    combo_avancar(&c);
    VERIFICA(combo_trocar_com_proximo(&c, 1));        /* [E T L B] */
    TipoGolpe e1[4] = { GOLPE_EMPURRAO, GOLPE_TAPA_LATERAL, GOLPE_LEVANTADOR, GOLPE_BARRIGADA };
    VERIFICA(sequencia_igual(&c, e1, 4));
    VERIFICA(c.atual == c.inicio);                    /* troca reinicia o atual */
    VERIFICA(combo_consistente(&c));

    VERIFICA(combo_trocar_com_proximo(&c, 3));        /* ultimo <-> primeiro: [B T L E] */
    TipoGolpe e2[4] = { GOLPE_BARRIGADA, GOLPE_TAPA_LATERAL, GOLPE_LEVANTADOR, GOLPE_EMPURRAO };
    VERIFICA(sequencia_igual(&c, e2, 4));
    VERIFICA(combo_golpe_atual(&c) == GOLPE_BARRIGADA);
    VERIFICA(combo_consistente(&c));
    combo_liberar(&c);
}

static void teste_trocar_invalido(void)
{
    TESTE("trocar com menos de 2 golpes ou posicao invalida retorna false");
    Combo c;
    combo_iniciar(&c);
    VERIFICA(!combo_trocar_com_proximo(&c, 0));
    combo_inserir(&c, 0, GOLPE_EMPURRAO);
    VERIFICA(!combo_trocar_com_proximo(&c, 0));
    combo_inserir(&c, 1, GOLPE_LEVANTADOR);
    VERIFICA(!combo_trocar_com_proximo(&c, -1));
    VERIFICA(!combo_trocar_com_proximo(&c, 2));
    VERIFICA(sequencia_igual(&c, QUATRO, 2));
    VERIFICA(combo_consistente(&c));
    combo_liberar(&c);
}

static void teste_liberar_e_reusar(void)
{
    TESTE("liberar deixa vazio, pode liberar de novo e reusar");
    Combo c;
    montar(&c, QUATRO, 4);
    combo_liberar(&c);
    VERIFICA(c.tamanho == 0);
    VERIFICA(c.inicio == NULL);
    VERIFICA(c.atual == NULL);
    combo_liberar(&c);                                /* vazio: nao pode quebrar */
    VERIFICA(combo_inserir(&c, 0, GOLPE_INVESTIDA));
    VERIFICA(c.tamanho == 1);
    VERIFICA(combo_consistente(&c));
    combo_liberar(&c);
}

int main(void)
{
    printf("Testes do combo (lista circular duplamente encadeada)\n");

    teste_iniciar_vazio();
    teste_um_no_aponta_para_si();
    teste_inserir_no_fim();
    teste_combo_cheio();
    teste_inserir_invalido();
    teste_inserir_no_inicio_e_meio();
    teste_inserir_nao_move_atual();
    teste_avancar_da_a_volta();
    teste_indice_e_reiniciar();
    teste_avancar_e_reiniciar_vazio();
    teste_golpe_em();
    teste_remover_meio();
    teste_remover_inicio();
    teste_remover_ultimo();
    teste_remover_atual();
    teste_remover_unico();
    teste_remover_invalido();
    teste_trocar();
    teste_trocar_invalido();
    teste_liberar_e_reusar();

    printf("\n%d verificacoes, %d falhas\n", verificacoes, falhas);
    if (falhas == 0) {
        printf("TODOS OS TESTES PASSARAM\n");
        return 0;
    }
    return 1;
}
