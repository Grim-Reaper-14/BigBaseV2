#pragma once

#include <string_view>

#include "range.hpp"

namespace memory
{
	class module final : public range
	{
	public:
		module() noexcept = default;
		explicit module(HMODULE module_handle) noexcept;
		explicit module(std::nullptr_t) noexcept;
		explicit module(std::string_view name);
		explicit module(std::wstring_view name);

		[[nodiscard]] handle get_export(std::string_view symbol_name) const;
		[[nodiscard]] HMODULE native_handle() const noexcept;
		[[nodiscard]] bool valid() const noexcept;

	private:
		void initialize(HMODULE module_handle) noexcept;
	};
}
