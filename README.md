# Sumô-Fu Panda

Jogo de luta de sumô em 3D, em C, para a disciplina de Estrutura de Dados.
Dois jogadores, cada um com um controle, tentam empurrar o adversário para fora do
ringue. Os lutadores têm corpos articulados com física (como em *Sumotori Dreams*),
cada jogador monta um combo de golpes antes da luta e cada acerto deixa hematomas
que enfraquecem o equilíbrio do adversário.

**Conteúdo da disciplina no jogo:**
- **Lista circular duplamente encadeada:** o combo de golpes (`src/combo.c`).
- **Lista encadeada ordenada + insertion sort:** os hematomas (`src/hematoma.c`).

**Situação:** jogável — menu, seleção, montagem do combo, luta com física, rounds e
resultado. Corpo articulado de 13 peças (com cotovelos e pés), 11 golpes tirados de técnicas
reais de sumô e 4 personagens:

| Personagem | Jeito de lutar | Modelo 3D |
|---|---|---|
| Po | pesado, firme, bamboleia | ainda em blocos (`docs/ASSETS.md`) |
| Tai Lung | leve e rápido, empurra forte | pintado |
| Oogway | o mais lento, sereno, difícil de mover | pintado |
| Minotauro | o mais pesado, tronco à frente | pintado |

## Comece por aqui

| Você é... | Leia |
|---|---|
| Qualquer pessoa do grupo | `docs/ARQUITETURA.md` |
| Integrante A (estruturas) | `docs/tarefas/TAREFA-A-estruturas.md` |
| Integrante B (interface) | `docs/tarefas/TAREFA-B-interface.md` |
| Arthur (telas de seleção e montagem) | `docs/tarefas/TAREFA-ARTHUR.md` |
| Mateus (estruturas e sons) | `docs/tarefas/TAREFA-MATEUS.md` |
| Principal (física e integração) | `docs/tarefas/TAREFA-PRINCIPAL.md` |
| Equipe de arte | `docs/ASSETS.md` |
| Golpes, animações e referências de sumô | `docs/ANIMACOES.md` |
| Uma IA ajudando alguém | `AGENTS.md` |

Por que as coisas são como são: `docs/DECISOES.md`.

## Pré-requisitos (Windows)

- Compilador **C e C++**: **MinGW-w64** com `gcc` e `g++` (por exemplo via MSYS2 UCRT64 ou
  w64devkit). Todos usam o mesmo. O `g++` é necessário porque o ODE é escrito em C++
  por dentro; o código do jogo é C99.
- **CMake** 3.20 ou mais novo.
- **Git**.
- Internet na primeira compilação (o CMake baixa a raylib 6.0 e o ODE 0.16.6).

## Compilar e rodar

```bash
cmake -S . -B build
cmake --build build
./build/jogo
```

Só os testes das estruturas (rápido, não baixa nada):

```bash
cmake -S . -B build-testes -DSUMO_SO_TESTES=ON
cmake --build build-testes
ctest --test-dir build-testes --output-on-failure
```

Verificações e demonstrações (detalhes em `docs/ARQUITETURA.md`, seção 10):

```bash
./build/demo_fisica --teste
./build/demo_partida --teste
./build/demo_partida
```

**No Windows 11:** se o Windows disser "Uma política de Controle de Aplicativo bloqueou
este arquivo" ao compilar ou rodar, é o *Smart App Control* barrando programas recém-compilados
(veja `docs/DECISOES.md`, 2026-11-05).

## Estrutura

```
src/        código do jogo (.h = contrato, .c = implementação)
testes/     testes das estruturas de dados (rodam no terminal)
demos/      programas de teste de partes do jogo
ferramentas/ inspeciona_modelo (ossos de um .glb), preparar_modelo.py (FBX -> GLB, Blender)
            e extrai_animacoes (animações -> ângulos das articulações)
assets/     modelos (.glb) e sons (.wav)
docs/       arquitetura, decisões, tarefas e guia de arte
```

## Controles

| Ação | Controle | Teclado J1 | Teclado J2 |
|---|---|---|---|
| Andar | analógico esquerdo | WASD | setas |
| Golpe / confirmar | A | C | K |
| Voltar / remover | B | X | J |
| Trocar golpe de lugar (montagem) | Y | Z | U |
| Firmar | gatilho direito | V | L |
| Pausa / pronto | Start | TAB | ENTER |
| Modo de depuração | | F1 | |

## Regras do repositório

- Os `.h` são o contrato entre os integrantes: só mudam com acordo do grupo
  e atualização de `docs/ARQUITETURA.md` no mesmo commit.
- Cada um edita só os arquivos da sua tarefa.
- Uma branch por tarefa; pull request revisado antes de entrar na `main`.
