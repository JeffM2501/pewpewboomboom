#pragma once

#include <string_view>
#include "player_state.h"
#include "event_source.h"

namespace ChatWindow
{
    static constexpr uint64_t ServerChatId = uint64_t(-1);
    static constexpr uint64_t SystemChatId = ServerChatId - 1;

    void AddLogLine(std::string_view data);
    void AddChatLine(PlayerState* from, std::string_view data);
    void AddSystemChatLine(std::string_view data);

    extern EventSource<std::string> OnSendChatMessage;

    void Show();
}