#pragma once

#include "gta/natives.hpp"

#include <cstddef>
#include <cstdint>

namespace rage
{
	// Minimal GTA V Enhanced scrProgram layout used by InitNativeTables.
	// The full class contains script bytecode and metadata that BigBaseV2 does
	// not need when asking the game to populate native entrypoints.
	struct scrProgram final
	{
		std::byte m_pad_0000[0x2C];
		std::uint32_t m_native_count;               // 0x2C
		std::byte m_pad_0030[0x10];
		scrNativeHandler* m_native_entrypoints;     // 0x40
		std::byte m_pad_0048[0x38];
	};

	static_assert(offsetof(scrProgram, m_native_count) == 0x2C);
	static_assert(offsetof(scrProgram, m_native_entrypoints) == 0x40);
	static_assert(sizeof(scrProgram) == 0x80);
}
