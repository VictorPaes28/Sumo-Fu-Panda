# Registro de decisões

Cada decisão de projeto entra aqui com data e motivo, para ninguém precisar
perguntar "por que é assim?". Decisão nova = linha nova (não apague as antigas;
marque como substituída).

| Data | Decisão | Motivo |
|---|---|---|
| 2026-10-21 | Filme: *Kung Fu Panda*; jogo de sumô inspirado em *Sumotori Dreams* | Party game com física, divertido para a turma |
| 2026-10-21 | 3D com raylib 6.0 | Simples, em C, carrega GLB, controles prontos |
| 2026-10-21 | Física com ODE 0.16.6, precisão simples | Corpos articulados de verdade, como no Sumotori; API em C |
| 2026-10-21 | Corpo com 9 peças rígidas e 8 articulações com motores (substituída em 2026-10-28: 13 peças) | Poucas peças = física estável; suficiente para o efeito |
| 2026-10-21 | ~~Modelos do Meshy cortados em peças no Blender, sem rig~~ (substituída: ver "modelo inteiro com esqueleto" abaixo) | Peças rígidas seguem a física direto; evita animação esquelética |
| 2026-10-21 | Pose de criação = T-pose | Mesma pose dos modelos; ângulos da Pose medidos a partir dela |
| 2026-10-21 | Combo montado = lista circular duplamente encadeada | Conteúdo da disciplina; o combo se repete (circular) e é editado (encadeada) |
| 2026-10-21 | Hematomas = lista ordenada + insertion sort | Conteúdo da disciplina; curas em tempos diferentes exigem ordem por momento de cura |
| 2026-10-21 | Dano enfraquece a mola de equilíbrio e aumenta o impulso recebido | Liga o dano à física: o lutador machucado cambaleia |
| 2026-10-21 | Cair dentro do ringue também perde o round | Como no sumô real; elimina o problema de levantar um ragdoll |
| 2026-10-21 | Lutador sempre vira para o adversário | Controle mais simples; luta frente a frente |
| 2026-10-21 | Botão A ignorado durante um golpe | Evita fila de comandos |
| 2026-10-21 | Combo só confirma com 4 golpes | Evita combo vazio e casos especiais |
| 2026-10-21 | Firmar não gasta stamina | Stamina é recurso de ataque |
| 2026-10-21 | Empate no round (os dois perdem juntos) repete o round | Regra simples e justa |
| 2026-10-21 | Combo pertence ao Jogador; Lutador é recriado a cada round | O combo sobrevive aos rounds e à revanche |
| 2026-10-21 | Relógio da partida só anda durante a luta | Hematomas não curam na contagem nem na pausa |
| 2026-10-21 | Forças reaplicadas a cada passo de física; contatos processados por passo | O ODE zera forças a cada passo |
| 2026-10-21 | Peças do mesmo lutador não colidem entre si | Evita tremedeira nas articulações |
| 2026-10-21 | Arquivo faltando (modelo/som) não é erro | O jogo roda antes da arte ficar pronta |
| 2026-10-21 | Sem IA na dinâmica; Meshy só para os modelos | Exigência do formulário (item i) |
| 2026-10-21 | ~~Queda só vale após `CORPO_TEMPO_QUEDA` inclinado~~ (substituída: ver "regra do sumô" abaixo) | Evita perder o round numa inclinação rápida |
| 2026-10-21 | Golpes têm até 2 peças de ataque | Empurrão com as duas mãos, barrigada com tronco e bacia |
| 2026-10-21 | Investida e barrigada dão avanço no próprio corpo | Sem isso a investida não "avança" |
| 2026-10-21 | Mesmo personagem para os dois é permitido; cor por jogador | Diferenciar na tela |
| 2026-10-21 | Jogo ligado como C++ no CMake (código continua C99) | O ODE é C++ por dentro |
| 2026-10-21 | `demo_fisica` separado do jogo | Verificação da semana 1 sem depender de A e B |
| 2026-10-21 | Testes das estruturas rodam no GitHub a cada push/PR | Pegar erro do trabalho feito com IA antes de juntar na main |

