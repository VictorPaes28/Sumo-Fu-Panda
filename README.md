# Sumô-Fu Panda

Jogo de luta de sumô em 3D, em C, para a disciplina de Estrutura de Dados.
Dois jogadores, cada um com um controle, tentam empurrar o adversário para fora do
ringue. Os lutadores têm corpos articulados com física (como em *Sumotori Dreams*),
cada jogador monta um combo de golpes antes da luta e cada acerto deixa hematomas
que enfraquecem o equilíbrio do adversário.

**Conteúdo da disciplina no jogo:**
- **Lista circular duplamente encadeada:** o combo de golpes (`src/combo.c`).
- **Lista encadeada ordenada + insertion sort:** os hematomas (`src/hematoma.c`).

**O que tem no jogo:** partida rápida e torneio da turma; 6 personagens (Po, Tai Lung,
Oogway, Kai, Macaco e Sumo Dream, o corpo de blocos da nossa primeira versão), cada um com
peso, força e equilíbrio próprios; 3 arenas (Ginásio Imperial, Pátio de Jade, Bosque de
Bambu); 11 golpes tirados de técnicas reais de sumô; corpo com física de 13 peças; música
e efeitos sonoros.

## Equipe

| Integrante | O que fez |
|---|---|
| Victor Paes | arquitetura, física do corpo e golpes, personagens 3D, HUD, tela inicial, torneio e arenas |
| Mateus Diniz | lista circular do combo, lista ordenada de hematomas, testes e sons |
| Breno Santiago | passadas do ragdoll, golpes e equilíbrio |
| Arthur von Sohsten | telas de seleção de personagem e de montagem do combo |

## Como jogar

1. Cada jogador escolhe um lutador; depois, os dois escolhem a arena.
2. Cada um monta o seu **combo**: a sequência de golpes que vai usar.
3. Na luta, cada aperto do botão de golpe solta o **próximo golpe do combo**; depois do
   último, volta para o primeiro (é a lista circular).
4. Cada golpe que acerta deixa um **hematoma** no outro: quanto mais machucado, mais fácil
   ele perde o equilíbrio. Os hematomas saram com o tempo.
5. Ganha o round quem tirar o outro do ringue ou derrubá-lo. Golpear cansa: fique de olho
   na barra de stamina.

## Documentação

| Para entender... | Leia |
|---|---|
| Os módulos e como eles se ligam | `docs/ARQUITETURA.md` |
| Por que as coisas são como são | `docs/DECISOES.md` |
| Os golpes e as técnicas de sumô | `docs/ANIMACOES.md` |
| Como entram os modelos 3D e os sons | `docs/ASSETS.md` |
| As regras para a IA que nos ajudou | `AGENTS.md` |

## Pré-requisitos (Windows)

- Compilador **C e C++**: **MinGW-w64** com `gcc` e `g++` (por exemplo via MSYS2 UCRT64 ou
  w64devkit). Todos usam o mesmo. O `g++` é necessário porque o ODE é escrito em C++
  por dentro; o código do jogo é C99.
- **CMake** 3.20 ou mais novo.
- **Git**.
- Internet na primeira compilação (o CMake baixa a raylib 6.0 e o ODE 0.16.6).

## Como executar o jogo

Na pasta do projeto, rode os três comandos (a primeira compilação leva alguns minutos):

```bash
cmake -S . -B build
cmake --build build
./build/jogo
```

No Prompt de Comando do Windows, o último passo é `build\jogo.exe`. O jogo abre no menu:
escolha **Partida rápida** com C (jogador 1) ou K (jogador 2). Controles completos abaixo.

## Testes

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
assets/     modelos (.glb), sons e música (.wav) e fontes (.ttf)
docs/       arquitetura, decisões, guia de arte e animações
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
- Cada um edita só os arquivos de que é dono (comentário "Dono:" no início de cada arquivo).
- Uma branch por assunto; pull request revisado antes de entrar na `main`.
