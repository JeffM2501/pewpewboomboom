#include "game_gui.h"

#include "guiControls/chat_window.h"
#include "guiControls/player_lisit_window.h"

#include "client_network_manager.h"

#include "imgui.h"
#include "extras/IconsFontAwesome6.h"

namespace GameGui
{
    constexpr ImVec4 GoodColor(0, 1, 0, 1);
    constexpr ImVec4 WarningColor(1, 1, 0, 1);
    constexpr ImVec4 BadColor(1, 0, 0, 1);

    static void ShowConnectionWindow()
    {
        ImVec2 windowSize(140, 48);

        ImVec2 pos(float(GetScreenWidth())- windowSize.x, 0);
        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(windowSize);
        if (ImGui::Begin("Network State", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
        {
            switch (Network.GetState())
            {
            case ConnectionState::Connected:
                ImGui::TextColored(GoodColor, ICON_FA_WIFI);
                ImGui::SameLine();
                ImGui::Text("%d (%d ms)", Network.GetCurrentServerTick(), Network.GetRTT());
                break;

            case ConnectionState::Connecting:
                ImGui::TextColored(WarningColor, ICON_FA_ARROWS_SPIN);
                break;

            case ConnectionState::Disconnected:
                ImGui::TextColored(BadColor, ICON_FA_BAN);
                if (Network.HadTimeout())
                {
                    ImGui::SameLine();
                    ImGui::TextColored(WarningColor,"Timout");
                }
                break;

            default:
                ImGui::TextColored(WarningColor, ICON_FA_QUESTION);
                break;
            }
        }
        ImGui::End();

    }

    void Show()
    {
        ChatWindow::Show();
        PlayerListWindow::Show();
        ShowConnectionWindow();
    }
}