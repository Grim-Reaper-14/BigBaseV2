#pragma once
#include "common.hpp"
#include <imgui.h>

struct ImGui_ImplDX12_InitInfo;

namespace big
{
	class renderer final
	{
		struct frame_context final
		{
			comptr<ID3D12CommandAllocator> command_allocator;
			std::uint64_t fence_value{};
		};

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
			return m_initialized && m_dxgi_swapchain && m_d3d_device && m_command_queue &&
				m_command_list && m_rtv_heap && m_srv_heap && !m_frames.empty() &&
				m_render_targets.size() == m_frames.size();
		}

		ImFont* m_font{};
		ImFont* m_monospace_font{};

	private:
		static constexpr std::uint32_t srv_descriptor_count = 256;

		bool create_srv_heap();
		bool create_frame_resources(UINT buffer_count);
		bool create_render_targets();
		bool initialize_dx12_backend();

		void cleanup_render_targets() noexcept;
		void cleanup_frame_resources() noexcept;
		void shutdown_dx12_backend() noexcept;
		void wait_for_frame(frame_context& frame) noexcept;
		void wait_for_gpu() noexcept;

		[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE srv_cpu_handle(std::uint32_t index) const noexcept;
		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE srv_gpu_handle(std::uint32_t index) const noexcept;

		static void allocate_srv_descriptor(
			ImGui_ImplDX12_InitInfo* info,
			D3D12_CPU_DESCRIPTOR_HANDLE* cpu_handle,
			D3D12_GPU_DESCRIPTOR_HANDLE* gpu_handle);
		static void free_srv_descriptor(
			ImGui_ImplDX12_InitInfo* info,
			D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle,
			D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle);

		bool m_initialized{};
		bool m_dx12_backend_initialized{};
		bool m_win32_backend_initialized{};

		comptr<IDXGISwapChain3> m_dxgi_swapchain;
		comptr<ID3D12Device> m_d3d_device;
		comptr<ID3D12CommandQueue> m_command_queue;
		comptr<ID3D12GraphicsCommandList> m_command_list;
		comptr<ID3D12DescriptorHeap> m_rtv_heap;
		comptr<ID3D12DescriptorHeap> m_srv_heap;
		comptr<ID3D12Fence> m_fence;

		HANDLE m_fence_event{};
		std::uint64_t m_last_fence_value{};
		UINT m_rtv_descriptor_size{};
		UINT m_srv_descriptor_size{};
		DXGI_FORMAT m_rtv_format{DXGI_FORMAT_R8G8B8A8_UNORM};

		std::vector<frame_context> m_frames;
		std::vector<comptr<ID3D12Resource>> m_render_targets;
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_rtv_handles;

		std::mutex m_srv_mutex;
		std::vector<std::uint32_t> m_free_srv_descriptors;
		std::vector<bool> m_srv_descriptor_in_use;
	};

	inline renderer* g_renderer{};
}
