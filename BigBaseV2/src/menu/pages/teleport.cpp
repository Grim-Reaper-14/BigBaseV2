#include "teleport.hpp"
#include "../widgets.hpp"

#include <imgui.h>

namespace big::menu_pages
{
	void draw_teleport()
	{
		menu_ui::page_title("Teleport", "Move to common destinations, map markers, or precise world coordinates.");

		if (menu_ui::begin_section("TeleportOptions", "Teleport Options", 105.0f))
		{
			ImGui::Checkbox("Keep Current Vehicle", &g_teleport_settings.keep_vehicle);
			ImGui::Checkbox("Load Ground Before Teleport", &g_teleport_settings.load_ground);
		}
		menu_ui::end_section();

		ImGui::Spacing();
		if (menu_ui::begin_section("TeleportQuick", "Quick Locations", 165.0f))
		{
			if (ImGui::Button("Waypoint", ImVec2(155.0f, 0.0f))) {}
			ImGui::SameLine();
			if (ImGui::Button("Objective", ImVec2(155.0f, 0.0f))) {}
			if (ImGui::Button("Los Santos Customs", ImVec2(155.0f, 0.0f))) {}
			ImGui::SameLine();
			if (ImGui::Button("Airport", ImVec2(155.0f, 0.0f))) {}
			if (ImGui::Button("Mount Chiliad", ImVec2(155.0f, 0.0f))) {}
			ImGui::SameLine();
			if (ImGui::Button("Fort Zancudo", ImVec2(155.0f, 0.0f))) {}
		}
		menu_ui::end_section();

		ImGui::Spacing();
		if (menu_ui::begin_section("TeleportCustom", "Custom Coordinates", 135.0f))
		{
			ImGui::SetNextItemWidth(-1.0f);
			ImGui::InputFloat3("##TeleportPosition", g_teleport_settings.custom_coordinates.data(), "%.3f");
			if (ImGui::Button("Teleport To Coordinates", ImVec2(220.0f, 0.0f)))
			{
				// Action wiring is handled by the teleport feature layer.
			}
		}
		menu_ui::end_section();
	}
}
