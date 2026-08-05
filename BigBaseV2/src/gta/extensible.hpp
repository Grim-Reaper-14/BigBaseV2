#pragma once

#include "joaat.hpp"
#include "ref_aware.hpp"

#include <cstdint>
#include <string_view>
#include <typeinfo>

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
		virtual bool is_of_type(std::uint32_t hash) = 0;
		virtual const std::uint32_t& get_type() = 0;

		template <typename T>
		[[nodiscard]] bool is_of_type()
		{
			static const auto type_hash = []
			{
				std::string_view name{typeid(T).name()};
				if (name.compare(0, 6, "class ") == 0)
					name.remove_prefix(6);
				else if (name.compare(0, 7, "struct ") == 0)
					name.remove_prefix(7);
				return rage::joaat(name);
			}();

			return is_of_type(type_hash);
		}

		fwExtensionContainer* m_extension_container{}; // 0x10
		void* m_extensible_unknown{};                   // 0x18
	};

	static_assert(sizeof(fwExtensionContainer) == 0x10);
	static_assert(sizeof(fwExtensibleBase) == 0x20);
}
