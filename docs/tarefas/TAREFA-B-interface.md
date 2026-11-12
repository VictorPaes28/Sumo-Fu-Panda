# Tarefa B — Interface, entrada, catálogos e recursos

**Dono:** Integrante B
**Dificuldade:** baixa a média. raylib 2D, sem física.
**Prazo sugerido:** catálogos (passo 1) em 2 dias; o resto até o fim da semana 2.

## Situação atual (MVP pronto)

Todos os arquivos desta tarefa **já estão implementados** (incluindo `ui.c`, que junta as
cores e funções de desenho comuns, e `demos/demo_telas.c`). O resto deste arquivo continua
valendo como especificação. Sua tarefa agora é:

1. **Entender o código** de cada tela e saber explicar como ela usa o combo (lista circular).
2. **Testar com controle de verdade** (Xbox ou compatível) e com teclado, os dois jogadores
   ao mesmo tempo, e anotar o que ficou estranho.
3. **Polir o visual** das telas: fonte maior ou uma fonte TTF (carregar em `recursos`),
   cores, animação simples de seleção. Use `demo_telas --capturas` para comparar antes/depois.
4. **Integrar a arte**: colocar os `.glb` de cada personagem em `assets/modelos/<pasta>/`,
   trocar as medidas provisórias de `personagem.c` pelas medidas da arte (`docs/ASSETS.md`)
   e conferir no `demo_partida`.
5. **Sons**: gerar no rFXGen os `.wav` listados em `docs/ASSETS.md`.

## Contexto em 30 segundos

O jogo é um sumô 3D em C com raylib. Você faz tudo o que o jogador **vê e aperta**
fora da luta em si: ler o controle, as telas 2D (menu, seleção de personagem, montagem
do combo, resultado), o HUD por cima da luta, carregar modelos e sons, e as tabelas de
golpes e personagens. A física e o 3D da luta são do Principal; o combo e os hematomas
são do Integrante A.

## Arquivos

| Pode editar | Não pode editar |
|---|---|
| `src/esqueleto.c`, `src/golpe.c`, `src/personagem.c` | qualquer `.h` |
| `src/entrada.c`, `src/recursos.c` | `src/combo.c`, `src/hematoma.c` (Integrante A) |
| `src/telas.c`, `src/montagem.c`, `src/hud.c` | física, corpo, lutador, combate, partida, render, main (Principal) |
| `demos/demo_telas.c` (criar) | `testes/` |

## Ordem de trabalho

### Passo 1 — catálogos (entregar primeiro: o Principal precisa deles para a física)

**`src/esqueleto.c`**: tabelas `static const` para as 4 funções de `esqueleto.h`.
- Nomes das peças: `"bacia"`, `"tronco"`, `"cabeca"`, `"braco_e"`, `"braco_d"`,
  `"coxa_e"`, `"coxa_d"`, `"canela_e"`, `"canela_d"`.
- Articulações: joelhos = `ARTIC_DOBRADICA`; as outras = `ARTIC_UNIVERSAL`.
- Pai/filha de cada articulação: está no comentário do enum `Articulacao`.
- `pose_zerar`: zera todos os ângulos.

**`src/golpe.c`**: tabela `static const Golpe CATALOGO[GOLPE_QTD]`, na ordem do enum.
Valores provisórios (o Principal ajusta depois; as poses ficam zeradas por enquanto):

| Golpe | nome | descricao | parte_ataque | parte_ataque_2 | avanco |
|---|---|---|---|---|---|
| EMPURRAO | "Empurrão" | "Empurra com as duas mãos." | BRACO_D | BRACO_E | 0 |
| LEVANTADOR | "Levantador" | "Ergue o adversário por baixo." | BRACO_D | BRACO_E | 0 |
| TAPA_LATERAL | "Tapa lateral" | "Tapa rápido que desvia para o lado." | BRACO_E | QTD (nenhuma) | 0 |
| BARRIGADA | "Barrigada" | "Golpe de barriga, curto e pesado." | TRONCO | BACIA | 30 |
| INVESTIDA | "Investida" | "Avança com o corpo todo." | TRONCO | CABECA | 90 |

| Golpe | impulso | impulso_vertical | dano | tempo_cura | custo_stamina | duracao | forte |
|---|---|---|---|---|---|---|---|
| EMPURRAO | 60 | 0 | 6 | 3 | 15 | 0.35 | false |
| LEVANTADOR | 30 | 70 | 8 | 4 | 20 | 0.5 | false |
| TAPA_LATERAL | 45 | 0 | 4 | 2.5 | 10 | 0.25 | false |
| BARRIGADA | 110 | 10 | 15 | 8 | 30 | 0.6 | true |
| INVESTIDA | 140 | 0 | 12 | 6 | 35 | 0.8 | true |

