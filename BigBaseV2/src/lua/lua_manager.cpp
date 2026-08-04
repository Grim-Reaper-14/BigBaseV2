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

	std::filesystem::path lua_manager::normalize_path(const std::filesystem::path& path)
	{
		std::error_code error;
		auto normalized = std::filesystem::weakly_canonical(path, error);
		if (error)
			normalized = std::filesystem::absolute(path, error);
		return error ? path.lexically_normal() : normalized.lexically_normal();
	}

	bool lua_manager::load_script(const std::filesystem::path& path, bool sandbox, std::string& error)
	{
		std::scoped_lock lock(m_mutex);
		const auto normalized_path = normalize_path(path);

		if (normalized_path.extension() != ".lua")
		{
			error = "Only .lua files can be loaded.";
			return false;
		}

		std::error_code filesystem_error;
		if (!std::filesystem::is_regular_file(normalized_path, filesystem_error) || filesystem_error)
		{
			error = filesystem_error ? filesystem_error.message() : "Script file does not exist.";
			return false;
		}

		for (auto iterator = m_scripts.begin(); iterator != m_scripts.end(); ++iterator)
		{
			if ((*iterator)->path != normalized_path)
				continue;

			call_unload(*(*iterator));
			m_scripts.erase(iterator);
			break;
		}

		auto environment = create_environment(sandbox);
		sol::load_result loaded = m_lua.load_file(normalized_path.string());
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
		instance->path = normalized_path;
		instance->sandboxed = sandbox;
		instance->environment = std::move(environment);
		instance->last_write_time = std::filesystem::last_write_time(normalized_path, filesystem_error);
		if (filesystem_error)
			instance->last_write_time = {};

		sol::object on_tick = instance->environment["on_tick"];
		if (on_tick.is<sol::protected_function>())
			instance->on_tick = on_tick.as<sol::protected_function>();

		sol::object on_unload = instance->environment["on_unload"];
		if (on_unload.is<sol::protected_function>())
			instance->on_unload = on_unload.as<sol::protected_function>();

		LOG_INFO("Loaded Lua script: {}{}", normalized_path.filename().string(), sandbox ? " [sandboxed]" : "");
		m_scripts.push_back(std::move(instance));
		error.clear();
		return true;
	}

	bool lua_manager::reload_script(const std::filesystem::path& path, std::string& error)
	{
		std::scoped_lock lock(m_mutex);
		const auto normalized_path = normalize_path(path);
		const auto iterator = std::find_if(m_scripts.begin(), m_scripts.end(), [&normalized_path](const auto& instance)
		{
			return instance->path == normalized_path;
		});

		if (iterator == m_scripts.end())
		{
			error = "The selected script is not loaded.";
			return false;
		}

		const bool sandbox = (*iterator)->sandboxed;
		return load_script(normalized_path, sandbox, error);
	}

	bool lua_manager::unload_script(const std::filesystem::path& path)
	{
		std::scoped_lock lock(m_mutex);
		const auto normalized_path = normalize_path(path);

		for (auto iterator = m_scripts.begin(); iterator != m_scripts.end(); ++iterator)
		{
			if ((*iterator)->path != normalized_path)
				continue;

			call_unload(*(*iterator));
			LOG_INFO("Unloaded Lua script: {}", normalized_path.filename().string());
			m_scripts.erase(iterator);
			return true;
		}

		return false;
	}

	void lua_manager::unload_all()
	{
		std::scoped_lock lock(m_mutex);
		for (auto& instance : m_scripts)
			call_unload(*instance);
		m_scripts.clear();
	}

	void lua_manager::tick()
	{
		std::scoped_lock lock(m_mutex);

		for (auto& instance : m_scripts)
		{
			if (instance->faulted || !instance->on_tick.valid())
				continue;

			sol::protected_function_result result = instance->on_tick();
			if (result.valid())
				continue;

			sol::error lua_error = result;
			instance->faulted = true;
			instance->last_error = lua_error.what();
			instance->on_tick = sol::protected_function{};
			LOG_ERROR("Lua on_tick error in {}: {}", instance->path.filename().string(), instance->last_error);
		}
	}

	std::size_t lua_manager::reload_changed_scripts()
	{
		std::scoped_lock lock(m_mutex);
		std::vector<std::pair<std::filesystem::path, bool>> changed;

		for (const auto& instance : m_scripts)
		{
			std::error_code error;
			const auto write_time = std::filesystem::last_write_time(instance->path, error);
			if (!error && write_time != instance->last_write_time)
				changed.emplace_back(instance->path, instance->sandboxed);
		}

		std::size_t reloaded{};
		for (const auto& [path, sandbox] : changed)
		{
			std::string error;
			if (load_script(path, sandbox, error))
			{
				++reloaded;
				LOG_INFO("Auto reloaded Lua script: {}", path.filename().string());
			}
			else
			{
				LOG_ERROR("Auto reload failed for {}: {}", path.filename().string(), error);
			}
		}

		return reloaded;
	}

	bool lua_manager::is_loaded(const std::filesystem::path& path) const
	{
		std::scoped_lock lock(m_mutex);
		const auto normalized_path = normalize_path(path);
		return std::any_of(m_scripts.begin(), m_scripts.end(), [&normalized_path](const auto& instance)
		{
			return instance->path == normalized_path;
		});
	}

	std::size_t lua_manager::loaded_count() const noexcept
	{
		std::scoped_lock lock(m_mutex);
		return m_scripts.size();
	}

	std::vector<lua_manager::script_status> lua_manager::statuses() const
	{
		std::scoped_lock lock(m_mutex);
		std::vector<script_status> result;
		result.reserve(m_scripts.size());
		for (const auto& instance : m_scripts)
			result.push_back({instance->path, instance->last_error, instance->sandboxed, instance->faulted});
		return result;
	}

	void lua_manager::script_func()
	{
		while (g_running)
		{
			if (g_lua_manager)
				g_lua_manager->tick();

			if (auto* current = script::get_current())
				current->yield();
			else
				break;
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
		api.set_function("log_info", [](const std::string& message) { LOG_INFO("[Lua] {}", message); });
		api.set_function("log_warning", [](const std::string& message) { LOG_TRACE("[Lua warning] {}", message); });
		api.set_function("log_error", [](const std::string& message) { LOG_ERROR("[Lua] {}", message); });
		api["version"] = "BigBaseV2-Sol2-Enhanced";
	}

	void lua_manager::call_unload(script_instance& instance) noexcept
	{
		if (!instance.on_unload.valid())
			return;

		sol::protected_function_result result = instance.on_unload();
		if (!result.valid())
		{
			sol::error lua_error = result;
			LOG_ERROR("Lua on_unload error in {}: {}", instance.path.filename().string(), lua_error.what());
		}

		instance.on_tick = sol::protected_function{};
		instance.on_unload = sol::protected_function{};
	}
}
