# Animações dos golpes

Os golpes são **curvas de chaves** (`ChaveGolpe` em `src/golpe.h`): em cada chave, quanto
cada articulação se afasta da guarda do personagem (pernas inclusive) e quanto os cotovelos
dobram. As chaves de hoje são escritas à mão em `src/golpe.c`, a partir de técnicas reais de
sumô (abaixo). O caminho do Mixamo continua pronto, mas foi testado e **não** é usado: as
animações de lá são genéricas (o "push" é empurrar uma caixa, os socos são de boxe).

## As técnicas de sumô usadas como referência

| Golpe do jogo | Técnica real | O que a curva faz |
|---|---|---|
| Empurrão | *tsuppari* (estocadas de palma) | afunda com as mãos no peito; estoca com a direita e depois com a esquerda, cada mão com o pé do mesmo lado deslizando à frente (como no treino de *teppo*) |
| Levantador | *hazu-oshi* (mão em forquilha sob as axilas) | afunda fundo com os cotovelos fechados; as pernas esticam e os braços empurram para cima e à frente, sem jogar o tronco para trás |
| Tapa lateral | *harite* (tapa de mão aberta) | arma o braço esquerdo para o lado, chicoteia na altura do rosto e cruza; a direita protege o peito |
| Barrigada | *gaburi-yori* (empurrar com o tronco) | abraça baixo e dá dois "pulos" com o quadril, projetando a barriga |
| Investida | *tachiai* (a saída) | agacha com o tronco à frente e a cabeça baixa; explode para a FRENTE com a ponta dos pés, antebraços subindo contra o peito do outro |
| Empurrar fora | *oshi-dashi* (empurrar para fora) | as duas palmas no peito ao mesmo tempo, dois empurrões com passos curtos à frente |
| Forçar fora | *yori-kiri* (forçar para fora segurando) | segura a faixa (pegada) e anda com o outro em três passos, quadril baixo |
| Projeção cima | *uwate-nage* (projeção pela pegada de cima) | a mão esquerda pega por cima do braço dele; o corpo gira para a esquerda, a perna recua e o braço puxa para baixo |
| Projeção baixo | *shitate-nage* (projeção pela pegada de baixo) | a mão direita entra por baixo da axila; gira para a direita puxando por baixo do centro dele |
| Tapa p/ baixo | *hataki-komi* (tapa para baixo) | contra-ataque: recua saindo da frente e bate de cima para baixo nas costas de quem avança |
| Puxar p/ baixo | *hiki-otoshi* (puxar para baixo) | segura os braços de quem empurra, recua de uma vez e o puxa para a frente e de lado |

Regras gerais tiradas das técnicas: **cotovelos fechados** perto do corpo, **quadril baixo**
o golpe todo (a força vem das pernas empurrando o chão), **mão e pé do mesmo lado** juntos, e
**antecipação afundando**, nunca recuando o tronco.

Como foi conferido: uma ferramenta de captura mostra cada golpe de lado, de frente e de cima;
outra mede o ângulo pedido e o real dos ombros (o braço físico leva ~50 ms para chegar, por isso
cada estocada "segura" esticada um instante e o cotovelo do desenho tem o mesmo atraso); e a
altura da bacia durante o golpe (nenhum golpe derruba o próprio lutador).

