# Tarefa A — Estruturas de dados (combo e hematomas)

**Dono:** Integrante A
**Dificuldade:** baixa a média. C puro, sem gráfico e sem física.
**Prazo sugerido:** fim da semana 1. O resto do jogo depende disso.

## Situação atual (MVP pronto)

`combo.c` e `hematoma.c` **já estão implementados e passam em todos os testes**. O resto
deste arquivo continua valendo como especificação. Sua tarefa agora é:

1. **Entender e saber explicar** cada função das duas listas (a professora vai perguntar).
   Leia o código linha por linha com a especificação do `.h` ao lado.
2. **Escrever `docs/ESTRUTURAS.md`** (seção 3 abaixo), com os desenhos das listas.
3. **Acrescentar testes** que você achar que faltam em `testes/` (só acrescentar; não mudar
   os que existem sem falar com o grupo). Ideias: inserir e remover alternado muitas vezes;
   trocar com o próximo duas vezes seguidas volta ao original.
4. **Revisar os pull requests** que mexerem nas listas.

## Contexto em 30 segundos

O jogo é um sumô 3D em C. Antes da luta, cada jogador monta um **combo** de 4 golpes;
na luta, cada aperto do botão A executa o golpe atual e passa para o próximo, e depois
do último volta ao primeiro. Cada golpe que acerta deixa um **hematoma** no adversário,
que cura sozinho depois de um tempo. Essas duas estruturas são **o conteúdo da disciplina
que a professora vai avaliar**, então precisam estar corretas e você precisa saber explicá-las.

## Arquivos

| Pode editar | Não pode editar |
|---|---|
| `src/combo.c` | qualquer `.h` |
| `src/hematoma.c` | `testes/` |
| `docs/ESTRUTURAS.md` (criar) | qualquer outro arquivo de `src/` |

## O que fazer

### 1. `src/combo.c` — lista circular duplamente encadeada

Implementar todas as funções declaradas em `src/combo.h`. O comentário de cada função
no `.h` é a especificação exata (o que fazer, o que retornar, os casos de erro).
Pontos que costumam dar errado:

- **Lista circular não termina em NULL.** Para percorrer, conte até `tamanho`.
  Um `while (n != NULL)` vira laço infinito.
- **1 nó** aponta para ele mesmo nos dois sentidos (`prox` e `ant`).
- **Remover o `inicio`** ou o **`atual`**: os dois ponteiros precisam andar para o próximo.
- **Remover o último nó restante**: `inicio` e `atual` viram `NULL`, `tamanho` vira 0.
- **Inserir não muda o `atual`**, exceto quando o combo estava vazio.
- `combo_trocar_com_proximo` no último troca com o primeiro, e no fim faz `atual = inicio`.
- Sempre `free` o nó removido. `combo_liberar` precisa funcionar com o combo vazio.
- Retornar `false` **sem alterar nada** nos casos inválidos.

Sugestão: escreva uma função auxiliar `static NoCombo *no_na_posicao(const Combo *c, int pos)`
e use em `inserir`, `remover`, `trocar` e `golpe_em`.

### 2. `src/hematoma.c` — lista ordenada + insertion sort

Implementar todas as funções de `src/hematoma.h`. Pontos que costumam dar errado:

- **Inserção é o insertion sort:** ande pela lista até achar o primeiro nó com
  `momento_cura` **maior** que o do novo e insira antes dele. Usar "maior" (e não
  "maior ou igual") é o que deixa a ordenação **estável**: empates ficam na ordem de chegada.
- Inserir no **início** (lista vazia ou novo cura antes de todos) muda `l->inicio`.
- `dano_total` é **atualizado** a cada inserção e remoção (não recalcule somando tudo).
  Quando a lista ficar vazia, zere `dano_total` (evita resto de arredondamento de `float`).
- `hematomas_curar` remove **só do início**, enquanto `momento_cura <= agora`.
- Recusar (`false`) dano `<= 0` e lista cheia (`HEMATOMAS_MAX`).

### 3. `docs/ESTRUTURAS.md` — explicação para a apresentação

Uma página, em português simples, para o grupo estudar antes da apresentação:

1. O que é uma lista circular duplamente encadeada e por que o combo usa uma
   (circular = o combo se repete; encadeada = inserir/remover sem deslocar;
   dupla = mover golpes e achar o anterior sem percorrer tudo).
2. Desenho em texto (ASCII) de um combo de 4 golpes com as setas `prox` e `ant`.
3. Passo a passo de `combo_remover` no meio, com desenho antes/depois.
4. O que é insertion sort, por que é o melhor caso aqui (cada hematoma chega numa
   lista já ordenada) e o que é estabilidade.
5. Exemplo: hematomas com cura em 8 s, 3 s e 5 s entrando nessa ordem, mostrando
   a lista depois de cada inserção.
6. Complexidade de cada função (está nos comentários do `.h`).

## Como testar

```bash
cmake -S . -B build -DSUMO_SO_TESTES=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Sem CMake:

```bash
gcc -std=c99 -Wall -Wextra -Isrc testes/teste_combo.c src/combo.c -o teste_combo
./teste_combo
gcc -std=c99 -Wall -Wextra -Isrc testes/teste_hematoma.c src/hematoma.c -o teste_hematoma
./teste_hematoma
```

Hoje os testes compilam e **falham** (as funções estão vazias). Seu trabalho está pronto
quando os dois mostram `TODOS OS TESTES PASSARAM`.

## Critérios de aceite

- [ ] `teste_combo` passa inteiro.
- [ ] `teste_hematoma` passa inteiro.
- [ ] Compila sem nenhum aviso com `-Wall -Wextra`.
- [ ] `combo.c` e `hematoma.c` não incluem `raylib.h` nem `ode/ode.h`.
- [ ] Nenhum `.h` e nenhum arquivo de `testes/` foi alterado.
- [ ] Todo `malloc` tem `free` correspondente.
- [ ] `docs/ESTRUTURAS.md` escrito, e você consegue explicar cada função sem olhar o código.

## Prompt para colar na sua IA

> Estou num projeto de faculdade em C99. Leia `AGENTS.md`, depois `docs/ARQUITETURA.md`,
> depois `docs/tarefas/TAREFA-A-estruturas.md`, depois `src/combo.h`, `src/hematoma.h`,
> `src/golpe.h`, `src/config.h`, `testes/teste_combo.c` e `testes/teste_hematoma.c`.
> Minha tarefa é implementar `src/combo.c` (lista circular duplamente encadeada) e
> `src/hematoma.c` (lista encadeada ordenada com insertion sort), seguindo exatamente
> os comentários dos `.h`. Não altere nenhum `.h` nem os testes. Implemente uma função
> por vez, explicando o raciocínio de cada uma para eu aprender, compile com
> `gcc -std=c99 -Wall -Wextra -Isrc` e rode os testes até todos passarem.
> Depois escreva `docs/ESTRUTURAS.md` conforme a seção 3 da tarefa.
