# Instruções para IAs (e para humanos) neste repositório

Você está ajudando um grupo de faculdade a fazer um **jogo de sumô 3D em C**,
inspirado em *Sumotori Dreams*, com personagens de *Kung Fu Panda*.
Leia este arquivo inteiro antes de escrever qualquer código.

## Situação do projeto

**O MVP está implementado e jogável**: todos os módulos de `src/` funcionam e os testes
passam. O trabalho agora é **melhorar com mudanças pequenas e testadas**, não reescrever.
Antes de mudar um módulo, leia o `.c` inteiro dele; depois de mudar, rode os testes e as
verificações (`docs/ARQUITETURA.md`, seção 10).

## Leitura obrigatória, nesta ordem

1. `docs/ARQUITETURA.md` — módulos, quem chama quem, o que acontece em um quadro.
2. O arquivo de tarefa da pessoa que você está ajudando, em `docs/tarefas/`.
3. Os `.h` dos módulos da tarefa, em `src/`. **Os `.h` são o contrato**: o comentário
   de cada função diz exatamente o que ela deve fazer.

## Regras que você NÃO pode quebrar

- **Não altere nenhum `.h`** sem que a pessoa diga explicitamente que o grupo aprovou.
  Os `.h` são usados por outros integrantes; mudar uma assinatura quebra o código deles.
- **Só edite os arquivos listados na tarefa** da pessoa. Os outros têm outro dono.
- **Não altere `testes/`** para fazer um teste passar. Corrija o código.
- **C99 puro.** Nada de C++, nada de bibliotecas além de raylib, ODE e a biblioteca padrão do C.
- **Sem variáveis globais.** O estado vive em structs passadas por ponteiro.
  Tabelas constantes (`static const`) dentro de um `.c` são permitidas.
- **`combo.c` e `hematoma.c` não podem incluir `raylib.h` nem `ode/ode.h`.**
- **Só `fisica.c` e `corpo.c` chamam funções do ODE** ou mexem em campos do ODE
  (`dBodyID`, `dGeomID`...). Os outros usam as funções de `corpo.h` e `fisica.h`.
- **Nunca copie `Partida`, `Lutador`, `Corpo` ou `Fisica` por valor** (`Lutador x = *l;`):
  o ODE guarda ponteiros para dentro dessas structs. Use ponteiros.
- **Números ajustáveis vão em `src/config.h`**, nunca soltos no código.
- **Quem aloca, libera.** Todo `malloc` tem o `free` correspondente na função de liberar do módulo.
- Funções de desenho **nunca alteram estado**; funções de lógica **nunca desenham**.

## Estilo

- Nomes em português, sem acento: `combo_avancar`, `dano_total`.
- Funções: `snake_case` com o nome do módulo como prefixo (`hud_desenhar`).
- Tipos: `PascalCase` com `typedef` (`ListaHematomas`). Enums: valores em `MAIUSCULAS` com prefixo.
- Comentários em português. Toda função pública já tem comentário no `.h`; no `.c`,
  comente o *porquê* de trechos não óbvios (não repita o que o código diz).
- Indentação de 4 espaços, chaves de função na linha seguinte, como nos arquivos existentes.
- Unidades no comentário de cada campo: metros, kg, segundos, radianos.

## Como compilar e testar

```bash
# Só os testes das estruturas (não baixa nada, rápido):
cmake -S . -B build -DSUMO_SO_TESTES=ON
cmake --build build
ctest --test-dir build --output-on-failure

# Tudo (baixa raylib 6.0 e ODE 0.16.6 na primeira vez):
cmake -S . -B build
cmake --build build
./build/jogo
```

Sem CMake, os testes também compilam direto com gcc:

```bash
gcc -std=c99 -Wall -Wextra -Isrc testes/teste_combo.c src/combo.c -o teste_combo
```

Verificações da física e da partida (sem janela):

```bash
./build/demo_fisica --teste
./build/demo_partida --teste
```

## Antes de dizer "pronto"

- O código compila **sem avisos** com `-Wall -Wextra`.
- Os testes passam (`ctest`), e `demo_fisica --teste` e `demo_partida --teste` passam
  se você mexeu em física, golpes, personagens, partida ou `config.h`.
- Os critérios de aceite do arquivo de tarefa foram conferidos um por um.
- Nenhum `.h` e nenhum arquivo de outro dono foi alterado.
- Se algo no contrato parecer errado ou faltar, **pare e avise a pessoa**
  em vez de mudar o `.h` por conta própria.
