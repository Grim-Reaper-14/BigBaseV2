#pragma once

#include "gta/natives.hpp"

#include <cstddef>
#include <cstdint>

namespace rage
{
	// GTA V Enhanced scrProgram layout used for native initialization and
	// script bytecode calls. The first 0x10 bytes belong to pgBase.
	struct scrProgram final
	{
		std::byte m_pg_base[0x10]{};
		std::uint8_t** m_code_blocks{};          // 0x10
		std::uint32_t m_hash{};                  // 0x18
		std::uint32_t m_code_size{};             // 0x1C
		std::uint32_t m_arg_count{};             // 0x20
		std::uint32_t m_local_count{};           // 0x24
		std::uint32_t m_global_count{};          // 0x28
		std::uint32_t m_native_count{};          // 0x2C
		void* m_local_data{};                    // 0x30
		void** m_global_data{};                  // 0x38
		scrNativeHandler* m_native_entrypoints{}; // 0x40
		std::uint32_t m_proc_count{};            // 0x48
		std::byte m_pad_004C[4]{};
		const char** m_proc_names{};             // 0x50
		std::uint32_t m_name_hash{};             // 0x58
		std::uint32_t m_ref_count{};             // 0x5C
		const char* m_name{};                    // 0x60
		const char** m_strings_data{};           // 0x68
		std::uint32_t m_strings_count{};         // 0x70
		std::byte m_breakpoints[0x0C]{};

		[[nodiscard]] bool valid() const noexcept
		{
			return m_code_blocks && m_code_size != 0;
		}

		[[nodiscard]] std::uint32_t code_page_count() const noexcept
		{
			return (m_code_size + 0x3FFF) >> 14;
		}

		[[nodiscard]] std::uint8_t* code_address(std::uint32_t index) const noexcept
		{
			if (!m_code_blocks || index >= m_code_size)
				return nullptr;

			return &m_code_blocks[index >> 14][index & 0x3FFF];
		}
	};

	static_assert(offsetof(scrProgram, m_code_blocks) == 0x10);
	static_assert(offsetof(scrProgram, m_native_count) == 0x2C);
	static_assert(offsetof(scrProgram, m_native_entrypoints) == 0x40);
	static_assert(offsetof(scrProgram, m_name_hash) == 0x58);
	static_assert(sizeof(scrProgram) == 0x80);
}
