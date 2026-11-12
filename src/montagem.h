/*
 * montagem.h — tela de montagem do combo.
 *
 * Tela dividida ao meio: jogador 1 à esquerda, jogador 2 à direita.
 * Cada lado mostra o catálogo de golpes (lista vertical, com nome e
 * descrição) e os COMBO_MAX espaços do combo (linha horizontal).
 *
 * Controles de cada jogador (só enquanto não estiver pronto):
 *   direcional_y  -> move cursor_catalogo (0 a GOLPE_QTD-1, com volta)
 *   direcional_x  -> move cursor_combo (0 a COMBO_MAX-1, sem volta)
 *   a             -> insere o golpe do catálogo na posição cursor_combo
 *                    (combo_inserir; se cursor_combo > tamanho, insere no fim);
 *                    com o combo cheio, SUBSTITUI o golpe do cursor
 *                    (combo_remover + combo_inserir na mesma posição)
 *   b             -> remove o golpe na posição cursor_combo (combo_remover);
 *                    com o combo vazio, devolve MONTAGEM_VOLTAR
 *   y             -> troca o golpe em cursor_combo com o próximo
 *                    (combo_trocar_com_proximo) e o cursor acompanha o golpe
 *   start         -> pronto, só se o combo estiver cheio (COMBO_MAX golpes)
 * Pronto: start de novo desfaz. Os dois prontos -> MONTAGEM_CONCLUIDA.
 *
 * O combo de cada jogador NÃO é criado aqui: ele pertence ao Jogador
 * (partida.h) e já pode vir preenchido de uma partida anterior.
 *
 * Dono: Integrante B.
 */
#ifndef MONTAGEM_H
#define MONTAGEM_H

#include <stdbool.h>
#include "combo.h"
#include "entrada.h"

typedef struct {
    int  cursor_catalogo[2];
    int  cursor_combo[2];
    bool pronto[2];
} TelaMontagem;

typedef enum { MONTAGEM_CONTINUA, MONTAGEM_CONCLUIDA, MONTAGEM_VOLTAR } AcaoMontagem;

void         montagem_iniciar(TelaMontagem *t);
/* combo_j1 e combo_j2 são os combos dos jogadores 1 e 2 (Jogador.combo). */
AcaoMontagem montagem_atualizar(TelaMontagem *t, Combo *combo_j1, Combo *combo_j2,
                                const Intencao in[2]);
void         montagem_desenhar(const TelaMontagem *t, const Combo *combo_j1,
                               const Combo *combo_j2);

#endif /* MONTAGEM_H */
