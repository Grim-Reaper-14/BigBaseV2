#pragma once

#include "common.hpp"

namespace big::lua
{
	class Lua_commands final
	{
	public:
		using command_handler = std::function<bool(const std::vector<std::string>&, std::string&)>;

		static Lua_commands& Instance();

		void Register(std::string name, command_handler handler);
		bool Execute(std::string_view command_line, std::string& result) const;
		void Clear();

		[[nodiscard]] std::vector<std::string> Names() const;

	private:
		static std::vector<std::string> Tokenize(std::string_view command_line);

		mutable std::mutex m_mutex;
		std::unordered_map<std::string, command_handler> m_commands;
	};
}
