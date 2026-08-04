#include "settings.hpp"
#include "../runtime.hpp"

#include <imgui.h>

namespace big::menu_pages
{
	void draw_settings()
	{
		ImGui::Text("Settings");
		ImGui::Separator();

		if (ImGui::Button("Unload BigBaseV2"))
		{
			menu_runtime::request_unload();
		}
	}
}
