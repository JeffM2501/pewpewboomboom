#include "connection_dialog.h"
#include "constants.h"
#include "client_network_manager.h"
#include "imgui.h"

namespace NetConnectionDialog
{
	static char HostBuffer[128] = "localhost";

	void ShowDialog()
	{
		if (Network.GetState() == ConnectionState::Connected)
		{
			return;
		}

		ImVec2 windowSize(300, ImGui::GetTextLineHeightWithSpacing() * 8);
		ImVec2 windowPos((ImGui::GetIO().DisplaySize.x - windowSize.x) * 0.5f, (ImGui::GetIO().DisplaySize.y - windowSize.y) * 0.5f);

		ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
		ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
		ImGui::SetNextWindowFocus();

		if (ImGui::Begin("Connection", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
		{
			if (ImGui::BeginTable("Connect", 2, ImGuiTableFlags_SizingStretchProp))
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted("Name");
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				ImGui::InputText("##Name", Network.GetPlayerName().Buffer(), kMaxNameSize);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted("Host");
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				ImGui::InputText("##Host", HostBuffer, sizeof(HostBuffer));

				ImGui::TableNextRow();
				ImGui::TableNextColumn();

				ImGui::BeginDisabled(Network.GetState() == ConnectionState::Connecting);
				if (ImGui::Button("Connect") || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER))
				{
					Network.BeginConnect(HostBuffer, 7777);
				}
				ImGui::EndDisabled();

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::EndTable();
			}
			if (Network.GetState() == ConnectionState::Connecting)
			{
				ImGui::TextUnformatted("Connecting...");
			}
			else if (Network.GetState() == ConnectionState::Disconnected)
			{
				ImGui::TextUnformatted("Disconnected");
				if (Network.HadTimeout())
				{
					ImGui::TextColored(ImVec4(1, 0, 0, 1), "ERROR: Timeout");
				}
			}
		}
		ImGui::End();
	}
}