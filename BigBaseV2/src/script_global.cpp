#include "common.hpp"
#include "pointers.hpp"
#include "script_global.hpp"

namespace big
{
	namespace
	{
		constexpr std::size_t global_page_shift = 0x12;
		constexpr std::size_t global_page_mask = 0x3F;
		constexpr std::size_t global_entry_mask = 0x3FFFF;
		constexpr std::size_t global_page_count = 0x40;
	}

	bool script_global::can_access() const noexcept
	{
		if (!g_pointers || !g_pointers->m_script_globals)
			return false;

		const auto page = (m_index >> global_page_shift) & global_page_mask;
		return page < global_page_count && g_pointers->m_script_globals[page] != nullptr;
	}

	void* script_global::get() const noexcept
	{
		if (!can_access())
			return nullptr;

		const auto page = (m_index >> global_page_shift) & global_page_mask;
		const auto offset = m_index & global_entry_mask;
		return g_pointers->m_script_globals[page] + offset;
	}
}
