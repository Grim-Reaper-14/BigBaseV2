#pragma once

#include "fwddec.hpp"

#include <cstddef>
#include <cstdint>
#include <intrin.h>

namespace rage
{
	class tlsContext
	{
	public:
		std::byte m_padding[0x7A0]{};          // 0x000
		scrThread* m_script_thread{};          // 0x7A0
		bool m_is_script_thread_active{};      // 0x7A8
		std::byte m_tail_padding[0x07]{};      // 0x7A9

		[[nodiscard]] static tlsContext* get() noexcept
		{
			const auto tls_array = __readgsqword(0x58);
			if (!tls_array)
				return nullptr;

			return *reinterpret_cast<tlsContext**>(tls_array);
		}
	};

	static_assert(offsetof(tlsContext, m_script_thread) == 0x7A0);
	static_assert(offsetof(tlsContext, m_is_script_thread_active) == 0x7A8);
	static_assert(sizeof(tlsContext) == 0x7B0);
}
