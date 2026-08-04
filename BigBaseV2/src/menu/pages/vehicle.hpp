#pragma once

namespace big::menu_pages
{
	struct vehicle_settings final
	{
		bool god_mode{};
		bool repair_loop{};
		bool seatbelt{};
		bool horn_boost{};
		bool rainbow_paint{};
		float acceleration_multiplier{1.0f};
		float gravity_multiplier{1.0f};
	};

	inline vehicle_settings g_vehicle_settings;

	void draw_vehicle();
}
