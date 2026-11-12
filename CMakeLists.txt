# =============================================================================
# Jogo de Sumô Kung Fu Panda — build
#
# Alvos:
#   teste_combo, teste_hematoma  testes das estruturas de dados (C puro, sem raylib/ODE)
#   jogo                         o jogo completo (baixa raylib 6.0 e ODE 0.16.6)
#   demo_telas                   demonstração das telas 2D, sem física (Integrante B)
#   demo_fisica                  demonstração de um corpo articulado, sem telas (Principal)
#
# Precisa de compilador C e C++ (gcc e g++ do MinGW-w64): o ODE é C++ por dentro.
#
# Para compilar só os testes (rápido, sem baixar nada):
#   cmake -S . -B build -DSUMO_SO_TESTES=ON
#   cmake --build build
#   ctest --test-dir build --output-on-failure
#
# Para compilar tudo:
#   cmake -S . -B build
#   cmake --build build
# =============================================================================
cmake_minimum_required(VERSION 3.20)
project(jogo_sumo C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(SUMO_SO_TESTES "Compila só os testes das estruturas de dados (sem raylib e ODE)" OFF)

# Avisos ligados para todo o código do grupo (não para as bibliotecas baixadas).
if(MSVC)
    set(SUMO_AVISOS /W4)
else()
    set(SUMO_AVISOS -Wall -Wextra -Wshadow -Wno-unused-parameter)
endif()

# -----------------------------------------------------------------------------
# Testes das estruturas de dados (combo.c e hematoma.c não dependem de nada)
# -----------------------------------------------------------------------------
enable_testing()

add_executable(teste_combo testes/teste_combo.c src/combo.c)
target_include_directories(teste_combo PRIVATE src)
target_compile_options(teste_combo PRIVATE ${SUMO_AVISOS})
add_test(NAME combo COMMAND teste_combo)

add_executable(teste_hematoma testes/teste_hematoma.c src/hematoma.c)
target_include_directories(teste_hematoma PRIVATE src)
target_compile_options(teste_hematoma PRIVATE ${SUMO_AVISOS})
add_test(NAME hematoma COMMAND teste_hematoma)

if(SUMO_SO_TESTES)
    return()
endif()

# -----------------------------------------------------------------------------
# Dependências: raylib 6.0 e ODE 0.16.6, baixadas na primeira configuração
# -----------------------------------------------------------------------------
include(FetchContent)

# O ODE é escrito em C++ por dentro: precisa do compilador C++ (g++) e o jogo
# precisa ser ligado (linkado) como C++ para levar junto a biblioteca do C++.
# O nosso código continua sendo C99.
enable_language(CXX)

# O CMakeLists do ODE pede uma versão antiga do CMake; o CMake 4 recusa sem isto.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# Bibliotecas estáticas: o .exe final não precisa de DLLs ao lado.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        6.0
    GIT_SHALLOW    TRUE)

