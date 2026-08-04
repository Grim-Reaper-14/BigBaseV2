#include "settings.hpp"
#include "../runtime.hpp"
#include "../widgets.hpp"

#include <imgui.h>

namespace big::menu_pages
{
	void draw_settings()
	{
		menu_ui::page_title("Settings", "Manage the menu runtime and safely unload BigBaseV2 from the game.");

		if (menu_ui::begin_section("SettingsRuntime", "Runtime", 135.0f))
		{
			menu_ui::status_badge("GTAV Enhanced target active", true);
			ImGui::TextDisabled("Menu key: INSERT");
			ImGui::TextDisabled("Unload key: END");
		}
		menu_ui::end_section();

		ImGui::Spacing();
		if (menu_ui::begin_section("SettingsDanger", "Unload", 125.0f))
		{
			ImGui::TextWrapped("Unload stops menu scripts, restores supported feature state, and removes the module.");
			if (ImGui::Button("Unload BigBaseV2", ImVec2(210.0f, 0.0f)))
				menu_runtime::request_unload();
		}
		menu_ui::end_section();
	}
}
