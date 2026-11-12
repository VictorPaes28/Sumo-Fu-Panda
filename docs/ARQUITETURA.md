# Arquitetura do jogo

> **Fonte oficial.** Este arquivo e os `.h` em `src/` são o contrato entre os integrantes.
> Mudou um tipo, uma função ou uma regra? Atualize este arquivo **no mesmo commit**.
> O PDF em `docs/` é um resumo antigo para leitura; em caso de diferença, vale este arquivo.

## 1. O jogo em uma frase

Dois lutadores de *Kung Fu Panda*, com corpos articulados simulados por física (ODE),
tentam empurrar um ao outro para fora de um ringue de sumô. Cada jogador monta um
**combo** de 4 golpes antes da luta (lista circular) e cada acerto deixa um **hematoma**
que enfraquece o equilíbrio do adversário até curar (lista ordenada + insertion sort).

## 2. Regras de jogo (decididas)

| Regra | Valor |
|---|---|
| Jogadores | 2, cada um com um controle; teclado como reserva |
| Vitória do round | o outro **sai do ringue** (bacia além de `RINGUE_RAIO`) ou **cai** (qualquer parte além dos pés encosta no chão por `CORPO_TEMPO_QUEDA`, como no sumô) |
| Empate no round | os dois perdem no mesmo passo → round repetido |
| Vitória da partida | `PARTIDA_ROUNDS_PARA_VENCER` rounds (melhor de 3) |
| Combo | exatamente `COMBO_MAX` (4) golpes para confirmar; circular; reinicia após `COMBO_TEMPO_REINICIO` sem golpear |
| Botão A durante um golpe | ignorado (sem fila de comandos) |
| Cada golpe | acerta no máximo uma vez; acerta quando uma das peças de ataque do golpe toca o adversário; encostar fora de um golpe só empurra |
| Fases do golpe | **preparo** (puxa o braço / abaixa), **ativo** (só aqui acerta; aplica o avanço), **volta** (vulnerável). Tempos em `golpe.c` |
| Golpes que avançam | impulso para a frente no próprio corpo no início da fase ativa (`Golpe.avanco`) |
| Acerto físico | a força entra **no ponto do toque** (golpe alto tomba, baixo desliza), cresce com a **velocidade da peça no toque** (`ACERTO_VEL_REFERENCIA`) e quem bate sofre **recuo** (`ACERTO_RECUO`) |
| Levar golpe | o alvo **cambaleia** (`ATORDOADO_*`): mola mais fraca e corpo tombando para onde foi empurrado |
| Firmar | também reduz o impulso recebido (`FIRMADO_IMPULSO`) e o tempo cambaleando pela metade |
| Sensação de impacto | no acerto, o jogo **congela** um instante (`PAUSA_IMPACTO_*`), a câmera **treme** e a porcentagem "pula" no HUD |
| Mesmo personagem | os dois podem escolher o mesmo; o render diferencia com uma cor por jogador |
| Hematoma | dano (%) + tempo de cura; a porcentagem enfraquece a mola de equilíbrio e aumenta o impulso recebido |
| Stamina | golpes gastam; regenera depois de `STAMINA_ESPERA`; sem stamina não golpeia |
| Firmar | segura o gatilho: mola de equilíbrio mais forte, anda mais devagar, **não gasta stamina** |
| Direção do lutador | sempre vira a bacia para o adversário (não há controle de rotação) |
| Cada round | lutadores recriados: de pé, stamina cheia, sem hematomas, combo reiniciado |
| Pausa | Start; controle desconectado também pausa; pausado + B volta ao menu |
| Física explodiu (NaN) | round repetido |

## 3. Módulos e donos

