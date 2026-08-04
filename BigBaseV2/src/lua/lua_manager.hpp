#pragma once

#include "common.hpp"

#include <sol/sol.hpp>

namespace big
{
	class lua_manager final
	{
	public:
		struct script_instance final
		{
			std::filesystem::path path;
			std::filesystem::file_time_type last_write_time{};
			sol::environment environment;
			sol::protected_function on_tick;
			sol::protected_function on_unload;
			std::string last_error;
			bool sandboxed{true};
			bool faulted{};
		};

		struct script_status final
		{
			std::filesystem::path path;
			std::string last_error;
			bool sandboxed{};
			bool faulted{};
		};

		lua_manager();
		~lua_manager();

		lua_manager(const lua_manager&) = delete;
		lua_manager(lua_manager&&) = delete;
		lua_manager& operator=(const lua_manager&) = delete;
		lua_manager& operator=(lua_manager&&) = delete;

		bool load_script(const std::filesystem::path& path, bool sandbox, std::string& error);
		bool reload_script(const std::filesystem::path& path, std::string& error);
		bool unload_script(const std::filesystem::path& path);
		void unload_all();
		void tick();
		std::size_t reload_changed_scripts();

		[[nodiscard]] bool is_loaded(const std::filesystem::path& path) const;
		[[nodiscard]] std::size_t loaded_count() const noexcept;
		[[nodiscard]] std::vector<script_status> statuses() const;

		static void script_func();

	private:
		[[nodiscard]] static std::filesystem::path normalize_path(const std::filesystem::path& path);
		[[nodiscard]] sol::environment create_environment(bool sandbox);
		void register_api();
		void call_unload(script_instance& script) noexcept;

		mutable std::recursive_mutex m_mutex;
		sol::state m_lua;
		std::vector<std::unique_ptr<script_instance>> m_scripts;
	};

	inline std::unique_ptr<lua_manager> g_lua_manager;
}
