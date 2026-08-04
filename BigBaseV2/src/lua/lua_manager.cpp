#include "lua_manager.hpp"

#include "logger.hpp"
#include "script.hpp"

namespace big
{
	lua_manager::lua_manager()
	{
		m_lua.open_libraries(
			sol::lib::base,
			sol::lib::package,
			sol::lib::coroutine,
			sol::lib::string,
			sol::lib::os,
			sol::lib::math,
			sol::lib::table,
			sol::lib::debug,
			sol::lib::io,
			sol::lib::utf8);

		register_api();
	}

	lua_manager::~lua_manager()
	{
		unload_all();
	}

	bool lua_manager::load_script(const std::filesystem::path& path, bool sandbox, std::string& error)
	{
		std::scoped_lock lock(m_mutex);

		if (path.extension() != ".lua")
		{
			error = "Only .lua files can be loaded.";
			return false;
		}

		if (!std::filesystem::is_regular_file(path))
		{
			error = "Script file does not exist.";
			return false;
		}

		for (auto iterator = m_scripts.begin(); iterator != m_scripts.end(); ++iterator)
		{
			if ((*iterator)->path == path)
			{
				call_unload(*(*iterator));
				m_scripts.erase(iterator);
				break;
			}
		}

		auto environment = create_environment(sandbox);
		sol::load_result loaded = m_lua.load_file(path.string());
		if (!loaded.valid())
		{
			sol::error lua_error = loaded;
			error = lua_error.what();
			return false;
		}

		sol::protected_function entry = loaded;
		sol::set_environment(environment, entry);

		sol::protected_function_result result = entry();
		if (!result.valid())
		{
			sol::error lua_error = result;
			error = lua_error.what();
			return false;
		}

		auto instance = std::make_unique<script_instance>();
		instance->path = path;
		instance->environment = std::move(environment);

		sol::object on_tick = instance->environment["on_tick"];
		if (on_tick.is<sol::protected_function>())
			instance->on_tick = on_tick.as<sol::protected_function>();

		sol::object on_unload = instance->environment["on_unload"];
		if (on_unload.is<sol::protected_function>())
			instance->on_unload = on_unload.as<sol::protected_function>();

		LOG_INFO("Loaded Lua script: {}", path.filename().string());
		m_scripts.push_back(std::move(instance));
		error.clear();
		return true;
	}

	bool lua_manager::unload_script(const std::filesystem::path& path)
	{
		std::scoped_lock lock(m_mutex);

		for (auto iterator = m_scripts.begin(); iterator != m_scripts.end(); ++iterator)
		{
			if ((*iterator)->path != path)
				continue;

			call_unload(*(*iterator));
			LOG_INFO("Unloaded Lua script: {}", path.filename().string());
			m_scripts.erase(iterator);
			return true;
		}

		return false;
	}

	void lua_manager::unload_all()
	{
		std::scoped_lock lock(m_mutex);

		for (auto& script_instance : m_scripts)
			call_unload(*script_instance);

		m_scripts.clear();
	}

	void lua_manager::tick()
	{
		std::scoped_lock lock(m_mutex);

		for (auto& script_instance : m_scripts)
		{
			if (!script_instance->on_tick.valid())
				continue;

			sol::protected_function_result result = script_instance->on_tick();
			if (!result.valid())
			{
				sol::error lua_error = result;
				LOG_ERROR("Lua on_tick error in {}: {}", script_instance->path.filename().string(), lua_error.what());
				script_instance->on_tick = sol::protected_function{};
			}
		}
	}

	bool lua_manager::is_loaded(const std::filesystem::path& path) const
	{
		std::scoped_lock lock(m_mutex);
		return std::any_of(m_scripts.begin(), m_scripts.end(), [&path](const auto& script_instance)
		{
			return script_instance->path == path;
		});
	}

	std::size_t lua_manager::loaded_count() const noexcept
	{
		std::scoped_lock lock(m_mutex);
		return m_scripts.size();
	}

	void lua_manager::script_func()
	{
		while (g_running)
		{
			if (g_lua_manager)
				g_lua_manager->tick();

			script::get_current()->yield();
		}
	}

	sol::environment lua_manager::create_environment(bool sandbox)
	{
		if (!sandbox)
			return sol::environment(m_lua, sol::create, m_lua.globals());

		sol::environment environment(m_lua, sol::create);

		constexpr const char* safe_globals[] =
		{
			"assert", "error", "ipairs", "next", "pairs", "pcall", "print",
			"rawequal", "rawget", "rawlen", "rawset", "select", "setmetatable",
			"tonumber", "tostring", "type", "xpcall", "_VERSION",
			"coroutine", "string", "table", "math", "utf8", "bigbase"
		};

		for (const char* name : safe_globals)
			environment[name] = m_lua[name];

		return environment;
	}

	void lua_manager::register_api()
	{
		sol::table api = m_lua.create_named_table("bigbase");
		api.set_function("log_info", [](const std::string& message)
		{
			LOG_INFO("[Lua] {}", message);
		});
		api.set_function("log_warning", [](const std::string& message)
		{
			LOG_TRACE("[Lua warning] {}", message);
		});
		api.set_function("log_error", [](const std::string& message)
		{
			LOG_ERROR("[Lua] {}", message);
		});
		api["version"] = "BigBaseV2-Sol2";
	}

	void lua_manager::call_unload(script_instance& script_instance) noexcept
	{
		if (!script_instance.on_unload.valid())
			return;

		sol::protected_function_result result = script_instance.on_unload();
		if (!result.valid())
		{
			sol::error lua_error = result;
			LOG_ERROR("Lua on_unload error in {}: {}", script_instance.path.filename().string(), lua_error.what());
		}
	}
}
