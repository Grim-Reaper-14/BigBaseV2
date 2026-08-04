#include "common.hpp"
#include "gui.hpp"
#include "menu.hpp"
#include "menu/pages/self.hpp"
#include "menu/runtime.hpp"
#include "menu/widgets.hpp"
#include "natives.hpp"
#include "script.hpp"

#include <imgui.h>

namespace big
{
	void gui::dx_init()
	{
		auto& style = ImGui::GetStyle();
		style.WindowPadding = {12.0f, 12.0f};
		style.FramePadding = {9.0f, 5.0f};
		style.ItemSpacing = {10.0f, 8.0f};
		style.ItemInnerSpacing = {7.0f, 6.0f};
		style.TouchExtraPadding = {0.0f, 0.0f};
		style.IndentSpacing = 20.0f;
		style.ScrollbarSize = 13.0f;
		style.GrabMinSize = 10.0f;
		style.WindowBorderSize = 1.0f;
		style.ChildBorderSize = 1.0f;
		style.PopupBorderSize = 1.0f;
		style.FrameBorderSize = 0.0f;
		style.TabBorderSize = 0.0f;
		style.WindowRounding = 8.0f;
		style.ChildRounding = 6.0f;
		style.FrameRounding = 5.0f;
		style.PopupRounding = 6.0f;
		style.ScrollbarRounding = 8.0f;
		style.GrabRounding = 5.0f;
		style.TabRounding = 5.0f;
		style.WindowTitleAlign = {0.5f, 0.5f};
		style.ButtonTextAlign = {0.5f, 0.5f};
		style.DisplaySafeAreaPadding = {3.0f, 3.0f};

		const ImVec4 accent = menu_ui::accent;
		const ImVec4 accent_hovered{0.38f, 0.60f, 1.00f, 1.00f};
		const ImVec4 accent_active{0.24f, 0.43f, 0.82f, 1.00f};
		auto& colors = style.Colors;
		colors[ImGuiCol_Text] = {0.92f, 0.94f, 0.98f, 1.00f};
		colors[ImGuiCol_TextDisabled] = menu_ui::text_muted;
		colors[ImGuiCol_WindowBg] = {0.045f, 0.050f, 0.065f, 0.98f};
		colors[ImGuiCol_ChildBg] = {0.00f, 0.00f, 0.00f, 0.00f};
		colors[ImGuiCol_PopupBg] = {0.065f, 0.072f, 0.090f, 0.98f};
		colors[ImGuiCol_Border] = {0.18f, 0.22f, 0.30f, 0.75f};
		colors[ImGuiCol_BorderShadow] = {0.00f, 0.00f, 0.00f, 0.00f};
		colors[ImGuiCol_FrameBg] = {0.105f, 0.118f, 0.150f, 1.00f};
		colors[ImGuiCol_FrameBgHovered] = {0.135f, 0.155f, 0.205f, 1.00f};
		colors[ImGuiCol_FrameBgActive] = {0.155f, 0.180f, 0.240f, 1.00f};
		colors[ImGuiCol_TitleBg] = {0.055f, 0.062f, 0.080f, 1.00f};
		colors[ImGuiCol_TitleBgActive] = {0.070f, 0.082f, 0.110f, 1.00f};
		colors[ImGuiCol_TitleBgCollapsed] = {0.045f, 0.050f, 0.065f, 0.90f};
		colors[ImGuiCol_MenuBarBg] = {0.060f, 0.068f, 0.088f, 1.00f};
		colors[ImGuiCol_ScrollbarBg] = {0.035f, 0.040f, 0.052f, 0.80f};
		colors[ImGuiCol_ScrollbarGrab] = {0.18f, 0.21f, 0.28f, 1.00f};
		colors[ImGuiCol_ScrollbarGrabHovered] = {0.24f, 0.28f, 0.37f, 1.00f};
		colors[ImGuiCol_ScrollbarGrabActive] = accent_active;
		colors[ImGuiCol_CheckMark] = accent;
		colors[ImGuiCol_SliderGrab] = accent;
		colors[ImGuiCol_SliderGrabActive] = accent_hovered;
		colors[ImGuiCol_Button] = {accent.x, accent.y, accent.z, 0.72f};
		colors[ImGuiCol_ButtonHovered] = accent_hovered;
		colors[ImGuiCol_ButtonActive] = accent_active;
		colors[ImGuiCol_Header] = {accent.x, accent.y, accent.z, 0.26f};
		colors[ImGuiCol_HeaderHovered] = {accent.x, accent.y, accent.z, 0.42f};
		colors[ImGuiCol_HeaderActive] = {accent.x, accent.y, accent.z, 0.58f};
		colors[ImGuiCol_Separator] = {0.18f, 0.22f, 0.30f, 0.80f};
		colors[ImGuiCol_SeparatorHovered] = accent_hovered;
		colors[ImGuiCol_SeparatorActive] = accent_active;
		colors[ImGuiCol_ResizeGrip] = {accent.x, accent.y, accent.z, 0.18f};
		colors[ImGuiCol_ResizeGripHovered] = {accent.x, accent.y, accent.z, 0.48f};
		colors[ImGuiCol_ResizeGripActive] = accent_active;
		colors[ImGuiCol_Tab] = {0.09f, 0.10f, 0.13f, 1.00f};
		colors[ImGuiCol_TabHovered] = {accent.x, accent.y, accent.z, 0.55f};
		colors[ImGuiCol_TabActive] = {accent.x, accent.y, accent.z, 0.38f};
		colors[ImGuiCol_TabUnfocused] = {0.065f, 0.072f, 0.090f, 1.00f};
		colors[ImGuiCol_TabUnfocusedActive] = {0.10f, 0.115f, 0.15f, 1.00f};
		colors[ImGuiCol_PlotLines] = accent;
		colors[ImGuiCol_PlotLinesHovered] = accent_hovered;
		colors[ImGuiCol_PlotHistogram] = {0.34f, 0.82f, 0.48f, 1.00f};
		colors[ImGuiCol_PlotHistogramHovered] = {0.42f, 0.92f, 0.56f, 1.00f};
		colors[ImGuiCol_TextSelectedBg] = {accent.x, accent.y, accent.z, 0.32f};
		colors[ImGuiCol_DragDropTarget] = accent_hovered;
		colors[ImGuiCol_NavHighlight] = accent;
		colors[ImGuiCol_NavWindowingHighlight] = {0.92f, 0.94f, 0.98f, 0.70f};
		colors[ImGuiCol_NavWindowingDimBg] = {0.00f, 0.00f, 0.00f, 0.35f};
		colors[ImGuiCol_ModalWindowDimBg] = {0.00f, 0.00f, 0.00f, 0.55f};
	}

	void gui::dx_on_tick()
	{
		g_menu.draw();
	}

	void gui::script_init()
	{
	}

	void gui::script_on_tick()
	{
		menu_pages::tick_self();

		if (g_gui.m_opened && menu_runtime::disable_game_controls.load(std::memory_order_relaxed))
			CONTROLS::DISABLE_ALL_CONTROL_ACTIONS(0);

		if (menu_runtime::consume_unload_request())
			g_running = false;
	}

	void gui::script_func()
	{
		g_gui.script_init();
		while (g_running)
		{
			g_gui.script_on_tick();
			script::get_current()->yield();
		}

		menu_pages::reset_self();
	}
}
