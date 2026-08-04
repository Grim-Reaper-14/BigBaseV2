#include "Themes_Manager.hpp"

#include "logger.hpp"

namespace big
{
	Themes_Manager::Themes_Manager()
	{
		m_directory = std::filesystem::current_path() / "Themes";
	}

	void Themes_Manager::Initialize()
	{
		std::scoped_lock lock(m_mutex);
		std::error_code error;
		std::filesystem::create_directories(m_directory, error);
		RegisterBuiltInThemes();
	}

	void Themes_Manager::CaptureCurrent(const std::string& name)
	{
		std::scoped_lock lock(m_mutex);
		const auto key = NormalizeName(name);
		if (!key.empty())
			m_themes[key] = FromCurrentStyle(name);
	}

	bool Themes_Manager::Apply(const std::string& name, std::string& status)
	{
		std::scoped_lock lock(m_mutex);
		const auto key = NormalizeName(name);
		const auto iterator = m_themes.find(key);
		if (iterator == m_themes.end())
		{
			status = "Theme not found.";
			return false;
		}

		ApplyToStyle(iterator->second);
		m_active_theme = key;
		status = "Applied theme " + iterator->second.name + ".";
		return true;
	}

	bool Themes_Manager::Apply(const Theme_Configuration& theme, std::string& status)
	{
		std::scoped_lock lock(m_mutex);
		const auto key = NormalizeName(theme.name);
		if (key.empty())
		{
			status = "Theme name is invalid.";
			return false;
		}

		m_themes[key] = theme;
		ApplyToStyle(m_themes[key]);
		m_active_theme = key;
		status = "Applied theme " + theme.name + ".";
		return true;
	}

	bool Themes_Manager::Save(const std::string& name, std::string& status) const
	{
		std::scoped_lock lock(m_mutex);
		const auto key = NormalizeName(name);
		const auto iterator = m_themes.find(key);
		if (iterator == m_themes.end())
		{
			status = "Theme not found.";
			return false;
		}

		nlohmann::json json;
		json["name"] = iterator->second.name;
		json["accent"] = {iterator->second.accent.x, iterator->second.accent.y, iterator->second.accent.z, iterator->second.accent.w};
		json["rounding"] = {
			{"window", iterator->second.window_rounding},
			{"child", iterator->second.child_rounding},
			{"frame", iterator->second.frame_rounding},
			{"popup", iterator->second.popup_rounding},
			{"scrollbar", iterator->second.scrollbar_rounding},
			{"grab", iterator->second.grab_rounding}
		};

		json["colors"] = nlohmann::json::array();
		for (const auto& color : iterator->second.colors)
			json["colors"].push_back({color.x, color.y, color.z, color.w});

		std::error_code error;
		std::filesystem::create_directories(m_directory, error);
		if (error)
		{
			status = "Failed to create the Themes directory: " + error.message();
			return false;
		}

		const auto destination = m_directory / (key + ".json");
		const auto temporary = destination.string() + ".tmp";
		std::ofstream output(temporary, std::ios::out | std::ios::trunc);
		if (!output)
		{
			status = "Failed to open the theme file for writing.";
			return false;
		}

		output << std::setw(4) << json;
		output.close();
		std::filesystem::remove(destination, error);
		error.clear();
		std::filesystem::rename(temporary, destination, error);
		if (error)
		{
			status = "Failed to finalize the theme file: " + error.message();
			return false;
		}

		status = "Saved theme " + iterator->second.name + ".";
		return true;
	}

	bool Themes_Manager::Load(const std::filesystem::path& path, std::string& status)
	{
		std::scoped_lock lock(m_mutex);
		std::ifstream input(path);
		if (!input)
		{
			status = "Failed to open the theme file.";
			return false;
		}

		try
		{
			nlohmann::json json;
			input >> json;

			Theme_Configuration theme{};
			theme.name = json.value("name", path.stem().string());
			const auto accent = json.at("accent");
			theme.accent = ImVec4(accent.at(0), accent.at(1), accent.at(2), accent.at(3));
			const auto rounding = json.at("rounding");
			theme.window_rounding = rounding.value("window", 8.0f);
			theme.child_rounding = rounding.value("child", 7.0f);
			theme.frame_rounding = rounding.value("frame", 5.0f);
			theme.popup_rounding = rounding.value("popup", 7.0f);
			theme.scrollbar_rounding = rounding.value("scrollbar", 9.0f);
			theme.grab_rounding = rounding.value("grab", 5.0f);

			const auto colors = json.at("colors");
			if (!colors.is_array() || colors.size() != ImGuiCol_COUNT)
				throw std::runtime_error("Theme color count does not match this ImGui version.");

			for (std::size_t index = 0; index < theme.colors.size(); ++index)
			{
				const auto& color = colors.at(index);
				theme.colors[index] = ImVec4(color.at(0), color.at(1), color.at(2), color.at(3));
			}

			const auto key = NormalizeName(theme.name);
			if (key.empty())
				throw std::runtime_error("Theme name is invalid.");

			m_themes[key] = theme;
			ApplyToStyle(m_themes[key]);
			m_active_theme = key;
			status = "Loaded theme " + theme.name + ".";
			return true;
		}
		catch (const std::exception& exception)
		{
			status = std::string("Failed to load theme: ") + exception.what();
			return false;
		}
	}

