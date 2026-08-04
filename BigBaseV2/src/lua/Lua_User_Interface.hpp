#pragma once

#include "common.hpp"

namespace big::lua
{
	class Lua_User_Interface final
	{
	public:
		static Lua_User_Interface& Instance();

		void Draw();

	private:
		int m_selected_index{-1};
		bool m_sandbox{true};
		bool m_auto_reload{false};
		std::string m_status{"Sol2 Lua runtime ready."};
	};
}