As peças são os valores de `ParteCorpo` (ex.: `PARTE_BRACO_D`; "QTD" = `PARTE_QTD`).
**Valores atuais:** os números desta tabela eram o ponto de partida; os valores em uso
(já balanceados) estão em `src/golpe.c`, que também tem `impulso_lateral`, as chaves do
movimento (`chaves`, ver `docs/ANIMACOES.md`) e os tempos das fases (`tempo_preparo`,
`tempo_ativo`, `duracao`). As fases e as chaves estão explicadas no comentário de `golpe.h`.
Os textos podem ter acento: a fonte padrão da raylib tem os caracteres do português
(salve o arquivo em UTF-8).
`golpe_dados` devolve `NULL` para tipo inválido (negativo ou `>= GOLPE_QTD`).

**`src/personagem.c`**: tabela `static const Personagem CATALOGO[]` com **Po** (pasta `"po"`)
e **Tai Lung** (pasta `"tai_lung"`). Enquanto a arte não entrega as medidas (`docs/ASSETS.md`),
use para os dois este corpo genérico de 1,72 m (T-pose, pés na origem, olhando para +Z;
**esquerda do personagem = x positivo**):

| Peça | centro (x, y, z) | tamanho (x, y, z) | massa |
|---|---|---|---|
| bacia | (0, 0.93, 0) | (0.42, 0.22, 0.30) | 18 |
| tronco | (0, 1.25, 0) | (0.48, 0.42, 0.34) | 25 |
| cabeca | (0, 1.59, 0) | (0.26, 0.26, 0.26) | 5 |
| braco_e | (0.54, 1.38, 0) | (0.60, 0.13, 0.13) | 4 |
| braco_d | (−0.54, 1.38, 0) | (0.60, 0.13, 0.13) | 4 |
| coxa_e | (0.12, 0.62, 0) | (0.18, 0.40, 0.20) | 8 |
| coxa_d | (−0.12, 0.62, 0) | (0.18, 0.40, 0.20) | 8 |
| canela_e | (0.12, 0.21, 0) | (0.14, 0.42, 0.16) | 5 |
| canela_d | (−0.12, 0.21, 0) | (0.14, 0.42, 0.16) | 5 |

| Articulação | âncora (x, y, z) |
|---|---|
| cintura | (0, 1.04, 0) |
| pescoço | (0, 1.46, 0) |
| ombro_e / ombro_d | (0.24, 1.38, 0) / (−0.24, 1.38, 0) |
| quadril_e / quadril_d | (0.12, 0.82, 0) / (−0.12, 0.82, 0) |
| joelho_e / joelho_d | (0.12, 0.42, 0) / (−0.12, 0.42, 0) |

`forca_equilibrio = 400`, `forca_andar = 600`, `velocidade_max = 1.5`.
Copie o `.estilo` (jeito de se mexer) de um personagem parecido já existente em
`personagem.c` (pesado: Po; ágil: Tai Lung) e ajuste aos poucos; os campos estão
explicados em `EstiloMovimento` (`personagem.h`). Depois de mudar a base das pernas,
rode `demo_fisica --teste`.
Deixe um comentário `/* PROVISÓRIO: trocar pelas medidas da arte */` na tabela.

### Passo 2 — `src/entrada.c`

Seguir a tabela de botões do comentário de `entrada.h`. Funções da raylib úteis:
`IsGamepadAvailable`, `GetGamepadAxisMovement` (`GAMEPAD_AXIS_LEFT_X/Y`,
`GAMEPAD_AXIS_RIGHT_TRIGGER`), `IsGamepadButtonPressed` (`GAMEPAD_BUTTON_RIGHT_FACE_DOWN` = A,
`..._RIGHT` = B, `..._UP` = Y, `GAMEPAD_BUTTON_MIDDLE_RIGHT` = Start,
`GAMEPAD_BUTTON_LEFT_FACE_UP/DOWN/LEFT/RIGHT` = D-pad), `IsKeyDown`, `IsKeyPressed`.
- Zona morta: se o tamanho do vetor do analógico for menor que `ENTRADA_ZONA_MORTA`, mover = (0, 0).
- **Inverter o eixo Y** do analógico (na raylib, para cima é negativo; na `Intencao`, positivo).
- O gatilho na raylib vai de −1 (solto) a 1 (apertado): converter para 0 a 1 antes de
  comparar com `ENTRADA_LIMIAR_GATILHO`.
