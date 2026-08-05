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
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

namespace big
{
	renderer::renderer()
	{
		try
		{
			if (!g_pointers || !g_pointers->renderer_ready())
				throw std::runtime_error("Renderer cannot initialize before the Enhanced DX12 pointers are ready.");

			m_command_queue = *g_pointers->m_command_queue;
			if (!m_command_queue)
				throw std::runtime_error("The Enhanced D3D12 command queue pointer is null.");

			IDXGISwapChain3* swapchain3{};
			const auto swapchain_result = (*g_pointers->m_swapchain)->QueryInterface(IID_PPV_ARGS(&swapchain3));
			if (FAILED(swapchain_result) || !swapchain3)
				throw std::runtime_error("Failed to acquire IDXGISwapChain3 from the Enhanced swapchain.");
			m_dxgi_swapchain.Attach(swapchain3);

			ID3D12Device* device{};
			const auto device_result = m_dxgi_swapchain->GetDevice(IID_PPV_ARGS(&device));
			if (FAILED(device_result) || !device)
				throw std::runtime_error("Failed to acquire the D3D12 device from the Enhanced swapchain.");
			m_d3d_device.Attach(device);

			if (FAILED(m_d3d_device->CreateFence(
				0,
				D3D12_FENCE_FLAG_NONE,
				IID_PPV_ARGS(m_fence.ReleaseAndGetAddressOf()))))
			{
				throw std::runtime_error("Failed to create the D3D12 renderer fence.");
			}

			m_fence_event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
			if (!m_fence_event)
				throw std::runtime_error("Failed to create the D3D12 renderer fence event.");

			DXGI_SWAP_CHAIN_DESC swapchain_desc{};
			if (FAILED(m_dxgi_swapchain->GetDesc(&swapchain_desc)) || swapchain_desc.BufferCount == 0)
				throw std::runtime_error("Failed to read the Enhanced swapchain description.");

			if (!create_srv_heap())
				throw std::runtime_error("Failed to create the ImGui DX12 SRV descriptor heap.");
			if (!create_frame_resources(swapchain_desc.BufferCount))
				throw std::runtime_error("Failed to create the ImGui DX12 frame resources.");
			if (!create_render_targets())
				throw std::runtime_error("Failed to create the ImGui DX12 render targets.");

			IMGUI_CHECKVERSION();
			if (!ImGui::CreateContext())
				throw std::runtime_error("Failed to create the ImGui context.");

			if (!ImGui_ImplWin32_Init(g_pointers->m_hwnd))
				throw std::runtime_error("Failed to initialize the ImGui Win32 backend.");
			m_win32_backend_initialized = true;

			if (!initialize_dx12_backend())
				throw std::runtime_error("Failed to initialize the ImGui 1.92 DX12 backend.");

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

			// The existing image loader owns D3D11 textures. Keep it disabled until
			// it is ported to the shared DX12 SRV heap instead of creating an
			// incompatible D3D11 device solely for menu images.
			g_image_loader_manager.Shutdown();
			LOG_WARNING("The custom image loader is disabled while its DX12 texture uploader is being ported.");

			m_initialized = true;
			g_renderer = this;
		}
		catch (...)
		{
			m_initialized = false;
			g_image_loader_manager.Shutdown();

			if (m_win32_backend_initialized)
			{
				ImGui_ImplWin32_Shutdown();
				m_win32_backend_initialized = false;
			}

			shutdown_dx12_backend();
			if (ImGui::GetCurrentContext())
				ImGui::DestroyContext();

			cleanup_render_targets();
			cleanup_frame_resources();
			if (m_fence_event)
			{
				CloseHandle(m_fence_event);
				m_fence_event = nullptr;
			}
			throw;
		}
	}

	renderer::~renderer()
	{
		m_initialized = false;
		g_image_loader_manager.Shutdown();
		wait_for_gpu();

		if (m_win32_backend_initialized)
		{
			ImGui_ImplWin32_Shutdown();
			m_win32_backend_initialized = false;
		}

		shutdown_dx12_backend();

		if (ImGui::GetCurrentContext())
			ImGui::DestroyContext();

		cleanup_render_targets();
		cleanup_frame_resources();

		if (m_fence_event)
		{
			CloseHandle(m_fence_event);
			m_fence_event = nullptr;
		}

		if (g_renderer == this)
			g_renderer = nullptr;
	}

