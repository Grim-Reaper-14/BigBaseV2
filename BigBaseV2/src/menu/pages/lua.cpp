#include "lua.hpp"

#include <imgui.h>

namespace big::menu_pages
{
	void lua_page_state::refresh()
	{
		m_scripts.clear();

		if (m_scripts_directory.empty())
		{
			m_scripts_directory = std::filesystem::current_path() / "scripts";
		}

		std::error_code error;
		std::filesystem::create_directories(m_scripts_directory, error);
		if (error)
		{
			status = "Failed to create scripts directory: " + error.message();
			selected_index = -1;
			return;
		}

		for (const auto& entry : std::filesystem::directory_iterator(m_scripts_directory, error))
		{
			if (error)
				break;

			if (!entry.is_regular_file())
				continue;

			if (entry.path().extension() == ".lua")
				m_scripts.push_back({entry.path()});
		}

		std::sort(m_scripts.begin(), m_scripts.end(), [](const auto& left, const auto& right)
		{
			return left.path.filename().string() < right.path.filename().string();
		});

		if (selected_index >= static_cast<int>(m_scripts.size()))
			selected_index = -1;

		status = "Lua runtime is not installed yet.";
	}

	const std::filesystem::path& lua_page_state::scripts_directory() const noexcept
	{
		return m_scripts_directory;
	}

	const std::vector<lua_script_entry>& lua_page_state::scripts() const noexcept
	{
		return m_scripts;
	}

	void draw_lua()
	{
		static bool initialized = false;
		if (!initialized)
		{
			g_lua_page.refresh();
			initialized = true;
		}

		ImGui::Text("Lua Manager");
		ImGui::Separator();
		ImGui::TextWrapped("Scripts directory: %s", g_lua_page.scripts_directory().string().c_str());

		if (ImGui::Button("Refresh Scripts"))
			g_lua_page.refresh();

		ImGui::SameLine();
		ImGui::TextDisabled("%d script(s)", static_cast<int>(g_lua_page.scripts().size()));

		ImGui::Checkbox("Sandbox Scripts", &g_lua_page.sandbox_scripts);
		ImGui::Checkbox("Auto Reload", &g_lua_page.auto_reload);

		ImGui::BeginChild("LuaScripts", ImVec2(0.0f, 220.0f), true);
		for (int index = 0; index < static_cast<int>(g_lua_page.scripts().size()); ++index)
		{
			const auto& script = g_lua_page.scripts()[index];
			const bool selected = g_lua_page.selected_index == index;
			if (ImGui::Selectable(script.path.filename().string().c_str(), selected))
				g_lua_page.selected_index = index;
		}

		if (g_lua_page.scripts().empty())
			ImGui::TextDisabled("No .lua scripts found.");
		ImGui::EndChild();

		const bool has_selection = g_lua_page.selected_index >= 0 &&
			g_lua_page.selected_index < static_cast<int>(g_lua_page.scripts().size());

		if (has_selection)
		{
			if (ImGui::Button("Load Selected"))
				g_lua_page.status = "Lua runtime is not installed; script was not loaded.";
			ImGui::SameLine();
			if (ImGui::Button("Unload Selected"))
				g_lua_page.status = "Lua runtime is not installed; no script is loaded.";
		}
		else
		{
			ImGui::TextDisabled("Select a script to enable load controls.");
		}

		ImGui::Separator();
		ImGui::TextWrapped("Status: %s", g_lua_page.status.c_str());
	}
}
