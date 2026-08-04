#include "world.hpp"

#include <imgui.h>

namespace big::menu_pages
{
	void draw_world()
	{
		ImGui::Text("World");
		ImGui::Separator();

		ImGui::TextDisabled("Time");
		ImGui::Checkbox("Freeze Time", &g_world_settings.freeze_time);
		ImGui::SliderInt("Hour", &g_world_settings.hour, 0, 23);
		ImGui::SliderInt("Minute", &g_world_settings.minute, 0, 59);
		ImGui::SliderFloat("Time Scale", &g_world_settings.time_scale, 0.1f, 2.0f, "%.1fx");

		ImGui::Spacing();
		ImGui::TextDisabled("Environment");
		ImGui::Checkbox("Clear Weather", &g_world_settings.clear_weather);
		ImGui::Checkbox("Blackout", &g_world_settings.blackout);
		ImGui::Checkbox("Low Gravity", &g_world_settings.low_gravity);

		ImGui::Spacing();
		ImGui::TextDisabled("Population");
		ImGui::Checkbox("Disable Traffic", &g_world_settings.disable_traffic);
		ImGui::Checkbox("Disable Pedestrians", &g_world_settings.disable_pedestrians);
	}
}
