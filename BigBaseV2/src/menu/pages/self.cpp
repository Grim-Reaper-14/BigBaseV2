#include "self.hpp"

#include "../../natives.hpp"
#include "../widgets.hpp"

#include <imgui.h>

namespace big::menu_pages
{
	void draw_self()
	{
		menu_ui::page_title("Self", "Player protection, movement, appearance, and quality-of-life controls.");

		const float column_width = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

		ImGui::BeginGroup();
		ImGui::PushItemWidth(column_width);
		if (menu_ui::begin_section("SelfProtection", "Protection", 225.0f))
		{
			menu_ui::toggle("God Mode", g_self_settings.god_mode, "Prevents normal damage from reducing player health.");
			menu_ui::toggle("Never Wanted", g_self_settings.never_wanted, "Continuously clears the local wanted level.");
			menu_ui::toggle("No Ragdoll", g_self_settings.no_ragdoll);
			menu_ui::toggle("No Critical Hits", g_self_settings.no_critical_hits);
			menu_ui::toggle("Unlimited Stamina", g_self_settings.unlimited_stamina);
		}
		menu_ui::end_section();
		ImGui::PopItemWidth();
		ImGui::EndGroup();

		ImGui::SameLine();
		ImGui::BeginGroup();
		ImGui::PushItemWidth(column_width);
		if (menu_ui::begin_section("SelfMovement", "Movement", 225.0f))
		{
			menu_ui::toggle("Super Jump", g_self_settings.super_jump);
			menu_ui::toggle("Fast Run", g_self_settings.fast_run);
			ImGui::BeginDisabled(!g_self_settings.fast_run.load(std::memory_order_relaxed));
			menu_ui::slider_float("Run Multiplier", g_self_settings.run_multiplier, 1.0f, 1.49f, "%.2fx");
			ImGui::EndDisabled();
		}
		menu_ui::end_section();
		ImGui::PopItemWidth();
		ImGui::EndGroup();

		ImGui::Spacing();
		if (menu_ui::begin_section("SelfAppearance", "Appearance & Maintenance", 135.0f))
		{
			menu_ui::toggle("Invisible", g_self_settings.invisible, "Hides the local player entity while enabled.");
			menu_ui::toggle("Keep Clean", g_self_settings.keep_clean, "Continuously clears blood damage from the player model.");
		}
		menu_ui::end_section();
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
