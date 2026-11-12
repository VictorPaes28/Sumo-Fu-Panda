# Tarefa do Arthur — Telas de seleção e de montagem

**Dono:** Arthur (papel de Integrante B nas telas)
**Dificuldade:** média. raylib 2D, sem física.
**Branch:** `tarefa-arthur-telas` (um pull request por tarefa)

## Para o Claude que vai ajudar o Arthur

Antes de escrever qualquer código:

1. Leia **`AGENTS.md`** inteiro. As regras dele valem sempre e estão acima de qualquer pedido.
2. Leia `docs/ARQUITETURA.md` e depois este arquivo.
3. Leia **o `.c` inteiro** de cada arquivo que for mudar (`src/telas.c`, `src/montagem.c`) e os
   `.h` correspondentes, que são o contrato.

Regras que mais pesam nesta tarefa:

- **Só edite os arquivos da tabela "Pode editar"** abaixo. Precisou de outro arquivo? Pare e
  peça ao Arthur para combinar com o dono.
- **Não altere nenhum `.h`** sem o Arthur dizer explicitamente que **o grupo aprovou**.
  As duas tarefas cabem sem mudar `.h`.
- **Funções de desenho nunca alteram estado** (`*_desenhar` recebe `const`).
- **Números ajustáveis** (tamanhos, cores, quantos itens cabem) vão em `src/config.h` ou em
  `static const` no topo do `.c`, nunca soltos no meio do código.
- **C99, sem variáveis globais**, nomes em português sem acento, comentários dizendo o *porquê*.
- **O código precisa ser fácil de explicar na apresentação.** Prefira funções pequenas, com
  nome que diz o que fazem, a um bloco grande e esperto.
- Antes de dizer "pronto": compila sem avisos, os testes e as duas demos passam (seção
  "Como testar") e os critérios de aceite foram conferidos um por um.

## Contexto em 30 segundos

Sumô 3D em C (raylib + ODE). Antes da luta, cada jogador **escolhe o personagem** (tela de
seleção, `telas.c`) e **monta o combo** de 4 golpes (tela de montagem, `montagem.c`). O combo é
a lista circular duplamente encadeada da disciplina (`combo.h`). O jogo cresceu, e as duas telas
ficaram pequenas para ele: agora são **4 personagens** e **11 golpes**.

## Arquivos

| Pode editar | Não pode editar |
|---|---|
| `src/telas.c` (só a parte da seleção) | qualquer `.h` (sem aprovação do grupo) |
| `src/montagem.c` | `src/combo.c`, `src/hematoma.c` (Mateus) |
| `src/config.h` (só acrescentar números das telas) | física, corpo, lutador, combate, partida, render |
| `demos/demo_telas.c` | `testes/` |

---

## Tarefa 1 — Seleção de personagens com os 4 lutadores

**Hoje:** cada jogador vê um carrossel `< Nome >` com três barras (peso, velocidade,
equilíbrio). Com 4 personagens, ninguém sabe quem existe sem ficar girando, e as barras usam
escalas fixas que já não servem: o Minotauro tem 113 kg e passa da barra de "Peso", que vai
até 110 kg; e a barra de "Equilíbrio" mostra a força total, que hoje é igual por quilo em
todos os personagens.

**O que fazer** (`selecao_desenhar` e funções auxiliares em `telas.c`):

1. **Grade com os 4 personagens** em cada lado da tela, um cartão por personagem, com o
   cartão escolhido destacado na cor do jogador. O controle continua igual: `direcional_x`
   troca de personagem, A confirma e B volta (o comportamento está no `telas.h` e não muda).
2. **Ficha do personagem escolhido**, com barras que comparam os personagens entre si:
   - **Peso:** massa total; a barra cheia é o personagem mais pesado do catálogo (calcule o
     máximo percorrendo `personagem_dados`, sem número fixo);
   - **Velocidade:** `velocidade_max`; cheia = o mais rápido do catálogo;
   - **Firmeza:** `forca_equilibrio`; cheia = o mais firme do catálogo.
