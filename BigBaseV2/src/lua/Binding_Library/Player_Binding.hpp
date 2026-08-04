#pragma once

#include "../Lua_Binding_Library.hpp"

namespace big::lua::bindings
{
	class Player_Binding final : public ILuaBinding
	{
	public:
		std::string_view Name() const noexcept override;
		void Bind(sol::state_view lua, sol::table& root) override;
	};
}
