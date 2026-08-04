#include "self.hpp"

#include <imgui.h>

namespace big::menu_pages
{
	void draw_self()
	{
		ImGui::Text("Self");
		ImGui::Separator();

		static bool god_mode = false;
		static bool never_wanted = false;

		ImGui::Checkbox("God Mode", &god_mode);
		ImGui::Checkbox("Never Wanted", &never_wanted);
	}
}
