#pragma once

#include <cstdint>

namespace rage
{
	class datBase
	{
	public:
		virtual ~datBase() = default;
	};

	// GTA V Enhanced pgBase vtable prefix. There is no virtual destructor in
	// slot zero; the five paging methods are the complete verified prefix.
	class pgBase
	{
	public:
		virtual void shutdown_class() = 0;
		virtual void set_handle_index(std::uint32_t index) = 0;
		virtual void validate(std::uint32_t value) = 0;
		virtual void init_class(std::int32_t value) = 0;
		virtual std::uint32_t get_handle_index() const
		{
			return 0;
		}

		[[nodiscard]] std::uint32_t map_size() const noexcept
		{
			return m_map_size;
		}

	protected:
		std::uint32_t m_map_size{}; // 0x08
	};

	static_assert(sizeof(datBase) == 0x08);
	static_assert(sizeof(pgBase) == 0x10);
}
