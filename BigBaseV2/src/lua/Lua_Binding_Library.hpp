#pragma once

#include "common.hpp"

#include <sol/sol.hpp>

namespace big::lua
{
	class ILuaBinding
	{
	public:
		virtual ~ILuaBinding() = default;
		virtual std::string_view Name() const noexcept = 0;
		virtual void Bind(sol::state_view lua, sol::table& root) = 0;
	};

	class Lua_Binding_Library final
	{
	public:
		static Lua_Binding_Library& Instance();

		void Add(std::unique_ptr<ILuaBinding> binding);
		void RegisterAll(sol::state_view lua);
		void Clear();

		[[nodiscard]] std::vector<std::string> Names() const;

	private:
		mutable std::mutex m_mutex;
		std::vector<std::unique_ptr<ILuaBinding>> m_bindings;
	};
}
