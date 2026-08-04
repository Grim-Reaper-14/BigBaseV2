#pragma once

#include "common.hpp"

namespace big
{
	struct runtime_configuration final
	{
		bool disable_game_controls{true};
		bool lua_sandbox{true};
		bool lua_auto_reload{false};
		bool autosave{true};
		int menu_key{VK_INSERT};
		int unload_key{VK_END};
		float accent[4]{0.22f, 0.48f, 0.95f, 1.0f};
		float window_rounding{8.0f};
		float frame_rounding{5.0f};
	};

	class configuration_manager final
	{
	public:
		configuration_manager();

		configuration_manager(const configuration_manager&) = delete;
		configuration_manager(configuration_manager&&) = delete;
		configuration_manager& operator=(const configuration_manager&) = delete;
		configuration_manager& operator=(configuration_manager&&) = delete;

		bool initialize(std::string& status);
		bool load(const std::string& profile, std::string& status);
		bool save(const std::string& profile, std::string& status) const;
		bool remove(const std::string& profile, std::string& status);
		void reset_defaults() noexcept;
		void apply_runtime() const noexcept;

		[[nodiscard]] std::vector<std::string> profiles() const;
		[[nodiscard]] const std::filesystem::path& directory() const noexcept;
		[[nodiscard]] const std::string& active_profile() const noexcept;
		[[nodiscard]] runtime_configuration& values() noexcept;
		[[nodiscard]] const runtime_configuration& values() const noexcept;

	private:
		[[nodiscard]] static std::string sanitize_profile(std::string profile);
		[[nodiscard]] std::filesystem::path profile_path(const std::string& profile) const;

		std::filesystem::path m_directory;
		std::string m_active_profile{"default"};
		runtime_configuration m_values{};
	};

	inline configuration_manager g_configuration;
}