	bool renderer::create_srv_heap()
	{
		D3D12_DESCRIPTOR_HEAP_DESC descriptor{};
		descriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		descriptor.NumDescriptors = srv_descriptor_count;
		descriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		if (FAILED(m_d3d_device->CreateDescriptorHeap(
			&descriptor,
			IID_PPV_ARGS(m_srv_heap.ReleaseAndGetAddressOf()))))
		{
			return false;
		}

		m_srv_descriptor_size = m_d3d_device->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		if (!m_srv_descriptor_size)
			return false;

		m_free_srv_descriptors.clear();
		m_srv_descriptor_in_use.assign(srv_descriptor_count, false);
		m_free_srv_descriptors.reserve(srv_descriptor_count);
		for (std::uint32_t index = srv_descriptor_count; index > 0; --index)
			m_free_srv_descriptors.push_back(index - 1);

		return true;
	}

	bool renderer::create_frame_resources(UINT buffer_count)
	{
		cleanup_frame_resources();
		if (!buffer_count)
			return false;

		m_frames.resize(buffer_count);
		for (auto& frame : m_frames)
		{
			if (FAILED(m_d3d_device->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(frame.command_allocator.ReleaseAndGetAddressOf()))))
			{
				cleanup_frame_resources();
				return false;
			}
		}

