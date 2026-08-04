#include "Lua_commands.hpp"

#include <cctype>

namespace big::lua
{
	Lua_commands& Lua_commands::Instance()
	{
		static Lua_commands instance;
		return instance;
	}

	void Lua_commands::Register(std::string name, command_handler handler)
	{
		std::transform(name.begin(), name.end(), name.begin(), [](unsigned char character)
		{
			return static_cast<char>(std::tolower(character));
		});

		std::scoped_lock lock(m_mutex);
		m_commands[std::move(name)] = std::move(handler);
	}

	bool Lua_commands::Execute(std::string_view command_line, std::string& result) const
	{
		auto tokens = Tokenize(command_line);
		if (tokens.empty())
		{
			result = "No command was provided.";
			return false;
		}

		std::transform(tokens.front().begin(), tokens.front().end(), tokens.front().begin(), [](unsigned char character)
		{
			return static_cast<char>(std::tolower(character));
		});

		command_handler handler;
		{
			std::scoped_lock lock(m_mutex);
			const auto iterator = m_commands.find(tokens.front());
			if (iterator == m_commands.end())
			{
				result = "Unknown command: " + tokens.front();
				return false;
			}
			handler = iterator->second;
		}

		tokens.erase(tokens.begin());
		return handler(tokens, result);
	}

	void Lua_commands::Clear()
	{
		std::scoped_lock lock(m_mutex);
		m_commands.clear();
	}

	std::vector<std::string> Lua_commands::Names() const
	{
		std::scoped_lock lock(m_mutex);
		std::vector<std::string> names;
		names.reserve(m_commands.size());
		for (const auto& [name, handler] : m_commands)
			names.push_back(name);
		std::sort(names.begin(), names.end());
		return names;
	}

	std::vector<std::string> Lua_commands::Tokenize(std::string_view command_line)
	{
		std::vector<std::string> tokens;
		std::string token;
		bool quoted = false;

		for (const char character : command_line)
		{
			if (character == '"')
			{
				quoted = !quoted;
				continue;
			}

			if (!quoted && std::isspace(static_cast<unsigned char>(character)))
			{
				if (!token.empty())
				{
					tokens.push_back(std::move(token));
					token.clear();
				}
				continue;
			}

			token.push_back(character);
		}

		if (!token.empty())
			tokens.push_back(std::move(token));

		return tokens;
	}
}
