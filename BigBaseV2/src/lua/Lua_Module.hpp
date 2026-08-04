#pragma once

#include "common.hpp"

namespace big::lua
{
	class Lua_Module final
	{
	public:
		static Lua_Module& Instance();

		void Initialize();
		void Shutdown();
		void Tick();

		[[nodiscard]] bool IsInitialized() const noexcept;

	private:
		bool m_initialized{};
	};
}
