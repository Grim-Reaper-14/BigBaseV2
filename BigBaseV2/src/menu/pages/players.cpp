#include "players.hpp"

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
		ImGui::Text("Players");
		ImGui::Separator();

		ImGui::InputText("Search", g_players_page.search.data(), g_players_page.search.size());
		ImGui::Checkbox("Auto Refresh", &g_players_page.auto_refresh);
		ImGui::SameLine();
		ImGui::Checkbox("Friends Only", &g_players_page.friends_only);

		if (ImGui::Button("Refresh Player List"))
		{
			// Session enumeration is handled by the players feature layer.
		}

		ImGui::BeginChild("PlayerList", ImVec2(0.0f, 250.0f), true);
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
}
