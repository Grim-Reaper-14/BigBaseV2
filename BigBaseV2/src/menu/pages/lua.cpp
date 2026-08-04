#include "lua.hpp"
#include "../widgets.hpp"

#include "../../lua/lua_manager.hpp"

#include <imgui.h>

namespace big::menu_pages
{
	void lua_page_state::refresh()
	{
		m_scripts.clear();

		if (m_scripts_directory.empty())
			m_scripts_directory = std::filesystem::current_path() / "scripts";

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

		status = error ? "Failed to scan scripts directory: " + error.message() : "Script list refreshed.";
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

		menu_ui::page_title("Lua", "Load and manage sandboxed Sol2 scripts from the local scripts directory.");

		if (menu_ui::begin_section("LuaRuntime", "Runtime", 145.0f))
		{
			ImGui::TextWrapped("Scripts directory: %s", g_lua_page.scripts_directory().string().c_str());
			const auto loaded_count = g_lua_manager ? g_lua_manager->loaded_count() : 0;
			ImGui::TextDisabled("%d scripts found | %d loaded", static_cast<int>(g_lua_page.scripts().size()), static_cast<int>(loaded_count));
			ImGui::Checkbox("Sandbox Scripts", &g_lua_page.sandbox_scripts);
			ImGui::SameLine();
			ImGui::Checkbox("Auto Reload", &g_lua_page.auto_reload);
			if (ImGui::Button("Refresh Scripts", ImVec2(165.0f, 0.0f)))
				g_lua_page.refresh();
		}
		menu_ui::end_section();

		ImGui::Spacing();
		if (menu_ui::begin_section("LuaScriptsCard", "Scripts", 285.0f))
		{
			ImGui::BeginChild("LuaScripts", ImVec2(0.0f, 205.0f), true);
			for (int index = 0; index < static_cast<int>(g_lua_page.scripts().size()); ++index)
			{
				const auto& script_entry = g_lua_page.scripts()[index];
				const bool selected = g_lua_page.selected_index == index;
				const bool loaded = g_lua_manager && g_lua_manager->is_loaded(script_entry.path);
				const std::string label = script_entry.path.filename().string() + (loaded ? " [loaded]" : "");
				if (ImGui::Selectable(label.c_str(), selected))
					g_lua_page.selected_index = index;
			}

			if (g_lua_page.scripts().empty())
				ImGui::TextDisabled("No .lua scripts found.");
			ImGui::EndChild();

			const bool has_selection = g_lua_page.selected_index >= 0 &&
				g_lua_page.selected_index < static_cast<int>(g_lua_page.scripts().size());

			if (has_selection && g_lua_manager)
			{
				const auto& selected_script = g_lua_page.scripts()[g_lua_page.selected_index].path;
				if (ImGui::Button("Load Selected", ImVec2(145.0f, 0.0f)))
				{
					std::string error;
					g_lua_page.status = g_lua_manager->load_script(selected_script, g_lua_page.sandbox_scripts, error)
						? "Loaded " + selected_script.filename().string()
						: "Load failed: " + error;
				}
				ImGui::SameLine();
				if (ImGui::Button("Unload Selected", ImVec2(145.0f, 0.0f)))
					g_lua_page.status = g_lua_manager->unload_script(selected_script)
						? "Unloaded " + selected_script.filename().string()
						: "The selected script is not loaded.";
				ImGui::SameLine();
				if (ImGui::Button("Unload All", ImVec2(120.0f, 0.0f)))
				{
					g_lua_manager->unload_all();
					g_lua_page.status = "All Lua scripts unloaded.";
				}
			}
			else if (!g_lua_manager)
				ImGui::TextDisabled("Sol2 runtime is not initialized.");
			else
				ImGui::TextDisabled("Select a script to enable load controls.");
		}
		menu_ui::end_section();

		ImGui::Spacing();
		if (menu_ui::begin_section("LuaStatus", "Status", 85.0f))
			ImGui::TextWrapped("%s", g_lua_page.status.c_str());
		menu_ui::end_section();
	}
}
