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
			sol::environment environment;
			sol::protected_function on_tick;
		};

		lua_manager();
		~lua_manager();

		lua_manager(const lua_manager&) = delete;
		lua_manager(lua_manager&&) = delete;
		lua_manager& operator=(const lua_manager&) = delete;
		lua_manager& operator=(lua_manager&&) = delete;

		bool load_script(const std::filesystem::path& path, bool sandbox, std::string& error);
		bool unload_script(const std::filesystem::path& path);
		void unload_all();
		void tick();

		[[nodiscard]] bool is_loaded(const std::filesystem::path& path) const;
		[[nodiscard]] std::size_t loaded_count() const noexcept;

	private:
		sol::environment create_environment(bool sandbox);
		void register_api();

		sol::state m_lua;
		std::vector<std::unique_ptr<script_instance>> m_scripts;
	};

	inline std::unique_ptr<lua_manager> g_lua_manager;
}
