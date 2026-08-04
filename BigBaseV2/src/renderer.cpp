#include "common.hpp"
#include "fonts.hpp"
#include "logger.hpp"
#include "gui.hpp"
#include "pointers.hpp"
#include "renderer.hpp"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace big
{
	renderer::renderer()
	{
		if (g_pointers == nullptr || g_pointers->m_swapchain == nullptr || *g_pointers->m_swapchain == nullptr)
		{
			throw std::runtime_error("Renderer initialization failed: swap chain is unavailable.");
		}

		if (g_pointers->m_hwnd == nullptr)
		{
			throw std::runtime_error("Renderer initialization failed: game window is unavailable.");
		}

		m_dxgi_swapchain = *g_pointers->m_swapchain;

		void* d3d_device{};
		const HRESULT device_result = m_dxgi_swapchain->GetDevice(__uuidof(ID3D11Device), &d3d_device);
		if (FAILED(device_result) || d3d_device == nullptr)
		{
			throw std::runtime_error("Renderer initialization failed: unable to acquire the D3D11 device.");
		}

		m_d3d_device.Attach(static_cast<ID3D11Device*>(d3d_device));
		m_d3d_device->GetImmediateContext(m_d3d_device_context.GetAddressOf());
		if (m_d3d_device_context == nullptr)
		{
			throw std::runtime_error("Renderer initialization failed: unable to acquire the D3D11 device context.");
		}

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		if (!ImGui_ImplWin32_Init(g_pointers->m_hwnd))
		{
			ImGui::DestroyContext();
			throw std::runtime_error("Renderer initialization failed: ImGui Win32 backend could not start.");
		}

		if (!ImGui_ImplDX11_Init(m_d3d_device.Get(), m_d3d_device_context.Get()))
		{
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
			throw std::runtime_error("Renderer initialization failed: ImGui DX11 backend could not start.");
		}

		ImFontConfig font_cfg{};
		font_cfg.FontDataOwnedByAtlas = false;
		strcpy_s(font_cfg.Name, "Rubik");

		m_font = io.Fonts->AddFontFromMemoryTTF(
			const_cast<std::uint8_t*>(font_rubik),
			sizeof(font_rubik),
			20.0f,
			&font_cfg);
		m_monospace_font = io.Fonts->AddFontDefault();

		if (m_font == nullptr || m_monospace_font == nullptr)
		{
			ImGui_ImplDX11_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
			throw std::runtime_error("Renderer initialization failed: required fonts could not be loaded.");
		}

		g_gui.dx_init();
		g_renderer = this;
	}

	renderer::~renderer()
	{
		g_renderer = nullptr;

		if (ImGui::GetCurrentContext() != nullptr)
		{
			ImGui_ImplDX11_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
		}

		m_d3d_device_context.Reset();
		m_d3d_device.Reset();
		m_dxgi_swapchain.Reset();
	}

	void renderer::on_present()
	{
		if (ImGui::GetCurrentContext() == nullptr)
			return;

		ImGuiIO& io = ImGui::GetIO();
		io.MouseDrawCursor = g_gui.m_opened;

		if (g_gui.m_opened)
			io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
		else
			io.ConfigFlags |= ImGuiConfigFlags_NoMouse;

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		if (g_gui.m_opened)
			g_gui.dx_on_tick();

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}

	void renderer::pre_reset()
	{
		if (ImGui::GetCurrentContext() != nullptr)
			ImGui_ImplDX11_InvalidateDeviceObjects();
	}

	void renderer::post_reset()
	{
		if (ImGui::GetCurrentContext() != nullptr)
			ImGui_ImplDX11_CreateDeviceObjects();
	}

	void renderer::wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		if (msg == WM_KEYUP && wparam == VK_INSERT)
			g_gui.m_opened = !g_gui.m_opened;

		if (g_gui.m_opened && ImGui::GetCurrentContext() != nullptr)
			ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
	}
}
