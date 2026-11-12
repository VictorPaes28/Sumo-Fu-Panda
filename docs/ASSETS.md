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

Gerados no rFXGen (ou outra fonte livre), em `.wav`, em `assets/sons/`:

| Arquivo | Quando toca |
|---|---|
| `golpe_leve.wav` | acerto de golpe leve |
| `golpe_forte.wav` | acerto de golpe forte |
| `queda.wav` | alguém caiu ou saiu do ringue |
| `contagem.wav` | cada número do 3, 2, 1 |
| `lute.wav` | início da luta |
| `vitoria.wav` | fim da partida |
| `menu_mover.wav` | mover o cursor nos menus |
| `menu_confirmar.wav` | confirmar nos menus |

Arquivo faltando não quebra o jogo: ele só fica sem aquele som ou modelo (desenha caixas).
