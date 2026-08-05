#pragma once
#include <intrin.h>
#include "fwddec.hpp"

namespace rage
{
	class tlsContext
	{
	public:
		char m_padding[0x7A0];                 // 0x000
		scrThread* m_script_thread;             // 0x7A0
		bool m_is_script_thread_active;         // 0x7A8
		char m_tail_padding[0x7];               // 0x7A9

		static tlsContext* get()
		{
			return *reinterpret_cast<tlsContext**>(__readgsqword(0x58));
		}
	};

	static_assert(offsetof(tlsContext, m_script_thread) == 0x7A0);
	static_assert(offsetof(tlsContext, m_is_script_thread_active) == 0x7A8);
	static_assert(sizeof(tlsContext) == 0x7B0);
}
