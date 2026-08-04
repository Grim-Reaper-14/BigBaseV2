#include "menu.hpp"

#include "menu/pages/lua.hpp"
#include "menu/pages/players.hpp"
#include "menu/pages/self.hpp"
#include "menu/pages/settings.hpp"
#include "menu/pages/teleport.hpp"
#include "menu/pages/vehicle.hpp"
#include "menu/pages/weapons.hpp"
#include "menu/pages/world.hpp"

#include <imgui.h>

namespace big
{
	void menu::draw()
	{
		ImGui::SetNextWindowSize(ImVec2(820.0f, 520.0f), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("BigBaseV2", nullptr, ImGuiWindowFlags_NoCollapse))
		{
			draw_sidebar();
			ImGui::SameLine();
			draw_page();
		}
		ImGui::End();
	}

	void menu::draw_sidebar()
	{
		ImGui::BeginChild("Sidebar", ImVec2(170.0f, 0.0f), true);

		ImGui::Text("BigBaseV2");
		ImGui::TextDisabled("Modular Menu Base");
		ImGui::Separator();

		ImGui::TextDisabled("Player");
		if (ImGui::Selectable("Self", m_current_page == page::self))
			m_current_page = page::self;
		if (ImGui::Selectable("Weapons", m_current_page == page::weapons))
			m_current_page = page::weapons;
		if (ImGui::Selectable("Vehicle", m_current_page == page::vehicle))
			m_current_page = page::vehicle;
		if (ImGui::Selectable("Teleport", m_current_page == page::teleport))
			m_current_page = page::teleport;

		ImGui::Spacing();
		ImGui::TextDisabled("Session");
		if (ImGui::Selectable("World", m_current_page == page::world))
			m_current_page = page::world;
		if (ImGui::Selectable("Players", m_current_page == page::players))
			m_current_page = page::players;

		ImGui::Spacing();
		ImGui::TextDisabled("System");
		if (ImGui::Selectable("Settings", m_current_page == page::settings))
			m_current_page = page::settings;
		if (ImGui::Selectable("Lua", m_current_page == page::lua))
			m_current_page = page::lua;

		const float footer_height = ImGui::GetTextLineHeightWithSpacing() * 2.0f;
		if (ImGui::GetContentRegionAvail().y > footer_height)
			ImGui::SetCursorPosY(ImGui::GetWindowHeight() - footer_height - ImGui::GetStyle().WindowPadding.y);

		ImGui::Separator();
		ImGui::TextDisabled("INSERT - Toggle Menu");
		ImGui::TextDisabled("END - Unload");

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
			menu_pages::draw_weapons();
			break;
		case page::vehicle:
			menu_pages::draw_vehicle();
			break;
		case page::teleport:
			menu_pages::draw_teleport();
			break;
		case page::world:
			menu_pages::draw_world();
			break;
		case page::players:
			menu_pages::draw_players();
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
