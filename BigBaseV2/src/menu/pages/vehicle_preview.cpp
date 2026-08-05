#include "vehicle_preview.hpp"

#include "../../logger.hpp"
#include "../../renderer.hpp"

#include <wincodec.h>

namespace big::menu_pages
{
	namespace
	{
		constexpr std::size_t preview_cache_capacity = 12;
		constexpr std::array<const wchar_t*, 8> preview_extensions{
			L".png", L".jpg", L".jpeg", L".bmp", L".gif", L".tif", L".tiff", L".ico"};

		class com_scope final
		{
		public:
			com_scope() noexcept
			{
				const HRESULT result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
				m_uninitialize = SUCCEEDED(result);
				m_available = SUCCEEDED(result) || result == RPC_E_CHANGED_MODE;
			}

			~com_scope()
			{
				if (m_uninitialize)
					CoUninitialize();
			}

			[[nodiscard]] bool available() const noexcept
			{
				return m_available;
			}

		private:
			bool m_uninitialize{};
			bool m_available{};
		};

		class event_handle final
		{
		public:
			event_handle() noexcept : m_handle(CreateEventW(nullptr, FALSE, FALSE, nullptr))
			{
			}

			~event_handle()
			{
				if (m_handle)
					CloseHandle(m_handle);
			}

			[[nodiscard]] HANDLE get() const noexcept
			{
				return m_handle;
			}

		private:
			HANDLE m_handle{};
		};

		struct cached_preview final
		{
			vehicle_preview view;
			comptr<ID3D12Resource> texture;
			D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor{};
			bool attempted{};
			std::uint64_t last_used{};
		};

		std::filesystem::path module_directory()
		{
			std::wstring path(32768, L'\0');
			const DWORD length = GetModuleFileNameW(
				g_hmodule,
				path.data(),
				static_cast<DWORD>(path.size()));
			if (!length || length >= path.size())
				return std::filesystem::current_path();

			path.resize(length);
			return std::filesystem::path(path).parent_path();
		}

		std::string normalize_model_name(std::string_view model_name)
		{
			std::string normalized;
			normalized.reserve(model_name.size());
			for (const unsigned char character : model_name)
			{
				if (std::isalnum(character) || character == '_' || character == '-')
					normalized.push_back(static_cast<char>(std::tolower(character)));
			}
			return normalized;
		}

		std::string hexadecimal_hash(std::uint32_t model_hash, bool prefix)
		{
			std::ostringstream stream;
			if (prefix)
				stream << "0x";
			stream << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << model_hash;
			return stream.str();
		}

		bool decode_image(
			const std::filesystem::path& path,
			std::vector<std::uint8_t>& pixels,
			std::uint32_t& width,
			std::uint32_t& height,
			std::string& error)
		{
			com_scope com;
			if (!com.available())
			{
				error = "COM initialization failed while loading the vehicle preview.";
				return false;
			}

			comptr<IWICImagingFactory> factory;
			if (FAILED(CoCreateInstance(
				CLSID_WICImagingFactory,
				nullptr,
				CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(factory.GetAddressOf()))))
			{
				error = "Failed to create the Windows Imaging Component factory.";
				return false;
			}

			comptr<IWICBitmapDecoder> decoder;
			if (FAILED(factory->CreateDecoderFromFilename(
				path.c_str(),
				nullptr,
				GENERIC_READ,
				WICDecodeMetadataCacheOnLoad,
				decoder.GetAddressOf())) || !decoder)
			{
				error = "WIC could not decode the vehicle preview image.";
				return false;
			}

			comptr<IWICBitmapFrameDecode> frame;
			if (FAILED(decoder->GetFrame(0, frame.GetAddressOf())) || !frame)
			{
				error = "Failed to read the first vehicle preview image frame.";
				return false;
			}

			UINT decoded_width{};
			UINT decoded_height{};
			if (FAILED(frame->GetSize(&decoded_width, &decoded_height)) || !decoded_width || !decoded_height)
			{
				error = "The vehicle preview image has invalid dimensions.";
				return false;
			}

			constexpr std::uint64_t maximum_dimension = 16384;
			if (decoded_width > maximum_dimension || decoded_height > maximum_dimension)
			{
				error = "The vehicle preview exceeds the 16384-pixel dimension limit.";
				return false;
			}

			comptr<IWICFormatConverter> converter;
			if (FAILED(factory->CreateFormatConverter(converter.GetAddressOf())) || !converter)
			{
				error = "Failed to create the WIC vehicle preview converter.";
				return false;
			}

			if (FAILED(converter->Initialize(
				frame.Get(),
				GUID_WICPixelFormat32bppRGBA,
				WICBitmapDitherTypeNone,
				nullptr,
				0.0,
				WICBitmapPaletteTypeCustom)))
			{
				error = "Failed to convert the vehicle preview to RGBA pixels.";
				return false;
			}

			const std::uint64_t stride = static_cast<std::uint64_t>(decoded_width) * 4;
			const std::uint64_t size = stride * decoded_height;
			if (stride > UINT_MAX || size > UINT_MAX || size > std::numeric_limits<std::size_t>::max())
			{
				error = "The decoded vehicle preview is too large.";
				return false;
			}

			pixels.resize(static_cast<std::size_t>(size));
			if (FAILED(converter->CopyPixels(
				nullptr,
				static_cast<UINT>(stride),
				static_cast<UINT>(size),
				pixels.data())))
			{
				error = "Failed to copy the decoded vehicle preview pixels.";
				return false;
			}

			width = decoded_width;
			height = decoded_height;
			return true;
		}

