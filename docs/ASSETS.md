# Arte: modelos, medidas e sons

Este documento diz **como entregar** os modelos para que o jogo consiga usá-los.

## Como o modelo entra no jogo

O personagem é **um modelo inteiro com esqueleto (rig)**, e não peças cortadas.
A física do jogo continua com 13 peças invisíveis (bacia, tronco, cabeça, 2 braços,
2 antebraços, 2 coxas, 2 canelas, 2 pés), e **cada osso do modelo segue a sua peça** (`src/pele.h`).
A malha dobra suave nas juntas, sem frestas.

Por isso, três coisas precisam bater:
1. **A pose:** o modelo vem em **T-pose** (braços abertos na horizontal), a mesma pose
   em que o corpo físico é criado.
2. **A orientação:** pés no chão, olhando para a frente (−Y no Blender, +Z no jogo),
   esquerda do personagem no +X.
3. **As medidas:** as peças físicas em `src/personagem.c` são medidas a partir dos ossos.

## Passo a passo (Meshy → jogo)

1. **No Meshy:** gerar o personagem em **T-pose**, com **rig (humanoide)** e **com textura**.
   Exportar em **GLB** de preferência (leva a textura junto). FBX também serve, mas confira
   se a textura veio junto (ver "Problemas conhecidos").
2. **Preparar o arquivo** (escala em metros, pés no chão, sem animação, GLB):
   ```bash
   blender -b --python ferramentas/preparar_modelo.py -- ENTRADA.fbx assets/modelos/<pasta>/<pasta>.glb
   ```
3. **Conferir e medir:**
   ```bash
   ./build/inspeciona_modelo assets/modelos/<pasta>/<pasta>.glb
   ```
   Mostra a altura, se os pés estão em y = 0 e a posição de cada osso, em metros.
4. **Preencher `src/personagem.c`** (Integrante B):
   - **âncoras das articulações** = posição dos ossos (ombro = `LeftArm`, quadril = `LeftUpLeg`,
     joelho = `LeftLeg`, cintura = `Spine`, pescoço = `Neck`);
   - `.modelo`, `.escala_modelo` (1.0 depois do passo 2) e `.ossos = OSSOS_MIXAMO`;
   - **caixas das peças pela MALHA** (não pelos ossos: os ossos ficam no meio do corpo e as
     caixas sairiam pequenas demais; os modelos se atravessariam sem a física perceber):
     ```bash
     ./build/inspeciona_modelo assets/modelos/<pasta>/<pasta>.glb --caixas <indice>
     ```
     A ferramenta imprime `.centro` e `.tamanho` prontos para colar e quanto da malha fica fora
     das caixas (o Tai Lung tem cerca de 8%). Iguale esquerda e direita; se alguma peça sair
     pequena demais (no Tai Lung, a cabeça), meça essa à mão.

   O Tai Lung já está feito assim e serve de exemplo.

   **Modelo pintado sem rig** (o Meshy exportou só a malha, como o Oogway e o Minotauro):
   o esqueleto com os nomes do Mixamo foi montado no Blender (juntas pelos centros da malha,
   pesos automáticos, braços girados para a T-pose). Para a textura, use
   `preparar_modelo.py ... --textura 2048`: mantém só a cor, em PNG (a raylib do jogo não lê
   JPEG) e 2048 x 2048 (a do Meshy vem com ~90 MB). Com pesos automáticos as coxas pegam
   parte da faixa: meça bacia e coxas à mão (veja os comentários do Oogway em `personagem.c`).
5. **Testar:**
   ```bash
   ./build/demo_partida --capturas 1 1
   ./build/demo_fisica --teste
   ```

## Ossos (rig do Mixamo, usado pelo Meshy)

A tabela `OSSOS_MIXAMO` em `personagem.c` já liga os ossos às peças. Se o rig tiver os nomes
`mixamorig:...`, não precisa mudar nada. Osso que não estiver na tabela (mãos, dedos dos pés,
topo da cabeça) segue o osso pai. Se algum osso da tabela não existir no modelo, o jogo
avisa no console (`PELE: osso ... não existe`).

| Peça física | Ossos |
|---|---|
| bacia | Hips |
| tronco | Spine, Spine1, Spine2, LeftShoulder, RightShoulder |
| cabeça | Neck, Head |
| braço esquerdo / direito | LeftArm, LeftForeArm / RightArm, RightForeArm |
| coxa esquerda / direita | LeftUpLeg / RightUpLeg |
| canela esquerda / direita | LeftLeg / RightLeg |
| antebraço esquerdo / direito | LeftForeArm / RightForeArm |
| pé esquerdo / direito | LeftFoot / RightFoot |