# Precisão simples (float), igual à raylib. NÃO mudar sem avisar o grupo:
# o tipo dReal do ODE muda junto e todo o código de física depende disso.
set(ODE_DOUBLE_PRECISION OFF CACHE BOOL "" FORCE)
set(ODE_WITH_DEMOS OFF CACHE BOOL "" FORCE)
set(ODE_WITH_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(ode
    GIT_REPOSITORY https://bitbucket.org/odedevs/ode.git
    GIT_TAG        0.16.6
    GIT_SHALLOW    TRUE)

FetchContent_MakeAvailable(raylib ode)

# -----------------------------------------------------------------------------
# Jogo
# -----------------------------------------------------------------------------
# Todos os .c de src/ entram no jogo. Arquivo novo em src/ entra sozinho
# (rode o cmake de novo depois de criar um arquivo).
file(GLOB SUMO_FONTES CONFIGURE_DEPENDS src/*.c)

add_executable(jogo ${SUMO_FONTES})
target_include_directories(jogo PRIVATE src)
target_compile_options(jogo PRIVATE ${SUMO_AVISOS})
target_link_libraries(jogo PRIVATE raylib ODE)
set_target_properties(jogo PROPERTIES LINKER_LANGUAGE CXX)

# Copia assets/ para o lado do .exe (recursos.c procura lá).
add_custom_command(TARGET jogo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:jogo>/assets)

# -----------------------------------------------------------------------------
# Demonstração da física (Principal) — um corpo articulado sozinho, sem telas.
# Serve para a verificação da semana 1 sem depender do trabalho de A e B.
# Só é criada quando demos/demo_fisica.c existir.
# -----------------------------------------------------------------------------
if(EXISTS ${CMAKE_SOURCE_DIR}/demos/demo_fisica.c)
    add_executable(demo_fisica
        demos/demo_fisica.c
        src/fisica.c src/corpo.c src/personagem.c src/esqueleto.c src/golpe.c
        src/animacao.c src/animacoes_dados.c)
    target_include_directories(demo_fisica PRIVATE src)
    target_compile_options(demo_fisica PRIVATE ${SUMO_AVISOS})
    target_link_libraries(demo_fisica PRIVATE raylib ODE)
    set_target_properties(demo_fisica PROPERTIES LINKER_LANGUAGE CXX)
endif()

# -----------------------------------------------------------------------------
# Demonstração da partida contra um robô (Principal) — luta sem os menus.
# Usa todo o src/ menos o main.c. Só é criada quando demos/demo_partida.c existir.
# -----------------------------------------------------------------------------
if(EXISTS ${CMAKE_SOURCE_DIR}/demos/demo_partida.c)
    set(SUMO_FONTES_SEM_MAIN ${SUMO_FONTES})
    list(FILTER SUMO_FONTES_SEM_MAIN EXCLUDE REGEX ".*/main\\.c$")
    add_executable(demo_partida demos/demo_partida.c ${SUMO_FONTES_SEM_MAIN})
    target_include_directories(demo_partida PRIVATE src)
    target_compile_options(demo_partida PRIVATE ${SUMO_AVISOS})
    target_link_libraries(demo_partida PRIVATE raylib ODE)
    set_target_properties(demo_partida PROPERTIES LINKER_LANGUAGE CXX)
    add_custom_command(TARGET demo_partida POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:demo_partida>/assets)
endif()

# -----------------------------------------------------------------------------
# Ferramenta da arte: mostra o esqueleto de um .glb (docs/ASSETS.md)
# -----------------------------------------------------------------------------
if(EXISTS ${CMAKE_SOURCE_DIR}/ferramentas/inspeciona_modelo.c)
    add_executable(inspeciona_modelo ferramentas/inspeciona_modelo.c
        src/personagem.c src/esqueleto.c)
    target_include_directories(inspeciona_modelo PRIVATE src)
    target_compile_options(inspeciona_modelo PRIVATE ${SUMO_AVISOS})
    target_link_libraries(inspeciona_modelo PRIVATE raylib)
endif()

# Ferramenta: converte animações de um modelo em ângulos das articulações
# e gera src/animacoes_dados.c (ver o comentário no início do arquivo).
if(EXISTS ${CMAKE_SOURCE_DIR}/ferramentas/extrai_animacoes.c)
    add_executable(extrai_animacoes ferramentas/extrai_animacoes.c src/esqueleto.c)
    target_include_directories(extrai_animacoes PRIVATE src)
    target_compile_options(extrai_animacoes PRIVATE ${SUMO_AVISOS})
    target_link_libraries(extrai_animacoes PRIVATE raylib)
endif()

# -----------------------------------------------------------------------------
# Demonstração das telas 2D (Integrante B) — sem física, sem ODE
# Só é criada quando demos/demo_telas.c existir.
# -----------------------------------------------------------------------------
if(EXISTS ${CMAKE_SOURCE_DIR}/demos/demo_telas.c)
    add_executable(demo_telas
        demos/demo_telas.c
        src/entrada.c src/recursos.c src/pele.c src/luz.c src/telas.c src/montagem.c src/hud.c src/ui.c
        src/combo.c src/golpe.c src/personagem.c src/esqueleto.c)
    target_include_directories(demo_telas PRIVATE src)
    target_compile_options(demo_telas PRIVATE ${SUMO_AVISOS})
    target_link_libraries(demo_telas PRIVATE raylib)
    add_custom_command(TARGET demo_telas POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:demo_telas>/assets)
endif()