		bool upload_texture(
			const std::vector<std::uint8_t>& pixels,
			std::uint32_t width,
			std::uint32_t height,
			cached_preview& output,
			std::string& error)
		{
			if (!g_renderer || !g_renderer->ready())
			{
				error = "The DX12 renderer is not ready for vehicle preview uploads.";
				return false;
			}

			auto* device = g_renderer->d3d_device();
			auto* queue = g_renderer->command_queue();
			if (!device || !queue || pixels.empty() || !width || !height)
			{
				error = "Invalid DX12 vehicle preview upload arguments.";
				return false;
			}

			D3D12_HEAP_PROPERTIES default_heap{};
			default_heap.Type = D3D12_HEAP_TYPE_DEFAULT;
			default_heap.CreationNodeMask = 1;
			default_heap.VisibleNodeMask = 1;

			D3D12_RESOURCE_DESC texture_description{};
			texture_description.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			texture_description.Width = width;
			texture_description.Height = height;
			texture_description.DepthOrArraySize = 1;
			texture_description.MipLevels = 1;
			texture_description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			texture_description.SampleDesc.Count = 1;
			texture_description.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

			comptr<ID3D12Resource> texture;
			if (FAILED(device->CreateCommittedResource(
				&default_heap,
				D3D12_HEAP_FLAG_NONE,
				&texture_description,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(texture.GetAddressOf()))))
			{
				error = "DX12 failed to create the vehicle preview texture.";
				return false;
			}

			D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
			UINT row_count{};
			UINT64 row_size{};
			UINT64 upload_size{};
			device->GetCopyableFootprints(
				&texture_description,
				0,
				1,
				0,
				&footprint,
				&row_count,
				&row_size,
				&upload_size);
			if (!upload_size || row_count != height || row_size < static_cast<UINT64>(width) * 4)
			{
				error = "DX12 returned an invalid vehicle preview upload footprint.";
				return false;
			}

			D3D12_HEAP_PROPERTIES upload_heap{};
			upload_heap.Type = D3D12_HEAP_TYPE_UPLOAD;
			upload_heap.CreationNodeMask = 1;
			upload_heap.VisibleNodeMask = 1;

			D3D12_RESOURCE_DESC upload_description{};
			upload_description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			upload_description.Width = upload_size;
			upload_description.Height = 1;
			upload_description.DepthOrArraySize = 1;
			upload_description.MipLevels = 1;
			upload_description.SampleDesc.Count = 1;
			upload_description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

			comptr<ID3D12Resource> upload_buffer;
			if (FAILED(device->CreateCommittedResource(
				&upload_heap,
				D3D12_HEAP_FLAG_NONE,
				&upload_description,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(upload_buffer.GetAddressOf()))))
			{
				error = "DX12 failed to create the vehicle preview upload buffer.";
				return false;
			}

			void* mapped{};
			const D3D12_RANGE read_range{0, 0};
			if (FAILED(upload_buffer->Map(0, &read_range, &mapped)) || !mapped)
			{
				error = "DX12 failed to map the vehicle preview upload buffer.";
				return false;
			}

			const std::size_t source_pitch = static_cast<std::size_t>(width) * 4;
			auto* destination = static_cast<std::uint8_t*>(mapped) + footprint.Offset;
			for (std::uint32_t row = 0; row < height; ++row)
			{
				std::memcpy(
					destination + static_cast<std::size_t>(row) * footprint.Footprint.RowPitch,
					pixels.data() + static_cast<std::size_t>(row) * source_pitch,
					source_pitch);
			}
			const D3D12_RANGE written_range{
				static_cast<SIZE_T>(footprint.Offset),
				static_cast<SIZE_T>(footprint.Offset + static_cast<UINT64>(footprint.Footprint.RowPitch) * height)};
			upload_buffer->Unmap(0, &written_range);

			comptr<ID3D12CommandAllocator> allocator;
			comptr<ID3D12GraphicsCommandList> command_list;
			if (FAILED(device->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(allocator.GetAddressOf()))) ||
				FAILED(device->CreateCommandList(
					0,
					D3D12_COMMAND_LIST_TYPE_DIRECT,
					allocator.Get(),
					nullptr,
					IID_PPV_ARGS(command_list.GetAddressOf()))))
			{
				error = "DX12 failed to create the vehicle preview upload command list.";
				return false;
			}

