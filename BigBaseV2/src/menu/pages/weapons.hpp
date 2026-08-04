#pragma once

namespace big::menu_pages
{
	struct weapons_settings final
	{
		bool infinite_ammo{};
		bool infinite_clip{};
		bool no_recoil{};
		bool no_spread{};
		bool rapid_fire{};
		bool explosive_ammo{};
		bool fire_ammo{};
		float damage_multiplier{1.0f};
		float fire_rate_multiplier{1.0f};
	};

	inline weapons_settings g_weapons_settings;

	void draw_weapons();
}
