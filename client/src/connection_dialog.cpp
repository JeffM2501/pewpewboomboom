#include "connection_dialog.h"
#include "constants.h"
#include "net_connection.h"
#include "imgui.h"

namespace NetConnectionDialog
{
	static char HostBuffer[128] = "localhost";

	void ShowDialog()
	{
		if (NetConnection::GetState() == ConnectionState::Connected)
			return;

		ImVec2 windowSize(250, 140);
		ImVec2 windowPos((ImGui::GetIO().DisplaySize.x - windowSize.x) * 0.5f, (ImGui::GetIO().DisplaySize.y - windowSize.y) * 0.5f);

		ImGui::SetNextWindowSize(windowSize, ImGuiCond_Appearing);
		ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Connection", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
		{
			if (ImGui::BeginTable("Connect", 2, ImGuiTableFlags_SizingStretchProp))
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted("Name");
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				ImGui::InputText("##Name", NetConnection::GetPlayerName(), kMaxNameSize);

                ImGui::TableNextRow();
				ImGui::TableNextColumn();
                ImGui::TextUnformatted("Host");
                ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputText("##Host", HostBuffer, sizeof(HostBuffer));

				ImGui::TableNextRow();
				ImGui::TableNextColumn();

				ImGui::BeginDisabled(NetConnection::GetState() == ConnectionState::Connecting);
				if (ImGui::Button("Connect"))
				{
					NetConnection::BeginConnect(HostBuffer, 7777);
				}
				ImGui::EndDisabled();

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::EndTable();
			}
			if (NetConnection::GetState() == ConnectionState::Connecting)
			{
				ImGui::TextUnformatted("Connecting...");
			}
			else if (NetConnection::GetState() == ConnectionState::Disconnected)
			{
				ImGui::TextUnformatted("Disconnected");
				if (NetConnection::HadTimeout())
					ImGui::TextColored(ImVec4(1,0,0,1), "ERROR: Timeout");
			}
		}
		ImGui::End();
	}
}