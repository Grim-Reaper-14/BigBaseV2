#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace big::menu_pages
{
	struct lua_script_entry final
	{
		std::filesystem::path path;
	};

	class lua_page_state final
	{
	public:
		void refresh();

		[[nodiscard]] const std::filesystem::path& scripts_directory() const noexcept;
		[[nodiscard]] const std::vector<lua_script_entry>& scripts() const noexcept;

		int selected_index{-1};
		bool sandbox_scripts{true};
		bool auto_reload{false};
		std::string status{"Lua runtime is not installed yet."};

	private:
		std::filesystem::path m_scripts_directory;
		std::vector<lua_script_entry> m_scripts;
	};

	inline lua_page_state g_lua_page;

	void draw_lua();
}
