#pragma once

#include "common.hpp"

#include <imgui.h>

namespace big
{
	class Image_Loader final
	{
	public:
		Image_Loader() = default;
		~Image_Loader() = default;

		Image_Loader(const Image_Loader&) = delete;
		Image_Loader(Image_Loader&&) noexcept = default;
		Image_Loader& operator=(const Image_Loader&) = delete;
		Image_Loader& operator=(Image_Loader&&) noexcept = default;

		bool LoadFromFile(ID3D11Device* device, const std::filesystem::path& path, std::string& error);
		bool LoadFromMemory(ID3D11Device* device, const void* data, std::size_t size, std::string& error);
		void Reset() noexcept;

		[[nodiscard]] bool IsLoaded() const noexcept;
		[[nodiscard]] ID3D11ShaderResourceView* GetShaderResourceView() const noexcept;
		[[nodiscard]] ImTextureID GetImGuiTextureID() const noexcept;
		[[nodiscard]] std::uint32_t GetWidth() const noexcept;
		[[nodiscard]] std::uint32_t GetHeight() const noexcept;
		[[nodiscard]] const std::filesystem::path& GetSourcePath() const noexcept;

	private:
		bool CreateTexture(
			ID3D11Device* device,
			const std::uint8_t* pixels,
			std::uint32_t width,
			std::uint32_t height,
			std::string& error);

		comptr<ID3D11Texture2D> m_texture;
		comptr<ID3D11ShaderResourceView> m_shader_resource_view;
		std::filesystem::path m_source_path;
		std::uint32_t m_width{};
		std::uint32_t m_height{};
	};
}
