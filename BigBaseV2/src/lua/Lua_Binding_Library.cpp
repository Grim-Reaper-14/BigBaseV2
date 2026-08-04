#include "Lua_Binding_Library.hpp"

namespace big::lua
{
	Lua_Binding_Library& Lua_Binding_Library::Instance()
	{
		static Lua_Binding_Library instance;
		return instance;
	}

	void Lua_Binding_Library::Add(std::unique_ptr<ILuaBinding> binding)
	{
		if (!binding)
			return;

		std::scoped_lock lock(m_mutex);
		m_bindings.push_back(std::move(binding));
	}

	void Lua_Binding_Library::RegisterAll(sol::state_view lua)
	{
		std::scoped_lock lock(m_mutex);
		sol::table root = lua["bigbase"].get_or_create<sol::table>();
		for (auto& binding : m_bindings)
			binding->Bind(lua, root);
	}

	void Lua_Binding_Library::Clear()
	{
		std::scoped_lock lock(m_mutex);
		m_bindings.clear();
	}

	std::vector<std::string> Lua_Binding_Library::Names() const
	{
		std::scoped_lock lock(m_mutex);
		std::vector<std::string> names;
		names.reserve(m_bindings.size());
		for (const auto& binding : m_bindings)
			names.emplace_back(binding->Name());
		std::sort(names.begin(), names.end());
		return names;
	}
}
