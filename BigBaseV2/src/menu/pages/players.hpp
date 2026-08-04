#pragma once

#include <array>
#include <string>
#include <vector>

namespace big::menu_pages
{
	struct player_entry final
	{
		int id{};
		std::string name;
		bool is_friend{};
	};

	struct players_page_state final
	{
		std::array<char, 64> search{};
		bool auto_refresh{true};
		bool friends_only{};
		int selected_player{-1};
		std::vector<player_entry> players;
	};

	inline players_page_state g_players_page;

	void draw_players();
}
