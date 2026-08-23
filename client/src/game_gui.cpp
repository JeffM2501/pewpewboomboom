#include "game_gui.h"

#include "guiControls/chat_window.h"
#include "guiControls/player_lisit_window.h"

namespace GameGui
{
    void Show()
    {
        ChatWindow::Show();
        PlayerListWindow::Show();
    }
}