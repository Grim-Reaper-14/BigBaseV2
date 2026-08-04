#pragma once

#include "common.hpp"

#include <imgui.h>

namespace big
{
	struct Theme_Configuration final
	{
		std::string name{"Default"};
		ImVec4 accent{0.22f, 0.48f, 0.95f, 1.0f};
		float window_rounding{8.0f};
		float child_rounding{7.0f};
		float frame_rounding{5.0f};
		float popup_rounding{7.0f};
		float scrollbar_rounding{9.0f};
		float grab_rounding{5.0f};
		std::array<ImVec4, ImGuiCol_COUNT> colors{};
	};

	class Themes_Manager final
	{
	public:
		Themes_Manager();

		Themes_Manager(const Themes_Manager&) = delete;
		Themes_Manager(Themes_Manager&&) = delete;
		Themes_Manager& operator=(const Themes_Manager&) = delete;
		Themes_Manager& operator=(Themes_Manager&&) = delete;

		void Initialize();
		void CaptureCurrent(const std::string& name);
		bool Apply(const std::string& name, std::string& status);
		bool Apply(const Theme_Configuration& theme, std::string& status);
		bool Save(const std::string& name, std::string& status) const;
		bool Load(const std::filesystem::path& path, std::string& status);
		bool Remove(const std::string& name, std::string& status);
		void ResetToDefault(std::string& status);

		[[nodiscard]] Theme_Configuration* Get(const std::string& name) noexcept;
		[[nodiscard]] const Theme_Configuration* Get(const std::string& name) const noexcept;
		[[nodiscard]] std::vector<std::string> Names() const;
		[[nodiscard]] const std::string& ActiveTheme() const noexcept;
		[[nodiscard]] const std::filesystem::path& Directory() const noexcept;

	private:
		void RegisterBuiltInThemes();
		[[nodiscard]] static std::string NormalizeName(std::string name);
		[[nodiscard]] static Theme_Configuration FromCurrentStyle(const std::string& name);
		static void ApplyToStyle(const Theme_Configuration& theme);

		mutable std::recursive_mutex m_mutex;
		std::filesystem::path m_directory;
		std::unordered_map<std::string, Theme_Configuration> m_themes;
		std::string m_active_theme{"default"};
	};

	inline Themes_Manager g_themes_manager;
}
