#include "network.hpp"
#include "../widgets.hpp"
#include "../../menu.hpp"
#include "../../network/join.hpp"
#include "../../pointers.hpp"

#include <imgui.h>

namespace big::menu_pages
{
	void draw_network()
	{
		menu_ui::page_title(
			"Network",
			"Inspect the current GTA V Enhanced session, switch session types, and view network runtime health.");

		const bool pointers_ready = g_pointers != nullptr;
		const bool session_started = pointers_ready &&
			g_pointers->m_is_session_started &&
			*g_pointers->m_is_session_started;
		const bool player_manager_ready = pointers_ready && g_pointers->m_network_player_mgr;
		const bool session_pointer_ready = pointers_ready && g_pointers->m_network_session;
		const bool object_manager_ready = pointers_ready && g_pointers->m_network_object_mgr;

		if (menu_ui::begin_section("NetworkOverview", "Session Overview", 155.0f))
		{
			menu_ui::status_badge(session_started ? "Online session active" : "No online session detected", session_started);
			ImGui::Text("Player manager: %s", player_manager_ready ? "Ready" : "Unavailable");
			ImGui::Text("Network session: %s", session_pointer_ready ? "Ready" : "Unavailable");
			ImGui::Text("Network object manager: %s", object_manager_ready ? "Ready" : "Unavailable");

			if (ImGui::Button("Open Player Roster", ImVec2(180.0f, 0.0f)))
				g_menu.m_current_page = menu::page::players;
		}
		menu_ui::end_section();

		ImGui::Spacing();
		if (menu_ui::begin_section("OnlineSessions", "Online Sessions", 285.0f))
		{
			const bool service_ready = network::join_service_ready();
			menu_ui::status_badge(
				service_ready ? "Session transition service ready" : "Session transition service unavailable",
				service_ready);
			ImGui::TextDisabled("Join a new session type:");
			ImGui::Spacing();

			ImGui::BeginDisabled(!service_ready);

			if (ImGui::Button("Public", ImVec2(110.0f, 0.0f)))
				network::queue_join_type(network::join_type::join_public);
			ImGui::SameLine();
			if (ImGui::Button("New Public", ImVec2(110.0f, 0.0f)))
				network::queue_join_type(network::join_type::new_public);
			ImGui::SameLine();
			if (ImGui::Button("Solo", ImVec2(110.0f, 0.0f)))
				network::queue_join_type(network::join_type::solo);

			if (ImGui::Button("Invite Only", ImVec2(110.0f, 0.0f)))
				network::queue_join_type(network::join_type::invite_only);
			ImGui::SameLine();
			if (ImGui::Button("Closed Friends", ImVec2(110.0f, 0.0f)))
				network::queue_join_type(network::join_type::closed_friends);
			ImGui::SameLine();
			if (ImGui::Button("Find Friend", ImVec2(110.0f, 0.0f)))
				network::queue_join_type(network::join_type::find_friend);

			if (ImGui::Button("Crew", ImVec2(110.0f, 0.0f)))
				network::queue_join_type(network::join_type::crew);
			ImGui::SameLine();
			if (ImGui::Button("Closed Crew", ImVec2(110.0f, 0.0f)))
				network::queue_join_type(network::join_type::closed_crew);
			ImGui::SameLine();
			if (ImGui::Button("Join Crew", ImVec2(110.0f, 0.0f)))
				network::queue_join_type(network::join_type::join_crew);

			if (ImGui::Button("SC TV", ImVec2(110.0f, 0.0f)))
				network::queue_join_type(network::join_type::sc_tv);
			ImGui::SameLine();

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.15f, 0.15f, 1.0f));
			if (ImGui::Button("Leave Online", ImVec2(110.0f, 0.0f)))
				network::queue_join_type(network::join_type::leave_online);
			ImGui::PopStyleColor();

			ImGui::EndDisabled();

			ImGui::Spacing();
			ImGui::Separator();
			const auto status = network::current_join_status();
			menu_ui::status_badge(status.message.c_str(), status.success);
			if (status.pending)
				ImGui::TextDisabled("The request will run on the next script-fiber tick.");

			ImGui::TextDisabled(
				"Direct Rockstar ID or username joining is not enabled without a verified session-by-handle backend.");
		}
		menu_ui::end_section();

		ImGui::Spacing();
		if (menu_ui::begin_section("NetworkRuntime", "Runtime Details", 175.0f))
		{
			if (!pointers_ready)
			{
				ImGui::TextDisabled("Pointer runtime is unavailable.");
			}
			else
			{
				if (g_pointers->m_game_version)
					ImGui::Text("Game version: %s", g_pointers->m_game_version);
				else
					ImGui::TextDisabled("Game version: unavailable");

				if (g_pointers->m_online_version)
					ImGui::Text("Online version: %s", g_pointers->m_online_version);
				else
					ImGui::TextDisabled("Online version: unavailable");

				if (g_pointers->m_region_code)
					ImGui::Text("Region code: %d", *g_pointers->m_region_code);
				else
					ImGui::TextDisabled("Region code: unavailable");

				if (g_pointers->m_network_time)
					ImGui::Text("Network time: %u", *g_pointers->m_network_time);
				else
					ImGui::TextDisabled("Network time: unavailable");

				if (g_pointers->m_game_timer)
					ImGui::Text("Game timer: %u", *g_pointers->m_game_timer);
				else
					ImGui::TextDisabled("Game timer: unavailable");
			}
		}
		menu_ui::end_section();

		ImGui::Spacing();
		if (menu_ui::begin_section("NetworkDiagnostics", "Enhanced Pointer Health", 145.0f))
		{
			if (!pointers_ready)
			{
				menu_ui::status_badge("Pointer system unavailable", false);
			}
			else
			{
				menu_ui::status_badge("Core runtime", g_pointers->core_ready());
				menu_ui::status_badge("Network runtime", g_pointers->network_ready());
				menu_ui::status_badge("Script runtime", g_pointers->scripts_ready());
				menu_ui::status_badge("DX12 renderer", g_pointers->renderer_ready());

				const auto& report = g_pointers->report();
				ImGui::TextDisabled(
					"Pointers: %zu/%zu required, %zu/%zu optional",
					report.required_found,
					report.required_total,
					report.optional_found,
					report.optional_total);
			}
		}
		menu_ui::end_section();
	}
}
