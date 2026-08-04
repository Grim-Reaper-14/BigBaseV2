#include "players.hpp"
#include "../widgets.hpp"

#include <algorithm>
#include <cctype>
#include <imgui.h>

namespace big::menu_pages
{
	namespace
	{
		bool contains_case_insensitive(const std::string& value, const char* query)
		{
			if (!query || query[0] == '\0')
				return true;

			std::string haystack = value;
			std::string needle = query;
			std::transform(haystack.begin(), haystack.end(), haystack.begin(), [](unsigned char character)
			{
				return static_cast<char>(std::tolower(character));
			});
			std::transform(needle.begin(), needle.end(), needle.begin(), [](unsigned char character)
			{
				return static_cast<char>(std::tolower(character));
			});
			return haystack.find(needle) != std::string::npos;
		}
	}

	void draw_players()
	{
		menu_ui::page_title("Players", "Browse the current session and inspect locally available player information.");

		if (menu_ui::begin_section("PlayerFilters", "Browser", 135.0f))
		{
			ImGui::SetNextItemWidth(-1.0f);
			ImGui::InputTextWithHint("##PlayerSearch", "Search players...", g_players_page.search.data(), g_players_page.search.size());
			ImGui::Checkbox("Auto Refresh", &g_players_page.auto_refresh);
			ImGui::SameLine();
			ImGui::Checkbox("Friends Only", &g_players_page.friends_only);
			if (ImGui::Button("Refresh Player List", ImVec2(175.0f, 0.0f)))
			{
				// Session enumeration is handled by the players feature layer.
			}
		}
		menu_ui::end_section();

		ImGui::Spacing();
		if (menu_ui::begin_section("PlayerListCard", "Session Players", 285.0f))
		{
			ImGui::BeginChild("PlayerList", ImVec2(0.0f, 210.0f), true);
			for (const auto& player : g_players_page.players)
			{
				if (g_players_page.friends_only && !player.is_friend)
					continue;
				if (!contains_case_insensitive(player.name, g_players_page.search.data()))
					continue;

				const std::string label = player.name + "##player_" + std::to_string(player.id);
				if (ImGui::Selectable(label.c_str(), g_players_page.selected_player == player.id))
					g_players_page.selected_player = player.id;
			}

			if (g_players_page.players.empty())
				ImGui::TextDisabled("No session players loaded.");
			ImGui::EndChild();

			ImGui::TextDisabled("Selected Player ID: %d", g_players_page.selected_player);
		}
		menu_ui::end_section();
	}
}
