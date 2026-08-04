#pragma once

#include <chrono>
#include <string>

namespace big::menu_pages
{
	class lua_page_state final
	{
	public:
		void initialize();
		void refresh();
		void tick_auto_reload();

		int selected_index{-1};
		bool sandbox_scripts{true};
		bool auto_reload{false};
		std::string status{"Sol2 runtime ready."};

	private:
		bool m_initialized{};
		std::chrono::steady_clock::time_point m_next_auto_reload_check{};
	};

	inline lua_page_state g_lua_page;

	void draw_lua();
}
