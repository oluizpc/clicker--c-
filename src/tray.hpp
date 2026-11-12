// Ícone na bandeja do sistema (perto do relógio) para o app minimizado.
#pragma once

#include <windows.h>

namespace tray {

// Mensagem que o Windows manda para a janela quando o ícone é clicado.
constexpr UINT kCallbackMsg = WM_APP + 1;

// IDs dos itens do menu de botão direito.
enum MenuId : UINT { kOpen = 1, kToggle = 2, kExit = 3 };

void init(HWND hwnd, HICON icon);
void show(const wchar_t* tip);     // coloca o ícone (ou atualiza, se já estiver lá)
void hide();                       // tira o ícone
void set_tip(const wchar_t* tip);  // muda o texto ao passar o mouse
bool visible();

// Mensagem "TaskbarCreated": o Explorer reiniciou e o ícone precisa voltar.
UINT taskbar_created_msg();

// Menu de botão direito na posição do cursor. Retorna o MenuId escolhido (0 = nada).
UINT popup_menu(bool show_toggle, bool active);

}  // namespace tray
