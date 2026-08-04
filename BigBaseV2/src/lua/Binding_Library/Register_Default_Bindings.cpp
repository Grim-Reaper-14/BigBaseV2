#include "Register_Default_Bindings.hpp"

#include "Core_Binding.hpp"
#include "Player_Binding.hpp"
#include "../Lua_Binding_Library.hpp"

namespace big::lua::bindings
{
	void Register_Default_Bindings()
	{
		auto& library = Lua_Binding_Library::Instance();
		library.Clear();
		library.Add(std::make_unique<Core_Binding>());
		library.Add(std::make_unique<Player_Binding>());
	}
}
