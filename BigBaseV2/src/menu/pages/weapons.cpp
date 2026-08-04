#include "weapons.hpp"

#include <imgui.h>

namespace big::menu_pages
{
	void draw_weapons()
	{
		ImGui::Text("Weapons");
		ImGui::Separator();

		ImGui::TextDisabled("Ammo");
		ImGui::Checkbox("Infinite Ammo", &g_weapons_settings.infinite_ammo);
		ImGui::Checkbox("Infinite Clip", &g_weapons_settings.infinite_clip);

		ImGui::Spacing();
		ImGui::TextDisabled("Accuracy");
		ImGui::Checkbox("No Recoil", &g_weapons_settings.no_recoil);
		ImGui::Checkbox("No Spread", &g_weapons_settings.no_spread);
		ImGui::Checkbox("Rapid Fire", &g_weapons_settings.rapid_fire);

		ImGui::Spacing();
		ImGui::TextDisabled("Projectile Effects");
		ImGui::Checkbox("Explosive Ammo", &g_weapons_settings.explosive_ammo);
		ImGui::Checkbox("Fire Ammo", &g_weapons_settings.fire_ammo);

		ImGui::Spacing();
		ImGui::TextDisabled("Modifiers");
		ImGui::SliderFloat("Damage Multiplier", &g_weapons_settings.damage_multiplier, 1.0f, 10.0f, "%.1fx");
		ImGui::SliderFloat("Fire Rate Multiplier", &g_weapons_settings.fire_rate_multiplier, 1.0f, 5.0f, "%.1fx");
	}
}
