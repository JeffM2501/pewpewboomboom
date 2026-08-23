#pragma once

#include <string_view>
#include "player_state.h"

namespace ChatWindow
{
    void AddLogLine(std::string_view data);
    void AddChatLine(PlayerState* from, std::string_view data);

    void Show();
}