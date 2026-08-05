#pragma once

#include "gta/joaat.hpp"
#include "gta/natives.hpp"

#include <cstddef>
#include <cstdint>

namespace rage
{
	inline constexpr std::size_t scr_program_capacity = 176;
	inline constexpr std::uint32_t scr_program_page_size = 0x4000;

	// Partial GTA V Enhanced scrProgram view. The first 0x10 bytes are the
	// pgBase vtable/page-map region. Keeping them opaque allows creation of the
	// temporary native-table initializer used by Enhanced without pretending
	// to implement pgBase's abstract virtual interface.
	struct scrProgram final
	{
		std::byte m_pg_base[0x10]{};
		std::uint8_t** m_code_blocks{};           // 0x10
		joaat_t m_hash{};                         // 0x18
		std::uint32_t m_code_size{};              // 0x1C
		std::uint32_t m_arg_count{};              // 0x20
		std::uint32_t m_local_count{};            // 0x24
		std::uint32_t m_global_count{};           // 0x28
		std::uint32_t m_native_count{};           // 0x2C
		void* m_local_data{};                     // 0x30
		void** m_global_data{};                   // 0x38
		scrNativeHandler* m_native_entrypoints{}; // 0x40
		std::uint32_t m_proc_count{};             // 0x48
		std::byte m_padding_4C[0x04]{};
		const char** m_proc_names{};              // 0x50
		joaat_t m_name_hash{};                    // 0x58
		std::uint32_t m_ref_count{};              // 0x5C
		const char* m_name{};                     // 0x60
		const char** m_strings_data{};            // 0x68
		std::uint32_t m_strings_count{};          // 0x70
		std::byte m_breakpoints[0x0C]{};

		[[nodiscard]] bool valid() const noexcept
		{
			return m_code_size != 0 && m_code_blocks != nullptr;
		}

		[[nodiscard]] std::uint32_t code_page_count() const noexcept
		{
			return (m_code_size + (scr_program_page_size - 1)) >> 14;
		}

		[[nodiscard]] std::uint32_t code_page_size(std::uint32_t page) const noexcept
		{
			const auto page_count = code_page_count();
			if (page >= page_count)
				return 0;
			if (page + 1 != page_count)
				return scr_program_page_size;

			const auto tail = m_code_size & (scr_program_page_size - 1);
			return tail != 0 ? tail : scr_program_page_size;
		}

		[[nodiscard]] std::uint32_t full_code_size() const noexcept
		{
			return m_code_size;
		}

		[[nodiscard]] std::uint8_t* code_page(std::uint32_t page) noexcept
		{
			return m_code_blocks && page < code_page_count() ? m_code_blocks[page] : nullptr;
		}

		[[nodiscard]] const std::uint8_t* code_page(std::uint32_t page) const noexcept
		{
			return m_code_blocks && page < code_page_count() ? m_code_blocks[page] : nullptr;
		}

		[[nodiscard]] std::uint8_t* code_address(std::uint32_t index) noexcept
		{
			if (index >= m_code_size)
				return nullptr;
			auto* page = code_page(index >> 14);
			return page ? &page[index & (scr_program_page_size - 1)] : nullptr;
		}

		[[nodiscard]] const std::uint8_t* code_address(std::uint32_t index) const noexcept
		{
			if (index >= m_code_size)
				return nullptr;
			const auto* page = code_page(index >> 14);
			return page ? &page[index & (scr_program_page_size - 1)] : nullptr;
		}

		[[nodiscard]] const char* string_address(std::uint32_t index) const noexcept
		{
			if (!m_strings_data || index >= m_strings_count)
				return nullptr;
			auto* page = m_strings_data[index >> 14];
			return page ? &page[index & (scr_program_page_size - 1)] : nullptr;
		}

		[[nodiscard]] scrNativeHandler* native_entrypoint_slot(scrNativeHandler entrypoint) noexcept
		{
			if (!m_native_entrypoints || !entrypoint)
				return nullptr;
			for (std::uint32_t index = 0; index < m_native_count; ++index)
			{
				if (m_native_entrypoints[index] == entrypoint)
					return m_native_entrypoints + index;
			}
			return nullptr;
		}
	};

	static_assert(offsetof(scrProgram, m_code_blocks) == 0x10);
	static_assert(offsetof(scrProgram, m_native_count) == 0x2C);
	static_assert(offsetof(scrProgram, m_native_entrypoints) == 0x40);
	static_assert(offsetof(scrProgram, m_name_hash) == 0x58);
	static_assert(offsetof(scrProgram, m_name) == 0x60);
	static_assert(sizeof(scrProgram) == 0x80);
}
