#pragma once

#include <cstddef>
#include <cstdint>

namespace rage
{
	template <typename T>
	class atArray
	{
	public:
		[[nodiscard]] T* begin() noexcept
		{
			return m_data;
		}

		[[nodiscard]] T* end() noexcept
		{
			return m_data ? m_data + m_size : nullptr;
		}

		[[nodiscard]] const T* begin() const noexcept
		{
			return m_data;
		}

		[[nodiscard]] const T* end() const noexcept
		{
			return m_data ? m_data + m_size : nullptr;
		}

		[[nodiscard]] T* data() noexcept
		{
			return m_data;
		}

		[[nodiscard]] const T* data() const noexcept
		{
			return m_data;
		}

		[[nodiscard]] std::uint16_t size() const noexcept
		{
			return m_size;
		}

		[[nodiscard]] std::uint16_t capacity() const noexcept
		{
			return m_capacity;
		}

		[[nodiscard]] bool empty() const noexcept
		{
			return m_size == 0;
		}

		[[nodiscard]] bool contains(const T& value) const
		{
			for (const auto& entry : *this)
			{
				if (entry == value)
					return true;
			}
			return false;
		}

		T& operator[](std::size_t index) noexcept
		{
			return m_data[index];
		}

		const T& operator[](std::size_t index) const noexcept
		{
			return m_data[index];
		}

		T* m_data{};
		std::uint16_t m_size{};
		std::uint16_t m_capacity{};
	};

	static_assert(sizeof(atArray<std::uint32_t>) == 0x10);
}
