#include "weapons.hpp"
#include "../widgets.hpp"

#include <imgui.h>

namespace big::menu_pages
{
	void draw_weapons()
	{
		menu_ui::page_title("Weapons", "Configure ammunition, accuracy, projectile effects, and weapon modifiers.");

		if (menu_ui::begin_section("WeaponsAmmo", "Ammunition", 105.0f))
		{
			ImGui::Checkbox("Infinite Ammo", &g_weapons_settings.infinite_ammo);
			ImGui::Checkbox("Infinite Clip", &g_weapons_settings.infinite_clip);
		}
		menu_ui::end_section();

		ImGui::Spacing();
		if (menu_ui::begin_section("WeaponsAccuracy", "Accuracy & Fire Control", 130.0f))
		{
			ImGui::Checkbox("No Recoil", &g_weapons_settings.no_recoil);
			ImGui::Checkbox("No Spread", &g_weapons_settings.no_spread);
			ImGui::Checkbox("Rapid Fire", &g_weapons_settings.rapid_fire);
		}
		menu_ui::end_section();

		ImGui::Spacing();
		if (menu_ui::begin_section("WeaponsEffects", "Projectile Effects", 105.0f))
		{
			ImGui::Checkbox("Explosive Ammo", &g_weapons_settings.explosive_ammo);
			ImGui::Checkbox("Fire Ammo", &g_weapons_settings.fire_ammo);
		}
		menu_ui::end_section();

		ImGui::Spacing();
		if (menu_ui::begin_section("WeaponsModifiers", "Modifiers", 125.0f))
		{
			ImGui::SliderFloat("Damage Multiplier", &g_weapons_settings.damage_multiplier, 1.0f, 10.0f, "%.1fx");
			ImGui::SliderFloat("Fire Rate Multiplier", &g_weapons_settings.fire_rate_multiplier, 1.0f, 5.0f, "%.1fx");
		}
		menu_ui::end_section();
	}
}
