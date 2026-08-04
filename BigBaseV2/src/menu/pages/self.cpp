#include "self.hpp"

#include "../../natives.hpp"

#include <imgui.h>

namespace big::menu_pages
{
	namespace
	{
		void draw_atomic_checkbox(const char* label, std::atomic_bool& value)
		{
			bool current = value.load(std::memory_order_relaxed);
			if (ImGui::Checkbox(label, &current))
				value.store(current, std::memory_order_relaxed);
		}
	}

	void draw_self()
	{
		ImGui::Text("Self");
		ImGui::Separator();

		draw_atomic_checkbox("God Mode", g_self_settings.god_mode);
		draw_atomic_checkbox("Never Wanted", g_self_settings.never_wanted);
		draw_atomic_checkbox("No Ragdoll", g_self_settings.no_ragdoll);
		draw_atomic_checkbox("No Critical Hits", g_self_settings.no_critical_hits);
		draw_atomic_checkbox("Unlimited Stamina", g_self_settings.unlimited_stamina);
		draw_atomic_checkbox("Super Jump", g_self_settings.super_jump);
		draw_atomic_checkbox("Invisible", g_self_settings.invisible);
		draw_atomic_checkbox("Keep Clean", g_self_settings.keep_clean);
		draw_atomic_checkbox("Fast Run", g_self_settings.fast_run);

		float run_multiplier = g_self_settings.run_multiplier.load(std::memory_order_relaxed);
		if (ImGui::SliderFloat("Run Multiplier", &run_multiplier, 1.0f, 1.49f))
			g_self_settings.run_multiplier.store(run_multiplier, std::memory_order_relaxed);
	}

	void tick_self()
	{
		const auto player = PLAYER::PLAYER_ID();
		const auto ped = PLAYER::PLAYER_PED_ID();

		ENTITY::SET_ENTITY_INVINCIBLE(ped, g_self_settings.god_mode.load(std::memory_order_relaxed));

		if (g_self_settings.never_wanted.load(std::memory_order_relaxed))
			PLAYER::CLEAR_PLAYER_WANTED_LEVEL(player);

		PED::SET_PED_CAN_RAGDOLL(ped, !g_self_settings.no_ragdoll.load(std::memory_order_relaxed));
		PED::SET_PED_SUFFERS_CRITICAL_HITS(ped, !g_self_settings.no_critical_hits.load(std::memory_order_relaxed));

		if (g_self_settings.super_jump.load(std::memory_order_relaxed))
			PLAYER::SET_SUPER_JUMP_THIS_FRAME(player);

		ENTITY::SET_ENTITY_VISIBLE(
			ped,
			!g_self_settings.invisible.load(std::memory_order_relaxed),
			false);

		if (g_self_settings.unlimited_stamina.load(std::memory_order_relaxed))
			PLAYER::RESTORE_PLAYER_STAMINA(player, 1.0f);

		const float run_multiplier = g_self_settings.fast_run.load(std::memory_order_relaxed)
			? g_self_settings.run_multiplier.load(std::memory_order_relaxed)
			: 1.0f;
		PLAYER::SET_RUN_SPRINT_MULTIPLIER_FOR_PLAYER(player, run_multiplier);

		if (g_self_settings.keep_clean.load(std::memory_order_relaxed))
			PED::CLEAR_PED_BLOOD_DAMAGE(ped);
	}

	void reset_self()
	{
		const auto player = PLAYER::PLAYER_ID();
		const auto ped = PLAYER::PLAYER_PED_ID();

		ENTITY::SET_ENTITY_INVINCIBLE(ped, false);
		ENTITY::SET_ENTITY_VISIBLE(ped, true, false);
		PED::SET_PED_CAN_RAGDOLL(ped, true);
		PED::SET_PED_SUFFERS_CRITICAL_HITS(ped, true);
		PLAYER::SET_RUN_SPRINT_MULTIPLIER_FOR_PLAYER(player, 1.0f);
	}
}
