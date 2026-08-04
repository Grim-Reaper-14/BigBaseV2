#pragma once

#include <array>

namespace big::menu_pages
{
	struct teleport_settings final
	{
		std::array<float, 3> custom_coordinates{0.0f, 0.0f, 0.0f};
		bool keep_vehicle{true};
		bool load_ground{true};
	};

	inline teleport_settings g_teleport_settings;

	void draw_teleport();
}
