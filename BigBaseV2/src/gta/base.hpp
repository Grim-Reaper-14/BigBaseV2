#pragma once

#include <cstdint>

namespace rage
{
	class datBase
	{
	public:
		virtual ~datBase() = default;
	};

	// GTA V Enhanced pgBase exposes five paging/metadata virtual functions.
	// It does not place a virtual destructor in slot zero.
	class pgBase
	{
	public:
		virtual void shutdown_class() {}
		virtual void set_handle_index(std::uint32_t) {}
		virtual void validate(std::uint32_t) {}
		virtual void init_class(std::int32_t) {}
		virtual std::uint32_t get_handle_index() const
		{
			return 0;
		}

	protected:
		std::uint32_t m_map_size{};
		std::uint32_t m_padding{};
	};

	static_assert(sizeof(datBase) == 0x08);
	static_assert(sizeof(pgBase) == 0x10);
}