| 2026-10-21 | Atrito do chão baixo (0,45) e frenagem fraca parado | Com atrito alto, ninguém saía do ringue |
| 2026-10-21 | Impulsos dos golpes ~2,5x maiores; `DANO_MAX` 100; +1,5% de impulso por ponto de dano | Partidas de robôs empatavam para sempre; agora terminam em ~50 s |
| 2026-10-21 | ~~Pernas sempre na base de sumô; pose do golpe só mexe na parte de cima~~ (substituída: golpes em curva com o corpo todo, abaixo) | Golpes não derrubam o próprio lutador |
| 2026-10-21 | Na montagem, A com o combo cheio substitui o golpe do cursor | Mais prático que tirar e pôr |
| 2026-10-21 | Módulo `ui` (cores e desenho 2D comuns) | Mesmo visual em todas as telas, sem código repetido |
| 2026-10-21 | `demo_partida` com robô (teste sem janela, capturas e treino) | Testar a luta sem duas pessoas |
| 2026-10-21 | Quem perde o round vira boneco mole (equilíbrio 0) | A queda é a parte engraçada |

| 2026-10-21 | Golpes em 3 fases (preparo, ativo, volta); só a fase ativa acerta | Golpes instantâneos pareciam robóticos e não davam chance de reagir |
| 2026-10-21 | Pose suave + rigidez variável + respiração + guarda de sumô | Corpo parado parecia estátua e as poses "teleportavam" |
| 2026-10-21 | Quem leva golpe cambaleia (mola mais fraca, tomba na direção do golpe) | Golpe sem reação parecia não ter acertado |
| 2026-10-21 | Pausa no impacto, tremor de câmera, porcentagem pulando no HUD | Sensação de impacto ("game feel") |
| 2026-10-21 | Firmar também reduz o impulso recebido | Firmar precisava de uma vantagem clara, não só ficar mais lento |
| 2026-10-21 | Câmera acompanha o meio da luta só deslizando (sem girar) | Menos estática, sem mudar o sentido do analógico |
| 2026-10-21 | Passos seguem a velocidade real da bacia | Ao ser empurrado, os pés tropeçam em vez de deslizar parados |
| 2026-10-21 | `dt` limitado a 0,1 s no `main.c` | Arrastar a janela não faz a luta dar um salto |

| 2026-10-21 | Acerto aplicado como força no ponto do toque (antes: velocidade somada no centro) | O golpe não tinha "física": não girava o corpo nem dependia de onde pegava |
| 2026-10-21 | Força do acerto proporcional à velocidade da peça no toque (0,45x a 1,5x) | Encostar e socar em cheio davam o mesmo resultado |
| 2026-10-21 | Recuo de 35% em quem bate | Ação e reação: bater também desequilibra |
| 2026-10-21 | Toque em peça leve transmite a força para tronco/bacia, no mesmo ponto | Arremessar só o braço deixava a física instável |

| 2026-10-21 | Equilíbrio, andar e virar dependem do apoio nos pés e do centro de massa | A mola de equilíbrio "salvava" o corpo até no ar; agora só quem está apoiado se equilibra |
| 2026-10-21 | Um pé no ar durante o próprio passo não conta como perder apoio | Senão o lutador caía só de andar |
| 2026-10-21 | Força de andar dividida entre bacia e pés | Toda na bacia, com os pés presos no chão, tombava o corpo |
| 2026-10-21 | Passo lateral (pernas abrem e fecham) ao andar de lado | O lutador anda de lado o tempo todo, virado para o adversário |
| 2026-10-21 | Atrito dos pés 0,45; resto do corpo 1,0 | Pés deslizam na areia; quem cai arrasta |
| 2026-10-21 | `ACERTO_FORCA` 0,80, `APOIO_BASE` 0,42 m, `APOIO_LIMITE` 1,0 m | Com o realismo novo as lutas acabavam em 2 ou 3 golpes e o dano quase não pesava; agora ~26 s por partida (robôs) e dano de até ~40% |

