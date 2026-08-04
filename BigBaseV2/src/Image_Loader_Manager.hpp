#pragma once

#include "Image_Loader.hpp"

namespace big
{
	class Image_Loader_Manager final
	{
	public:
		Image_Loader_Manager() = default;
		~Image_Loader_Manager();

		Image_Loader_Manager(const Image_Loader_Manager&) = delete;
		Image_Loader_Manager(Image_Loader_Manager&&) = delete;
		Image_Loader_Manager& operator=(const Image_Loader_Manager&) = delete;
		Image_Loader_Manager& operator=(Image_Loader_Manager&&) = delete;

		bool Initialize(ID3D11Device* device, std::string& error) noexcept;
		void Shutdown() noexcept;

		bool Load(const std::string& name, const std::filesystem::path& path, std::string& error);
		bool LoadFromMemory(const std::string& name, const void* data, std::size_t size, std::string& error);
		bool Reload(const std::string& name, std::string& error);
		bool Remove(const std::string& name) noexcept;
		void Clear() noexcept;

		[[nodiscard]] Image_Loader* Get(const std::string& name) noexcept;
		[[nodiscard]] const Image_Loader* Get(const std::string& name) const noexcept;
		[[nodiscard]] ImTextureID GetTextureID(const std::string& name) const noexcept;
		[[nodiscard]] bool Contains(const std::string& name) const noexcept;
		[[nodiscard]] std::size_t Count() const noexcept;
		[[nodiscard]] bool Ready() const noexcept;
		[[nodiscard]] std::vector<std::string> Names() const;

	private:
		[[nodiscard]] static std::string NormalizeName(std::string name);

		mutable std::recursive_mutex m_mutex;
		comptr<ID3D11Device> m_device;
		std::unordered_map<std::string, std::unique_ptr<Image_Loader>> m_images;
	};

	inline Image_Loader_Manager g_image_loader_manager;
}
