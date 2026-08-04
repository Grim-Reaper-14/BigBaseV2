#include "Image_Loader.hpp"

#include "logger.hpp"

#include <imgui.h>
#include <wincodec.h>

namespace big
{
	namespace
	{
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

		bool decode_frame(
			IWICBitmapFrameDecode* frame,
			std::vector<std::uint8_t>& pixels,
			std::uint32_t& width,
			std::uint32_t& height,
			std::string& error)
		{
			if (!frame)
			{
				error = "The image decoder returned no frame.";
				return false;
			}

			UINT decoded_width{};
			UINT decoded_height{};
			if (FAILED(frame->GetSize(&decoded_width, &decoded_height)) || decoded_width == 0 || decoded_height == 0)
			{
				error = "The image has invalid dimensions.";
				return false;
			}

			constexpr std::uint64_t max_dimension = 16384;
			if (decoded_width > max_dimension || decoded_height > max_dimension)
			{
				error = "The image dimensions exceed the supported maximum of 16384 pixels.";
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

			comptr<IWICFormatConverter> converter;
			if (FAILED(factory->CreateFormatConverter(converter.GetAddressOf())) || !converter)
			{
				error = "Failed to create the WIC pixel-format converter.";
				return false;
			}

			if (FAILED(converter->Initialize(
				frame,
				GUID_WICPixelFormat32bppRGBA,
				WICBitmapDitherTypeNone,
				nullptr,
				0.0,
				WICBitmapPaletteTypeCustom)))
			{
				error = "Failed to convert the image to RGBA pixels.";
				return false;
			}

			const std::uint64_t stride64 = static_cast<std::uint64_t>(decoded_width) * 4;
			const std::uint64_t size64 = stride64 * decoded_height;
			if (stride64 > UINT_MAX || size64 > UINT_MAX || size64 > std::numeric_limits<std::size_t>::max())
			{
				error = "The decoded image is too large.";
				return false;
			}

			pixels.resize(static_cast<std::size_t>(size64));
			if (FAILED(converter->CopyPixels(
				nullptr,
				static_cast<UINT>(stride64),
				static_cast<UINT>(size64),
				pixels.data())))
			{
				error = "Failed to copy decoded image pixels.";
				return false;
			}

			width = decoded_width;
			height = decoded_height;
			return true;
		}
	}

	bool Image_Loader::LoadFromFile(ID3D11Device* device, const std::filesystem::path& path, std::string& error)
	{
		Reset();
		error.clear();

		if (!device)
		{
			error = "A valid D3D11 device is required.";
			return false;
		}

		std::error_code filesystem_error;
		if (!std::filesystem::is_regular_file(path, filesystem_error) || filesystem_error)
		{
			error = "The image file does not exist or cannot be accessed.";
			return false;
		}

		com_scope com;
		if (!com.available())
		{
			error = "COM initialization failed.";
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
			error = "WIC could not decode the selected image file.";
			return false;
		}

		comptr<IWICBitmapFrameDecode> frame;
		if (FAILED(decoder->GetFrame(0, frame.GetAddressOf())))
		{
			error = "Failed to read the first image frame.";
			return false;
		}

		std::vector<std::uint8_t> pixels;
		std::uint32_t width{};
		std::uint32_t height{};
		if (!decode_frame(frame.Get(), pixels, width, height, error) ||
			!CreateTexture(device, pixels.data(), width, height, error))
		{
			Reset();
			return false;
		}

		m_source_path = std::filesystem::weakly_canonical(path, filesystem_error);
		if (filesystem_error)
			m_source_path = path.lexically_normal();

		return true;
	}

