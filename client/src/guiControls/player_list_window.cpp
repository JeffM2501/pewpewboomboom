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
        float neededSize = ((Network.GetPlayerList().Size()) * ImGui::GetFrameHeightWithSpacing() * 0.75f);
        if (neededSize > 0 && neededSize < currentSize.y)
            currentSize.y = neededSize;

        ImGui::SetNextWindowSize(currentSize);

        if (ImGui::Begin("Players", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoResize))
        {
            if (ImGui::BeginTable("PlayerList", 3))
            {
                ImGui::TableSetupColumn("Player", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Kills", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Deaths", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableHeadersRow();

                Network.GetPlayerList().DoForEachPlayer([](ClientPlayerState* player)
                    {
                        if (!player->IsLocalPlayer)
                            return;
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextUnformatted(ICON_FA_USER_ASTRONAUT);
                        ImGui::SameLine();

                        ImGui::TextColored(ImVec4{ 0.5f, 1.0f, 0.5f, 1.0f }, player->Name.Data());

                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%d", player->Kills);

                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("%d", player->Deaths);

                    });

                Network.GetPlayerList().DoForEachPlayer([](ClientPlayerState* player)
                    {
                        if (player->IsLocalPlayer)
                            return;
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextUnformatted(ICON_FA_USER);
                        ImGui::SameLine();
                        ImGui::TextUnformatted(player->Name.Data());

                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%d", player->Kills);

                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("%d", player->Deaths);
                    });

                ImGui::EndTable();
            }
        }
        ImGui::End();
    }
}