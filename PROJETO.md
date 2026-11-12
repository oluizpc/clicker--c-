# DzkLP - Clicker (C++), resumo do projeto

Documento de retomada. Atualizado em 12/11/2026.

## O que é

Versão em **C++** do autoclicker que antes era em Rust (projeto separado, mantido como
referência). Interface com **Dear ImGui** sobre **Direct3D 11**, cliques no nível do sistema
operacional (Windows `SendInput`). É uma ferramenta de SO, não injeta em jogo nenhum.

- **Executável:** `build\DzkLP-Clicker.exe` (caminhos relativos à raiz do repositório)
- `.exe` autossuficiente (runtime estático): só usa DLLs que já vêm no Windows 10/11.
- Lê e grava o mesmo `autoclicker_config.json` da versão Rust (formato idêntico).

## Funcionalidades (todas portadas da versão Rust)

- CPS de 1 a 100 com **jitter** (variação entre mínimo e máximo)
- Botão **esquerdo / direito / meio**
- Modos: **Toggle**, **Segurar tecla**, **Segurar mouse esquerdo**
- **Captura de hotkey** (clica em "Tecla" e aperta qualquer tecla; Esc cancela)
- **Atraso ao iniciar**, **limite de cliques**, **rajada**
- **Tela inicial de escolha da janela** (filtro "Só Minecraft" / "Todas", busca, duplo clique)
- **Perfis** salvos em disco, **tema claro/escuro**
- **Timing de alta precisão** (timer de alta resolução + spin nos últimos 1,5 ms)
- Layout responsivo (3 colunas / 2+1 / 1), menu lateral, card de status com CPS alvo e real
- Ícone "D" embutido no .exe e na janela; suporte a DPI alto (telas com escala 125%, 150%...)
- **Minimizar para a bandeja** (opção em Aparência > Janela, ligada por padrão): clique no ícone
  volta a janela; botão direito tem Abrir, Iniciar/Parar (modo toggle) e Sair. Na bandeja o app
  não desenha nada (CPU ~0%) e o texto do ícone mostra se está ativo ou parado.
- **Janela lembra tamanho, posição e se estava maximizada** (campo `window` no JSON, fora dos
  perfis). Se o lugar salvo não estiver em nenhum monitor (ex.: monitor desconectado), abre no padrão.

## Estrutura do código

| Arquivo              | Responsabilidade                                          |
|----------------------|-----------------------------------------------------------|
| `src/main.cpp`       | Janela Win32 + Direct3D 11, loop de mensagens, threads    |
| `src/app.cpp/.hpp`   | Interface: tela de escolha, menu lateral, páginas         |
| `src/ui.cpp/.hpp`    | Componentes: cartões, pílulas, botões, logo, textos       |
| `src/theme.cpp/.hpp` | Paleta (ardósia + âmbar), estilo, fontes, escala de DPI   |
| `src/worker.cpp/.hpp`| Threads de hotkey e de clique + estado compartilhado      |
| `src/tray.cpp/.hpp`  | Ícone e menu na bandeja do sistema                        |
| `src/input.cpp/.hpp` | WinAPI: enviar clique, ler tecla, listar janelas, UTF-8   |
| `src/config.cpp/.hpp`| Settings + perfis em JSON                                 |
| `res/app.rc`         | Ícone e informações de versão do .exe                     |
| `tests/cps_test.cpp` | Teste de precisão das threads (não clica de verdade)      |
| `third_party/`       | Dear ImGui 1.92.5 e nlohmann/json 3.11.3 (copiados aqui)  |

A versão do app fica no `project(... VERSION x.y.z)` do `CMakeLists.txt`.

## Como compilar

Precisa de MinGW-w64 (g++ 13 ou mais novo), CMake e Ninja no PATH. O `build.ps1` também
procura em `C:\mingw64\bin`, se essa pasta existir.

Na raiz do repositório:

```
.\build.ps1          # gera build\DzkLP-Clicker.exe
.\build.ps1 -Test    # e roda o teste de CPS
```

Resultado do teste em 12/11/2026: CPS real dentro de 0,5% do configurado de 10 a 100 CPS;
limite, janela-alvo, atraso, rajada e jitter conferidos.

## Diferenças em relação à versão Rust

- O ícone da janela vem do `.ico` embutido (antes era desenhado em tempo de execução).
- Perfis são salvos no disco na hora (antes só ao mudar alguma configuração ou fechar).
- Enter no campo de nome do perfil também salva.
- No layout de 2 colunas os cartões não esticam, para o "Avançado" continuar visível.
- Visual muito próximo, mas não idêntico pixel a pixel (Segoe UI no lugar da fonte do egui).

## Pendentes (melhorias)

- [x] Minimizar para a **bandeja** (system tray), feito em 12/11/2026
- [x] Mostrar a **tecla atual** também no card de status, feito em 12/11/2026
- [x] Janela **lembrar tamanho/posição**, feito em 12/11/2026
- [ ] Jitter de **posição** do cursor (mover alguns pixels por clique)

## Fora de escopo (combinado)

Recursos de cliente de cheat de Minecraft que **injetam no jogo** (módulos de combate,
movimento, ESP etc.) **não serão construídos aqui**. O que segue bem-vindo: evoluir o
autoclicker de nível de SO (funções, usabilidade, interface).