| 2026-10-21 | **Substitui** "modelos cortados em 9 peças": personagem é um modelo inteiro com esqueleto (rig) cujos ossos seguem as peças físicas (`pele`) | Peças rígidas ficavam com cara de boneco articulado e frestas nas juntas |
| 2026-10-21 | Rig no padrão Mixamo (o do Meshy); tabela `OSSOS_MIXAMO` reutilizável | Todo personagem do Meshy liga os ossos do mesmo jeito |
| 2026-10-21 | Medidas físicas do personagem tiradas dos ossos (`inspeciona_modelo`) | Física e visual precisam bater, senão o golpe "atravessa" sem contato |
| 2026-10-21 | `ferramentas/preparar_modelo.py` (Blender) converte o FBX do Meshy em GLB | O FBX vem em centímetros e centralizado na cintura; a raylib não lê FBX |
| 2026-10-21 | Tai Lung com modelo real (1,59 m); Po continua com caixas e corpo provisório | Primeiro modelo entregue pela arte |

| 2026-10-21 | Parado e andar vêm de animações reais convertidas em ângulos (retarget), somadas como variação à postura de sumô | Ondas senoidais feitas à mão pareciam robóticas |
| 2026-10-21 | Fonte das animações: "Robot" de Quaternius (CC0, exemplos da raylib); dados gerados em `animacoes_dados.c` | O FBX do Tai Lung não tem animações; o jogo não depende do arquivo de origem |
| 2026-10-21 | Eixos das articulações centralizados em `esqueleto_eixos` | A física (`corpo.c`) e o retarget usam exatamente a mesma definição |
| 2026-10-21 | ~~Golpes continuam com poses próprias~~ (substituída: curvas de chaves, abaixo) | As animações disponíveis não têm movimentos de sumô |

