#include "menu.hpp"

#include <imgui.h>

namespace big
{
	void menu::draw()
	{
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
		ImGui::BeginChild("Sidebar", ImVec2(150, 0), true);

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
		if (ImGui::Selectable("Settings", m_current_page == page::settings))
			m_current_page = page::settings;

		ImGui::EndChild();
	}

	void menu::draw_page()
	{
		ImGui::BeginChild("Content", ImVec2(0, 0), true);

		switch (m_current_page)
		{
		case page::self:
			ImGui::Text("Self");
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
			ImGui::Text("Settings");
			break;
		}

		ImGui::EndChild();
	}
}
