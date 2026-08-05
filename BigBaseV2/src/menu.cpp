#include "menu.hpp"

#include "menu/pages/lua.hpp"
#include "menu/pages/network.hpp"
#include "menu/pages/players.hpp"
#include "menu/pages/self.hpp"
#include "menu/pages/settings.hpp"
#include "menu/pages/teleport.hpp"
#include "menu/pages/vehicle.hpp"
#include "menu/pages/weapons.hpp"
#include "menu/pages/world.hpp"
#include "menu/widgets.hpp"

#include <imgui.h>

namespace big
{
	namespace
	{
		const char* page_name(menu::page page)
		{
			switch (page)
			{
			case menu::page::self: return "Self";
			case menu::page::weapons: return "Weapons";
			case menu::page::vehicle: return "Vehicle";
			case menu::page::teleport: return "Teleport";
			case menu::page::world: return "World";
			case menu::page::network: return "Network";
			case menu::page::players: return "Players";
			case menu::page::settings: return "Settings";
			case menu::page::lua: return "Lua";
			}
			return "Unknown";
		}

		bool navigation_item(const char* label, bool selected)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.08f, 0.5f));
			const bool clicked = ImGui::Selectable(label, selected, 0, ImVec2(0.0f, 34.0f));
			ImGui::PopStyleVar();
			return clicked;
		}
	}

	void menu::draw()
	{
		ImGui::SetNextWindowSize(ImVec2(940.0f, 610.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints(ImVec2(760.0f, 480.0f), ImVec2(1600.0f, 1000.0f));

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.045f, 0.050f, 0.062f, 0.98f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.14f, 0.16f, 0.20f, 1.0f));

		if (ImGui::Begin("BigBaseV2 Enhanced", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar))
		{
			draw_sidebar();
			ImGui::SameLine();
			draw_page();
		}
		ImGui::End();

		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar(2);
	}

	void menu::draw_sidebar()
	{
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.060f, 0.067f, 0.082f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 7.0f);
		ImGui::BeginChild("Sidebar", ImVec2(190.0f, 0.0f), true);

		ImGui::PushStyleColor(ImGuiCol_Text, menu_ui::accent);
		ImGui::TextUnformatted("BIGBASE V2");
		ImGui::PopStyleColor();
		ImGui::TextDisabled("GTA V Enhanced");
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::TextDisabled("PLAYER");
		if (navigation_item("  Self", m_current_page == page::self)) m_current_page = page::self;
		if (navigation_item("  Weapons", m_current_page == page::weapons)) m_current_page = page::weapons;
		if (navigation_item("  Vehicle", m_current_page == page::vehicle)) m_current_page = page::vehicle;
		if (navigation_item("  Teleport", m_current_page == page::teleport)) m_current_page = page::teleport;

		ImGui::Spacing();
		ImGui::TextDisabled("SESSION");
		if (navigation_item("  World", m_current_page == page::world)) m_current_page = page::world;
		if (navigation_item("  Network", m_current_page == page::network)) m_current_page = page::network;
		if (navigation_item("  Players", m_current_page == page::players)) m_current_page = page::players;

		ImGui::Spacing();
		ImGui::TextDisabled("SYSTEM");
		if (navigation_item("  Settings", m_current_page == page::settings)) m_current_page = page::settings;
		if (navigation_item("  Lua Scripts", m_current_page == page::lua)) m_current_page = page::lua;

		const float footer_height = ImGui::GetTextLineHeightWithSpacing() * 3.2f;
		if (ImGui::GetContentRegionAvail().y > footer_height)
			ImGui::SetCursorPosY(ImGui::GetWindowHeight() - footer_height - ImGui::GetStyle().WindowPadding.y);

		ImGui::Separator();
		menu_ui::status_badge("Enhanced Runtime", true);
		ImGui::TextDisabled("INSERT  Menu");
		ImGui::TextDisabled("END     Safe unload");

		ImGui::EndChild();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
	}

	void menu::draw_page()
	{
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.050f, 0.055f, 0.068f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 7.0f);
		ImGui::BeginChild("Content", ImVec2(0.0f, 0.0f), true);

		ImGui::TextDisabled("BIGBASEV2 / %s", page_name(m_current_page));
		ImGui::Spacing();

		switch (m_current_page)
		{
		case page::self: menu_pages::draw_self(); break;
		case page::weapons: menu_pages::draw_weapons(); break;
		case page::vehicle: menu_pages::draw_vehicle(); break;
		case page::teleport: menu_pages::draw_teleport(); break;
		case page::world: menu_pages::draw_world(); break;
		case page::network: menu_pages::draw_network(); break;
		case page::players: menu_pages::draw_players(); break;
		case page::settings: menu_pages::draw_settings(); break;
		case page::lua: menu_pages::draw_lua(); break;
		}

		ImGui::EndChild();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
	}
}