| Módulo | O que faz | Usa raylib | Usa ODE | Dono |
|---|---|:-:|:-:|---|
| `config.h` | constantes ajustáveis | | | todos (cada um na sua área) |
| `esqueleto` | peças, articulações, `Pose` | | | B |
| `golpe` | catálogo de golpes | | | B (tabela) / Principal (valores) |
| `personagem` | catálogo de personagens | | | B (tabela) / Principal (valores) |
| **`combo`** | **lista circular duplamente encadeada** | | | **A** |
| **`hematoma`** | **lista ordenada + insertion sort** | | | **A** |
| `entrada` | controle/teclado → `Intencao` | ✓ | | B |
| `recursos` | carregar modelos e sons | ✓ | | B |
| `telas` | menu, seleção, resultado | ✓ | | B |
| `montagem` | tela de montagem do combo | ✓ | | B |
| `hud` | desenho 2D por cima da luta | ✓ | | B |
| `ui` | cores e funções de desenho 2D comuns (telas, montagem, HUD) | ✓ | | B |
| `pele` | modelo com esqueleto cujos ossos seguem as peças físicas | ✓ | | Principal |
| `animacao` | animações reais (parado, andar) como ângulos das articulações; `animacoes_dados.c` é gerado | | | Principal |
| `luz` | shader de iluminação simples dos modelos (direcional + ambiente + contorno) | ✓ | | Principal |
| `fisica` | mundo do ODE, passo fixo, contatos | | ✓ | Principal |
| `corpo` | corpo articulado, motores, equilíbrio | ✓ | ✓ | Principal |
| `lutador` | corpo + stamina + hematomas + golpe | ✓ | | Principal |
| `combate` | contatos → acertos | ✓ | | Principal |
| `partida` | rounds, placar, laço da física | ✓ | | Principal |
| `render` | câmera, ringue, corpos em 3D | ✓ | | Principal |
| `main.c` | janela, telas, sons, laço principal | ✓ | | Principal |

## 4. Dependências (quem pode incluir quem)

```
main.c
  |-- telas, montagem, hud, recursos, entrada      (Integrante B)
  |     recursos ----> pele -> personagem          (sem ODE)
  |-- render ----------> partida, recursos, corpo
  `-- partida ---------> fisica, lutador, combate, hud (só o tipo InfoHud)
        combate -------> lutador, fisica, golpe, hematoma
        lutador -------> corpo, combo, hematoma, golpe, entrada
        corpo ---------> fisica, personagem, esqueleto          [ODE]
        fisica --------> esqueleto, config                      [ODE]
  montagem ------------> combo, golpe, entrada
  hud -----------------> golpe, config

C puro (sem raylib, sem ODE): config, esqueleto, golpe, personagem, combo, hematoma
```

Regra: nada inclui `main.c`; nenhum módulo de lógica inclui `render.h`, `hud.h`
(exceto `partida.h`, só pelo tipo `InfoHud`), `telas.h` ou `montagem.h`.

Sobre o ODE: `corpo.h` e `fisica.h` incluem `ode/ode.h` porque as structs guardam
tipos do ODE (`dBodyID`, `dWorldID`), então quem inclui `lutador.h` ou `partida.h`
recebe o cabeçalho do ODE junto. A regra é: **só `fisica.c` e `corpo.c` chamam funções
do ODE ou mexem nos campos do ODE**. Os outros módulos usam as funções de `corpo.h`.
Os módulos do Integrante B não incluem nenhum desses cabeçalhos.

**Nunca copie `Partida`, `Lutador`, `Corpo` ou `Fisica` por valor.** As formas do ODE
guardam ponteiros para dentro do `Corpo`; uma cópia deixa esses ponteiros apontando
para o lugar errado. Passe sempre ponteiros.

## 5. Tempo de vida dos dados

| Dado | Criado | Destruído | Onde mora |
|---|---|---|---|
| `Jogador` (personagem + combo) | início do programa | fim do programa | `main.c` |
| `Partida` (física, placar, relógio) | `partida_iniciar` | `partida_liberar` | `main.c` |
| `Lutador` (corpo, stamina, hematomas) | início de cada round | fim de cada round | `Partida` |
| Nós do combo (`malloc`) | `combo_inserir` | `combo_remover` / `combo_liberar` | `Jogador.combo` |
| Nós dos hematomas (`malloc`) | `hematoma_inserir` | `hematomas_curar` / `hematomas_liberar` | `Lutador.hematomas` |
| Objetos do ODE | `fisica_iniciar`, `corpo_criar` | `corpo_destruir`, `fisica_liberar` | `Partida`, `Corpo` |
| Modelos e sons | `recursos_carregar` | `recursos_liberar` | `main.c` |

O combo é do **Jogador**, não do Lutador: assim ele sobrevive à troca de rounds e à revanche.

## 6. Fluxo de telas (`main.c`)

```
MENU -> SELECAO -> MONTAGEM -> LUTA -> RESULTADO
          ^  voltar   ^  voltar          | revanche -> LUTA
          |           +---- remontar ----+ menu -> MENU
