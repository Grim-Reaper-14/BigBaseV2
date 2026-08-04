#include "../common.hpp"
#include "module.hpp"

namespace memory
{
	module::module(HMODULE module_handle) noexcept
	{
		initialize(module_handle);
	}

	module::module(std::nullptr_t) noexcept :
		module(GetModuleHandleW(nullptr))
	{
	}

	module::module(std::string_view name) noexcept
	{
		const std::string owned_name(name);
		initialize(owned_name.empty() ? nullptr : GetModuleHandleA(owned_name.c_str()));
	}

	module::module(std::wstring_view name) noexcept
	{
		const std::wstring owned_name(name);
		initialize(owned_name.empty() ? nullptr : GetModuleHandleW(owned_name.c_str()));
	}

	handle module::get_export(std::string_view symbol_name) const noexcept
	{
		if (!valid() || symbol_name.empty())
			return {};

		const std::string owned_name(symbol_name);
		return handle(reinterpret_cast<void*>(GetProcAddress(native_handle(), owned_name.c_str())));
	}

	HMODULE module::native_handle() const noexcept
	{
		return m_base.as<HMODULE>();
	}

	bool module::valid() const noexcept
	{
		return !empty();
	}

	void module::initialize(HMODULE module_handle) noexcept
	{
		m_base = handle(module_handle);
		m_size = 0;
		if (!module_handle)
			return;

		const auto* dos_header = m_base.as<const IMAGE_DOS_HEADER*>();
		if (!dos_header || dos_header->e_magic != IMAGE_DOS_SIGNATURE || dos_header->e_lfanew <= 0)
		{
			m_base = {};
			return;
		}

		const auto nt_address = m_base.add(static_cast<std::size_t>(dos_header->e_lfanew));
		const auto* nt_header = nt_address.as<const IMAGE_NT_HEADERS64*>();
		if (!nt_header ||
			nt_header->Signature != IMAGE_NT_SIGNATURE ||
			nt_header->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
			nt_header->OptionalHeader.SizeOfImage == 0)
		{
			m_base = {};
			return;
		}

		m_size = nt_header->OptionalHeader.SizeOfImage;
	}
}
