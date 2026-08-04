#include "lua.hpp"
#include "../widgets.hpp"

#include "../../lua/Lua_Scripts_Manager.hpp"
#include "../../lua/lua_manager.hpp"

#include <imgui.h>

namespace big::menu_pages
{
	void lua_page_state::initialize()
	{
		if (m_initialized)
			return;

		lua::Lua_Scripts_Manager::Instance().Initialize();
		m_next_auto_reload_check = std::chrono::steady_clock::now();
		m_initialized = true;
	}

	void lua_page_state::refresh()
	{
		initialize();
		lua::Lua_Scripts_Manager::Instance().Refresh();

		const auto size = static_cast<int>(lua::Lua_Scripts_Manager::Instance().Scripts().size());
		if (selected_index >= size)
			selected_index = -1;

		status = "Script list refreshed.";
	}

	void lua_page_state::tick_auto_reload()
	{
		if (!auto_reload || !g_lua_manager)
			return;

		const auto now = std::chrono::steady_clock::now();
		if (now < m_next_auto_reload_check)
			return;

		m_next_auto_reload_check = now + std::chrono::seconds(1);
		const auto count = g_lua_manager->reload_changed_scripts();
		if (count > 0)
		{
			status = "Auto reloaded " + std::to_string(count) + (count == 1 ? " script." : " scripts.");
			lua::Lua_Scripts_Manager::Instance().Refresh();
		}
	}

	void draw_lua()
	{
		g_lua_page.initialize();
		g_lua_page.tick_auto_reload();

		auto& scripts_manager = lua::Lua_Scripts_Manager::Instance();
		const auto& scripts = scripts_manager.Scripts();

		menu_ui::page_title("Lua", "Load and manage isolated Sol2 scripts from the local scripts directory.");

		if (menu_ui::begin_section("LuaRuntime", "Runtime", 155.0f))
		{
			ImGui::TextWrapped("Scripts directory: %s", scripts_manager.Directory().string().c_str());
			const auto loaded_count = g_lua_manager ? g_lua_manager->loaded_count() : 0;
			ImGui::TextDisabled("%d scripts found | %d loaded", static_cast<int>(scripts.size()), static_cast<int>(loaded_count));
			ImGui::Checkbox("Sandbox Scripts", &g_lua_page.sandbox_scripts);
			ImGui::SameLine();
			ImGui::Checkbox("Auto Reload", &g_lua_page.auto_reload);
			if (ImGui::Button("Refresh Scripts", ImVec2(165.0f, 0.0f)))
				g_lua_page.refresh();
		}
		menu_ui::end_section();

		ImGui::Spacing();
		if (menu_ui::begin_section("LuaScriptsCard", "Scripts", 310.0f))
		{
			ImGui::BeginChild("LuaScripts", ImVec2(0.0f, 205.0f), true);
			for (int index = 0; index < static_cast<int>(scripts.size()); ++index)
			{
				const auto& entry = scripts[index];
				const bool selected = g_lua_page.selected_index == index;
				const std::string label = entry.path.filename().string() + (entry.loaded ? " [loaded]" : "");
				if (ImGui::Selectable(label.c_str(), selected))
					g_lua_page.selected_index = index;
			}

			if (scripts.empty())
				ImGui::TextDisabled("No .lua scripts found.");
			ImGui::EndChild();

			const bool has_selection = g_lua_page.selected_index >= 0 &&
				g_lua_page.selected_index < static_cast<int>(scripts.size());

			if (has_selection && g_lua_manager)
			{
				const auto index = static_cast<std::size_t>(g_lua_page.selected_index);
				if (ImGui::Button("Load Selected", ImVec2(135.0f, 0.0f)))
					scripts_manager.Load(index, g_lua_page.sandbox_scripts, g_lua_page.status);

				ImGui::SameLine();
				if (ImGui::Button("Reload Selected", ImVec2(145.0f, 0.0f)))
					scripts_manager.Reload(index, g_lua_page.sandbox_scripts, g_lua_page.status);

				ImGui::SameLine();
				if (ImGui::Button("Unload Selected", ImVec2(145.0f, 0.0f)))
					scripts_manager.Unload(index, g_lua_page.status);

				if (ImGui::Button("Unload All", ImVec2(135.0f, 0.0f)))
				{
					scripts_manager.UnloadAll();
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
		if (menu_ui::begin_section("LuaStatus", "Status and diagnostics", 145.0f))
		{
			ImGui::TextWrapped("%s", g_lua_page.status.c_str());

			if (g_lua_manager)
			{
				for (const auto& script_status : g_lua_manager->statuses())
				{
					if (!script_status.faulted)
						continue;

					ImGui::Separator();
					ImGui::Text("Faulted: %s", script_status.path.filename().string().c_str());
					ImGui::TextWrapped("%s", script_status.last_error.c_str());
				}
			}
		}
		menu_ui::end_section();
	}
}
