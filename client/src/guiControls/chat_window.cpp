#include "chat_window.h"

#include "client_network_manager.h"
#include "constants.h"

#include <string>
#include <deque>
#include <unordered_map>

#include "raylib.h"
#include "imgui.h"

namespace ChatWindow
{
    std::deque<std::string> LogLines;

    EventSource<std::string> OnSendChatMessage;

    struct ChatMessage
    {
        uint64_t PlayerId;
        std::string Message;
    };

    std::deque<ChatMessage> ChatLines;
    std::unordered_map<uint64_t, std::string> ChatUsers;

    constexpr size_t MaxLogHistory = 10;
    constexpr size_t MaxChatHistory = 20;

    char PendingChatLine[kMaxChatLineSize] = { 0 };

    ImVec2 ChatBoxSize(600, 200);

    void AddLogLine(std::string_view message)
    {
        LogLines.push_back(std::string(message));

        while (LogLines.size() > MaxLogHistory)
            LogLines.pop_front();
    }

    void AddChatLine(PlayerState* from, std::string_view data)
    {
        uint64_t id = ServerChatId;
        if (from != nullptr)
        {
            if (!ChatUsers.contains(from->PlayerID))
                ChatUsers.emplace(from->PlayerID, std::string(from->Name));

            id = from->PlayerID;
        }

        ChatLines.push_back(ChatMessage{ id, std::string(data)});

        while (ChatLines.size() > MaxChatHistory)
            ChatLines.pop_front();
    }

    void AddSystemChatLine(std::string_view data)
    {
        ChatLines.push_back(ChatMessage{ SystemChatId, std::string(data) });

        while (ChatLines.size() > MaxChatHistory)
            ChatLines.pop_front();
    }

    void Show()
    {
        ImVec2 pos(0, float(GetScreenHeight()) - ChatBoxSize.y);
        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(ChatBoxSize);

        if (ImGui::Begin("Messages", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
        {
            if (ImGui::BeginTabBar("ChatTabs"))
            {
                if (ImGui::BeginTabItem("Chat"))
                {
                    ImVec2 chatSize = ImGui::GetContentRegionAvail();
                    chatSize.y -= ImGui::GetTextLineHeightWithSpacing() + (ImGui::GetStyle().ItemSpacing.y * 2);

                    if (ImGui::BeginChild("ChatScroller", chatSize, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
                    {
                        if (ImGui::BeginTable("ChatTable", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV))
                        {
                            for (auto& line : ChatLines)
                            {
                                ImGui::TableNextRow();
                                ImGui::TableNextColumn();
                                std::string name = "Server";
                                if (line.PlayerId == SystemChatId)
                                {
                                    name = "System";
                                }
                                else if (line.PlayerId != ServerChatId)
                                {
                                    name = ChatUsers[line.PlayerId];
                                }

                                ImGui::TextUnformatted(name.c_str());

                                ImGui::TableNextColumn();
                                ImGui::TextUnformatted(line.Message.c_str());
                            }
                            ImGui::EndTable();

                            ImGui::SetScrollHereY(1.0f);
                        }
                    }
                    ImGui::EndChild();

                    ImGui::Text("%s:", Network.GetPlayerName());
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                    if (ImGui::InputText("###ChatInput", PendingChatLine, kMaxChatLineSize, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll))
                    {
                        OnSendChatMessage.Invoke(std::string(PendingChatLine), nullptr);

                        PendingChatLine[0] = '\0';
                    }

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Logs"))
                {
                    if (ImGui::BeginChild("Log", ImVec2(0,0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
                    {
                        for (auto& logItem : LogLines)
                        {
                            ImGui::TextUnformatted(logItem.c_str());
                        }
                    }
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }
}