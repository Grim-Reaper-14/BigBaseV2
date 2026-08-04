#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "fwddec.hpp"

namespace memory
{
	class pattern final
	{
		friend class range;

	public:
		explicit pattern(std::string_view ida_signature);
		pattern(const void* bytes, std::string_view mask);

		pattern(const char* ida_signature) :
			pattern(std::string_view(ida_signature ? ida_signature : ""))
		{
		}

		[[nodiscard]] bool empty() const noexcept
		{
			return m_bytes.empty();
		}

		[[nodiscard]] std::size_t size() const noexcept
		{
			return m_bytes.size();
		}

	private:
		std::vector<std::optional<std::uint8_t>> m_bytes;
	};
}
