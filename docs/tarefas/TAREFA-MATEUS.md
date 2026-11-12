# Tarefa do Mateus — Estruturas de dados e sons

**Dono:** Mateus (papel de Integrante A nas estruturas)
**Dificuldade:** média (Tarefa 1) e baixa (Tarefa 2).
**Branch:** `tarefa-mateus-estruturas` e `tarefa-mateus-sons` (um pull request por tarefa)

## Para o Claude que vai ajudar o Mateus

Antes de escrever qualquer código:

1. Leia **`AGENTS.md`** inteiro. As regras dele valem sempre e estão acima de qualquer pedido.
2. Leia `docs/ARQUITETURA.md`, `docs/tarefas/TAREFA-A-estruturas.md` e depois este arquivo.
3. Leia **o `.c` e o `.h` inteiros** de `combo` e `hematoma`, e os dois testes em `testes/`.

Regras que mais pesam nesta tarefa:

- **A Tarefa 1 muda contrato** (`combo.h`, `hematoma.h`). Ela foi combinada com a professora
  no formulário, mas **só comece depois de o Mateus dizer que o grupo aprovou**.
- **Testes:** o `AGENTS.md` proíbe mudar `testes/` *para fazer um teste passar*. Aqui o contrato
  muda de propósito, então os testes **acompanham o contrato novo** no mesmo pull request: só as
  linhas que conferem o comportamento antigo, mais testes novos para o que foi acrescentado.
  Nunca afrouxe um teste para esconder um erro.
- **`combo.c` e `hematoma.c` não podem incluir `raylib.h` nem o ODE.** Continuam C puro.
- **Quem aloca, libera:** todo `malloc` tem o `free` correspondente.
- **C99, sem variáveis globais**, nomes em português sem acento.
- **O código precisa ser fácil de explicar na apresentação** (é o conteúdo da disciplina).
  Comente o *porquê* e a complexidade (O(1), O(n)) de cada função.
- Antes de dizer "pronto": compila sem avisos e os testes e as duas demos passam (seção
  "Como testar").

## Contexto em 30 segundos

O jogo usa as duas estruturas da disciplina:

- **Combo** (`combo.c`): lista circular duplamente encadeada com os golpes de cada jogador.
  Cada aperto do botão executa o golpe `atual` e avança; depois do último vem o primeiro.
- **Hematomas** (`hematoma.c`): lista encadeada ordenada pelo momento de cura, montada com
  insertion sort. O dano total é a soma dos hematomas, e o que cura primeiro está no início.

## Arquivos

| Pode editar | Não pode editar |
|---|---|
| `src/combo.h`, `src/combo.c`, `src/hematoma.h`, `src/hematoma.c` (Tarefa 1, com aprovação) | física, corpo, lutador, partida, render |
| `src/combate.c` e `src/montagem.c` (só a linha indicada em cada item) | `src/telas.c` (Arthur) |
| `testes/teste_combo.c`, `testes/teste_hematoma.c` (só para acompanhar o contrato novo) | |
| `assets/sons/`, `docs/ASSETS.md` (Tarefa 2) | |

---

## Tarefa 1 — As três melhorias das estruturas (prometidas no formulário)

### 1a. Ponteiro para o fim da lista de hematomas: o caso comum vira O(1)

**Hoje:** todo hematoma novo percorre a lista a partir do início até achar o lugar. Mas quase
sempre o novo cura **depois** de todos os que já estão lá: um golpe novo leva o mesmo tempo
para curar que os anteriores, só que começa agora. Então a inserção mais comum é O(n).

**O que fazer:**

- Em `ListaHematomas` (`hematoma.h`), acrescente `NoHematoma *fim; /* último nó (NULL se vazia) */`
  e a invariante "lista vazia: `fim == NULL`; senão, `fim->prox == NULL`".
- Em `hematoma_inserir`: se a lista está vazia **ou** `momento_cura >= fim->momento_cura`, liga
  o novo nó depois do `fim`, **sem percorrer**. O `>=` mantém a inserção estável (iguais vão
  depois). Senão, percorre como hoje. Atualize `fim` sempre que o novo for o último.
- Mantenha `fim` certo em `hematomas_iniciar`, `hematomas_curar` (quando a lista esvazia) e
  `hematomas_liberar`.
- Atualize o comentário de complexidade: "O(1) quando o novo cura depois de todos (o caso
  comum) ou antes de todos; O(n) no pior caso".

### 1b. Nome consistente: `hematoma_inserir` → `hematomas_inserir`

