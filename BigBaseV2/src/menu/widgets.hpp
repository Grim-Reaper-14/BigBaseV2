#pragma once

#include <imgui.h>

#include <atomic>
#include <string_view>

namespace big::menu_ui
{
	inline constexpr ImVec4 accent{0.31f, 0.52f, 0.95f, 1.00f};
	inline constexpr ImVec4 text_muted{0.52f, 0.56f, 0.64f, 1.00f};
	inline constexpr ImVec4 panel_background{0.075f, 0.082f, 0.098f, 1.00f};

	inline void page_title(const char* title, const char* description)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, accent);
		ImGui::TextUnformatted(title);
		ImGui::PopStyleColor();
		ImGui::PushStyleColor(ImGuiCol_Text, text_muted);
		ImGui::TextWrapped("%s", description);
		ImGui::PopStyleColor();
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
	}

	inline bool begin_section(const char* id, const char* title, float height = 0.0f)
	{
		ImGui::PushStyleColor(ImGuiCol_ChildBg, panel_background);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);

		const bool visible = ImGui::BeginChild(id, ImVec2(0.0f, height), true);
		ImGui::PushStyleColor(ImGuiCol_Text, accent);
		ImGui::TextUnformatted(title);
		ImGui::PopStyleColor();
		ImGui::Separator();
		ImGui::Spacing();
		return visible;
	}

	inline void end_section()
	{
		ImGui::EndChild();
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor();
	}

	inline bool toggle(const char* label, std::atomic_bool& value, const char* help = nullptr)
	{
		bool current = value.load(std::memory_order_relaxed);
		const bool changed = ImGui::Checkbox(label, &current);
		if (changed)
			value.store(current, std::memory_order_relaxed);

		if (help && ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::TextWrapped("%s", help);
			ImGui::EndTooltip();
		}
		return changed;
	}

	inline bool slider_float(const char* label, std::atomic<float>& value, float minimum, float maximum, const char* format = "%.2f")
	{
		float current = value.load(std::memory_order_relaxed);
		const bool changed = ImGui::SliderFloat(label, &current, minimum, maximum, format);
		if (changed)
			value.store(current, std::memory_order_relaxed);
		return changed;
	}

	inline void status_badge(const char* label, bool active)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, active ? ImVec4(0.34f, 0.82f, 0.48f, 1.0f) : text_muted);
		ImGui::TextUnformatted(label);
		ImGui::PopStyleColor();
	}
}