	bool Image_Loader::LoadFromMemory(ID3D11Device* device, const void* data, std::size_t size, std::string& error)
	{
		Reset();
		error.clear();

		if (!device || !data || size == 0)
		{
			error = "A valid D3D11 device and non-empty image buffer are required.";
			return false;
		}

		if (size > UINT_MAX)
		{
			error = "The image buffer is too large for WIC.";
			return false;
		}

		com_scope com;
		if (!com.available())
		{
			error = "COM initialization failed.";
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

		comptr<IWICStream> stream;
		if (FAILED(factory->CreateStream(stream.GetAddressOf())) ||
			FAILED(stream->InitializeFromMemory(
				static_cast<BYTE*>(const_cast<void*>(data)),
				static_cast<DWORD>(size))))
		{
			error = "Failed to create a WIC stream from the image buffer.";
			return false;
		}

		comptr<IWICBitmapDecoder> decoder;
		if (FAILED(factory->CreateDecoderFromStream(
			stream.Get(),
			nullptr,
			WICDecodeMetadataCacheOnLoad,
			decoder.GetAddressOf())) || !decoder)
		{
			error = "WIC could not decode the image buffer.";
			return false;
		}

		comptr<IWICBitmapFrameDecode> frame;
		if (FAILED(decoder->GetFrame(0, frame.GetAddressOf())))
		{
			error = "Failed to read the first image frame.";
			return false;
		}

		std::vector<std::uint8_t> pixels;
		std::uint32_t width{};
		std::uint32_t height{};
		if (!decode_frame(frame.Get(), pixels, width, height, error) ||
			!CreateTexture(device, pixels.data(), width, height, error))
		{
			Reset();
			return false;
		}

		return true;
	}

	void Image_Loader::Reset() noexcept
	{
		m_shader_resource_view.Reset();
		m_texture.Reset();
		m_source_path.clear();
		m_width = 0;
		m_height = 0;
	}

	bool Image_Loader::IsLoaded() const noexcept
	{
		return m_texture && m_shader_resource_view && m_width > 0 && m_height > 0;
	}

	ID3D11ShaderResourceView* Image_Loader::GetShaderResourceView() const noexcept
	{
		return m_shader_resource_view.Get();
	}

	ImTextureID Image_Loader::GetImGuiTextureID() const noexcept
	{
		return static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(m_shader_resource_view.Get()));
	}

	std::uint32_t Image_Loader::GetWidth() const noexcept
	{
		return m_width;
	}

	std::uint32_t Image_Loader::GetHeight() const noexcept
	{
		return m_height;
	}

	const std::filesystem::path& Image_Loader::GetSourcePath() const noexcept
	{
		return m_source_path;
	}

	bool Image_Loader::CreateTexture(
		ID3D11Device* device,
		const std::uint8_t* pixels,
		std::uint32_t width,
		std::uint32_t height,
		std::string& error)
	{
		if (!device || !pixels || width == 0 || height == 0)
		{
			error = "Invalid texture creation arguments.";
			return false;
		}

		D3D11_TEXTURE2D_DESC texture_description{};
		texture_description.Width = width;
		texture_description.Height = height;
		texture_description.MipLevels = 1;
		texture_description.ArraySize = 1;
		texture_description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texture_description.SampleDesc.Count = 1;
		texture_description.Usage = D3D11_USAGE_IMMUTABLE;
		texture_description.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		D3D11_SUBRESOURCE_DATA initial_data{};
		initial_data.pSysMem = pixels;
		initial_data.SysMemPitch = width * 4;

		if (FAILED(device->CreateTexture2D(
			&texture_description,
			&initial_data,
			m_texture.GetAddressOf())) || !m_texture)
		{
			error = "D3D11 failed to create the image texture.";
			return false;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC view_description{};
		view_description.Format = texture_description.Format;
		view_description.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		view_description.Texture2D.MipLevels = 1;

		if (FAILED(device->CreateShaderResourceView(
			m_texture.Get(),
			&view_description,
			m_shader_resource_view.GetAddressOf())) || !m_shader_resource_view)
		{
			error = "D3D11 failed to create the image shader-resource view.";
			m_texture.Reset();
			return false;
		}

		m_width = width;
		m_height = height;
		return true;
	}
}
