#pragma once

#include <string_view>

namespace big::game_target
{
	inline constexpr std::string_view edition = "GTAV Enhanced";
	inline constexpr std::string_view executable = "GTA5_Enhanced.exe";
	inline constexpr std::string_view battleye_executable = "GTA5_Enhanced_BE.exe";
	inline constexpr std::string_view crossmap_prefix = "enhanced-";
	inline constexpr bool enhanced = true;
	inline constexpr bool legacy = false;
}