Referências:
[tachiai](https://en.wikipedia.org/wiki/Tachi-ai),
[o tachiai explicado (nippon.com)](https://www.nippon.com/en/japan-topics/d00933/),
[técnicas e kimarite (japantravelplanning)](https://japantravelplanning.com/sumo-techniques/),
[treino de teppo e suriashi (USA Sumo)](https://www.usasumo.com/learn/sumo-training/),
[treinos de sumô (livestrong)](https://www.livestrong.com/article/479298-sumo-wrestling-techniques-and-training-routines/),
[tsuppari, mão aberta (akban)](https://www.akban.org/wiki/Tsupari),
[gaburi-yori (romajidesu)](https://m.romajidesu.com/dictionary/meaning-of-がぶりより.html).

## Trocar por animação gravada (Mixamo ou vídeo do grupo)

Se o grupo gravar os golpes em vídeo e converter em FBX (Rokoko Vision, DeepMotion), ou
achar uma animação boa no Mixamo, o caminho abaixo transforma em chaves.

### O que baixar (Mixamo)

Site: <https://www.mixamo.com> (grátis, precisa de conta Adobe). Use o personagem padrão
(**Y Bot**): o jogo só aproveita os ângulos dos ossos, então serve para qualquer personagem.

Busque pelos termos abaixo, assista e escolha a que mais parece o golpe. Os nomes exatos
mudam; o que importa é o movimento.

| Golpe do jogo | Buscar no Mixamo | O que procurar |
|---|---|---|
| Empurrão | `push`, `shove` | as duas mãos empurrando à frente na altura do peito |
| Levantador | `lift`, `pick up`, `heavy lift` | agacha e ergue algo com os dois braços |
| Tapa lateral | `slap`, `backhand`, `hook` | um braço varrendo de lado |
| Barrigada | `belly bump`, `hip bump`, `chest bump` | projeta o tronco ou a barriga à frente |
| Investida | `tackle`, `shoulder charge`, `body block` | avança de ombro, corpo baixo |
| (reação, futuro) | `hit reaction`, `shoved reaction` | quem leva o golpe |
| (guarda, futuro) | `fighting idle`, `sumo` | parado em posição de luta |

**Configurações ao baixar:**
- Format: **FBX Binary (.fbx)**
- Skin: **With Skin** (sem a malha, o GLB fica sem "skin" e a raylib não lê o esqueleto)
- Frames per Second: **30**
- **In Place** marcado, quando aparecer (o jogo já tem o próprio avanço)
- Keyframe Reduction: **none**

Salve com o nome do golpe, por exemplo `empurrao.fbx`.

### Como converter (Principal ou IA)

1. FBX → GLB mantendo a animação (Blender instalado):
   ```bash
   blender -b --python ferramentas/preparar_modelo.py -- empurrao.fbx empurrao.glb --com-animacao
   ```
2. GLB → chaves, com a duração do golpe em segundos (a de `golpe.c`):
   ```bash
   ./build/extrai_animacoes --golpe empurrao.glb 0.50
   ```
   A ferramenta imprime `static const ChaveGolpe CHAVES_NOVO[] = {...}`, pronto para colar,
   e sugere o **tempo do acerto** (o momento em que a mão anda mais rápido).
   Se o arquivo tiver várias animações, passe um trecho do nome no fim do comando.
3. Em `src/golpe.c`: substitua as chaves do golpe pelo bloco impresso (renomeie
   `CHAVES_NOVO`) e ajuste `tempo_preparo` para um pouco antes do acerto sugerido.
4. Teste: `demo_fisica --teste`, `demo_partida --teste` e jogue.

**Se as pernas exagerarem** (o lutador cai sozinho no próprio golpe), diminua os números das
articulações de quadril e joelho nas chaves (ou multiplique por 0,5). Animações de academia e
de dança costumam mexer demais as pernas; o corpo físico precisa continuar de pé.

### Por que não usar a animação "inteira"

O corpo é físico: os motores seguem a curva, e a física decide o resto (contato, peso,
queda). Por isso o golpe é somado à guarda e não substitui o corpo. O cotovelo é uma junta
física (`ART_COTOVELO_*`): a estocada estica o antebraço de verdade, e é ele que acerta.

### Fontes livres (alternativas)

- **Quaternius Universal Animation Library** (CC0): <https://quaternius.itch.io/universal-animation-library>
  (a versão grátis tem *push*; a 2 tem combos corpo a corpo). Já vem em glTF: pule o passo 1.
- **Evitar:** o conjunto da Bandai Namco é não comercial e proíbe obras derivadas
  (a conversão para o jogo seria uma); os dados da CMU têm qualidade irregular.
