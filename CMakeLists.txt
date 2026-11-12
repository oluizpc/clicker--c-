cmake_minimum_required(VERSION 3.20)
project(DzkLPClicker VERSION 0.1.0 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui)

add_executable(clicker
  src/main.cpp
  src/app.cpp
  src/config.cpp
  src/input.cpp
  src/theme.cpp
  src/tray.cpp
  src/ui.cpp
  src/worker.cpp
  res/app.rc
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
  ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
  ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
)

# O executável final já sai com o nome certo.
set_target_properties(clicker PROPERTIES OUTPUT_NAME "DzkLP-Clicker")

# Sem janela de console no Release (no Debug o console ajuda a ver erros).
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set_target_properties(clicker PROPERTIES WIN32_EXECUTABLE ON)
endif()

target_include_directories(clicker PRIVATE
  src
  third_party
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
  ${IMGUI_DIR}/misc/cpp
)

# Versão vem do project() acima, gerada em build/generated/version.h.
configure_file(res/version.h.in ${CMAKE_BINARY_DIR}/generated/version.h @ONLY)
target_include_directories(clicker PRIVATE ${CMAKE_BINARY_DIR}/generated)

target_compile_definitions(clicker PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

target_link_libraries(clicker PRIVATE d3d11 dxgi d3dcompiler dwmapi winmm)

if(MINGW)
  # .exe autossuficiente: runtime C/C++ embutido, roda em qualquer Windows sem instalar nada.
  target_link_options(clicker PRIVATE -static -static-libgcc -static-libstdc++)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(clicker PRIVATE -O2)
    target_link_options(clicker PRIVATE -s)
  endif()
endif()

# Teste de precisão das threads (não clica de verdade): cmake --build build --target cps_test
add_executable(cps_test EXCLUDE_FROM_ALL tests/cps_test.cpp src/worker.cpp)
target_include_directories(cps_test PRIVATE src)
target_compile_definitions(cps_test PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(cps_test PRIVATE winmm)
if(MINGW)
  target_link_options(cps_test PRIVATE -static)
endif()
