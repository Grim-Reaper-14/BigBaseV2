#pragma once

#include "common.hpp"

namespace big
{
	class script_global final
	{
	public:
		constexpr explicit script_global(std::size_t index) noexcept :
			m_index(index)
		{
		}

		[[nodiscard]] constexpr script_global at(std::ptrdiff_t index) const noexcept
		{
			return script_global(static_cast<std::size_t>(static_cast<std::ptrdiff_t>(m_index) + index));
		}

		[[nodiscard]] constexpr script_global at(std::ptrdiff_t index, std::size_t size) const noexcept
		{
			return script_global(static_cast<std::size_t>(static_cast<std::ptrdiff_t>(m_index) + 1 + index * static_cast<std::ptrdiff_t>(size)));
		}

		template <typename T>
		[[nodiscard]] std::enable_if_t<std::is_pointer_v<T>, T> as() const noexcept
		{
			return static_cast<T>(get());
		}

		template <typename T>
		[[nodiscard]] std::enable_if_t<std::is_lvalue_reference_v<T>, T> as() const
		{
			auto* pointer = get();
			if (!pointer)
				throw std::runtime_error("Attempted to access an unavailable GTA V Enhanced script global.");

			return *static_cast<std::add_pointer_t<std::remove_reference_t<T>>>(pointer);
		}

		[[nodiscard]] bool can_access() const noexcept;
		[[nodiscard]] constexpr std::size_t index() const noexcept
		{
			return m_index;
		}

	private:
		[[nodiscard]] void* get() const noexcept;

		std::size_t m_index{};
	};
}
