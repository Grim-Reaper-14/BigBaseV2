#include "Lua_User_Interface.hpp"

#include "Lua_Scripts_Manager.hpp"

#include <imgui.h>

namespace big::lua
{
	Lua_User_Interface& Lua_User_Interface::Instance()
	{
		static Lua_User_Interface instance;
		return instance;
	}

	void Lua_User_Interface::Draw()
	{
		auto& manager = Lua_Scripts_Manager::Instance();
		if (manager.Directory().empty())
			manager.Initialize();

		ImGui::Text("Lua Manager (Sol2)");
		ImGui::Separator();
		ImGui::TextWrapped("Scripts directory: %s", manager.Directory().string().c_str());

		if (ImGui::Button("Refresh"))
			manager.Refresh();
		ImGui::SameLine();
		ImGui::Checkbox("Sandbox", &m_sandbox);
		ImGui::SameLine();
		ImGui::Checkbox("Auto Reload", &m_auto_reload);

		const auto& scripts = manager.Scripts();
		ImGui::BeginChild("LuaScriptList", ImVec2(0.0f, 220.0f), true);
		for (int index = 0; index < static_cast<int>(scripts.size()); ++index)
		{
			std::string label = scripts[index].loaded ? "[Loaded] " : "[Stopped] ";
			label += scripts[index].path.filename().string();
			if (ImGui::Selectable(label.c_str(), m_selected_index == index))
				m_selected_index = index;
		}
		if (scripts.empty())
			ImGui::TextDisabled("No .lua scripts found.");
		ImGui::EndChild();

		const bool valid = m_selected_index >= 0 && m_selected_index < static_cast<int>(scripts.size());
		if (valid)
		{
			if (ImGui::Button("Load"))
				manager.Load(static_cast<std::size_t>(m_selected_index), m_sandbox, m_status);
			ImGui::SameLine();
			if (ImGui::Button("Reload"))
				manager.Reload(static_cast<std::size_t>(m_selected_index), m_sandbox, m_status);
			ImGui::SameLine();
			if (ImGui::Button("Unload"))
				manager.Unload(static_cast<std::size_t>(m_selected_index), m_status);
		}
		else
		{
			ImGui::TextDisabled("Select a script to manage it.");
		}

		if (ImGui::Button("Unload All"))
		{
			manager.UnloadAll();
			m_status = "All Lua scripts unloaded.";
		}

		ImGui::Separator();
		ImGui::TextWrapped("Status: %s", m_status.c_str());
	}
}
