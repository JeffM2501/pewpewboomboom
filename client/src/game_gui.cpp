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
        ImVec2 windowSize(140, ImGui::GetTextLineHeightWithSpacing()*2 + ImGui::GetFrameHeightWithSpacing());

        ImVec2 pos(float(GetScreenWidth())- windowSize.x, 0);
        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(windowSize);
        if (ImGui::Begin("Network State", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
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

    inline void SetDarkPastelImGuiStyle()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        // Backgrounds 
        colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.13f, 0.15f, 1.00f); // Dark grey base
        colors[ImGuiCol_ChildBg] = ImVec4(0.14f, 0.15f, 0.17f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.10f, 0.12f, 0.95f);
        colors[ImGuiCol_Border] = ImVec4(0.30f, 0.33f, 0.42f, 0.40f);

        //Text 
        colors[ImGuiCol_Text] = ImVec4(0.90f, 0.93f, 0.95f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.65f, 0.70f, 1.00f);

        // Headers 
        colors[ImGuiCol_Header] = ImVec4(0.36f, 0.42f, 0.55f, 0.60f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.44f, 0.50f, 0.68f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.46f, 0.55f, 0.75f, 1.00f);

        // Buttons 
        colors[ImGuiCol_Button] = ImVec4(0.28f, 0.34f, 0.48f, 0.70f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.36f, 0.45f, 0.65f, 0.85f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.50f, 0.70f, 1.00f);

        // Frames 
        colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.22f, 0.28f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.28f, 0.32f, 0.42f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.32f, 0.38f, 0.50f, 1.00f);

        // Tabs 
        colors[ImGuiCol_Tab] = ImVec4(0.26f, 0.30f, 0.42f, 0.80f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.36f, 0.42f, 0.58f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.42f, 0.50f, 0.68f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.20f, 0.24f, 0.32f, 0.80f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.30f, 0.36f, 0.50f, 1.00f);

        // Titles 
        colors[ImGuiCol_TitleBg] = ImVec4(0.20f, 0.25f, 0.30f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.25f, 0.30f, 0.40f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.12f, 0.15f, 0.75f);

        // Scrollbars 
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.13f, 0.14f, 0.18f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.30f, 0.38f, 0.60f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.40f, 0.50f, 0.80f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.50f, 0.65f, 1.00f);

        // Checkboxes / Radios 
        colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.85f, 1.00f, 1.00f);

        // Sliders 
        colors[ImGuiCol_SliderGrab] = ImVec4(0.50f, 0.65f, 0.90f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.60f, 0.75f, 1.00f, 1.00f);

        // Resize Grip 
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.30f, 0.40f, 0.50f, 0.60f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.40f, 0.50f, 0.60f, 0.80f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.50f, 0.60f, 0.80f, 1.00f);

        // Separator 
        colors[ImGuiCol_Separator] = ImVec4(0.35f, 0.40f, 0.48f, 0.7f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.50f, 0.60f, 0.72f, 0.9f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.65f, 0.70f, 0.85f, 1.0f);

        // Menus and Tooltips 
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.15f, 0.17f, 1.00f);
        // colors[ImGuiCol_TooltipBg]          = ImVec4(0.18f, 0.20f, 0.25f, 0.95f);

        // Drag & Drop 
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.50f, 0.85f, 1.00f, 0.90f);

        // Style Metrics 
        style.WindowRounding = 8.0f;
        style.ChildRounding = 6.0f;
        style.FrameRounding = 5.0f;
        style.PopupRounding = 6.0f;
        style.ScrollbarRounding = 5.0f;
        style.GrabRounding = 4.0f;
        style.TabRounding = 5.0f;

        style.WindowBorderSize = 0.0f;
        style.FrameBorderSize = 0.0f;
        style.PopupBorderSize = 1.0f;

        style.WindowPadding = ImVec2(16, 16);
        style.FramePadding = ImVec2(10, 6);
        style.ItemSpacing = ImVec2(10, 10);
        style.ItemInnerSpacing = ImVec2(6, 4);
        style.IndentSpacing = 20.0f;
    }

    void InstallStyle()
    {
        SetDarkPastelImGuiStyle();
    }

    void Show()
    {
        ChatWindow::Show();
        PlayerListWindow::Show();
        ShowConnectionWindow();
    }
}