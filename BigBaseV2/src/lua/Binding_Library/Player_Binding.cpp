#include "Player_Binding.hpp"

#include "../../natives.hpp"

namespace big::lua::bindings
{
	std::string_view Player_Binding::Name() const noexcept
	{
		return "player";
	}

	void Player_Binding::Bind(sol::state_view, sol::table& root)
	{
		sol::table player = root["player"].get_or_create<sol::table>();
		player.set_function("id", []
		{
			return PLAYER::PLAYER_ID();
		});
		player.set_function("ped", []
		{
			return PLAYER::PLAYER_PED_ID();
		});
		player.set_function("wanted_level", []
		{
			return PLAYER::GET_PLAYER_WANTED_LEVEL(PLAYER::PLAYER_ID());
		});
		player.set_function("health", []
		{
			return ENTITY::GET_ENTITY_HEALTH(PLAYER::PLAYER_PED_ID());
		});
		player.set_function("max_health", []
		{
			return ENTITY::GET_ENTITY_MAX_HEALTH(PLAYER::PLAYER_PED_ID());
		});
	}
}
