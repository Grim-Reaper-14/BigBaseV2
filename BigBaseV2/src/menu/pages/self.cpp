#include "self.hpp"

#include "../../natives.hpp"

#include <imgui.h>

namespace big::menu_pages
{
	void draw_self()
	{
		ImGui::Text("Self");
		ImGui::Separator();

		ImGui::Checkbox("God Mode", &g_self_settings.god_mode);
		ImGui::Checkbox("Never Wanted", &g_self_settings.never_wanted);
		ImGui::Checkbox("No Ragdoll", &g_self_settings.no_ragdoll);
		ImGui::Checkbox("No Critical Hits", &g_self_settings.no_critical_hits);
		ImGui::Checkbox("Unlimited Stamina", &g_self_settings.unlimited_stamina);
		ImGui::Checkbox("Super Jump", &g_self_settings.super_jump);
		ImGui::Checkbox("Invisible", &g_self_settings.invisible);
		ImGui::Checkbox("Keep Clean", &g_self_settings.keep_clean);
		ImGui::Checkbox("Fast Run", &g_self_settings.fast_run);

		ImGui::SliderFloat("Run Multiplier", &g_self_settings.run_multiplier, 1.0f, 5.0f);
	}

	void tick_self()
	{
		auto ped = PLAYER::PLAYER_PED_ID();

		ENTITY::SET_ENTITY_INVINCIBLE(ped, g_self_settings.god_mode);

		if (g_self_settings.never_wanted)
		{
			PLAYER::CLEAR_PLAYER_WANTED_LEVEL(PLAYER::PLAYER_ID());
		}

		PED::SET_PED_CAN_RAGDOLL(ped, !g_self_settings.no_ragdoll);
		PED::SET_PED_SUFFERS_CRITICAL_HITS(ped, !g_self_settings.no_critical_hits);

		if (g_self_settings.super_jump)
		{
			PLAYER::SET_SUPER_JUMP_THIS_FRAME(PLAYER::PLAYER_ID());
		}

		if (g_self_settings.invisible)
		{
			ENTITY::SET_ENTITY_VISIBLE(ped, false, false);
		}
		else
		{
			ENTITY::SET_ENTITY_VISIBLE(ped, true, false);
		}

		if (g_self_settings.unlimited_stamina)
		{
			PLAYER::RESTORE_PLAYER_STAMINA(PLAYER::PLAYER_ID(), 100.0f);
		}

		if (g_self_settings.fast_run)
		{
			PLAYER::SET_RUN_SPRINT_MULTIPLIER_FOR_PLAYER(PLAYER::PLAYER_ID(), g_self_settings.run_multiplier);
		}

		if (g_self_settings.keep_clean)
		{
			PED::CLEAR_PED_BLOOD_DAMAGE(ped);
		}
	}
}
