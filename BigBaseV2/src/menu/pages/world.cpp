#include "world.hpp"
#include "../widgets.hpp"

#include <imgui.h>

namespace big::menu_pages
{
	void draw_world()
	{
		menu_ui::page_title("World", "Control time, weather, gravity, traffic, and ambient population settings.");

		if (menu_ui::begin_section("WorldTime", "Time", 145.0f))
		{
			ImGui::Checkbox("Freeze Time", &g_world_settings.freeze_time);
			ImGui::SliderInt("Hour", &g_world_settings.hour, 0, 23);
			ImGui::SliderInt("Minute", &g_world_settings.minute, 0, 59);
			ImGui::SliderFloat("Time Scale", &g_world_settings.time_scale, 0.1f, 2.0f, "%.1fx");
		}
		menu_ui::end_section();

		ImGui::Spacing();
		if (menu_ui::begin_section("WorldEnvironment", "Environment", 130.0f))
		{
			ImGui::Checkbox("Clear Weather", &g_world_settings.clear_weather);
			ImGui::Checkbox("Blackout", &g_world_settings.blackout);
			ImGui::Checkbox("Low Gravity", &g_world_settings.low_gravity);
		}
		menu_ui::end_section();

		ImGui::Spacing();
		if (menu_ui::begin_section("WorldPopulation", "Population", 105.0f))
		{
			ImGui::Checkbox("Disable Traffic", &g_world_settings.disable_traffic);
			ImGui::Checkbox("Disable Pedestrians", &g_world_settings.disable_pedestrians);
		}
		menu_ui::end_section();
	}
}
