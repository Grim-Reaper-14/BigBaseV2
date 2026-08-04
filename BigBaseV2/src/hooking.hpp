#pragma once
#include "common.hpp"
#include "detour_hook.hpp"
#include "gta/fwddec.hpp"
#include "script_hook.hpp"
#include "vmt_hook.hpp"

namespace big
{
	struct hooks final
	{
		static bool run_script_threads(std::uint32_t ops_to_execute);
		static void* convert_thread_to_fiber(void* param);

		static constexpr std::size_t swapchain_num_funcs = 19;
		static constexpr std::size_t swapchain_present_index = 8;
		static constexpr std::size_t swapchain_resizebuffers_index = 13;
		static HRESULT swapchain_present(IDXGISwapChain* swapchain, UINT sync_interval, UINT flags);
		static HRESULT swapchain_resizebuffers(IDXGISwapChain* swapchain, UINT buffer_count, UINT width, UINT height, DXGI_FORMAT new_format, UINT swapchain_flags);

		static LRESULT wndproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
		static BOOL set_cursor_pos(int x, int y);

		static constexpr std::size_t main_persistent_num_funcs = 16;
		static constexpr std::size_t main_persistent_dtor_index = 0;
		static constexpr std::size_t main_persistent_is_networked_index = 6;
		static void main_persistent_dtor(CGameScriptHandler* handler, bool free_memory);
		static bool main_persistent_is_networked(CGameScriptHandler* handler);
	};

	class minhook_keepalive final
	{
	public:
		minhook_keepalive();
		~minhook_keepalive();

		minhook_keepalive(const minhook_keepalive&) = delete;
		minhook_keepalive& operator=(const minhook_keepalive&) = delete;
	};

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
		void ensure_dynamic_hooks();

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
		detour_hook m_run_script_threads_hook;
		detour_hook m_convert_thread_to_fiber_hook;
		std::unique_ptr<vmt_hook> m_main_persistent_hook;
	};

	inline hooking* g_hooking{};
}
