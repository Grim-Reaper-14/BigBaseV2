#pragma once

#include "common.hpp"

namespace big::lua
{
	struct Lua_Script_Info final
	{
		std::filesystem::path path;
		bool loaded{};
	};

	class Lua_Scripts_Manager final
	{
	public:
		static Lua_Scripts_Manager& Instance();

		void Initialize(std::filesystem::path scripts_directory = {});
		void Refresh();

		bool Load(std::size_t index, bool sandbox, std::string& status);
		bool Reload(std::size_t index, bool sandbox, std::string& status);
		bool Unload(std::size_t index, std::string& status);
		void UnloadAll();

		[[nodiscard]] const std::filesystem::path& Directory() const noexcept;
		[[nodiscard]] const std::vector<Lua_Script_Info>& Scripts() const noexcept;

	private:
		std::filesystem::path m_directory;
		std::vector<Lua_Script_Info> m_scripts;
	};
}