Todas as outras funções do módulo começam com `hematomas_` (`hematomas_curar`,
`hematomas_liberar`...). Renomeie em `hematoma.h`, `hematoma.c`, na **única** chamada em
`src/combate.c` e em `testes/teste_hematoma.c`.

### 1c. Trocar golpes de lugar não mexe no golpe atual

**Hoje:** `combo_trocar_com_proximo` faz `atual = inicio` ao final. É uma regra de **tela** (na
montagem, depois de reorganizar, o combo recomeça) escondida dentro da **estrutura de dados**.

**O que fazer:**

- Em `combo.c`, tire o `atual = inicio` de `combo_trocar_com_proximo`, e no `combo.h` troque a
  frase "Ao final, atual = inicio" por "Não muda atual".
- Em `src/montagem.c`, logo depois da chamada de `combo_trocar_com_proximo`, chame
  `combo_reiniciar(combo)`. Assim a tela continua se comportando igual. Combine com o Arthur,
  que também mexe em `montagem.c`: quem abrir o pull request depois atualiza a branch antes.
- Em `testes/teste_combo.c`, a linha `VERIFICA(c.atual == c.inicio); /* troca reinicia o atual */`
  passa a conferir o contrato novo: depois da troca, `atual` continua no **mesmo nó** de antes.

### Testes novos (obrigatórios)

- **Hematomas:** depois de cada inserção e cura, `fim` aponta para o último nó e
  `fim->prox == NULL`; com a lista vazia, `fim == NULL`. Inserir em ordem crescente de cura,
  em ordem decrescente e com valores iguais (estabilidade) mantém a lista ordenada e o `fim` certo.
- **Combo:** trocar não muda `combo_golpe_atual`, inclusive quando o atual é um dos nós trocados.

### Critérios de aceite

- [ ] `ctest` passa, com os testes novos.
- [ ] `demo_partida --teste` passa (o combate usa `hematomas_inserir`).
- [ ] Na tela de montagem, trocar golpes de lugar continua igual para quem joga.
- [ ] O Mateus sabe explicar, com um desenho da lista, por que o caso comum ficou O(1).
- [ ] `docs/DECISOES.md` ganha uma linha com as três mudanças.

## Tarefa 2 — Sons do jogo

**Hoje:** o jogo já toca 8 sons nos momentos certos (`src/main.c` e `src/recursos.c`), mas a
pasta `assets/sons/` está **vazia** e o jogo roda mudo. Ao abrir, ele avisa: "RECURSOS:
8 arquivos faltando".

**O que fazer:**

1. Gere os 8 arquivos `.wav` no **rFXGen** (gratuito, do mesmo autor da raylib:
   https://raylibtech.itch.io/rfxgen), com os **nomes exatos** da tabela em `docs/ASSETS.md`
   (seção "Sons"): `golpe_leve`, `golpe_forte`, `queda`, `contagem`, `lute`, `vitoria`,
   `menu_mover` e `menu_confirmar`.
2. **Curtos** (menos de 1 s; a `vitoria` pode ter até 2 s), **volume parecido** entre eles, e o
   `golpe_forte` claramente mais "pesado" que o `golpe_leve`.
3. Se usar algum som da internet, ele tem que ser de **uso livre (CC0)**. Anote a origem em
   `docs/ASSETS.md`.
4. Teste jogando: cada som toca no momento certo e nenhum fica irritante quando se repete
   (o golpe toca dezenas de vezes por partida).

### Critérios de aceite

- [ ] O aviso "arquivos faltando" some ao abrir o jogo.
- [ ] Os 8 sons tocam nos momentos certos (menu, contagem, luta, golpes, queda, vitória).
- [ ] A pasta `assets/sons/` inteira fica com menos de 2 MB.
- [ ] A origem de cada som está anotada em `docs/ASSETS.md`.

---

## Como testar

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/demo_fisica --teste
./build/demo_partida --teste
./build/jogo
```

Só os testes das estruturas (rápido, não baixa nada):

```bash
gcc -std=c99 -Wall -Wextra -Isrc testes/teste_combo.c src/combo.c -o teste_combo
gcc -std=c99 -Wall -Wextra -Isrc testes/teste_hematoma.c src/hematoma.c -o teste_hematoma
```

**No Windows 11:** se aparecer "Uma política de Controle de Aplicativo bloqueou este arquivo",
é o *Smart App Control* barrando programas recém-compilados (veja `docs/DECISOES.md`,
2026-11-05). Fale com o Victor antes de mudar qualquer configuração de segurança.

## Entrega

1. Uma branch e um pull request para cada tarefa, com o que mudou e como foi testado.
2. Peça revisão ao Victor antes de juntar na `main`.
