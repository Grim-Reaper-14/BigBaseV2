#include "common.hpp"
#include "configuration.hpp"
#include "fonts.hpp"
#include "gui.hpp"
#include "Image_Loader_Manager.hpp"
#include "logger.hpp"
#include "pointers.hpp"
#include "renderer.hpp"
#include "Themes_Manager.hpp"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

namespace big
{
	renderer::renderer()
	{
		if (!g_pointers || !g_pointers->renderer_ready())
			throw std::runtime_error("Renderer cannot initialize before the DXGI swapchain is ready.");

		m_dxgi_swapchain = *g_pointers->m_swapchain;

		void* device{};
		const auto device_result = m_dxgi_swapchain->GetDevice(__uuidof(ID3D11Device), &device);
		if (FAILED(device_result) || !device)
			throw std::runtime_error("Failed to acquire the D3D11 device from the swapchain.");

		m_d3d_device.Attach(static_cast<ID3D11Device*>(device));
		m_d3d_device->GetImmediateContext(m_d3d_device_context.GetAddressOf());
		if (!m_d3d_device_context)
			throw std::runtime_error("Failed to acquire the D3D11 immediate context.");

		IMGUI_CHECKVERSION();
		if (!ImGui::CreateContext())
			throw std::runtime_error("Failed to create the ImGui context.");

		if (!ImGui_ImplDX11_Init(m_d3d_device.Get(), m_d3d_device_context.Get()))
			throw std::runtime_error("Failed to initialize the ImGui DX11 backend.");
		m_dx11_backend_initialized = true;

		if (!ImGui_ImplWin32_Init(g_pointers->m_hwnd))
			throw std::runtime_error("Failed to initialize the ImGui Win32 backend.");
		m_win32_backend_initialized = true;

		auto& io = ImGui::GetIO();
		ImFontConfig font_config{};
		font_config.FontDataOwnedByAtlas = false;
		std::strncpy(font_config.Name, "Rubik", sizeof(font_config.Name) - 1);

		m_font = io.Fonts->AddFontFromMemoryTTF(
			const_cast<std::uint8_t*>(font_rubik),
			sizeof(font_rubik),
			20.0f,
			&font_config);
		m_monospace_font = io.Fonts->AddFontDefault();

		if (!m_font)
		{
			LOG_WARNING("Rubik font initialization failed; falling back to the default ImGui font.");
			m_font = m_monospace_font;
		}

		g_gui.dx_init();

		const auto& configuration = g_configuration.values();
		auto& style = ImGui::GetStyle();
		style.WindowRounding = configuration.window_rounding;
		style.ChildRounding = configuration.window_rounding;
		style.FrameRounding = configuration.frame_rounding;
		style.GrabRounding = configuration.frame_rounding;
		style.ScrollbarRounding = configuration.frame_rounding;
		const ImVec4 accent(configuration.accent[0], configuration.accent[1], configuration.accent[2], configuration.accent[3]);
		style.Colors[ImGuiCol_CheckMark] = accent;
		style.Colors[ImGuiCol_SliderGrab] = accent;
		style.Colors[ImGuiCol_SliderGrabActive] = accent;
		style.Colors[ImGuiCol_HeaderActive] = accent;
		style.Colors[ImGuiCol_ResizeGripActive] = accent;
		style.Colors[ImGuiCol_NavHighlight] = accent;

		g_themes_manager.Initialize();
		g_themes_manager.CaptureCurrent("Configured");

		std::string image_manager_error;
		if (!g_image_loader_manager.Initialize(m_d3d_device.Get(), image_manager_error))
			LOG_WARNING("Image loader manager initialization failed: {}", image_manager_error);

		m_initialized = true;
		g_renderer = this;
	}

	renderer::~renderer()
	{
		m_initialized = false;
		g_image_loader_manager.Shutdown();

		if (m_win32_backend_initialized)
		{
			ImGui_ImplWin32_Shutdown();
			m_win32_backend_initialized = false;
		}

		if (m_dx11_backend_initialized)
		{
			ImGui_ImplDX11_Shutdown();
			m_dx11_backend_initialized = false;
		}

		if (ImGui::GetCurrentContext())
			ImGui::DestroyContext();

		if (g_renderer == this)
			g_renderer = nullptr;
	}

	void renderer::on_present()
	{
		if (!ready() || !ImGui::GetCurrentContext())
			return;

		auto& io = ImGui::GetIO();
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
		if (m_dx11_backend_initialized)
			ImGui_ImplDX11_InvalidateDeviceObjects();
	}

	void renderer::post_reset()
	{
		if (m_dx11_backend_initialized && !ImGui_ImplDX11_CreateDeviceObjects())
			LOG_ERROR("Failed to recreate ImGui DX11 device objects after resize.");
	}

	void renderer::wndproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
	{
		if (!m_initialized || !ImGui::GetCurrentContext())
			return;

		if (message == WM_KEYUP && static_cast<int>(wparam) == g_configuration.values().menu_key)
			g_gui.m_opened = !g_gui.m_opened;

		if (g_gui.m_opened)
			ImGui_ImplWin32_WndProcHandler(hwnd, message, wparam, lparam);
	}
}