	bool Themes_Manager::Remove(const std::string& name, std::string& status)
	{
		std::scoped_lock lock(m_mutex);
		const auto key = NormalizeName(name);
		if (key == "default" || key == "midnight" || key == "crimson")
		{
			status = "Built-in themes cannot be removed.";
			return false;
		}

		if (m_themes.erase(key) == 0)
		{
			status = "Theme not found.";
			return false;
		}

		if (m_active_theme == key)
			ResetToDefault(status);
		else
			status = "Removed theme.";
		return true;
	}

	void Themes_Manager::ResetToDefault(std::string& status)
	{
		Apply("default", status);
	}

	Theme_Configuration* Themes_Manager::Get(const std::string& name) noexcept
	{
		std::scoped_lock lock(m_mutex);
		const auto iterator = m_themes.find(NormalizeName(name));
		return iterator == m_themes.end() ? nullptr : &iterator->second;
	}

	const Theme_Configuration* Themes_Manager::Get(const std::string& name) const noexcept
	{
		std::scoped_lock lock(m_mutex);
		const auto iterator = m_themes.find(NormalizeName(name));
		return iterator == m_themes.end() ? nullptr : &iterator->second;
	}

	std::vector<std::string> Themes_Manager::Names() const
	{
		std::scoped_lock lock(m_mutex);
		std::vector<std::string> names;
		names.reserve(m_themes.size());
		for (const auto& [key, theme] : m_themes)
			names.push_back(theme.name);
		std::sort(names.begin(), names.end());
		return names;
	}

	const std::string& Themes_Manager::ActiveTheme() const noexcept
	{
		return m_active_theme;
	}

	const std::filesystem::path& Themes_Manager::Directory() const noexcept
	{
		return m_directory;
	}

	void Themes_Manager::RegisterBuiltInThemes()
	{
		if (!ImGui::GetCurrentContext())
			return;

		ImGui::StyleColorsDark();
		auto default_theme = FromCurrentStyle("Default");
		default_theme.accent = ImVec4(0.22f, 0.48f, 0.95f, 1.0f);
		m_themes["default"] = default_theme;

		auto midnight = default_theme;
		midnight.name = "Midnight";
		midnight.accent = ImVec4(0.36f, 0.58f, 1.0f, 1.0f);
		midnight.colors[ImGuiCol_WindowBg] = ImVec4(0.035f, 0.045f, 0.075f, 1.0f);
		midnight.colors[ImGuiCol_ChildBg] = ImVec4(0.045f, 0.055f, 0.09f, 1.0f);
		midnight.colors[ImGuiCol_Button] = ImVec4(0.12f, 0.20f, 0.36f, 1.0f);
		midnight.colors[ImGuiCol_ButtonHovered] = ImVec4(0.18f, 0.31f, 0.54f, 1.0f);
		m_themes["midnight"] = midnight;

		auto crimson = default_theme;
		crimson.name = "Crimson";
		crimson.accent = ImVec4(0.90f, 0.16f, 0.22f, 1.0f);
		crimson.colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.025f, 0.03f, 1.0f);
		crimson.colors[ImGuiCol_Button] = ImVec4(0.38f, 0.06f, 0.09f, 1.0f);
		crimson.colors[ImGuiCol_ButtonHovered] = ImVec4(0.58f, 0.08f, 0.13f, 1.0f);
		m_themes["crimson"] = crimson;
	}

	std::string Themes_Manager::NormalizeName(std::string name)
	{
		name.erase(std::remove_if(name.begin(), name.end(), [](unsigned char character)
		{
			return !(std::isalnum(character) || character == '-' || character == '_' || character == ' ');
		}), name.end());
		std::transform(name.begin(), name.end(), name.begin(), [](unsigned char character)
		{
			return character == ' ' ? '_' : static_cast<char>(std::tolower(character));
		});
		return name;
	}

	Theme_Configuration Themes_Manager::FromCurrentStyle(const std::string& name)
	{
		Theme_Configuration theme{};
		theme.name = name;
		if (!ImGui::GetCurrentContext())
			return theme;

		const auto& style = ImGui::GetStyle();
		theme.window_rounding = style.WindowRounding;
		theme.child_rounding = style.ChildRounding;
		theme.frame_rounding = style.FrameRounding;
		theme.popup_rounding = style.PopupRounding;
		theme.scrollbar_rounding = style.ScrollbarRounding;
		theme.grab_rounding = style.GrabRounding;
		std::copy(std::begin(style.Colors), std::end(style.Colors), theme.colors.begin());
		return theme;
	}

	void Themes_Manager::ApplyToStyle(const Theme_Configuration& theme)
	{
		if (!ImGui::GetCurrentContext())
			return;

		auto& style = ImGui::GetStyle();
		style.WindowRounding = theme.window_rounding;
		style.ChildRounding = theme.child_rounding;
		style.FrameRounding = theme.frame_rounding;
		style.PopupRounding = theme.popup_rounding;
		style.ScrollbarRounding = theme.scrollbar_rounding;
		style.GrabRounding = theme.grab_rounding;
		std::copy(theme.colors.begin(), theme.colors.end(), std::begin(style.Colors));

		style.Colors[ImGuiCol_CheckMark] = theme.accent;
		style.Colors[ImGuiCol_SliderGrab] = theme.accent;
		style.Colors[ImGuiCol_SliderGrabActive] = theme.accent;
		style.Colors[ImGuiCol_HeaderActive] = theme.accent;
	}
}
