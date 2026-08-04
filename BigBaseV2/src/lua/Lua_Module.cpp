#include "Lua_Module.hpp"

#include "lua_manager.hpp"

namespace big::lua
{
	Lua_Module& Lua_Module::Instance()
	{
		static Lua_Module instance;
		return instance;
	}

	void Lua_Module::Initialize()
	{
		if (m_initialized)
			return;

		if (!g_lua_manager)
			g_lua_manager = std::make_unique<lua_manager>();

		m_initialized = true;
	}

	void Lua_Module::Shutdown()
	{
		if (!m_initialized)
			return;

		if (g_lua_manager)
			g_lua_manager->unload_all();

		g_lua_manager.reset();
		m_initialized = false;
	}

	void Lua_Module::Tick()
	{
		if (m_initialized && g_lua_manager)
			g_lua_manager->tick();
	}

	bool Lua_Module::IsInitialized() const noexcept
	{
		return m_initialized;
	}
}
