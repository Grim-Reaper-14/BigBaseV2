#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace memory
{
	class handle final
	{
	public:
		constexpr handle() noexcept = default;
		constexpr handle(std::nullptr_t) noexcept
		{
		}

		explicit handle(void* pointer) noexcept :
			m_value(reinterpret_cast<std::uintptr_t>(pointer))
		{
		}

		explicit constexpr handle(std::uintptr_t address) noexcept :
			m_value(address)
		{
		}

		template <typename T>
		[[nodiscard]] T as() const noexcept
		{
			if constexpr (std::is_pointer_v<T>)
			{
				return reinterpret_cast<T>(m_value);
			}
			else if constexpr (std::is_lvalue_reference_v<T>)
			{
				using value_type = std::remove_reference_t<T>;
				return *reinterpret_cast<std::add_pointer_t<value_type>>(m_value);
			}
			else
			{
				static_assert(std::is_same_v<T, std::uintptr_t>, "memory::handle::as<T>() only supports pointer, lvalue-reference, or uintptr_t types.");
				return m_value;
			}
		}

		template <typename T>
		[[nodiscard]] T read() const noexcept
		{
			static_assert(std::is_trivially_copyable_v<T>, "memory::handle::read<T>() requires a trivially copyable type.");
			T value{};
			if (m_value)
				std::memcpy(&value, reinterpret_cast<const void*>(m_value), sizeof(T));
			return value;
		}

		template <typename T>
		[[nodiscard]] constexpr handle add(T offset) const noexcept
		{
			static_assert(std::is_integral_v<T> || std::is_enum_v<T>, "memory::handle offsets must be integral values.");
			return handle(m_value + static_cast<std::uintptr_t>(offset));
		}

		template <typename T>
		[[nodiscard]] constexpr handle sub(T offset) const noexcept
		{
			static_assert(std::is_integral_v<T> || std::is_enum_v<T>, "memory::handle offsets must be integral values.");
			return handle(m_value - static_cast<std::uintptr_t>(offset));
		}

		[[nodiscard]] handle rip(std::ptrdiff_t displacement_offset = 0, std::ptrdiff_t instruction_size = 4) const noexcept
		{
			if (!m_value)
				return {};

			const auto displacement_address = add(displacement_offset);
			const auto displacement = displacement_address.read<std::int32_t>();
			return displacement_address.add(instruction_size).add(displacement);
		}

		[[nodiscard]] constexpr std::uintptr_t value() const noexcept
		{
			return m_value;
		}

		[[nodiscard]] explicit constexpr operator bool() const noexcept
		{
			return m_value != 0;
		}

		friend constexpr bool operator==(handle left, handle right) noexcept
		{
			return left.m_value == right.m_value;
		}

		friend constexpr bool operator!=(handle left, handle right) noexcept
		{
			return !(left == right);
		}

		friend constexpr bool operator<(handle left, handle right) noexcept
		{
			return left.m_value < right.m_value;
		}

		friend constexpr bool operator<=(handle left, handle right) noexcept
		{
			return left.m_value <= right.m_value;
		}

		friend constexpr bool operator>(handle left, handle right) noexcept
		{
			return right < left;
		}

		friend constexpr bool operator>=(handle left, handle right) noexcept
		{
			return right <= left;
		}

	private:
		std::uintptr_t m_value{};
	};
}
