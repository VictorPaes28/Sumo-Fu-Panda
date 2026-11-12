# Tarefa Principal — física, luta e integração

**Dono:** Principal (líder técnico)
**Dificuldade:** alta. É o coração do jogo e o maior risco.

## Arquivos

`src/fisica.c`, `src/corpo.c`, `src/lutador.c`, `src/combate.c`, `src/partida.c`,
`src/render.c`, `src/main.c`, `CMakeLists.txt`, valores finais de `golpe.c` e `personagem.c`
(combinar com o Integrante B), e a revisão dos pull requests dos outros.

O `main.c` já está escrito (fluxo de telas, pausa, sons). Os outros estão vazios, com a
especificação nos `.h`.

## Situação atual (MVP pronto)

Todos os módulos estão implementados e o jogo é jogável do menu ao resultado.
Resultado das verificações automáticas na data do MVP:

- `demo_fisica --teste`: **FISICA OK** — fica de pé 30 s (bacia entre 0,86 e 0,87 m), anda,
  olha para o adversário, empurrão leve não derruba sem dano mas derruba com dano máximo,
  todos os sinais das articulações conferem, contatos registrados.
- `demo_partida --teste`: **PARTIDA OK** — 20 partidas robô contra robô, todas terminaram,
  média de 49 s por partida, hematomas sobem e curam.
- Testes das listas: 100% passando. Compilação sem avisos.

Próximos passos do Principal:

1. **Jogar com pessoas** (`demo_partida` contra o robô e o `jogo` com dois controles) e ajustar
   `config.h`, `golpe.c` e `personagem.c`. Rode as duas verificações depois de cada ajuste.
2. **Integrar os modelos** do Meshy quando a arte entregar (o render já desenha o `.glb` de cada
   peça quando ele existe) e conferir frestas nas juntas.
3. Decidir os pontos em aberto de `docs/DECISOES.md` (tempo limite do round, por exemplo).
4. Revisar os PRs de A e B.

## Semana 1 — verificação da física (concluída)

Ponto de partida: o projeto **RayLibOdeRagDoll** (Chris Camacho, jan/2026), que já junta
raylib, ODE e ragdoll com articulações motorizadas.

Faça a verificação em **`demos/demo_fisica.c`** (o CMake cria o alvo `demo_fisica` quando
o arquivo existe): uma janela com o ringue, um ou dois corpos e teclas de teste
(empurrar, aplicar pose, resetar). Ele só usa `fisica`, `corpo`, `personagem`,
`esqueleto` e `golpe`, então não depende do combo (A) nem das telas (B).
Se os catálogos do Integrante B ainda não chegaram, use temporariamente os valores
provisórios da tabela em `docs/tarefas/TAREFA-B-interface.md`.

| # | Critério | Como verificar |
|---|---|---|
| 1 | raylib e ODE compilam pelo CMake nas máquinas do grupo | `cmake -S . -B build && cmake --build build` sem erro |
| 2 | Um corpo de 9 peças (caixas) fica de pé sozinho por 30 s com a mola de equilíbrio | Rodar e olhar, sem tremer nem explodir |
| 3 | O corpo anda pelo ringue com o analógico | |
| 4 | Um impulso forte derruba o corpo; com alguma parte além dos pés no chão, `corpo_caido` fica verdadeiro | Tecla de teste no modo F1 |
| 5 | Uma pose de golpe é aplicada pelos motores e o contato com outro corpo é registrado | Contar `qtd_contatos` na tela no modo F1 |

Os 5 funcionando: segue o corpo articulado. Algum falhando no fim da semana: plano B
(`docs/ARQUITETURA.md`, seção 10), mudando só `corpo.c` e `fisica.c`.

## Ordem sugerida de implementação

1. **`fisica.c`**: `dWorldCreate`, `dHashSpaceCreate`, `dJointGroupCreate`, gravidade,
   `dCreatePlane` (chão), CFM/ERP. Callback de colisão (`dSpaceCollide`): pegar os
   `RefPeca` com `dGeomGetData`; ignorar pares do mesmo lutador; gerar até
   `FISICA_PONTOS_POR_PAR` contatos com `dCollide`; atrito `FISICA_ATRITO_PES`, `FISICA_ATRITO_CORPO_CHAO` ou
   `FISICA_ATRITO_CORPOS`; se os dois são lutadores diferentes, registrar um `Contato`
   com a velocidade relativa. Depois `dWorldQuickStep(FISICA_DT)` e `dJointGroupEmpty`.