```

Cada tela segue o padrão `xxx_iniciar` / `acao = xxx_atualizar(tela, in)` / `xxx_desenhar(tela)`.
Telas não trocam de tela nem tocam sons: devolvem uma ação, e o `main.c` decide.

## 7. Um quadro da luta (`partida_atualizar`)

```
1. main      in[i] = entrada_ler(i)                     uma vez por quadro, para os 2
2. partida   lutador_comandar(l, &in[i], pos_adversario)
                 -> movimento, firmar, olhar para o adversário
                 -> se A e pode golpear: começa o golpe, gasta stamina, combo_avancar
3. partida   acumulador += dt
             enquanto acumulador >= FISICA_DT (máx. FISICA_MAX_PASSOS_POR_QUADRO):
                 lutador_passo_fisica(l)   x2   -> equilíbrio pelo dano, corpo_passo
                                                  (forças reaplicadas EM CADA passo)
                 fisica_passo(&fisica)          -> colisões, contatos, dWorldQuickStep
                 combate_processar(...)         -> contatos -> hematoma_inserir (insertion sort)
                                                   + impulso
                 relogio += FISICA_DT
                 acumulador -= FISICA_DT
4. partida   lutador_atualizar(l, dt, relogio) x2 -> hematomas_curar, stamina, fim do golpe,
                                                     reinício do combo, queda
