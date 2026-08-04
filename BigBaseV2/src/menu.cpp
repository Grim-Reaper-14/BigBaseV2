#include "menu.hpp"

#include "menu/pages/lua.hpp"
#include "menu/pages/self.hpp"
#include "menu/pages/settings.hpp"

#include <imgui.h>

namespace big
{
	void menu::draw()
	{
		ImGui::SetNextWindowSize(ImVec2(720.0f, 460.0f), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("BigBaseV2"))
		{
			draw_sidebar();
			ImGui::SameLine();
			draw_page();
		}
		ImGui::End();
	}

	void menu::draw_sidebar()
	{
		ImGui::BeginChild("Sidebar", ImVec2(150.0f, 0.0f), true);

		if (ImGui::Selectable("Self", m_current_page == page::self))
			m_current_page = page::self;
		if (ImGui::Selectable("Weapons", m_current_page == page::weapons))
			m_current_page = page::weapons;
		if (ImGui::Selectable("Vehicle", m_current_page == page::vehicle))
			m_current_page = page::vehicle;
		if (ImGui::Selectable("Teleport", m_current_page == page::teleport))
			m_current_page = page::teleport;
		if (ImGui::Selectable("World", m_current_page == page::world))
			m_current_page = page::world;
		if (ImGui::Selectable("Players", m_current_page == page::players))
			m_current_page = page::players;

		ImGui::Separator();
		ImGui::TextDisabled("Settings");
		if (ImGui::Selectable("Settings", m_current_page == page::settings))
			m_current_page = page::settings;
		if (ImGui::Selectable("Lua", m_current_page == page::lua))
			m_current_page = page::lua;

		ImGui::EndChild();
	}

	void menu::draw_page()
	{
		ImGui::BeginChild("Content", ImVec2(0.0f, 0.0f), true);

		switch (m_current_page)
		{
		case page::self:
			menu_pages::draw_self();
			break;
		case page::weapons:
			ImGui::Text("Weapons");
			break;
		case page::vehicle:
			ImGui::Text("Vehicle");
			break;
		case page::teleport:
			ImGui::Text("Teleport");
			break;
		case page::world:
			ImGui::Text("World");
			break;
		case page::players:
			ImGui::Text("Players");
			break;
		case page::settings:
			menu_pages::draw_settings();
			break;
		case page::lua:
			menu_pages::draw_lua();
			break;
		}

		ImGui::EndChild();
	}
}
