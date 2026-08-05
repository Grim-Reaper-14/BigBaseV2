#pragma once

#include "ref_aware.hpp"

#include <cstdint>

namespace rage
{
	class fwExtension
	{
	public:
		virtual ~fwExtension() = default;
		virtual void unknown_0x08() = 0;
		virtual void unknown_0x10() = 0;
		virtual std::uint32_t get_id() = 0;
	};

	struct fwExtensionContainer
	{
		fwExtension* m_entry{};
		fwExtensionContainer* m_next{};
	};

	class fwExtensibleBase : public fwRefAwareBase
	{
	public:
		[[nodiscard]] fwExtension* find_extension(std::uint32_t id) noexcept
		{
			for (auto* node = m_extension_container; node; node = node->m_next)
			{
				if (node->m_entry && node->m_entry->get_id() == id)
					return node->m_entry;
			}

			return nullptr;
		}

		[[nodiscard]] const fwExtension* find_extension(std::uint32_t id) const noexcept
		{
			for (auto* node = m_extension_container; node; node = node->m_next)
			{
				if (node->m_entry && node->m_entry->get_id() == id)
					return node->m_entry;
			}

			return nullptr;
		}

		fwExtensionContainer* m_extension_container{}; // 0x10
		void* m_extensible_unknown{};                   // 0x18
	};

	static_assert(sizeof(fwExtension) == 0x08);
	static_assert(sizeof(fwExtensionContainer) == 0x10);
	static_assert(sizeof(fwExtensibleBase) == 0x20);
}
