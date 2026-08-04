#pragma once

#include <cstddef>
#include <vector>

#include "fwddec.hpp"
#include "handle.hpp"

namespace memory
{
	class range
	{
	public:
		constexpr range() noexcept = default;
		constexpr range(handle base, std::size_t size) noexcept :
			m_base(base),
			m_size(size)
		{
		}

		[[nodiscard]] constexpr handle begin() const noexcept
		{
			return m_base;
		}

		[[nodiscard]] constexpr handle end() const noexcept
		{
			return m_base.add(m_size);
		}

		[[nodiscard]] constexpr std::size_t size() const noexcept
		{
			return m_size;
		}

		[[nodiscard]] constexpr bool empty() const noexcept
		{
			return !m_base || m_size == 0;
		}

		[[nodiscard]] bool contains(handle address) const noexcept;
		[[nodiscard]] bool contains(handle address, std::size_t length) const noexcept;

		[[nodiscard]] handle scan(const pattern& signature) const noexcept;
		[[nodiscard]] std::vector<handle> scan_all(const pattern& signature) const;

	protected:
		handle m_base{};
		std::size_t m_size{};
	};
}
