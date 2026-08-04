#pragma once

namespace big::menu_pages
{
	struct world_settings final
	{
		bool freeze_time{};
		bool blackout{};
		bool clear_weather{};
		bool low_gravity{};
		bool disable_traffic{};
		bool disable_pedestrians{};
		int hour{12};
		int minute{};
		float time_scale{1.0f};
	};

	inline world_settings g_world_settings;

	void draw_world();
}
