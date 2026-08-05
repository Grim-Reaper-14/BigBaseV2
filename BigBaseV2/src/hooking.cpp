#include "common.hpp"
#include "gui.hpp"
#include "hooking.hpp"
#include "logger.hpp"
#include "memory/module.hpp"
#include "pointers.hpp"
#include "renderer.hpp"

#include <MinHook.h>

namespace big
{
	hooking::hooking() :
		m_swapchain_hook(*g_pointers->m_swapchain, hooks::swapchain_num_funcs),
		m_set_cursor_pos_hook("SetCursorPos", memory::module("user32.dll").get_export("SetCursorPos").as<void*>(), &hooks::set_cursor_pos),
		m_convert_thread_to_fiber_hook("ConvertThreadToFiber", memory::module("kernel32.dll").get_export("ConvertThreadToFiber").as<void*>(), &hooks::convert_thread_to_fiber)
	{
		m_swapchain_hook.hook(hooks::swapchain_present_index, &hooks::swapchain_present);
		m_swapchain_hook.hook(hooks::swapchain_resizebuffers_index, &hooks::swapchain_resizebuffers);
		g_hooking = this;
	}

	hooking::~hooking()
	{
		disable();
		if (g_hooking == this)
			g_hooking = nullptr;
	}

	void hooking::enable()
	{
		if (m_enabled)
			return;

		if (!g_pointers || !g_pointers->core_ready() || !g_pointers->renderer_ready())
			throw std::runtime_error("Cannot enable hooks before pointer and renderer initialization.");

		m_swapchain_hook.enable();

		SetLastError(ERROR_SUCCESS);
		const auto previous = SetWindowLongPtrW(
			g_pointers->m_hwnd,
			GWLP_WNDPROC,
			reinterpret_cast<LONG_PTR>(&hooks::wndproc));
		if (!previous && GetLastError() != ERROR_SUCCESS)
		{
			m_swapchain_hook.disable();
			throw std::runtime_error("Failed to install the window procedure hook.");
		}
		m_original_wndproc = reinterpret_cast<WNDPROC>(previous);

		try
		{
			m_set_cursor_pos_hook.enable();
			m_convert_thread_to_fiber_hook.enable();
			m_enabled = true;
		}
		catch (...)
		{
			m_set_cursor_pos_hook.disable();
			if (m_original_wndproc)
			{
				SetWindowLongPtrW(
					g_pointers->m_hwnd,
					GWLP_WNDPROC,
					reinterpret_cast<LONG_PTR>(m_original_wndproc));
				m_original_wndproc = nullptr;
			}
			m_swapchain_hook.disable();
			throw;
		}
	}

	void hooking::disable() noexcept
	{
		if (!m_enabled)
			return;

		m_enabled = false;
		m_convert_thread_to_fiber_hook.disable();
		m_set_cursor_pos_hook.disable();

		if (m_original_wndproc && g_pointers && g_pointers->m_hwnd)
		{
			SetWindowLongPtrW(
				g_pointers->m_hwnd,
				GWLP_WNDPROC,
				reinterpret_cast<LONG_PTR>(m_original_wndproc));
			m_original_wndproc = nullptr;
		}

		m_swapchain_hook.disable();
	}

	minhook_keepalive::minhook_keepalive()
	{
		const auto status = MH_Initialize();
		if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED)
			throw std::runtime_error("MinHook initialization failed.");
	}

	minhook_keepalive::~minhook_keepalive()
	{
		const auto status = MH_Uninitialize();
		if (status != MH_OK && status != MH_ERROR_NOT_INITIALIZED)
			LOG_ERROR("MinHook uninitialization failed with status {}.", static_cast<int>(status));
	}

	void* hooks::convert_thread_to_fiber(void* param)
	{
		if (IsThreadAFiber())
			return GetCurrentFiber();

		return g_hooking->m_convert_thread_to_fiber_hook.get_original<decltype(&convert_thread_to_fiber)>()(param);
	}

	HRESULT hooks::swapchain_present(IDXGISwapChain* swapchain, UINT sync_interval, UINT flags)
	{
		if (g_running && g_renderer)
			g_renderer->on_present();

		return g_hooking->m_swapchain_hook.get_original<decltype(&swapchain_present)>(swapchain_present_index)(swapchain, sync_interval, flags);
	}

	HRESULT hooks::swapchain_resizebuffers(IDXGISwapChain* swapchain, UINT buffer_count, UINT width, UINT height, DXGI_FORMAT new_format, UINT swapchain_flags)
	{
		const auto original = g_hooking->m_swapchain_hook.get_original<decltype(&swapchain_resizebuffers)>(swapchain_resizebuffers_index);

		if (!g_running || !g_renderer)
			return original(swapchain, buffer_count, width, height, new_format, swapchain_flags);

		g_renderer->pre_reset();
		const auto result = original(swapchain, buffer_count, width, height, new_format, swapchain_flags);
		if (SUCCEEDED(result))
			g_renderer->post_reset();

		return result;
	}

	LRESULT hooks::wndproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
	{
		if (g_running && g_renderer)
			g_renderer->wndproc(hwnd, message, wparam, lparam);

		return CallWindowProcW(g_hooking->m_original_wndproc, hwnd, message, wparam, lparam);
	}

	BOOL hooks::set_cursor_pos(int x, int y)
	{
		if (g_gui.m_opened)
			return TRUE;

		return g_hooking->m_set_cursor_pos_hook.get_original<decltype(&set_cursor_pos)>()(x, y);
	}
}
