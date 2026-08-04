#include "Lua_Scripts_Manager.hpp"

#include "lua_manager.hpp"
#include "logger.hpp"

namespace big::lua
{
	Lua_Scripts_Manager& Lua_Scripts_Manager::Instance()
	{
		static Lua_Scripts_Manager instance;
		return instance;
	}

	void Lua_Scripts_Manager::Initialize(std::filesystem::path scripts_directory)
	{
		std::error_code error;
		m_directory = scripts_directory.empty()
			? std::filesystem::current_path(error) / "scripts"
			: std::move(scripts_directory);

		if (error)
			m_directory = "scripts";

		std::filesystem::create_directories(m_directory, error);
		if (error)
			LOG_ERROR("Failed to create Lua scripts directory {}: {}", m_directory.string(), error.message());

		m_directory = std::filesystem::weakly_canonical(m_directory, error);
		if (error)
			m_directory = m_directory.lexically_normal();

		Refresh();
	}

	void Lua_Scripts_Manager::Refresh()
	{
		m_scripts.clear();
		if (m_directory.empty())
			Initialize();

		std::error_code error;
		std::filesystem::directory_iterator iterator(m_directory, error);
		if (error)
		{
			LOG_ERROR("Failed to scan Lua scripts directory {}: {}", m_directory.string(), error.message());
			return;
		}

		for (const auto& entry : iterator)
		{
			if (!entry.is_regular_file(error) || error)
			{
				error.clear();
				continue;
			}

			if (entry.path().extension() != ".lua")
				continue;

			m_scripts.push_back({entry.path().lexically_normal(), g_lua_manager && g_lua_manager->is_loaded(entry.path())});
		}

		std::sort(m_scripts.begin(), m_scripts.end(), [](const auto& left, const auto& right)
		{
			return left.path.filename().string() < right.path.filename().string();
		});
	}

	bool Lua_Scripts_Manager::Load(std::size_t index, bool sandbox, std::string& status)
	{
		if (!g_lua_manager || index >= m_scripts.size())
		{
			status = "Lua manager is unavailable or the selection is invalid.";
			return false;
		}

		std::string error;
		const bool loaded = g_lua_manager->load_script(m_scripts[index].path, sandbox, error);
		status = loaded ? "Loaded " + m_scripts[index].path.filename().string() : "Load failed: " + error;
		Refresh();
		return loaded;
	}

	bool Lua_Scripts_Manager::Reload(std::size_t index, bool sandbox, std::string& status)
	{
		if (!g_lua_manager || index >= m_scripts.size())
		{
			status = "Lua manager is unavailable or the selection is invalid.";
			return false;
		}

		std::string error;
		bool reloaded{};
		if (g_lua_manager->is_loaded(m_scripts[index].path))
			reloaded = g_lua_manager->reload_script(m_scripts[index].path, error);
		else
			reloaded = g_lua_manager->load_script(m_scripts[index].path, sandbox, error);

		status = reloaded ? "Reloaded " + m_scripts[index].path.filename().string() : "Reload failed: " + error;
		Refresh();
		return reloaded;
	}

	bool Lua_Scripts_Manager::Unload(std::size_t index, std::string& status)
	{
		if (!g_lua_manager || index >= m_scripts.size())
		{
			status = "Lua manager is unavailable or the selection is invalid.";
			return false;
		}

		const auto name = m_scripts[index].path.filename().string();
		const bool unloaded = g_lua_manager->unload_script(m_scripts[index].path);
		status = unloaded ? "Unloaded " + name : "The selected script was not loaded.";
		Refresh();
		return unloaded;
	}

	void Lua_Scripts_Manager::UnloadAll()
	{
		if (g_lua_manager)
			g_lua_manager->unload_all();
		Refresh();
	}

	const std::filesystem::path& Lua_Scripts_Manager::Directory() const noexcept
	{
		return m_directory;
	}

	const std::vector<Lua_Script_Info>& Lua_Scripts_Manager::Scripts() const noexcept
	{
		return m_scripts;
	}
}