## Problemas conhecidos (vistos no Tai Lung, 21/10)

| Problema | Efeito no jogo | Como resolver |
|---|---|---|
| FBX sem textura e sem material | personagem todo branco | exportar do Meshy **com textura**, de preferência em GLB |
| FBX em centímetros (esqueleto 0,01, malha 100) | malha minúscula, ossos gigantes | `ferramentas/preparar_modelo.py` corrige |
| Modelo centralizado na cintura (pés em −0,8 m) | personagem enterrado no chão | `preparar_modelo.py` corrige |
| ~31 mil triângulos | funciona, mas pesa (a deformação é feita na CPU) | reduzir para ~10 mil (Decimate no Blender ou opção do Meshy) |
| FBX **sem** rig | não serve: sem ossos não há como seguir a física | sempre gerar com rig |

## Sons

Em `assets/sons/`, em `.wav` (ou `.ogg`):

| Arquivo | Quando toca | Origem |
|---|---|---|
| `golpe_leve.wav` | acerto de golpe leve | gerado no rFXGen |
| `golpe_forte.wav` | acerto de golpe forte | gerado no rFXGen |
| `queda.wav` | alguém caiu ou saiu do ringue | gerado no rFXGen |
| `contagem.wav` | cada número do 3, 2, 1 | gerado no rFXGen |
| `lute.wav` | início da luta (depois do 3, 2, 1) | gongo chinês: trecho do vídeo "Chinese Gong Sound Effect" do canal Supa Sounds (YouTube) |
| `vitoria.wav` | fim da partida | gerado no rFXGen |
| `menu_mover.wav` | mover o cursor nos menus | gerado no rFXGen |
| `menu_confirmar.wav` | confirmar nos menus | gerado no rFXGen |
| `menu_abertura.ogg` | uma vez, toda vez que o menu inicial abre | trecho (4,5 s) de "Closing Act (feat. lil nico)", de Scream Geronimo |

Origem: 7 sons foram gerados pelo grupo no **rFXGen** (versão no navegador,
https://raylibtech.itch.io/rfxgen) em 2026-11-14, a partir dos presets e com os parâmetros
ajustados à mão. Formato: WAV, 44100 Hz, 16 bits, mono; até 1 s cada (a `vitoria`, até 2 s).

Em 2026-11-17 entraram dois áudios de terceiros, **sem licença livre**, usados só porque o
jogo é um trabalho acadêmico, sem fins lucrativos, apresentado apenas na disciplina
(decisão do grupo):
- `lute.wav`: o gongo, com o silêncio do começo cortado, 2,5 s com o som sumindo no final e
  volume ajustado aos outros sons (WAV, 44100 Hz, 16 bits, mono).
- `menu_abertura.ogg`: os 4,5 primeiros segundos da música, com 0,3 s de saída suave.

Se o jogo for publicado ou usado fora da disciplina, esses dois precisam ser trocados por
áudios próprios ou livres (por exemplo, o gongo CC BY 4.0 de kevp888 no Freesound:
https://freesound.org/people/kevp888/sounds/711170/, creditando o autor).

## Música

Em `assets/musica/`, em `.ogg` (lida aos poucos com `LoadMusicStream`, em loop):

| Arquivo | Quando toca | Origem |
|---|---|---|
| `menus.ogg` | de fundo, baixa, na seleção de personagem e na montagem do combo | "Oogway Ascends", de Hans Zimmer e John Powell (trilha de *Kung Fu Panda*, DreamWorks) |

Só o ponto alto da faixa: de 0:58 a 1:55 (57 s, OGG estéreo, ~0,9 MB), do pico até o
clímax; 0,8 s de entrada e 3 s de saída suaves, para o loop não dar tranco. O volume é ajustado no jogo
(`MUSICA_MENUS_VOLUME` em `config.h`). É uma obra protegida, **sem licença livre**, usada só
porque o jogo é um trabalho acadêmico, sem fins lucrativos, apresentado apenas na disciplina
(decisão do grupo). Se o jogo for publicado ou usado fora da disciplina, trocar por uma
música própria ou livre.

Arquivo faltando não quebra o jogo: ele só fica sem aquele som ou modelo (desenha caixas).