5. partida   regras: saiu do ringue? caiu? empate? NaN?  -> estado do round
6. main      sons a partir de partida.eventos
7. main      render_cena(...) e hud_desenhar(...)     só leitura
```

## 8. Física: detalhes que não podem ser esquecidos

- **Passo fixo** de `FISICA_DT` (1/120 s). Quem conta os passos é a `partida`.
- **Forças são zeradas pelo ODE depois de cada passo.** Por isso `corpo_passo`
  roda antes de **cada** `fisica_passo`, e não uma vez por quadro.
- **Contatos são por passo.** `fisica_passo` zera a lista e registra os toques do passo;
  `combate_processar` roda logo depois, no mesmo passo.
- **Peças do mesmo lutador não colidem entre si** (filtro no callback de colisão).
- **Precisão simples** (`dSINGLE`, `float`), fixada no `CMakeLists.txt`. Não mudar.
- **Pose de criação = T-pose.** O corpo é criado na mesma pose dos modelos do Meshy;
  os ângulos de `Pose` são medidos a partir dela.
- **Articulações:** joelhos = dobradiça (1 eixo); cintura, pescoço, ombros e quadris = universal (2 eixos).
- **Mola de equilíbrio:** torque no tronco (e na bacia) puxando para a vertical, com força
  `forca_equilibrio * equilibrio`, onde `equilibrio = max(EQUILIBRIO_MINIMO, 1 - dano/DANO_MAX)`,
  vezes `EQUILIBRIO_FIRMADO` ao firmar.
- **Queda:** `corpo_caido` = alguma peça além dos pés (joelho e canela inclusive) encosta no
  chão no último passo de física (`no_chao` de `fisica.h`); o lutador vira `LUTADOR_CAIDO`
  depois de `CORPO_TEMPO_QUEDA` seguidos assim (só filtra um contato solto). Encostando
  no chão, também não pode começar golpe (`lutador_pode_golpear`).
- **Eventos por quadro:** como há vários passos por quadro, a partida junta os eventos de
  combate de todos os passos (OU lógico) antes de entregar ao `main.c`.
- **Compilação:** o ODE é C++ por dentro; o CMake liga o jogo como C++ (o nosso código
  continua C99). Por isso o grupo precisa de `g++` instalado junto com o `gcc`.

## 9. Coordenadas e unidades

- Metros, quilogramas, segundos, radianos.
- Y para cima. Centro do ringue na origem. Chão em y = 0.
- Personagem na pose de criação olha para **+Z**, pés em (0, 0, 0).
- `Intencao.mover`: x > 0 = direita da tela, y > 0 = para cima no analógico.
  A conversão para o mundo (X, Z) usa a direção da câmera e é feita em `lutador.c`.

- **Sinal dos ângulos:** o ODE mede o ângulo das juntas universais no sentido contrário
  ao do eixo dado; por isso os eixos em `corpo.c` parecem "invertidos". O significado de
  cada sinal está em `esqueleto.h` e é conferido por `demo_fisica --teste`.
- **Pernas:** a pose de um golpe só mexe em cintura, pescoço e ombros; as pernas ficam
  sempre na base de sumô (quadril para a frente e aberto, joelho dobrado) com o ciclo
  de passos ao andar.
- **Atrito baixo de propósito** (`FISICA_ATRITO_PES`; o resto do corpo usa `FISICA_ATRITO_CORPO_CHAO`): os lutadores deslizam depois
  de um empurrão, que é o que permite tirar alguém do ringue.

### O que deixa o movimento menos "robótico"

| Técnica | Onde | O que faz |
|---|---|---|
| Pose suave | `corpo.c` (`pose_suave`) | a pose dos motores anda até a pose pedida aos poucos, sem "teleporte" |
| Rigidez | `corpo_definir_rigidez`, `RIGIDEZ_*` | braços soltos parado, firmes no golpe; muda força e rapidez dos motores |
| Animações de verdade | `animacao.h`, `animar` em `corpo.c` | parado e andar vêm de animações reais convertidas em ângulos das articulações; a VARIAÇÃO delas é somada à guarda e à base de sumô (seção 10c) |
| Golpe em curva | `golpe.h` (`ChaveGolpe`) | antecipação, ataque com o corpo todo (afunda e empurra o chão), passar do ponto e volta, em vez de pular entre duas poses |
| Corpo de 13 peças | `esqueleto.h`, `corpo.c` | antebraço com cotovelo físico (bate, cede, é empurrado) e pé com tornozelo; braços e pernas são cápsulas (escorregam em vez de prender pela quina) |
| Músculo de mola | `musculo` em `corpo.c`, `MUSCULO_*` | cada junta é mola + amortecedor (implícito no ODE): cede proporcional ao empurrão e volta; velocidade limitada (força-velocidade de Hill); cintura e pescoço mais macios |
| Massas reais | `personagem.c` (De Leva, 1996), `centro_massa` | proporções do corpo humano com o jeito de cada um (barriga do Po à frente, ombros pesados do Tai Lung) |
| Limites macios e atrito interno | `JUNTA_*` | a junta é freada no limite como por um ligamento, e o membro solto balança e para |
| Pés no chão | `pes_retos`, `TORNOZELO_*` | no ar a sola fica reta para pisar; apoiado, o tornozelo segura a canela e faz o equilíbrio pelo pé (1º nível, antes do quadril e do passo); a ponta do pé tem mais atrito (`FISICA_ATRITO_PONTA`) |
| Impulso no corpo inteiro | `corpo_impulso` | avanço e empurrão dividem o impulso pelas peças conforme a massa: nenhuma peça dispara e arranca os pés do chão |
| Cansaço | `corpo_definir_forca_muscular`, `CANSACO_FORCA_MINIMA` | stamina baixa amolece os músculos |
| Pegada na faixa | `corpo_agarrar`, `Golpe.agarra`, `AGARRAR_*` | nos golpes de pegada (barrigada, forçar fora, projeções) a mão que encosta na bacia/tronco do outro prende (junta de bola) e solta sozinha |
| Alavanca | `Golpe.derruba`, `combate.c` | projeções e puxões põem a força no alto do tronco do alvo (o corpo gira em volta dos pés) e não dependem da velocidade da mão |
| Estilo por personagem | `EstiloMovimento` em `personagem.h` | cada personagem tem a própria base, guarda, ritmo e tamanho do passo, balanço dos braços, gingado e força dos reflexos (o panda bamboleia, o leopardo anda baixo e rápido) |
| Guarda de sumô | pose neutra em `corpo.c` (do estilo) | braços meio à frente, tronco inclinado |
| Inclinação | `corpo_definir_inclinacao` | inclina para a frente ao andar e tomba para trás ao levar golpe |
| Passos pela velocidade real | `animar` em `corpo.c` | os pés acompanham o deslize depois de um empurrão (tropeça) |
| Entrada com peso | `ENTRADA_SUAVIZACAO` | a direção acompanha o analógico aos poucos ao começar e parar |
| Fases do golpe | `golpe.h` | preparo antes do ataque: o golpe "carrega" e o adversário pode reagir |
| Impacto | `partida.c`, `render.c`, `hud.c` | pausa no impacto, tremor de câmera, número de dano pulando |
| Empurrar pelo peito | `combate.c` (`centro_do_peito`), `no_peito` em golpe.h | a força dos golpes de empurrar entra no centro do peito, mesmo se o toque foi no braço da guarda |
| Pernas no reflexo | `reflexo_das_pernas` em corpo.c | quem se desequilibra afunda e abre a base, e dá passos para trás |
| Dano em sequência | `combate_aplicar_golpe`, `DANO_ESCALA_SEGUIDOS` | golpes seguidos valem menos: uma rajada não leva o dano às alturas em 2 s |
| Câmera com zoom | `render_atualizar_camera`, `CAMERA_ZOOM_*` | perto no corpo a corpo, abre quando os lutadores se afastam |
| Força no ponto | `corpo_impulso_no_ponto`, `combate.c` | o golpe empurra e gira o corpo a partir de onde tocou; a força vai para a peça pesada mais próxima (tronco ou bacia), no mesmo ponto, para a física não tremer |
| Velocidade do toque | `fisica.c` (`dBodyGetPointVel`) | soco em cheio empurra mais que um encostar |
| Ação e reação | `ACERTO_RECUO` | quem bate também é empurrado para trás |
| Reação a impacto | `corpo_receber_contato`, `REACAO_*` | todo choque entre os lutadores (golpe ou encontrão) faz as articulações da peça atingida cederem e voltarem aos poucos: o corpo reage como carne, não como plástico |
| Golpe entregue aos poucos | `corpo_impulso_no_ponto`, `ACERTO_PASSOS` | a força do golpe vai em alguns passos, no mesmo ponto da peça, em vez de um tranco que faz as peças afundarem |
| Contatos firmes | `fisica.c` (`ERP_CONTATO`, `CFM_CONTATO`, camada de contato, `CORRECAO_MAX`) | as peças não afundam umas nas outras nem "estouram" ao desafundar |
| Caixas pelo modelo | `inspeciona_modelo --caixas` | as caixas físicas cobrem a malha do modelo: o que se vê encostar, encosta na física |
| Tônus | `aplicar_motores` em `corpo.c`, `TONUS_*` | com equilíbrio baixo, todos os motores (pernas inclusive) perdem força: quem perde o round desmonta como boneco de pano |
| Reflexos de equilíbrio | `medir_desequilibrio` e seguintes em `corpo.c`, `REFLEXO_*` | o desequilíbrio é medido pelo *capture point* (centro de massa + velocidade × √(altura/g), descontando o andar pedido). Fora dos pés, com ~165 ms de atraso (tempo de reação humano): braços giram e se abrem de contrapeso, cintura e cabeça compensam para o outro lado, e caindo para a frente os braços vão à frente para se proteger. Machucado, reage pior |
| Passo de recuperação | `comecar_passo`, `pose_do_passo`, `forcas_do_passo`, `PASSO_*` | desequilíbrio grande: o lutador pisa no *capture point* (onde, pisando, o corpo para), com a perna do lado da queda. As forças do passo são internas (o contrário vai na bacia): o passo reposiciona o pé, não empurra o corpo |
| Iluminação | `luz.h` | luz direcional, ambiente de céu e chão e um contorno fraco nos modelos |
| Apoio nos pés | `calcular_apoio` em `corpo.c`, `APOIO_*` | mola de equilíbrio, andar e virar só têm força total com os pés no chão e o centro de massa em cima deles; no ar quase nada; com o peso longe dos pés (além de `APOIO_LIMITE`) não há como se salvar |
| Andar empurrando o chão | `andar` em `corpo.c` | a força de andar vai metade na bacia e metade nos pés (toda na bacia tombava o corpo) |
| Passo na direção do movimento | `animar` em `corpo.c` | para a frente/trás ao avançar e recuar, abrindo e fechando as pernas ao ir de lado |
| Atrito por peça | `fisica.c` | pés deslizam na areia (`FISICA_ATRITO_PES`); o resto do corpo arrasta (`FISICA_ATRITO_CORPO_CHAO`) |
| Força geral | `ACERTO_FORCA` | um único número para deixar os golpes mais fortes ou mais fracos |
| Câmera que acompanha | `render_atualizar_camera` | segue o meio da luta (só desliza, nunca gira: o analógico continua valendo igual) |

## 10. Programas de demonstração e verificação

| Programa | Para que serve |
|---|---|
| `teste_combo`, `teste_hematoma` | testes das estruturas de dados (terminal, também rodam no GitHub) |
| `demo_fisica --teste` | confere a física sem janela: fica de pé, anda, cai, sinais das articulações, contatos |
| `demo_fisica` | janela com dois corpos e teclas de teste (WASD, 1-5, F, ESPAÇO, R) |
| `demo_partida --teste` | 20 partidas robô contra robô sem janela; confere rounds, vencedor e hematomas |
| `demo_partida` | você contra o robô, sem os menus (bom para ajustar a jogabilidade) |
| `demo_partida --capturas`, `demo_telas --capturas` | salvam capturas de tela em PNG e fecham sozinhos |
| `demo_telas` | todas as telas 2D sem física |
| `inspeciona_modelo <arquivo.glb>` | mostra altura, pés e a posição de cada osso de um modelo (âncoras das articulações) |
| `inspeciona_modelo <arquivo.glb> --caixas <personagem>` | mede as caixas físicas pela MALHA do modelo e mostra quanto dela fica fora (para `personagem.c`) |
| `extrai_animacoes <modelo_animado.glb> src/animacoes_dados.c` | converte as animações "Idle" e "Walk" de um modelo em ângulos das articulações (seção 10c) |
| `ferramentas/preparar_modelo.py` | script do Blender: FBX do Meshy → GLB em metros, pés no chão, sem animação (`docs/ASSETS.md`) |

## 10b. Modelos dos personagens (pele)

O personagem é **um modelo inteiro com esqueleto**, e não peças cortadas. A física continua
com as 13 peças; `pele.c` faz cada osso seguir a sua peça (tabela `Personagem.ossos`):

```
ao carregar:  ajuste[osso] = osso na T-pose  x  inversa(peça na pose de criação)
a cada quadro: osso no mundo = ajuste[osso]  x  peça agora     (osso sem peça: segue o pai)
               -> UpdateModelAnimation (deforma a malha) -> DrawModel
