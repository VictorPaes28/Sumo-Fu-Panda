# preparar_modelo.py — prepara um modelo com rig (FBX do Meshy/Mixamo) para o jogo.
#
# Uso (no terminal, com o Blender instalado):
#   blender -b --python ferramentas/preparar_modelo.py -- ENTRADA.fbx SAIDA.glb
#   blender -b --python ferramentas/preparar_modelo.py -- ANIMACAO.fbx SAIDA.glb --com-animacao
#       (animação do Mixamo para extrai_animacoes --golpe: mantém a animação;
#        o FBX pode vir sem malha, "Without Skin")
#   blender -b --python ferramentas/preparar_modelo.py -- PINTADO.fbx SAIDA.glb --textura 2048
#       (modelo pintado do Meshy: mantém só a textura de cor, reduzida para
#        2048 x 2048, e descarta os mapas que o jogo não usa: normal, metal e
#        rugosidade. A textura do Meshy vem com ~90 MB.)
#
# O que faz:
#   1. remove as animações (a pose de descanso, T-pose, é a que o jogo usa);
#   2. aplica as escalas e rotações do FBX (o Meshy/Mixamo exporta em
#      centímetros, com o esqueleto em 0,01 e a malha em 100; sem isso, no jogo
#      a malha fica com milímetros e os ossos com metros);
#   3. coloca os pés em y = 0;
#   4. exporta GLB com Y para cima (o formato que a raylib lê).
#
# Depois, confira com:  inspeciona_modelo SAIDA.glb
# e copie as posições dos ossos para personagem.c (docs/ASSETS.md).
import os
import tempfile
import bpy
import sys

argv = sys.argv[sys.argv.index("--") + 1:]
entrada, saida = argv[0], argv[1]
com_animacao = "--com-animacao" in argv[2:]
textura = int(argv[argv.index("--textura") + 1]) if "--textura" in argv else 0

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=entrada)
arm = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
malhas = [o for o in bpy.data.objects if o.type == 'MESH']

# 1. Sem animação: vale a pose de descanso. (Com --com-animacao, fica: o
#    extrator usa só os giros dos ossos, que não mudam com a escala.)
if not com_animacao:
    arm.animation_data_clear()
    for acao in list(bpy.data.actions):
        bpy.data.actions.remove(acao)


def aplicar(objeto, local, rotacao, escala):
    bpy.ops.object.select_all(action='DESELECT')
    objeto.select_set(True)
    bpy.context.view_layer.objects.active = objeto
    bpy.ops.object.transform_apply(location=local, rotation=rotacao, scale=escala)


# 2. Separa a malha do esqueleto (mantendo a posição) e aplica as transformações.
for m in malhas:
    mundo = m.matrix_world.copy()
    m.parent = None
    m.matrix_world = mundo
for o in [arm] + malhas:
    aplicar(o, True, True, True)

# 3. Pés no chão (só com malha; a animação "Without Skin" não tem).
bpy.context.view_layer.update()
zmin = 0.0
if malhas:
    zmin = min((m.matrix_world @ v.co).z for m in malhas for v in m.data.vertices)
    for o in [arm] + malhas:
        o.location.z -= zmin
        aplicar(o, True, False, False)

# Religa a malha ao esqueleto.
for m in malhas:
    m.parent = arm
    if not any(md.type == 'ARMATURE' for md in m.modifiers):
        md = m.modifiers.new("Armature", 'ARMATURE')
        md.object = arm

# Textura: o shader do jogo (src/luz.c) só usa a cor. Os outros mapas só
# deixariam o arquivo maior.
if textura > 0:
    for mat in bpy.data.materials:
        if not mat.use_nodes:
            continue
        nos = mat.node_tree.nodes
        bsdf = next((n for n in nos if n.type == 'BSDF_PRINCIPLED'), None)
        if bsdf is None:
            continue
        cor = bsdf.inputs['Base Color'].links
        manter = cor[0].from_node if cor else None
        for n in list(nos):
            if n.type == 'TEX_IMAGE' and n != manter:
                nos.remove(n)
        for nome in ('Normal', 'Metallic', 'Roughness'):
            for ligacao in list(bsdf.inputs[nome].links):
                mat.node_tree.links.remove(ligacao)
        bsdf.inputs['Metallic'].default_value = 0.0
        bsdf.inputs['Roughness'].default_value = 1.0
        if manter is not None and manter.image is not None:
            img = manter.image
            if max(img.size) > textura:
                img.scale(textura, textura)
            # A raylib do jogo não lê JPEG, e o exportador mantém o formato de
            # dentro da imagem: grava um PNG de verdade e usa ele.
            caminho = os.path.join(tempfile.mkdtemp(), "cor.png")
            img.file_format = 'PNG'
            img.save(filepath=caminho)
            manter.image = bpy.data.images.load(caminho)
            manter.image.pack()
            print("textura de cor:", img.name, tuple(manter.image.size), "PNG")

print("pés deslocados em %.3f m" % -zmin)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.export_scene.gltf(filepath=saida, export_format='GLB', export_yup=True,
                          export_animations=com_animacao)
print("exportado:", saida)
