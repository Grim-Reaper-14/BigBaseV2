#include "lua.hpp"

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

		if (error)
			status = "Failed to scan scripts directory: " + error.message();
		else
			status = "Script list refreshed.";
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

		ImGui::Text("Lua Manager (Sol2)");
		ImGui::Separator();
		ImGui::TextWrapped("Scripts directory: %s", g_lua_page.scripts_directory().string().c_str());

		if (ImGui::Button("Refresh Scripts"))
			g_lua_page.refresh();

		ImGui::SameLine();
		ImGui::TextDisabled("%d found", static_cast<int>(g_lua_page.scripts().size()));

		const auto loaded_count = g_lua_manager ? g_lua_manager->loaded_count() : 0;
		ImGui::SameLine();
		ImGui::TextDisabled("%d loaded", static_cast<int>(loaded_count));

		ImGui::Checkbox("Sandbox Scripts", &g_lua_page.sandbox_scripts);
		ImGui::Checkbox("Auto Reload", &g_lua_page.auto_reload);
		if (g_lua_page.auto_reload)
			ImGui::TextDisabled("Auto Reload tracking will be added after the base runtime is validated.");

		ImGui::BeginChild("LuaScripts", ImVec2(0.0f, 220.0f), true);
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

			if (ImGui::Button("Load Selected"))
			{
				std::string error;
				if (g_lua_manager->load_script(selected_script, g_lua_page.sandbox_scripts, error))
					g_lua_page.status = "Loaded " + selected_script.filename().string();
				else
					g_lua_page.status = "Load failed: " + error;
			}

			ImGui::SameLine();
			if (ImGui::Button("Unload Selected"))
			{
				if (g_lua_manager->unload_script(selected_script))
					g_lua_page.status = "Unloaded " + selected_script.filename().string();
				else
					g_lua_page.status = "The selected script is not loaded.";
			}

			ImGui::SameLine();
			if (ImGui::Button("Unload All"))
			{
				g_lua_manager->unload_all();
				g_lua_page.status = "All Lua scripts unloaded.";
			}
		}
		else if (!g_lua_manager)
		{
			ImGui::TextDisabled("Sol2 runtime is not initialized.");
		}
		else
		{
			ImGui::TextDisabled("Select a script to enable load controls.");
		}

		ImGui::Separator();
		ImGui::TextWrapped("Status: %s", g_lua_page.status.c_str());
	}
}
