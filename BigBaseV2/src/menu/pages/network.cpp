#include "network.hpp"
#include "../widgets.hpp"
#include "../../menu.hpp"
#include "../../pointers.hpp"

#include <imgui.h>

namespace big::menu_pages
{
	void draw_network()
	{
		menu_ui::page_title(
			"Network",
			"Inspect the current GTA V Enhanced session, network runtime, and player-roster availability.");

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