2. **`corpo.c`**: criar as 9 caixas (`dBodyCreate`, `dMassSetBoxTotal`, `dCreateBox`,
   `dGeomSetBody`, `dGeomSetData(&c->refs[i])`) com `personagem->centro/tamanho/massa`,
   giradas por `angulo_y` em volta dos pés. Articulações com `esqueleto_tipo_articulacao`
   e `esqueleto_pecas_da_articulacao`, ancoradas em `personagem->ancora`, com limites.
   `corpo_passo`: para cada articulação, velocidade do motor = `CORPO_GANHO_MOTOR * erro`
   com `dParamFMax = CORPO_FORCA_MOTOR`; mola de equilíbrio = torque no tronco e na bacia
   na direção que leva o "para cima" deles ao Y do mundo; força de andar na bacia
   (limitada por `velocidade_max * fator_velocidade`); torque de giro para `alvo_olhar`;
   ciclo das pernas alternando os quadris enquanto anda. `corpo_transformacao`: converter
   a rotação 3x4 do ODE (`dBodyGetRotation`) e a posição para `Matrix` da raylib.
   Pose neutra: braços para baixo (ombros girados ~1,3 rad).
3. **`render.c`**: sem modelo, caixa do tamanho da peça (`DrawCube` dentro de
   `rlPushMatrix` + `rlMultMatrixf(MatrixToFloat(m))`). Com modelo: `pele_desenhar` com as
   matrizes das peças (seção 10b de `docs/ARQUITETURA.md`).
   **Cor por jogador** (`tint` e faixa colorida): os dois podem escolher o mesmo personagem.
4. **`lutador.c`**: seguir `lutador.h`. Conversão de `Intencao.mover` (tela) para o mundo
   (X, Z) usando a direção da câmera projetada no chão. Ao iniciar o golpe, aplicar
   `golpe->avanco` no tronco e na bacia, na direção de `corpo_frente`. Queda: contar
   `tempo_inclinado` enquanto `corpo_caido` for verdadeiro; zera quando ficar falso.
5. **`combate.c`**: um contato conta como acerto se uma das peças é a `parte_ataque` ou a
   `parte_ataque_2` do golpe em andamento de um lutador, a outra peça é do adversário,
   e o golpe ainda não acertou.
6. **`partida.c`**: estados, contagem, relógio, laço de passos fixos (ordem exata em
   `docs/ARQUITETURA.md`, seção 7), regras de round e `partida_preencher_hud`.
   Juntar os `EventosCombate` de todos os passos do quadro (OU lógico) em `p->eventos`.
   Posições iniciais: (±`RINGUE_DISTANCIA_INICIAL`/2, 0, 0), virados um para o outro.
7. **Ajuste fino** (semanas 2 e 3): chaves dos golpes em `golpe.c` (`docs/ANIMACOES.md`), massas e forças em
   `personagem.c` e `config.h`. Teste com os colegas.

## Cronograma

| Semana | Principal | Integrante A | Integrante B | Arte |
|---|---|---|---|---|
| 1 | Verificação da física (5 critérios) | combo + hematoma + testes | catálogos (2 dias), entrada, recursos | Po cortado em 9 peças + medidas |
| 2 | lutador, combate, partida, render | `ESTRUTURAS.md`, revisar PRs, ajudar B na montagem | telas, HUD, montagem, `demo_telas` | Tai Lung + sons |
| 3 | integração, ajuste de golpes e forças | testes de jogo, lista de bugs | polimento visual das telas | outros personagens (se o pipeline funcionou) |
| 4 | correções, apresentação | apresentação das estruturas | correções | material da apresentação |

## Como integrar o trabalho dos outros

- Cada integrante trabalha numa branch (`a/estruturas`, `b/interface`).
- Pull request para `main`; você revisa: compila sem aviso? testes passam? algum `.h` mudou?
- Mudança de `.h` só com combinação no grupo e atualização de `docs/ARQUITETURA.md` no mesmo PR.
