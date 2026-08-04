#include "Image_Loader_Manager.hpp"

#include "logger.hpp"

namespace big
{
	Image_Loader_Manager::~Image_Loader_Manager()
	{
		Shutdown();
	}

	bool Image_Loader_Manager::Initialize(ID3D11Device* device, std::string& error) noexcept
	{
		std::scoped_lock lock(m_mutex);
		error.clear();

		if (!device)
		{
			error = "A valid D3D11 device is required to initialize the image manager.";
			return false;
		}

		m_device = device;
		return true;
	}

	void Image_Loader_Manager::Shutdown() noexcept
	{
		std::scoped_lock lock(m_mutex);
		m_images.clear();
		m_device.Reset();
	}

	bool Image_Loader_Manager::Load(const std::string& name, const std::filesystem::path& path, std::string& error)
	{
		std::scoped_lock lock(m_mutex);
		error.clear();

		if (!m_device)
		{
			error = "The image manager is not initialized.";
			return false;
		}

		const auto key = NormalizeName(name);
		if (key.empty())
		{
			error = "An image name is required.";
			return false;
		}

		auto image = std::make_unique<Image_Loader>();
		if (!image->LoadFromFile(m_device.Get(), path, error))
			return false;

		m_images[key] = std::move(image);
		LOG_INFO("Loaded image '{}' from {}.", key, path.string());
		return true;
	}

	bool Image_Loader_Manager::LoadFromMemory(
		const std::string& name,
		const void* data,
		std::size_t size,
		std::string& error)
	{
		std::scoped_lock lock(m_mutex);
		error.clear();

		if (!m_device)
		{
			error = "The image manager is not initialized.";
			return false;
		}

		const auto key = NormalizeName(name);
		if (key.empty())
		{
			error = "An image name is required.";
			return false;
		}

		auto image = std::make_unique<Image_Loader>();
		if (!image->LoadFromMemory(m_device.Get(), data, size, error))
			return false;

		m_images[key] = std::move(image);
		LOG_INFO("Loaded image '{}' from memory.", key);
		return true;
	}

	bool Image_Loader_Manager::Reload(const std::string& name, std::string& error)
	{
		std::scoped_lock lock(m_mutex);

		const auto key = NormalizeName(name);
		const auto iterator = m_images.find(key);
		if (iterator == m_images.end())
		{
			error = "The requested image is not loaded.";
			return false;
		}

		const auto source_path = iterator->second->GetSourcePath();
		if (source_path.empty())
		{
			error = "Images loaded from memory cannot be reloaded from disk.";
			return false;
		}

		auto replacement = std::make_unique<Image_Loader>();
		if (!replacement->LoadFromFile(m_device.Get(), source_path, error))
			return false;

		iterator->second = std::move(replacement);
		LOG_INFO("Reloaded image '{}'.", key);
		return true;
	}

	bool Image_Loader_Manager::Remove(const std::string& name) noexcept
	{
		std::scoped_lock lock(m_mutex);
		return m_images.erase(NormalizeName(name)) > 0;
	}

	void Image_Loader_Manager::Clear() noexcept
	{
		std::scoped_lock lock(m_mutex);
		m_images.clear();
	}

	Image_Loader* Image_Loader_Manager::Get(const std::string& name) noexcept
	{
		std::scoped_lock lock(m_mutex);
		const auto iterator = m_images.find(NormalizeName(name));
		return iterator == m_images.end() ? nullptr : iterator->second.get();
	}

	const Image_Loader* Image_Loader_Manager::Get(const std::string& name) const noexcept
	{
		std::scoped_lock lock(m_mutex);
		const auto iterator = m_images.find(NormalizeName(name));
		return iterator == m_images.end() ? nullptr : iterator->second.get();
	}

	ImTextureID Image_Loader_Manager::GetTextureID(const std::string& name) const noexcept
	{
		const auto* image = Get(name);
		return image ? image->GetImGuiTextureID() : nullptr;
	}

	bool Image_Loader_Manager::Contains(const std::string& name) const noexcept
	{
		std::scoped_lock lock(m_mutex);
		return m_images.contains(NormalizeName(name));
	}

	std::size_t Image_Loader_Manager::Count() const noexcept
	{
		std::scoped_lock lock(m_mutex);
		return m_images.size();
	}

	bool Image_Loader_Manager::Ready() const noexcept
	{
		std::scoped_lock lock(m_mutex);
		return static_cast<bool>(m_device);
	}

	std::vector<std::string> Image_Loader_Manager::Names() const
	{
		std::scoped_lock lock(m_mutex);
		std::vector<std::string> names;
		names.reserve(m_images.size());
		for (const auto& [name, image] : m_images)
			names.push_back(name);

		std::sort(names.begin(), names.end());
		return names;
	}

	std::string Image_Loader_Manager::NormalizeName(std::string name)
	{
		name.erase(name.begin(), std::find_if(name.begin(), name.end(), [](unsigned char character)
		{
			return !std::isspace(character);
		}));
		name.erase(std::find_if(name.rbegin(), name.rend(), [](unsigned char character)
		{
			return !std::isspace(character);
		}).base(), name.end());

		std::transform(name.begin(), name.end(), name.begin(), [](unsigned char character)
		{
			return static_cast<char>(std::tolower(character));
		});
		return name;
	}
}