			D3D12_TEXTURE_COPY_LOCATION destination_location{};
			destination_location.pResource = texture.Get();
			destination_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			destination_location.SubresourceIndex = 0;

			D3D12_TEXTURE_COPY_LOCATION source_location{};
			source_location.pResource = upload_buffer.Get();
			source_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			source_location.PlacedFootprint = footprint;
			command_list->CopyTextureRegion(
				&destination_location,
				0,
				0,
				0,
				&source_location,
				nullptr);

			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Transition.pResource = texture.Get();
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			command_list->ResourceBarrier(1, &barrier);

			if (FAILED(command_list->Close()))
			{
				error = "DX12 failed to close the vehicle preview upload command list.";
				return false;
			}

			comptr<ID3D12Fence> upload_fence;
			event_handle upload_event;
			if (FAILED(device->CreateFence(
				0,
				D3D12_FENCE_FLAG_NONE,
				IID_PPV_ARGS(upload_fence.GetAddressOf()))) || !upload_event.get())
			{
				error = "DX12 failed to create vehicle preview upload synchronization.";
				return false;
			}

			ID3D12CommandList* command_lists[]{command_list.Get()};
			queue->ExecuteCommandLists(1, command_lists);
			if (FAILED(queue->Signal(upload_fence.Get(), 1)))
			{
				g_renderer->synchronize_gpu();
				error = "DX12 failed to signal the vehicle preview upload fence.";
				return false;
			}

			if (upload_fence->GetCompletedValue() < 1)
			{
				if (FAILED(upload_fence->SetEventOnCompletion(1, upload_event.get())) ||
					WaitForSingleObject(upload_event.get(), INFINITE) != WAIT_OBJECT_0)
				{
					g_renderer->synchronize_gpu();
					error = "DX12 failed while waiting for the vehicle preview upload.";
					return false;
				}
			}

			D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor{};
			D3D12_GPU_DESCRIPTOR_HANDLE gpu_descriptor{};
			if (!g_renderer->reserve_srv_descriptor(cpu_descriptor, gpu_descriptor))
			{
				error = "The shared DX12 SRV heap has no free descriptor for the vehicle preview.";
				return false;
			}

			D3D12_SHADER_RESOURCE_VIEW_DESC view_description{};
			view_description.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			view_description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			view_description.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			view_description.Texture2D.MipLevels = 1;
			device->CreateShaderResourceView(texture.Get(), &view_description, cpu_descriptor);

			output.texture = std::move(texture);
			output.cpu_descriptor = cpu_descriptor;
			output.view.texture_id = static_cast<ImTextureID>(gpu_descriptor.ptr);
			output.view.width = width;
			output.view.height = height;
			return true;
		}

		class vehicle_preview_cache final
		{
		public:
			~vehicle_preview_cache()
			{
				shutdown();
			}