		if (FAILED(m_d3d_device->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_frames.front().command_allocator.Get(),
			nullptr,
			IID_PPV_ARGS(m_command_list.ReleaseAndGetAddressOf()))))
		{
			cleanup_frame_resources();
			return false;
		}

		if (FAILED(m_command_list->Close()))
		{
			cleanup_frame_resources();
			return false;
		}

		return true;
	}

	bool renderer::create_render_targets()
	{
		cleanup_render_targets();

		DXGI_SWAP_CHAIN_DESC swapchain_desc{};
		if (FAILED(m_dxgi_swapchain->GetDesc(&swapchain_desc)) || !swapchain_desc.BufferCount)
			return false;
		if (swapchain_desc.BufferCount != m_frames.size())
			return false;

		m_rtv_format = swapchain_desc.BufferDesc.Format;
		if (m_rtv_format == DXGI_FORMAT_UNKNOWN)
			m_rtv_format = DXGI_FORMAT_R8G8B8A8_UNORM;

		D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
		heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		heap_desc.NumDescriptors = swapchain_desc.BufferCount;
		heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		if (FAILED(m_d3d_device->CreateDescriptorHeap(
			&heap_desc,
			IID_PPV_ARGS(m_rtv_heap.ReleaseAndGetAddressOf()))))
		{
			return false;
		}

		m_rtv_descriptor_size = m_d3d_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		if (!m_rtv_descriptor_size)
			return false;

		m_render_targets.resize(swapchain_desc.BufferCount);
		m_rtv_handles.resize(swapchain_desc.BufferCount);

		auto handle = m_rtv_heap->GetCPUDescriptorHandleForHeapStart();
		for (UINT index = 0; index < swapchain_desc.BufferCount; ++index)
		{
			m_rtv_handles[index] = handle;
			if (FAILED(m_dxgi_swapchain->GetBuffer(
				index,
				IID_PPV_ARGS(m_render_targets[index].ReleaseAndGetAddressOf()))))
			{
				cleanup_render_targets();
				return false;
			}

			m_d3d_device->CreateRenderTargetView(m_render_targets[index].Get(), nullptr, handle);
			handle.ptr += m_rtv_descriptor_size;
		}

		return true;
	}

	bool renderer::initialize_dx12_backend()
	{
		if (!m_d3d_device || !m_command_queue || !m_srv_heap || m_frames.empty())
			return false;

		ImGui_ImplDX12_InitInfo info{};
		info.Device = m_d3d_device.Get();
		info.CommandQueue = m_command_queue.Get();
		info.NumFramesInFlight = static_cast<int>(m_frames.size());
		info.RTVFormat = m_rtv_format;
		info.DSVFormat = DXGI_FORMAT_UNKNOWN;
		info.UserData = this;
		info.SrvDescriptorHeap = m_srv_heap.Get();
		info.SrvDescriptorAllocFn = &renderer::allocate_srv_descriptor;
		info.SrvDescriptorFreeFn = &renderer::free_srv_descriptor;

		if (!ImGui_ImplDX12_Init(&info))
			return false;

		m_dx12_backend_initialized = true;
		return true;
	}

	void renderer::on_present()
	{
		if (!ready() || !ImGui::GetCurrentContext())
			return;

		const UINT backbuffer_index = m_dxgi_swapchain->GetCurrentBackBufferIndex();
		if (backbuffer_index >= m_frames.size() || backbuffer_index >= m_render_targets.size())
			return;

		auto& frame = m_frames[backbuffer_index];
		wait_for_frame(frame);

		if (FAILED(frame.command_allocator->Reset()))
		{
			LOG_ERROR("Failed to reset the DX12 command allocator.");
			return;
		}
		if (FAILED(m_command_list->Reset(frame.command_allocator.Get(), nullptr)))
		{
			LOG_ERROR("Failed to reset the DX12 command list.");
			return;
		}

		auto& io = ImGui::GetIO();
		io.MouseDrawCursor = g_gui.m_opened;
		if (g_gui.m_opened)
			io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
		else
			io.ConfigFlags |= ImGuiConfigFlags_NoMouse;

		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		if (g_gui.m_opened)
			g_gui.dx_on_tick();

		ImGui::Render();

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = m_render_targets[backbuffer_index].Get();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		m_command_list->ResourceBarrier(1, &barrier);

		m_command_list->OMSetRenderTargets(1, &m_rtv_handles[backbuffer_index], FALSE, nullptr);
		ID3D12DescriptorHeap* descriptor_heaps[] = {m_srv_heap.Get()};
		m_command_list->SetDescriptorHeaps(1, descriptor_heaps);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_command_list.Get());

		std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
		m_command_list->ResourceBarrier(1, &barrier);

		if (FAILED(m_command_list->Close()))
		{
			LOG_ERROR("Failed to close the DX12 command list.");
			return;
		}

		ID3D12CommandList* command_lists[] = {m_command_list.Get()};
		m_command_queue->ExecuteCommandLists(1, command_lists);

		const auto fence_value = ++m_last_fence_value;
		if (FAILED(m_command_queue->Signal(m_fence.Get(), fence_value)))
		{
			LOG_ERROR("Failed to signal the DX12 renderer fence.");
			return;
		}
		frame.fence_value = fence_value;
	}

	void renderer::pre_reset()
	{
		if (!m_initialized)
			return;

		wait_for_gpu();
		if (m_dx12_backend_initialized)
			ImGui_ImplDX12_InvalidateDeviceObjects();
		cleanup_render_targets();
	}

	void renderer::post_reset()
	{
		if (!m_dxgi_swapchain || !m_d3d_device)
			return;

		DXGI_SWAP_CHAIN_DESC swapchain_desc{};
		if (FAILED(m_dxgi_swapchain->GetDesc(&swapchain_desc)) || !swapchain_desc.BufferCount)
		{
			m_initialized = false;
			LOG_ERROR("Failed to read the DX12 swapchain after ResizeBuffers.");
			return;
		}

		const bool frame_count_changed = swapchain_desc.BufferCount != m_frames.size();
		if (frame_count_changed)
		{
			shutdown_dx12_backend();
			if (!create_frame_resources(swapchain_desc.BufferCount))
			{
				m_initialized = false;
				LOG_ERROR("Failed to rebuild DX12 frame resources after ResizeBuffers.");
				return;
			}
		}

		if (!create_render_targets())
		{
			m_initialized = false;
			LOG_ERROR("Failed to rebuild DX12 render targets after ResizeBuffers.");
			return;
		}

		if (frame_count_changed)
		{
			if (!initialize_dx12_backend())
			{
				m_initialized = false;
				LOG_ERROR("Failed to reinitialize ImGui DX12 after the back-buffer count changed.");
			}
		}
		else if (m_dx12_backend_initialized && !ImGui_ImplDX12_CreateDeviceObjects())
		{
			m_initialized = false;
			LOG_ERROR("Failed to recreate ImGui DX12 device objects after ResizeBuffers.");
		}
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

	void renderer::cleanup_render_targets() noexcept
	{
		m_render_targets.clear();
		m_rtv_handles.clear();
		m_rtv_heap.Reset();
		m_rtv_descriptor_size = 0;
	}

	void renderer::cleanup_frame_resources() noexcept
	{
		m_command_list.Reset();
		m_frames.clear();
	}

	void renderer::shutdown_dx12_backend() noexcept
	{
		if (!m_dx12_backend_initialized)
			return;

		ImGui_ImplDX12_Shutdown();
		m_dx12_backend_initialized = false;
	}

	void renderer::wait_for_frame(frame_context& frame) noexcept
	{
		if (!frame.fence_value || !m_fence || !m_fence_event)
			return;

		const auto fence_value = frame.fence_value;
		if (m_fence->GetCompletedValue() < fence_value)
		{
			if (SUCCEEDED(m_fence->SetEventOnCompletion(fence_value, m_fence_event)))
				WaitForSingleObject(m_fence_event, INFINITE);
		}
		frame.fence_value = 0;
	}

	void renderer::wait_for_gpu() noexcept
	{
		if (!m_command_queue || !m_fence || !m_fence_event)
			return;

		const auto fence_value = ++m_last_fence_value;
		if (FAILED(m_command_queue->Signal(m_fence.Get(), fence_value)))
			return;
		if (m_fence->GetCompletedValue() < fence_value &&
			SUCCEEDED(m_fence->SetEventOnCompletion(fence_value, m_fence_event)))
		{
			WaitForSingleObject(m_fence_event, INFINITE);
		}

		for (auto& frame : m_frames)
			frame.fence_value = 0;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE renderer::srv_cpu_handle(std::uint32_t index) const noexcept
	{
		auto handle = m_srv_heap->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += static_cast<SIZE_T>(index) * m_srv_descriptor_size;
		return handle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE renderer::srv_gpu_handle(std::uint32_t index) const noexcept
	{
		auto handle = m_srv_heap->GetGPUDescriptorHandleForHeapStart();
		handle.ptr += static_cast<UINT64>(index) * m_srv_descriptor_size;
		return handle;
	}

	void renderer::allocate_srv_descriptor(
		ImGui_ImplDX12_InitInfo* info,
		D3D12_CPU_DESCRIPTOR_HANDLE* cpu_handle,
		D3D12_GPU_DESCRIPTOR_HANDLE* gpu_handle)
	{
		if (!cpu_handle || !gpu_handle)
			return;

		*cpu_handle = {};
		*gpu_handle = {};
		if (!info || !info->UserData)
			return;

		auto* self = static_cast<renderer*>(info->UserData);
		std::scoped_lock lock(self->m_srv_mutex);
		if (self->m_free_srv_descriptors.empty())
		{
			LOG_ERROR("The ImGui DX12 SRV descriptor heap is exhausted.");
			return;
		}

		const auto index = self->m_free_srv_descriptors.back();
		self->m_free_srv_descriptors.pop_back();
		self->m_srv_descriptor_in_use[index] = true;
		*cpu_handle = self->srv_cpu_handle(index);
		*gpu_handle = self->srv_gpu_handle(index);
	}

	void renderer::free_srv_descriptor(
		ImGui_ImplDX12_InitInfo* info,
		D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle,
		D3D12_GPU_DESCRIPTOR_HANDLE)
	{
		if (!info || !info->UserData || !cpu_handle.ptr)
			return;

		auto* self = static_cast<renderer*>(info->UserData);
		if (!self->m_srv_heap || !self->m_srv_descriptor_size)
			return;

		const auto base = self->m_srv_heap->GetCPUDescriptorHandleForHeapStart();
		if (cpu_handle.ptr < base.ptr)
			return;

		const auto offset = cpu_handle.ptr - base.ptr;
		if (offset % self->m_srv_descriptor_size != 0)
			return;

		const auto index = static_cast<std::uint32_t>(offset / self->m_srv_descriptor_size);
		if (index >= self->m_srv_descriptor_in_use.size())
			return;

		std::scoped_lock lock(self->m_srv_mutex);
		if (!self->m_srv_descriptor_in_use[index])
			return;

		self->m_srv_descriptor_in_use[index] = false;
		self->m_free_srv_descriptors.push_back(index);
	}
}
