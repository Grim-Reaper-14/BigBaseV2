#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <utility>

namespace rage
{
	using joaat_t = std::uint32_t;

	[[nodiscard]] inline constexpr char joaat_to_lower(char value) noexcept
	{
		return value >= 'A' && value <= 'Z' ? static_cast<char>(value + ('a' - 'A')) : value;
	}

	[[nodiscard]] inline constexpr joaat_t joaat_finalize(joaat_t hash) noexcept
	{
		hash += hash << 3;
		hash ^= hash >> 11;
		hash += hash << 15;
		return hash;
	}

	[[nodiscard]] inline constexpr joaat_t joaat(std::string_view value) noexcept
	{
		joaat_t hash = 0;
		for (const char character : value)
		{
			hash += static_cast<std::uint8_t>(joaat_to_lower(character));
			hash += hash << 10;
			hash ^= hash >> 6;
		}
		return joaat_finalize(hash);
	}

	[[nodiscard]] inline joaat_t joaat(const char* value) noexcept
	{
		return value ? joaat(std::string_view{value}) : 0;
	}

	template <std::size_t CharacterCount>
	struct constexpr_joaat
	{
		char data[CharacterCount]{};

		template <std::size_t... Indices>
		constexpr constexpr_joaat(const char* value, std::index_sequence<Indices...>) noexcept :
			data{value[Indices]...}
		{
		}

		[[nodiscard]] constexpr joaat_t operator()() const noexcept
		{
			return joaat(std::string_view{data, CharacterCount});
		}
	};
}

#define RAGE_JOAAT_IMPL(value) (::rage::constexpr_joaat<sizeof(value) - 1>((value), std::make_index_sequence<sizeof(value) - 1>{})())
#define RAGE_JOAAT(value) (std::integral_constant<::rage::joaat_t, RAGE_JOAAT_IMPL(value)>::value)
