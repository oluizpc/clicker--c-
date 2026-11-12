# DzkLP - Clicker (C++)

Auto clicker próprio, em C++ com Dear ImGui. CPS configurável com variação, botão esquerdo/direito/meio,
modos toggle/segurar, hotkey global, atraso, limite, rajada, filtro de janela, perfis e tema claro/escuro.

## Compilar

Precisa de MinGW-w64, CMake e Ninja (aqui em `C:\mingw64\bin`).

```
.\build.ps1
```

Gera `build\DzkLP-Clicker.exe`, que roda sozinho em qualquer Windows 10/11.

## Usar

1. Abra o `DzkLP-Clicker.exe` e escolha a janela onde os cliques vão valer (ou "qualquer janela").
2. Ajuste CPS, botão, modo e a tecla.
3. Aperte a tecla (padrão **F6**) para ligar/desligar, ou segure, nos modos de segurar.

Configurações e perfis ficam em `autoclicker_config.json`, ao lado do `.exe`.

## Observação

Alguns jogos/anticheats bloqueiam cliques sintéticos via `SendInput` ou detectam automação.
Projeto pessoal/educacional; use dentro das regras do software onde for aplicar.
