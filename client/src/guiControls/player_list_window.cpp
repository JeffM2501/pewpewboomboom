#include "player_lisit_window.h"

#include "imgui.h"
#include "raylib.h"
#include "client_network_manager.h"
#include "extras/IconsFontAwesome6.h"

namespace PlayerListWindow
{
    static ImVec2 WindowSize(200, 400);

    void Show()
    {
        ImVec2 pos(0, 0);
        ImGui::SetNextWindowPos(pos);

        ImVec2 currentSize = WindowSize;
        float neededSize = ((Network.GetPlayerList().Size() + 1) * ImGui::GetFrameHeightWithSpacing());
        if (neededSize > 0 && neededSize < currentSize.y)
            currentSize.y = neededSize;

        ImGui::SetNextWindowSize(currentSize);

        if (ImGui::Begin("Players", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoResize))
        {

            Network.GetPlayerList().DoForEachPlayer([](ClientPlayerState* player)
                {
                    if (!player->IsLocalPlayer)
                        return;
                    ImGui::TextUnformatted(ICON_FA_STAR);
                    ImGui::SameLine();

                    ImGui::TextColored(ImVec4{ 0.5f, 1.0f, 0.5f, 1.0f }, player->Name.Data());
                });

            Network.GetPlayerList().DoForEachPlayer([](ClientPlayerState* player)
                {
                    if (player->IsLocalPlayer)
                        return;

                    ImGui::TextUnformatted(ICON_FA_CIRCLE);
                    ImGui::SameLine();
                    ImGui::TextUnformatted(player->Name.Data());
                });
        }
        ImGui::End();

    }
}