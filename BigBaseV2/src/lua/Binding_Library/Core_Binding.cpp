#include "Core_Binding.hpp"

#include "../../logger.hpp"

namespace big::lua::bindings
{
	std::string_view Core_Binding::Name() const noexcept
	{
		return "core";
	}

	void Core_Binding::Bind(sol::state_view, sol::table& root)
	{
		sol::table core = root["core"].get_or_create<sol::table>();
		core["version"] = "BigBaseV2-Sol2";
		core.set_function("log_info", [](const std::string& message)
		{
			LOG_INFO("[Lua] {}", message);
		});
		core.set_function("log_error", [](const std::string& message)
		{
			LOG_ERROR("[Lua] {}", message);
		});
	}
}
