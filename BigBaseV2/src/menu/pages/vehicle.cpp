#include "vehicle.hpp"

#include <imgui.h>

namespace big::menu_pages
{
	void draw_vehicle()
	{
		ImGui::Text("Vehicle");
		ImGui::Separator();

		ImGui::TextDisabled("Protection");
		ImGui::Checkbox("Vehicle God Mode", &g_vehicle_settings.god_mode);
		ImGui::Checkbox("Repair Loop", &g_vehicle_settings.repair_loop);
		ImGui::Checkbox("Seatbelt", &g_vehicle_settings.seatbelt);

		ImGui::Spacing();
		ImGui::TextDisabled("Driving");
		ImGui::Checkbox("Horn Boost", &g_vehicle_settings.horn_boost);
		ImGui::SliderFloat("Acceleration", &g_vehicle_settings.acceleration_multiplier, 1.0f, 10.0f, "%.1fx");
		ImGui::SliderFloat("Gravity", &g_vehicle_settings.gravity_multiplier, 0.1f, 3.0f, "%.1fx");

		ImGui::Spacing();
		ImGui::TextDisabled("Appearance");
		ImGui::Checkbox("Rainbow Paint", &g_vehicle_settings.rainbow_paint);

		ImGui::Spacing();
		if (ImGui::Button("Repair Current Vehicle", ImVec2(190.0f, 0.0f)))
		{
			// Action wiring is intentionally handled by the vehicle feature layer.
		}
		ImGui::SameLine();
		if (ImGui::Button("Clean Current Vehicle", ImVec2(190.0f, 0.0f)))
		{
			// Action wiring is intentionally handled by the vehicle feature layer.
		}
	}
}
