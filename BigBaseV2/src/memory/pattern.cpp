#include "../common.hpp"
#include "pattern.hpp"

namespace memory
{
	namespace
	{
		[[nodiscard]] std::optional<std::uint8_t> hex_value(char character) noexcept
		{
			if (character >= '0' && character <= '9')
				return static_cast<std::uint8_t>(character - '0');
			if (character >= 'a' && character <= 'f')
				return static_cast<std::uint8_t>(character - 'a' + 10);
			if (character >= 'A' && character <= 'F')
				return static_cast<std::uint8_t>(character - 'A' + 10);
			return std::nullopt;
		}
	}

	pattern::pattern(std::string_view ida_signature)
	{
		std::size_t index{};
		while (index < ida_signature.size())
		{
			while (index < ida_signature.size() && std::isspace(static_cast<unsigned char>(ida_signature[index])))
				++index;

			if (index >= ida_signature.size())
				break;

			if (ida_signature[index] == '?')
			{
				m_bytes.emplace_back(std::nullopt);
				++index;
				if (index < ida_signature.size() && ida_signature[index] == '?')
					++index;
			}
			else
			{
				if (index + 1 >= ida_signature.size())
					throw std::invalid_argument("IDA pattern contains an incomplete byte token.");

				const auto high = hex_value(ida_signature[index]);
				const auto low = hex_value(ida_signature[index + 1]);
				if (!high || !low)
					throw std::invalid_argument("IDA pattern contains a non-hexadecimal byte token.");

				m_bytes.emplace_back(static_cast<std::uint8_t>((*high << 4) | *low));
				index += 2;
			}

			if (index < ida_signature.size() && !std::isspace(static_cast<unsigned char>(ida_signature[index])))
				throw std::invalid_argument("IDA pattern tokens must be separated by whitespace.");
		}

		if (m_bytes.empty())
			throw std::invalid_argument("IDA pattern cannot be empty.");
	}

	pattern::pattern(const void* bytes, std::string_view mask)
	{
		if (!bytes && !mask.empty())
			throw std::invalid_argument("Pattern byte buffer cannot be null when the mask is non-empty.");
		if (mask.empty())
			throw std::invalid_argument("Pattern mask cannot be empty.");

		const auto* byte_data = static_cast<const std::uint8_t*>(bytes);
		m_bytes.reserve(mask.size());
		for (std::size_t index = 0; index < mask.size(); ++index)
		{
			const char token = mask[index];
			if (token == '?')
				m_bytes.emplace_back(std::nullopt);
			else if (token == 'x' || token == 'X')
				m_bytes.emplace_back(byte_data[index]);
			else
				throw std::invalid_argument("Pattern mask only supports 'x' and '?' characters.");
		}
	}
}