| 2026-10-21 | **Corrige** "medidas físicas tiradas dos ossos": caixas medidas pela malha (`inspeciona_modelo --caixas`) | Medido: 80% da malha do Tai Lung ficava fora das caixas (até 20 cm); os modelos se atravessavam sem contato físico. Agora 7,5% |
| 2026-10-21 | Contatos mais firmes (ERP 0,45, CFM 1e-5, camada 2 mm, correção máx. 3 m/s) e golpe entregue em 4 passos | Medido: contatos afundando mais de 4 cm caíram de 1,6% para 0,05% |
| 2026-10-21 | Reação a impacto: articulações da peça atingida cedem e recuperam em 0,4 s | Encontrões sem golpe não causavam reação ("bonecos de plástico") |
| 2026-10-21 | Iluminação simples (shader próprio) só nos modelos | Sem luz, o modelo sem textura virava silhueta |
| 2026-10-21 | Faixa colorida só nos personagens de caixas; com modelo, a cor vem do tom | Com a bacia do tamanho real, a faixa virava uma "tábua" |
| 2026-10-21 | Base de sumô 0,35 (quadril) x 0,40 (joelho) e mola de equilíbrio mirando 0,08 rad à frente (`INCLINACAO_BASE`, em `corpo.c`; depois movidos para o estilo do personagem) | Medido: com 0,30 x 0,55 a bacia ficava a -16° (lutador "sentado" para trás). Valores testados no Po e no Tai Lung |
| 2026-10-21 | Tônus: equilíbrio baixo enfraquece todos os motores, pernas inclusive (`TONUS_*`) | Quem caía continuava com as pernas duras; agora desmonta como boneco de pano |
| 2026-10-21 | Investida acerta só com o tronco (sem a cabeça) | Parecia dois bonecos batendo a cabeça |
| 2026-10-21 | `EstiloMovimento` no `personagem.h` (aprovado pelo grupo): postura, andar e reflexos por personagem. Substitui `CORPO_PERIODO_PASSO`, `ANIM_PESO_*`, `INCLINACAO_BASE` e as constantes de base/guarda do `corpo.c` | Cada personagem precisa se mexer de um jeito (panda, macaco, touro...) |
| 2026-10-21 | Reflexos de equilíbrio pelo *capture point*: braços de contrapeso/proteção, cintura e cabeça compensando, passo de recuperação | Ninguém cai com os braços duros; gente de verdade dá um passo e balança os braços para não cair. Medido: empurrão médio (160 N.s) agora é salvo com 2 a 4 passos; o forte (250 a 320) ainda derruba |
| 2026-10-21 | Reflexo com atraso (~165 ms) e mais fraco com dano | Tempo de reação humano; o dano continua sendo o que derruba |
| 2026-10-21 | Campos de estado do reflexo e do passo na struct `Corpo` (`corpo.h`), sem mudar funções | O passo dura vários quadros e precisa de memória; o estado não pode ser global |
| 2026-10-21 | `demo_fisica`: "golpe alto tomba mais" mede a inclinação na direção do golpe, a partir da postura inicial | O reflexo dobra a cintura para a frente contra a queda; a medida antiga (inclinação em qualquer direção) contava isso como tombar |
| 2026-10-21 | **Substitui** a queda por inclinação: cai quem encosta no chão com qualquer parte além dos pés (regra do sumô), por 0,1 s; no chão não golpeia | Gravado: lutadores ficavam até 1,7 s sentados no chão sem perder (e golpeando), e um lutador curvado, de pé, perdeu. Depois: ninguém perde de pé, e o tempo no chão sem perder caiu para o próprio tombo (0,2 a 0,5 s) |
| 2026-10-21 | Animação de parado entra inteira na parte de cima e só 25% nas pernas (`PARADO_PERNAS`) | Medido: o Po ia e voltava 13 cm parado e disparava o reflexo; agora 3,6 cm (Tai Lung 2,3 cm) |
| 2026-10-21 | **Substitui** as 2 poses por golpe: cada golpe é uma curva de chaves (`ChaveGolpe`), diferença para a guarda, com pernas e cotovelos | Pular entre 2 poses parecia robótico ("braços estáticos"). A curva tem antecipação, ataque com o corpo todo e passar do ponto; medido: a bacia afunda 2 a 5 cm no preparo e sobe 2 a 6 cm no ataque, sem o lutador cair sozinho |
| 2026-10-21 | Cotovelo só visual (o antebraço do modelo dobra no desenho; a física continua com o braço reto) | Cotovelo físico pediria 2 peças a mais e deixaria a física mais frágil; o visual resolve o "braço de pau". No acerto o braço está esticado, então visual e física batem |
| 2026-10-21 | Chaves podem vir do Mixamo (`extrai_animacoes --golpe`, `preparar_modelo.py --com-animacao`) | Movimentos reais; guia em `docs/ANIMACOES.md` |
| 2026-10-21 | Testada e descartada: clavícula do modelo seguindo o braço | Não mudou nada visível no Tai Lung; o ombro "inchado" com o braço levantado vem dos pesos da malha do Meshy |
| 2026-10-21 | Chaves dos golpes refeitas a partir de técnicas reais de sumô (tsuppari, hazu-oshi, harite, gaburi-yori, tachiai); Mixamo descartado | As animações do Mixamo são genéricas (empurrar caixa, boxe). Referências em `docs/ANIMACOES.md` |
| 2026-10-21 | Empurrão com duas estocadas alternadas (`tempo_ativo` 0,20) e barrigada com dois pulos (`tempo_ativo` 0,28) | É como as técnicas são; a janela de acerto cobre os dois movimentos |
| 2026-10-21 | Cotovelo do desenho com o mesmo atraso do braço físico no golpe (`COTOVELO_SUAVIZACAO_GOLPE`) | Medido: o braço leva ~50 ms para chegar; sem atraso, o cotovelo dobrava de volta antes de o braço esticar |
| 2026-10-21 | O cotovelo do desenho estica quando o antebraço encostaria no outro (`corpo_limitar_cotovelos`, `COTOVELO_FRACAO` 0,26, `COTOVELO_RAIO` 0,12 m) | Medido (malha de um contra as caixas do outro): com o cotovelo visual, 24% dos quadros tinham malha mais de 6 cm dentro do outro (até 12,7 cm), quase tudo antebraço e pata; agora 1% (pior 6 cm). A física já separava os corpos: sem cotovelo era 0% |
| 2026-10-21 | Contato entre lutadores mais firme: 50 iterações do solucionador (eram 20), ERP 0,8 entre os lutadores, atrito peça x peça 0,35 (era 0,6) | Medido: o maior tempo "grudado" caiu de 14 s para ~5 s, e o afundamento físico ficou passageiro (até 0,5 s); mesmo custo de partida |
| 2026-10-21 | O passo de recuperação não pisa a menos de 0,45 m da bacia do adversário | A mola do passo empurrava o pé para dentro das pernas do outro |
| 2026-10-21 | Golpes recebidos em sequência valem menos (100%, 60%, 36%...; zera após 1,5 s sem levar golpe) | Medido: o maior salto de dano em 2 s caía de 33% (o mesmo do vídeo do grupo) para 19%. Cortar o dano de todos os golpes foi testado e descartado: o dano empacava em ~12% e as partidas dobravam de duração |
| 2026-10-21 | Câmera com zoom: perto (55% da distância) com os lutadores colados, abre quando se afastam; mira no peito quando perto; acompanha até 1,6 m do centro | Os lutadores ficavam pequenos na tela; só a distância muda (não a direção), então o analógico continua igual |
| 2026-10-21 | Câmera: zoom máximo 72% (era 55%), mais lento e com faixa maior | Muito perto e "respirando" a cada empurrão |
| 2026-10-21 | Golpe leve não congela o jogo (hitstop só no forte, 60 ms); acerto sem pausa não interrompe os passos de física | Medido: ~1 pausa por segundo, 5,4% do tempo congelado: parecia travar. O desenho leva 3,6 ms por quadro, então não era desempenho |
| 2026-10-21 | Empurrão, barrigada e investida empurram pelo centro do peito (`no_peito` em golpe.h); empurrão mais forte (impulso 380, avanço 60), barrigada 310, investida 360 | Medido: 86% dos acertos pegavam no braço da guarda, 46 cm para o lado, e o empurrão recuava o alvo só 11 cm. Agora 33 cm |
| 2026-10-21 | Pernas no reflexo: afunda e abre a base; passo de recuperação mais cedo (`PASSO_LIMIAR` 0,25) | Quem levava golpe escorregava inteiro sem dar passo (até 47% das vezes na barrigada); agora 2% a 9% |
| 2026-10-21 | Menos quedas: mola sem tombar o corpo no golpe (`INCLINACAO_IMPACTO` 0), mais firme cambaleando (0,85), pernas cedem menos no impacto, força do golpe mais na bacia (65%), pés deslizam mais (atrito 0,32) | Medido: 9 em 10 quedas eram o lutador SENTANDO logo após um golpe (o corpo girava em volta dos pés presos). Rounds por saída do ringue x por queda: 1 x 44 antes, 10 x 38 agora; partidas com mediana de ~34 s |
| 2026-10-21 | `ACERTO_FORCA` 0,95 (era 0,80): a força com 0% de dano é a que antes só vinha com ~12% | Pedido do grupo: o empurrão bom era com 10 a 15% de dano; antes disso ficavam grudados. Medido: empurrão recua 66 cm (era 33), rounds por saída x queda 22 x 27 (era 10 x 38), maior tempo grudado 3,2 s (era 5,3), partidas com mediana de 29 s |
| 2026-10-21 | Grupo aprovou as mudanças nos .h: `personagem.h` (EstiloMovimento, osso_cotovelo), `corpo.h` (reflexos, passo, cotovelos, movimento extra, `corpo_limitar_cotovelos`), `golpe.h` (chaves no lugar de pose_preparo/pose, `golpe_amostrar`, `no_peito`), `pele.h` (cotovelos em `pele_desenhar`), `lutador.h` (golpes seguidos), `render.h` (zoom); comentários em `combate.h` e `partida.h` | Exigência do AGENTS.md para mudar contratos |
| 2026-10-28 | Grupo aprovou as mudanças nos .h do corpo de 13 peças: `esqueleto.h` (antebraços, pés, cotovelos, tornozelos), `personagem.h` (`centro_massa`; sai `osso_cotovelo`), `golpe.h` (`ChaveGolpe` só com a pose; `agarra`), `corpo.h` (sai o cotovelo visual; `corpo_definir_forca_muscular`, `corpo_agarrar`, `corpo_soltar`, `corpo_impulso` no corpo inteiro), `pele.h` (sem `cotovelo`), `config.h`; regra de queda só nos pés | Exigência do AGENTS.md para mudar contratos |
| 2026-10-28 | **Substitui** "9 peças e 8 articulações" e o cotovelo visual: 13 peças (antebraço com cotovelo físico, pé com tornozelo) e 12 articulações; braços e pernas viram cápsulas | Medido (medidor por peça): braços sem cotovelo e pés sem tornozelo não tinham física; tronco, bacia e cabeça andavam como um bloco (cintura cedia 4°). Malha dentro do outro: 1% dos quadros (meta ≤ 2%) |
| 2026-10-28 | Regra de queda: só as solas dos pés podem tocar o chão (joelho e canela também perdem) | Regra real do sumô; antes a canela contava como pé porque o pé não existia |
| 2026-10-28 | Massas pela tabela de De Leva (1996) com desvio por personagem e centro de massa deslocado (`centro_massa`) | As diferenças entre os personagens passam a vir da física (barriga do Po, ombros do Tai Lung) |
| 2026-10-28 | Músculo de mola (mola + amortecedor implícitos no motor do ODE, `MUSCULO_*`), com velocidade limitada e cintura/pescoço mais macios | O motor antigo corria atrás do alvo com força máxima (parecia animação). Medido: cintura cede 8 a 9° quando tocada (era 4 a 6°), quadris 10 a 20°. Sem o limite de velocidade, a perna esticava de uma vez e o lutador pulava no próprio golpe |
| 2026-10-28 | Impulso de avanço e de empurrão no corpo inteiro, dividido pela massa (`corpo_impulso`) | Inteiro na bacia (mais leve agora), ela disparava e arrancava os pés do chão; trancos entre peças caíram de 28-41 m/s para 2-3,5 m/s no tronco |
| 2026-10-28 | Mola de equilíbrio **mantida inteira** (`MOLA_EQUILIBRIO_FRACAO` 1,0); o plano era 1/3 | Testado 1/3, 1/2 e 2/3, com a parte que sai virando torque dos músculos das pernas com reação nos pés: os rounds caíram para 16-20 s e quase todos terminavam em queda (o lutador senta). Fica para um controlador de equilíbrio de verdade. O tornozelo (`TORNOZELO_EQUILIBRIO`) e o atrito da ponta do pé ficaram: parado, o Po balança 1,9 cm (era 3,2) e o Tai Lung 1,1 cm (era 1,7) |
| 2026-10-28 | Cansaço: stamina zerada deixa os músculos com 75% da força (`CANSACO_FORCA_MINIMA`) | Medido: cansado (stamina < 40) cai 12,8 vezes por minuto; descansado, 3 |
| 2026-10-28 | Pegada na faixa só na barrigada (`Golpe.agarra`): junta de bola mão-outro, solta entre 0,25 e 1,2 s, ou com mais de 900 N, ou se alguém cai | Gaburi-yori de verdade segura a faixa. Medido: 78 pegadas em 72 barrigadas, média 0,25 s, maior 1,03 s; nenhuma presa |
| 2026-10-28 | Grupo aprovou 6 golpes novos no fim do enum de `golpe.h` (*oshi-dashi*, *yori-kiri*, *uwate-nage*, *shitate-nage*, *hataki-komi*, *hiki-otoshi*) e os campos `derruba` e `PES` (tornozelos nas chaves); `avanco` e `impulso` negativos (recuar, puxar) | Mais variedade, só técnicas reais de sumô (kimarite), escritas à mão: as animações prontas (Mixamo, Quaternius) são de briga, não de sumô |
| 2026-10-28 | Projeções e puxões são alavancas (`derruba`): força no alto do tronco e sem o fator de velocidade da mão | A mão segura devagar: com o fator, a força caía para 45% e nada acontecia. Medido (alvo parado, 1 golpe): projeção por cima derruba 29% e joga 1,5 m de lado; por baixo 14% e 1,2 m. Com 450 N.s derrubava 93% (forte demais) |
| 2026-10-28 | Puxar p/ baixo e tapa p/ baixo saem DE LADO e sem pegada | Peito com peito, puxar para si só fazia o outro bater no atacante; a pegada o segurava junto. Contra quem avança, o puxado passa direto (até 1 m) |
| 2026-10-28 | Os 5 golpes antigos usam o cotovelo físico (estocadas esticam de verdade, tapa estala o antebraço) e a ponta do pé (investida, empurrão) | O corpo novo permite; partidas continuam com 22 a 25 s |
| 2026-10-28 | Tela de montagem com 11 golpes fica com outro integrante | A lista hoje cabe ~8; até lá ela passa do painel (só visual) |
| 2026-10-28 | Oogway entra no catálogo (personagem 3): malha pintada do Meshy sem rig; esqueleto com os nomes do Mixamo montado no Blender (juntas pelos centros da malha, pesos automáticos, braços girados para a T-pose); `preparar_modelo.py --textura 2048` (só a cor, em PNG: a raylib do jogo não lê JPEG) | Pedido do usuário. Medido: parado balança 0,2 cm. Com `forca_equilibrio` 1000 ganhava 12 de 12 do Tai Lung. Equilibrado pelo que combina com ele (velho): o equilíbrio por kg igual ao dos outros (950, 9,3/kg; com 650 ele era o peso-pesado que mais recuava), e a fraqueza na velocidade (0,8 m/s) e na força de andar e empurrar (560). Medido: ganha 7 de 12 do Po e 6 de 12 do Tai Lung; anda a 0,67 m/s (Po 0,95, Tai Lung 1,18) |
| 2026-10-28 | Minotauro entra no catálogo (personagem 4): malha pintada do Meshy sem rig; esqueleto com os nomes do Mixamo montado no Blender (juntas pelas fatias da malha, pesos automáticos); `preparar_modelo.py --textura 2048`; bacia, tronco e coxas ajustados à mão como no Oogway | Pedido do usuário. Com robôs, 20 partidas contra cada um: ganha 13 do Po, 9 do Tai Lung e 12 do Oogway (`forca_andar` 750, `forca_equilibrio` 850, `velocidade_max` 0,9) |
| 2026-10-28 | Mais saídas do ringue: ringue no tamanho real (raio 2,3 m), quem está desequilibrado não começa golpe e as pernas largam o golpe em andamento (`GOLPE_REFLEXO_*`), passo de recuperação mais largo (`PASSO_MAX` 0,60) e 15% do impulso no ponto do toque (era 35%) | Medido em 60 partidas: rounds terminados fora passaram de 14% para 27%; metade das quedas era de quem estava golpeando desequilibrado (agora 0 a 5%). Testados e descartados: pés mais escorregadios e não afundar os joelhos caindo para trás (pioraram) |
| 2026-10-28 | Pausa de impacto do golpe forte 35 ms (era 60) | Com os golpes novos (quase todos fortes) a pausa aparecia a cada 2-3 s e parecia travamento |
| 2026-10-28 | Minotauro: pé físico do tamanho do dos outros (20 x 30 cm; os cascos medidos davam 15 x 22), braço e canela com a medida comprida no sentido do membro (a cápsula ficava deitada) e equilíbrio 1050 (9,3/kg, como os outros; era 850 = 7,5/kg) | O touro, pesado em cima e de pernas curtas, caía 2,6 vezes por minuto de luta; agora 1,0 (Po 2,5, Tai Lung 3,7, Oogway 1,8), coerente com ser o mais pesado (113 kg). Rounds fora do ringue: 30% |
| 2026-11-05 | Smart App Control do Windows bloqueava o `ninja.exe` e o `demo_partida.exe` ("política de Controle de Aplicativo"); o usuário o desligou. Pasta `build-make` (MinGW Makefiles) fica como alternativa ao `build` | Sem isso não dava para compilar com o CMake nem rodar as verificações |
| 2026-11-05 | Tapa lateral acerta também com o braço (e janela de 0,16 s); investida acerta também com o antebraço direito | Medido (48 partidas, todos os golpes): o tapa só acertava com o Tai Lung (Po 7%, Oogway 21%, Minotauro 23%); agora 50 a 94%. A investida do Minotauro acertava 39% (os chifres chegavam antes do tronco); agora 100% |
| 2026-11-05 | Oogway: juntas do ombro e do cotovelo na profundidade dos ossos (z -0,15), e não no meio do corpo | O casco empurra os ombros para trás; com a junta no meio, o braço girava em volta do ponto errado. Medido: empurrão 67 -> 100%, tapa 27 -> 50%, empurrar fora 22 -> 55%; no total 84% dos golpes acertam |
| 2026-11-12 | Nome do jogo: **Sumô-Fu Panda** | Escolhido pelo grupo (é também o nome do repositório no GitHub) |
| 2026-11-12 | Fila do golpe (`GOLPE_BUFFER` 0,25 s, `Lutador.golpe_pedido`): o aperto fica guardado e sai assim que o lutador puder golpear; grupo aprovou a mudança no `lutador.h` | O aperto era jogado fora quando o lutador ainda terminava um golpe ou se equilibrava, e o botão parecia não responder. Medido (jogador que aperta no ritmo de uma pessoa): apertos que viram golpe passaram de 47% para 57%; o resto é martelar o botão durante um mesmo golpe |
| 2026-11-12 | Telas de seleção e montagem para 4 personagens e 11 golpes: seleção em grade (`SELECAO_CARTOES_POR_LINHA` 4) com barras comparadas ao máximo do catálogo (peso, velocidade, firmeza) e uma frase por personagem numa tabela em `telas.c`; montagem com lista rolando (`MONTAGEM_GOLPES_VISIVEIS` 7, janela centrada no cursor), setas desenhadas, selos FORTE/FAIXA e descrição em caixa fixa com quebra de linha | Com 4 personagens e 11 golpes as telas antigas não cabiam: o Minotauro (113 kg) estourava a barra de peso fixa em 110 kg e os últimos golpes caíam por cima do combo. A janela centrada não precisa de estado novo em `TelaMontagem` (sem mudar o .h); a fonte padrão da raylib não tem ▲ ▼ |

## Em aberto

- **Tempo limite do round?** Com robôs, alguns rounds demoram (há rounds de mais de 40 s no meio do
  ringue); com pessoas, provavelmente menos. Testar com a turma antes de decidir.

- Modelo 3D do Po (hoje em blocos). Personagens: Po, Tai Lung, Oogway e Minotauro.
- Valores finais de golpes, massas e forças (ajuste na semana 2).
- O combo fica salvo entre execuções do jogo? (hoje: só enquanto o jogo está aberto)