- No teclado, `mover` em diagonal deve ter tamanho 1 (normalizar).
- No teclado, WASD/setas também geram `direcional_x/y` com `IsKeyPressed`.

### Passo 3 — `src/recursos.c`

- Modelos: um por personagem com `Personagem.modelo`, em
  `TextFormat("%sassets/modelos/%s/%s", GetApplicationDirectory(), pasta, modelo)`,
  carregado com `pele_carregar` (o modelo inteiro com esqueleto; ver `docs/ASSETS.md`).
- Sons: antes de carregar, `FileExists`. Faltando: `tem_som` = false e `TraceLog(LOG_WARNING, ...)`.
- Tabela `static const char *` com os nomes dos sons na ordem de `SomJogo` (lista em `recursos.h`).
- `recursos_liberar` só descarrega o que foi carregado.

### Passo 4 — `src/telas.c` e `src/hud.c`

- Regras de cada tela: comentários de `telas.h` e `hud.h`.
- Desenho só com funções 2D da raylib (`DrawText`, `MeasureText`, `DrawRectangle`,
  `DrawRectangleLines`...). Centralize textos com `MeasureText`.
- Tamanho da tela: `JANELA_LARGURA` x `JANELA_ALTURA` (config.h).
- `_desenhar` **não** chama `BeginDrawing`/`EndDrawing` (o `main.c` chama) e **não** altera nada.
- Visual: fundo escuro, texto claro, uma cor de destaque por jogador
  (jogador 1 = laranja, jogador 2 = azul, por exemplo). Simples e legível de longe.

### Passo 5 — `src/montagem.c` (depende do combo do Integrante A)

- Controles: comentário de `montagem.h`. Usar **só** as funções de `combo.h`
  (`combo_inserir`, `combo_remover`, `combo_trocar_com_proximo`, `combo_golpe_em`, `tamanho`).
- Nomes e descrições dos golpes: `golpe_dados(tipo)->nome` e `->descricao`.
- Enquanto o `combo.c` do Integrante A não estiver pronto, a montagem não funciona: faça os
  outros passos antes, ou use a branch do Integrante A.

### Passo 6 — `demos/demo_telas.c`

Um `main` de teste, **sem física**, que abre a janela e passa por todas as suas telas:
MENU → SELEÇÃO → MONTAGEM → "luta falsa" → RESULTADO.
Na "luta falsa", preencha um `InfoHud` à mão, com valores que mudam com o tempo
(dano subindo, stamina descendo, contagem 3-2-1-LUTE, combo_atual girando) e chame
`hud_desenhar`. Aperte A para ir ao resultado. O CMake cria o alvo `demo_telas`
sozinho quando esse arquivo existe (rode o cmake de novo).

## Critérios de aceite

- [ ] `demo_telas` compila sem avisos e roda do menu até o resultado, com controle **e** com teclado.
- [ ] Sem controle conectado, os dois jogadores conseguem jogar pelo teclado.
- [ ] Analógico parado não move nada (zona morta) e para cima dá `mover.y > 0`.
- [ ] Na montagem: inserir, remover, trocar e "pronto" funcionam para os dois jogadores ao mesmo tempo;
      "pronto" só com 4 golpes; B com combo vazio volta para a seleção.
- [ ] Sem nenhum arquivo em `assets/`, o jogo abre e só mostra avisos no console.
- [ ] Nenhuma função `_desenhar` altera estado; nenhuma tela toca som.
- [ ] Nenhum `.h` e nenhum arquivo de outro dono foi alterado.
- [ ] Os catálogos (passo 1) foram entregues ao Principal primeiro.

## Prompt para colar na sua IA

> Estou num projeto de faculdade em C99 com raylib 6.0. Leia `AGENTS.md`, depois
> `docs/ARQUITETURA.md`, depois `docs/tarefas/TAREFA-B-interface.md`, depois os `.h`:
> `config.h`, `esqueleto.h`, `golpe.h`, `personagem.h`, `entrada.h`, `recursos.h`,
> `telas.h`, `montagem.h`, `hud.h` e `combo.h` (só para usar). Minha tarefa é implementar
> os `.c` listados na tarefa, na ordem dos passos, seguindo exatamente os comentários dos `.h`.
> Não altere nenhum `.h` nem arquivos de outros integrantes. Faça um passo por vez,
> compile sem avisos (`-Wall -Wextra`) e me diga como testar cada passo antes de seguir.
> No passo 6, crie `demos/demo_telas.c` para eu testar todas as telas sem a física.
