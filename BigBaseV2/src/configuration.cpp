#include "configuration.hpp"

#include "logger.hpp"
#include "menu/pages/lua.hpp"
#include "menu/runtime.hpp"

#include <nlohmann/json.hpp>

namespace big
{
	using json = nlohmann::json;

	configuration_manager::configuration_manager() :
		m_directory(std::filesystem::current_path() / "Configurations")
	{
	}

	bool configuration_manager::initialize(std::string& status)
	{
		std::error_code error;
		std::filesystem::create_directories(m_directory, error);
		if (error)
		{
			status = "Failed to create configuration directory: " + error.message();
			return false;
		}

		if (std::filesystem::exists(profile_path(m_active_profile), error))
			return load(m_active_profile, status);

		reset_defaults();
		return save(m_active_profile, status);
	}

	bool configuration_manager::load(const std::string& profile, std::string& status)
	{
		const auto clean_profile = sanitize_profile(profile);
		if (clean_profile.empty())
		{
			status = "Configuration profile name is invalid.";
			return false;
		}

		std::ifstream stream(profile_path(clean_profile));
		if (!stream)
		{
			status = "Configuration profile does not exist.";
			return false;
		}

		try
		{
			json document;
			stream >> document;

			auto values = runtime_configuration{};
			const auto& runtime = document.value("runtime", json::object());
			const auto& lua = document.value("lua", json::object());
			const auto& input = document.value("input", json::object());
			const auto& theme = document.value("theme", json::object());

			values.disable_game_controls = runtime.value("disable_game_controls", values.disable_game_controls);
			values.autosave = runtime.value("autosave", values.autosave);
			values.lua_sandbox = lua.value("sandbox", values.lua_sandbox);
			values.lua_auto_reload = lua.value("auto_reload", values.lua_auto_reload);
			values.menu_key = input.value("menu_key", values.menu_key);
			values.unload_key = input.value("unload_key", values.unload_key);
			values.window_rounding = theme.value("window_rounding", values.window_rounding);
			values.frame_rounding = theme.value("frame_rounding", values.frame_rounding);

			if (theme.contains("accent") && theme["accent"].is_array() && theme["accent"].size() == 4)
			{
				for (std::size_t index = 0; index < 4; ++index)
					values.accent[index] = theme["accent"][index].get<float>();
			}

			values.window_rounding = std::clamp(values.window_rounding, 0.0f, 20.0f);
			values.frame_rounding = std::clamp(values.frame_rounding, 0.0f, 20.0f);
			for (auto& component : values.accent)
				component = std::clamp(component, 0.0f, 1.0f);

			m_values = values;
			m_active_profile = clean_profile;
			apply_runtime();
			status = "Loaded configuration: " + clean_profile;
			LOG_INFO("{}", status);
			return true;
		}
		catch (const std::exception& exception)
		{
			status = "Failed to parse configuration: " + std::string(exception.what());
			LOG_ERROR("{}", status);
			return false;
		}
	}

	bool configuration_manager::save(const std::string& profile, std::string& status) const
	{
		const auto clean_profile = sanitize_profile(profile);
		if (clean_profile.empty())
		{
			status = "Configuration profile name is invalid.";
			return false;
		}

		std::error_code error;
		std::filesystem::create_directories(m_directory, error);
		if (error)
		{
			status = "Failed to create configuration directory: " + error.message();
			return false;
		}

		json document = {
			{"version", 1},
			{"runtime", {
				{"disable_game_controls", m_values.disable_game_controls},
				{"autosave", m_values.autosave}
			}},
			{"lua", {
				{"sandbox", m_values.lua_sandbox},
				{"auto_reload", m_values.lua_auto_reload}
			}},
			{"input", {
				{"menu_key", m_values.menu_key},
				{"unload_key", m_values.unload_key}
			}},
			{"theme", {
				{"accent", {m_values.accent[0], m_values.accent[1], m_values.accent[2], m_values.accent[3]}},
				{"window_rounding", m_values.window_rounding},
				{"frame_rounding", m_values.frame_rounding}
			}}
		};

		const auto target = profile_path(clean_profile);
		const auto temporary = target.string() + ".tmp";
		std::ofstream stream(temporary, std::ios::trunc);
		if (!stream)
		{
			status = "Failed to open configuration for writing.";
			return false;
		}

		stream << document.dump(4) << '\n';
		stream.close();
		if (!stream)
		{
			status = "Failed while writing configuration.";
			std::filesystem::remove(temporary, error);
			return false;
		}

		std::filesystem::remove(target, error);
		error.clear();
		std::filesystem::rename(temporary, target, error);
		if (error)
		{
			status = "Failed to finalize configuration: " + error.message();
			std::filesystem::remove(temporary, error);
			return false;
		}

		status = "Saved configuration: " + clean_profile;
		LOG_INFO("{}", status);
		return true;
	}

	bool configuration_manager::remove(const std::string& profile, std::string& status)
	{
		const auto clean_profile = sanitize_profile(profile);
		if (clean_profile.empty() || clean_profile == "default")
		{
			status = "The default profile cannot be deleted.";
			return false;
		}

		std::error_code error;
		if (!std::filesystem::remove(profile_path(clean_profile), error))
		{
			status = error ? "Failed to delete configuration: " + error.message() : "Configuration profile does not exist.";
			return false;
		}

		if (m_active_profile == clean_profile)
		{
			m_active_profile = "default";
			load(m_active_profile, status);
		}
		else
		{
			status = "Deleted configuration: " + clean_profile;
		}
		return true;
	}

	void configuration_manager::reset_defaults() noexcept
	{
		m_values = runtime_configuration{};
		apply_runtime();
	}

	void configuration_manager::apply_runtime() const noexcept
	{
		menu_runtime::disable_game_controls.store(m_values.disable_game_controls, std::memory_order_relaxed);
		menu_pages::g_lua_page.sandbox_scripts = m_values.lua_sandbox;
		menu_pages::g_lua_page.auto_reload = m_values.lua_auto_reload;
	}

	std::vector<std::string> configuration_manager::profiles() const
	{
		std::vector<std::string> result;
		std::error_code error;
		for (const auto& entry : std::filesystem::directory_iterator(m_directory, error))
		{
			if (error)
				break;
			if (entry.is_regular_file() && entry.path().extension() == ".json")
				result.push_back(entry.path().stem().string());
		}
		std::sort(result.begin(), result.end());
		return result;
	}

	const std::filesystem::path& configuration_manager::directory() const noexcept
	{
		return m_directory;
	}

	const std::string& configuration_manager::active_profile() const noexcept
	{
		return m_active_profile;
	}

	runtime_configuration& configuration_manager::values() noexcept
	{
		return m_values;
	}

	const runtime_configuration& configuration_manager::values() const noexcept
	{
		return m_values;
	}

	std::string configuration_manager::sanitize_profile(std::string profile)
	{
		profile.erase(std::remove_if(profile.begin(), profile.end(), [](unsigned char character)
		{
			return !(std::isalnum(character) || character == '-' || character == '_');
		}), profile.end());
		if (profile.size() > 48)
			profile.resize(48);
		return profile;
	}

	std::filesystem::path configuration_manager::profile_path(const std::string& profile) const
	{
		return m_directory / (sanitize_profile(profile) + ".json");
	}
}
