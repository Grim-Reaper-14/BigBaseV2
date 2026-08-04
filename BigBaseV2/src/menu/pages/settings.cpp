#include "settings.hpp"
#include "../runtime.hpp"
#include "../widgets.hpp"

#include "../../configuration.hpp"
#include "../../diagnostics.hpp"

#include <imgui.h>

namespace big::menu_pages
{
	namespace
	{
		char profile_name[49] = "default";
		std::string status = "Configuration system ready.";

		void apply_theme_preview()
		{
			auto& values = g_configuration.values();
			auto& style = ImGui::GetStyle();
			style.WindowRounding = values.window_rounding;
			style.ChildRounding = values.window_rounding;
			style.FrameRounding = values.frame_rounding;
			style.GrabRounding = values.frame_rounding;
			style.ScrollbarRounding = values.frame_rounding;

			const ImVec4 accent(values.accent[0], values.accent[1], values.accent[2], values.accent[3]);
			style.Colors[ImGuiCol_CheckMark] = accent;
			style.Colors[ImGuiCol_SliderGrab] = accent;
			style.Colors[ImGuiCol_SliderGrabActive] = accent;
			style.Colors[ImGuiCol_HeaderActive] = accent;
			style.Colors[ImGuiCol_ResizeGripActive] = accent;
			style.Colors[ImGuiCol_NavHighlight] = accent;
		}
	}

	void draw_settings()
	{
		auto& values = g_configuration.values();
		menu_ui::page_title("Settings", "Manage profiles, runtime behavior, appearance, diagnostics, and safe unloading.");

		if (menu_ui::begin_section("ConfigurationProfiles", "Configurations", 205.0f))
		{
			ImGui::TextDisabled("Directory: %s", g_configuration.directory().string().c_str());
			ImGui::Text("Active profile: %s", g_configuration.active_profile().c_str());
			ImGui::SetNextItemWidth(220.0f);
			ImGui::InputText("Profile", profile_name, sizeof(profile_name));

			if (ImGui::Button("Save", ImVec2(100.0f, 0.0f)))
			{
				if (g_configuration.save(profile_name, status))
					g_configuration.load(profile_name, status);
			}
			ImGui::SameLine();
			if (ImGui::Button("Load", ImVec2(100.0f, 0.0f)))
				g_configuration.load(profile_name, status);
			ImGui::SameLine();
			if (ImGui::Button("Delete", ImVec2(100.0f, 0.0f)))
				g_configuration.remove(profile_name, status);
			ImGui::SameLine();
			if (ImGui::Button("Defaults", ImVec2(100.0f, 0.0f)))
			{
				g_configuration.reset_defaults();
				apply_theme_preview();
				status = "Restored default runtime settings.";
			}

			const auto profiles = g_configuration.profiles();
			if (!profiles.empty() && ImGui::BeginCombo("Available", g_configuration.active_profile().c_str()))
			{
				for (const auto& profile : profiles)
				{
					const bool selected = profile == g_configuration.active_profile();
					if (ImGui::Selectable(profile.c_str(), selected))
					{
						std::strncpy(profile_name, profile.c_str(), sizeof(profile_name) - 1);
						profile_name[sizeof(profile_name) - 1] = '\0';
						g_configuration.load(profile, status);
						apply_theme_preview();
					}
					if (selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}
		menu_ui::end_section();

		ImGui::Spacing();
		if (menu_ui::begin_section("RuntimeSettings", "Runtime and Lua", 175.0f))
		{
			if (ImGui::Checkbox("Disable Game Controls While Menu Is Open", &values.disable_game_controls))
				g_configuration.apply_runtime();
			if (ImGui::Checkbox("Sandbox Lua Scripts", &values.lua_sandbox))
				g_configuration.apply_runtime();
			if (ImGui::Checkbox("Auto Reload Changed Lua Scripts", &values.lua_auto_reload))
				g_configuration.apply_runtime();
			ImGui::Checkbox("Autosave Active Profile On Unload", &values.autosave);
			ImGui::TextDisabled("Menu key: virtual-key %d | Unload key: virtual-key %d", values.menu_key, values.unload_key);
		}
		menu_ui::end_section();

		ImGui::Spacing();
		if (menu_ui::begin_section("ThemeSettings", "Appearance", 185.0f))
		{
			bool changed = ImGui::ColorEdit4("Accent", values.accent, ImGuiColorEditFlags_AlphaBar);
			changed |= ImGui::SliderFloat("Window Rounding", &values.window_rounding, 0.0f, 20.0f, "%.1f");
			changed |= ImGui::SliderFloat("Frame Rounding", &values.frame_rounding, 0.0f, 20.0f, "%.1f");
			if (changed)
				apply_theme_preview();
		}
		menu_ui::end_section();

		ImGui::Spacing();
		if (menu_ui::begin_section("RuntimeDiagnostics", "Diagnostics", 225.0f))
		{
			const auto diagnostics = collect_runtime_diagnostics();
			menu_ui::status_badge("Enhanced pointers", diagnostics.pointers_ready);
			menu_ui::status_badge("D3D11 renderer", diagnostics.renderer_ready);
			menu_ui::status_badge("Hooks enabled", diagnostics.hooks_enabled);
			menu_ui::status_badge("Native cache healthy", diagnostics.native_cache_healthy);
			menu_ui::status_badge("Sol2 runtime", diagnostics.lua_ready);
			ImGui::TextDisabled("Native handlers: %zu cached | %zu missing", diagnostics.native_cached, diagnostics.native_missing);
			ImGui::TextDisabled("Scripts: %zu registered | %zu Lua loaded", diagnostics.scripts_registered, diagnostics.lua_scripts_loaded);
			ImGui::TextDisabled("Fiber jobs queued: %zu", diagnostics.fiber_jobs_queued);
		}
		menu_ui::end_section();

		ImGui::Spacing();
		if (menu_ui::begin_section("SettingsStatus", "Status", 80.0f))
			ImGui::TextWrapped("%s", status.c_str());
		menu_ui::end_section();

		ImGui::Spacing();
		if (menu_ui::begin_section("SettingsDanger", "Unload", 125.0f))
		{
			ImGui::TextWrapped("Unload stops scripts, restores feature state, saves the active profile when enabled, and removes the module.");
			if (ImGui::Button("Unload BigBaseV2", ImVec2(210.0f, 0.0f)))
				menu_runtime::request_unload();
		}
		menu_ui::end_section();
	}
}