```

Requisitos: modelo em T-pose, olhando para +Z, pés em y = 0, em metros, e medidas das peças
batendo com os ossos (`docs/ASSETS.md`). Sem modelo (`Personagem.modelo == NULL` ou arquivo
faltando), o personagem é desenhado com caixas. `pele.h` recebe só as matrizes das peças,
não o `Corpo`, para não depender do ODE.

Rode `demo_fisica --teste` e `demo_partida --teste` depois de mexer em física, golpes,
personagens ou `config.h`.

## 10c. Animações (a física seguindo animações de verdade)

Os motores das articulações não seguem mais ondas senoidais feitas à mão: seguem
**animações reais** (parado e andar), convertidas para os ângulos das articulações.

```
ferramenta (uma vez):  modelo animado .glb --extrai_animacoes--> src/animacoes_dados.c
  para cada quadro: giro de cada peça desde a T-pose -> giro do filho em relação ao pai
                    -> decomposto nos eixos de esqueleto_eixos() -> ângulos (rad)
jogo (a cada passo):   alvo = guarda/golpe (parte de cima) + base de sumô (pernas)
                            + variação de PARADO (sempre)
                            + variação de ANDAR (pelo ritmo e direção do movimento)
                       -> motores -> física
```

- Usa a **variação** (quadro − média da animação), e não a pose inteira: o movimento natural
  entra, a postura de sumô e os golpes continuam valendo.
- Como são só ângulos, as mesmas animações servem para **qualquer personagem**.
- Origem atual: animações *Robot_Idle* e *Robot_Walking* do modelo "Robot" de Quaternius
  (domínio público, CC0), que vem nos exemplos da raylib
  (`build/_deps/raylib-src/examples/models/resources/models/gltf/robot.glb`).
  Para trocar por animações do Meshy/Mixamo, rode `extrai_animacoes` com o novo modelo
  (ele reconhece o esqueleto do Mixamo) e recompile.
- Os golpes são **curvas de chaves** (`ChaveGolpe`, `golpe_amostrar`): cada chave diz quanto
  cada articulação (pernas e cotovelos inclusive) se afasta da guarda; entre as chaves a
  curva é suave (Hermite). O lutador toca a curva a cada quadro e o corpo a soma por cima da
  postura (`corpo_definir_movimento_extra`). Hoje as chaves são feitas à mão; para trocá-las
  por animações do Mixamo, ver `docs/ANIMACOES.md` (`extrai_animacoes --golpe`).
- **Cotovelo físico:** o antebraço é uma peça (`PARTE_ANTEBRACO_*`, `ART_COTOVELO_*`); a guarda
  do personagem (`guarda_cotovelo`) mais o golpe dobram a junta de verdade.

## 11. Plano B (se a física precisar ser trocada)

A verificação da semana 1 passou (ver `docs/tarefas/TAREFA-PRINCIPAL.md`), então o plano B
não é necessário hoje. Se um dia for, só `corpo.c` e `fisica.c` mudam:
o corpo passa a ser uma cápsula com física simples e o ODE fica só para o ragdoll da queda.
As funções de `corpo.h` continuam as mesmas, então nenhum outro módulo muda.
