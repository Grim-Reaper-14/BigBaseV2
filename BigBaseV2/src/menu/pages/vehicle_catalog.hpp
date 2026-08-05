#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace big::menu_pages
{
	struct vehicle_catalog_entry final
	{
		std::uint32_t model_hash;
		const char* model_name;
	};

	constexpr std::uint32_t vehicle_joaat(const char* value) noexcept
	{
		if (!value)
			return 0;

		std::uint32_t hash{};
		while (*value)
		{
			auto character = static_cast<std::uint8_t>(*value++);
			if (character >= static_cast<std::uint8_t>('A') &&
				character <= static_cast<std::uint8_t>('Z'))
			{
				character = static_cast<std::uint8_t>(character + ('a' - 'A'));
			}

			hash += character;
			hash += hash << 10;
			hash ^= hash >> 6;
		}

		hash += hash << 3;
		hash ^= hash >> 11;
		hash += hash << 15;
		return hash;
	}

	template <std::size_t Size>
	constexpr bool vehicle_catalog_hashes_valid(
		const std::array<vehicle_catalog_entry, Size>& entries) noexcept
	{
		for (const auto& entry : entries)
		{
			if (!entry.model_name || vehicle_joaat(entry.model_name) != entry.model_hash)
				return false;
		}
		return true;
	}

#include "vehicle_catalog_generated.hpp"
}
