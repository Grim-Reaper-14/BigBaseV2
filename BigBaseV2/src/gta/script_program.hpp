#pragma once

// Compatibility header retained for older includes. The authoritative GTA V
// Enhanced program layout lives in scr_program.hpp.
#include "joaat.hpp"
#include "scr_program.hpp"

#include <cstddef>
#include <cstdint>

namespace rage
{
#pragma pack(push, 1)
	struct scrProgramTableEntry
	{
		scrProgram* m_program{};       // 0x00
		std::uint32_t m_padding{};     // 0x08
		joaat_t m_hash{};              // 0x0C
	};

	class scrProgramTable
	{
	public:
		[[nodiscard]] scrProgram* find_script(joaat_t hash) const noexcept
		{
			if (!m_data)
				return nullptr;

			for (std::uint32_t index = 0; index < m_size; ++index)
			{
				const auto& entry = m_data[index];
				if (entry.m_hash == hash)
					return entry.m_program;
			}

			return nullptr;
		}

		[[nodiscard]] scrProgramTableEntry* begin() noexcept
		{
			return m_data;
		}

		[[nodiscard]] scrProgramTableEntry* end() noexcept
		{
			return m_data ? m_data + m_size : nullptr;
		}

		[[nodiscard]] const scrProgramTableEntry* begin() const noexcept
		{
			return m_data;
		}

		[[nodiscard]] const scrProgramTableEntry* end() const noexcept
		{
			return m_data ? m_data + m_size : nullptr;
		}

		scrProgramTableEntry* m_data{}; // 0x00
		std::byte m_padding[0x10]{};     // 0x08
		std::uint32_t m_size{};          // 0x18
	};
#pragma pack(pop)

	static_assert(sizeof(scrProgramTableEntry) == 0x10);
	static_assert(sizeof(scrProgramTable) == 0x1C);
}