			const vehicle_preview* get(std::string_view model_name, std::uint32_t model_hash)
			{
				ensure_directory();
				const std::string key = normalize_model_name(model_name) + ':' + std::to_string(model_hash);
				auto& entry = m_entries[key];
				entry.last_used = ++m_usage_counter;
				if (entry.attempted)
					return entry.view ? &entry.view : nullptr;

				entry.attempted = true;
				const auto path = find_path(model_name, model_hash);
				if (path.empty())
				{
					trim();
					return nullptr;
				}

				std::vector<std::uint8_t> pixels;
				std::uint32_t width{};
				std::uint32_t height{};
				if (!decode_image(path, pixels, width, height, m_last_error) ||
					!upload_texture(pixels, width, height, entry, m_last_error))
				{
					trim();
					return nullptr;
				}

				entry.view.source_path = path;
				m_last_error.clear();
				LOG_INFO("Loaded DX12 vehicle preview for '{}' from {}.", model_name, path.string());
				trim();
				return &entry.view;
			}

			void refresh()
			{
				clear_entries();
				m_last_error.clear();
				ensure_directory(true);
			}

			void shutdown() noexcept
			{
				clear_entries();
				m_last_error.clear();
			}

			const std::filesystem::path& directory()
			{
				ensure_directory();
				return m_directory;
			}

			const std::string& error() const noexcept
			{
				return m_last_error;
			}

		private:
			void ensure_directory(bool recreate = false)
			{
				if (m_directory.empty())
					m_directory = module_directory() / L"Images" / L"Vehicles";
				if (!recreate && m_directory_ready)
					return;

				std::error_code error;
				std::filesystem::create_directories(m_directory, error);
				m_directory_ready = !error;
				if (error)
					m_last_error = "Could not create the vehicle preview directory: " + error.message();
			}

			std::filesystem::path find_path(std::string_view model_name, std::uint32_t model_hash) const
			{
				const std::array<std::string, 4> stems{
					normalize_model_name(model_name),
					hexadecimal_hash(model_hash, false),
					hexadecimal_hash(model_hash, true),
					std::to_string(model_hash)};

				for (const auto& stem : stems)
				{
					if (stem.empty())
						continue;
					for (const auto* extension : preview_extensions)
					{
						auto candidate = m_directory / std::filesystem::path(stem);
						candidate.replace_extension(extension);
						std::error_code error;
						if (std::filesystem::is_regular_file(candidate, error) && !error)
							return candidate;
					}
				}
				return {};
			}

			void release_entry(cached_preview& entry, bool synchronize) noexcept
			{
				if (entry.texture && g_renderer)
				{
					if (synchronize)
						g_renderer->synchronize_gpu();
					g_renderer->release_srv_descriptor(entry.cpu_descriptor);
				}
				entry.texture.Reset();
				entry.cpu_descriptor = {};
				entry.view = {};
			}

			void clear_entries() noexcept
			{
				if (!m_entries.empty() && g_renderer)
					g_renderer->synchronize_gpu();
				for (auto& [key, entry] : m_entries)
				{
					(void)key;
					release_entry(entry, false);
				}
				m_entries.clear();
				m_usage_counter = 0;
			}

			void trim()
			{
				while (m_entries.size() > preview_cache_capacity)
				{
					auto oldest = std::min_element(
						m_entries.begin(),
						m_entries.end(),
						[](const auto& left, const auto& right)
						{
							return left.second.last_used < right.second.last_used;
						});
					if (oldest == m_entries.end())
						break;
					release_entry(oldest->second, true);
					m_entries.erase(oldest);
				}
			}

			std::filesystem::path m_directory;
			std::unordered_map<std::string, cached_preview> m_entries;
			std::string m_last_error;
			std::uint64_t m_usage_counter{};
			bool m_directory_ready{};
		};

		vehicle_preview_cache g_vehicle_preview_cache;
	}

	const vehicle_preview* get_vehicle_preview(std::string_view model_name, std::uint32_t model_hash)
	{
		return g_vehicle_preview_cache.get(model_name, model_hash);
	}

	void refresh_vehicle_previews()
	{
		g_vehicle_preview_cache.refresh();
	}

	void shutdown_vehicle_previews() noexcept
	{
		g_vehicle_preview_cache.shutdown();
	}

	const std::filesystem::path& vehicle_preview_directory()
	{
		return g_vehicle_preview_cache.directory();
	}

	const std::string& vehicle_preview_error()
	{
		return g_vehicle_preview_cache.error();
	}
}
