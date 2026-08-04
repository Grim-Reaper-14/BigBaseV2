#include "teleport.hpp"

#include <imgui.h>

namespace big::menu_pages
{
	void draw_teleport()
	{
		ImGui::Text("Teleport");
		ImGui::Separator();

		ImGui::Checkbox("Keep Current Vehicle", &g_teleport_settings.keep_vehicle);
		ImGui::Checkbox("Load Ground Before Teleport", &g_teleport_settings.load_ground);

		ImGui::Spacing();
		ImGui::TextDisabled("Quick Locations");
		if (ImGui::Button("Waypoint", ImVec2(150.0f, 0.0f))) {}
		ImGui::SameLine();
		if (ImGui::Button("Objective", ImVec2(150.0f, 0.0f))) {}
		if (ImGui::Button("Los Santos Customs", ImVec2(150.0f, 0.0f))) {}
		ImGui::SameLine();
		if (ImGui::Button("Airport", ImVec2(150.0f, 0.0f))) {}
		if (ImGui::Button("Mount Chiliad", ImVec2(150.0f, 0.0f))) {}
		ImGui::SameLine();
		if (ImGui::Button("Fort Zancudo", ImVec2(150.0f, 0.0f))) {}

		ImGui::Spacing();
		ImGui::TextDisabled("Custom Coordinates");
		ImGui::InputFloat3("Position", g_teleport_settings.custom_coordinates.data(), "%.3f");
		if (ImGui::Button("Teleport To Coordinates", ImVec2(220.0f, 0.0f)))
		{
			// Action wiring is handled by the teleport feature layer.
		}
	}
}