3. **Uma frase sobre o jeito de lutar** de cada um, por exemplo "Leve e rápido, empurra forte"
   para o Tai Lung. Use uma tabela `static const` em `telas.c` indexada pelo nome do
   personagem. Se preferir pôr a frase dentro do `Personagem` (`personagem.h`), é mudança de
   contrato: só com aprovação do grupo.
4. Funciona com **qualquer quantidade** de personagens (de 1 a `MAX_PERSONAGENS`): nada de
   "4" escrito no código.

**Critérios de aceite:**

- [ ] Os 4 personagens aparecem na tela, para os dois jogadores ao mesmo tempo.
- [ ] As barras nunca passam do tamanho do painel, e a do personagem mais forte no quesito fica cheia.
- [ ] Os dois jogadores escolhem e confirmam ao mesmo tempo, com controle e com teclado.
- [ ] Com um personagem a mais no catálogo, a tela continua certa (teste de mentira: duplique uma
      entrada em `personagem.c`, confira e desfaça).
- [ ] `demo_telas --capturas` mostra a tela nova sem texto cortado ou sobreposto.

## Tarefa 2 — Montagem do combo com 11 golpes

**Hoje:** a lista do catálogo desenha um golpe a cada 46 px a partir de y = 175, e cabem uns
8 golpes. Com 11, os últimos **passam do painel** e caem por cima dos espaços do combo.

**O que fazer** (`desenhar_lado` e funções auxiliares em `montagem.c`):

1. **Lista com rolagem:** mostra só os golpes que cabem (por exemplo 7, definido em
   `static const` ou em `config.h`) e rola para acompanhar o cursor, com setas ▲ ▼ quando há
   mais golpes acima ou abaixo. O cursor continua dando a volta do último para o primeiro.
2. **Descrição do golpe selecionado** em uma caixa fixa, com quebra de linha se ela for longa.
   As descrições dos contra-ataques dizem quando usar ("Contra-ataque: use quando ele vier para
   cima."), e isso precisa aparecer inteiro.
3. **Ficha curta do golpe:** dano, stamina e se é **forte** (`Golpe.forte`) ou **segura a faixa**
   (`Golpe.agarra`), por exemplo com um selo pequeno ao lado do nome.
4. Nada muda na lógica do combo: inserir, remover, trocar e pronto continuam como estão em
   `montagem.h`.

**Critérios de aceite:**

- [ ] Os 11 golpes são acessíveis; nenhum texto sai do painel nem cobre os espaços do combo.
- [ ] A rolagem acompanha o cursor nas duas direções, inclusive na volta do 11º para o 1º.
- [ ] A descrição inteira de qualquer golpe aparece.
- [ ] Com um golpe a mais ou a menos no catálogo, a tela continua certa (a quantidade vem de `GOLPE_QTD`).
- [ ] Os dois jogadores montam o combo ao mesmo tempo, com controle e com teclado.

**Atenção:** o Mateus vai mudar **uma linha** de `montagem.c` (chamar `combo_reiniciar` depois
de `combo_trocar_com_proximo`). Combinem a ordem: quem abrir o pull request depois atualiza a
branch com a `main` antes.

---

## Como testar

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/demo_fisica --teste
./build/demo_partida --teste
./build/demo_telas                # navegar à mão
./build/demo_telas --capturas     # capturas PNG de cada tela
```

**No Windows 11:** se aparecer "Uma política de Controle de Aplicativo bloqueou este arquivo",
é o *Smart App Control* barrando programas recém-compilados (veja `docs/DECISOES.md`,
2026-11-05). Fale com o Victor antes de mudar qualquer configuração de segurança.

## Entrega

1. Uma branch por tarefa e um pull request para a `main`, com capturas antes e depois na descrição.
2. Peça revisão ao Victor antes de juntar.
3. Se uma decisão nova surgir (um número, um jeito de desenhar), registre uma linha em `docs/DECISOES.md`.
