#include "../common.hpp"
#include "pattern.hpp"
#include "range.hpp"

namespace memory
{
	namespace
	{
		[[nodiscard]] bool pattern_matches(
			const std::uint8_t* target,
			const std::optional<std::uint8_t>* signature,
			std::size_t length) noexcept
		{
			for (std::size_t index = 0; index < length; ++index)
			{
				if (signature[index] && *signature[index] != target[index])
					return false;
			}

			return true;
		}
	}

	bool range::contains(handle address) const noexcept
	{
		if (empty() || !address)
			return false;

		return address >= begin() && address < end();
	}

	bool range::contains(handle address, std::size_t length) const noexcept
	{
		if (length == 0)
			return contains(address);
		if (empty() || !address || length > m_size)
			return false;

		const auto start = address.value();
		const auto base = begin().value();
		if (start < base)
			return false;

		const auto offset = start - base;
		return offset <= m_size - length;
	}

	handle range::scan(const pattern& signature) const noexcept
	{
		const auto length = signature.m_bytes.size();
		if (empty() || length == 0 || length > m_size)
			return {};

		const auto* bytes = signature.m_bytes.data();
		const auto last_offset = m_size - length;
		for (std::size_t offset = 0; offset <= last_offset; ++offset)
		{
			const auto address = m_base.add(offset);
			if (pattern_matches(address.as<const std::uint8_t*>(), bytes, length))
				return address;
		}

		return {};
	}

	std::vector<handle> range::scan_all(const pattern& signature) const
	{
		std::vector<handle> results;
		const auto length = signature.m_bytes.size();
		if (empty() || length == 0 || length > m_size)
			return results;

		const auto* bytes = signature.m_bytes.data();
		const auto last_offset = m_size - length;
		for (std::size_t offset = 0; offset <= last_offset; ++offset)
		{
			const auto address = m_base.add(offset);
			if (pattern_matches(address.as<const std::uint8_t*>(), bytes, length))
				results.push_back(address);
		}

		return results;
	}
}
