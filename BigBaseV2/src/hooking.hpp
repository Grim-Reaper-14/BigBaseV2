#pragma once
#include "common.hpp"
#include "detour_hook.hpp"
#include "vmt_hook.hpp"

namespace big
{
	struct hooks final
	{
		static void* convert_thread_to_fiber(void* param);

		static constexpr std::size_t swapchain_num_funcs = 19;
		static constexpr std::size_t swapchain_present_index = 8;
		static constexpr std::size_t swapchain_resizebuffers_index = 13;
		static HRESULT swapchain_present(IDXGISwapChain* swapchain, UINT sync_interval, UINT flags);
		static HRESULT swapchain_resizebuffers(IDXGISwapChain* swapchain, UINT buffer_count, UINT width, UINT height, DXGI_FORMAT new_format, UINT swapchain_flags);

		static LRESULT wndproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
		static BOOL set_cursor_pos(int x, int y);
	};

	class minhook_keepalive final
	{
	public:
		minhook_keepalive();
		~minhook_keepalive();

		minhook_keepalive(const minhook_keepalive&) = delete;
		minhook_keepalive& operator=(const minhook_keepalive&) = delete;
	};

	// Owns renderer, input, and fiber compatibility hooks. The game's main
	// script-loop detour is owned exclusively by main_hook.
	class hooking final
	{
		friend hooks;

	public:
		hooking();
		~hooking();

		hooking(const hooking&) = delete;
		hooking(hooking&&) = delete;
		hooking& operator=(const hooking&) = delete;
		hooking& operator=(hooking&&) = delete;

		void enable();
		void disable() noexcept;

		[[nodiscard]] bool enabled() const noexcept
		{
			return m_enabled;
		}

	private:
		bool m_enabled{};
		minhook_keepalive m_minhook_keepalive;

		vmt_hook m_swapchain_hook;
		WNDPROC m_original_wndproc{};
		detour_hook m_set_cursor_pos_hook;
		detour_hook m_convert_thread_to_fiber_hook;
	};

	inline hooking* g_hooking{};
}
