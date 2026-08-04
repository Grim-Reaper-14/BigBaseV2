#pragma once
#include "common.hpp"
#include <imgui.h>

namespace big
{
	class renderer final
	{
	public:
		renderer();
		~renderer();

		renderer(const renderer&) = delete;
		renderer(renderer&&) = delete;
		renderer& operator=(const renderer&) = delete;
		renderer& operator=(renderer&&) = delete;

		void on_present();
		void pre_reset();
		void post_reset();
		void wndproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

		[[nodiscard]] bool ready() const noexcept
		{
			return m_initialized && m_dxgi_swapchain && m_d3d_device && m_d3d_device_context;
		}

		ImFont* m_font{};
		ImFont* m_monospace_font{};

	private:
		bool m_initialized{};
		bool m_dx11_backend_initialized{};
		bool m_win32_backend_initialized{};
		comptr<IDXGISwapChain> m_dxgi_swapchain;
		comptr<ID3D11Device> m_d3d_device;
		comptr<ID3D11DeviceContext> m_d3d_device_context;
	};

	inline renderer* g_renderer{};
}
