#pragma once

#include <imgui.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace big::menu_pages
{
	struct vehicle_preview final
	{
		ImTextureID texture_id{};
		std::uint32_t width{};
		std::uint32_t height{};
		std::filesystem::path source_path;

		[[nodiscard]] explicit operator bool() const noexcept
		{
			return texture_id != ImTextureID{} && width > 0 && height > 0;
		}
	};

	[[nodiscard]] const vehicle_preview* get_vehicle_preview(
		std::string_view model_name,
		std::uint32_t model_hash);
	void refresh_vehicle_previews();
	void shutdown_vehicle_previews() noexcept;
	[[nodiscard]] const std::filesystem::path& vehicle_preview_directory();
	[[nodiscard]] const std::string& vehicle_preview_error();
}
