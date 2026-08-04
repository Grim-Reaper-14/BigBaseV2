#pragma once

#include <array>
#include <atomic>
#include <string>

namespace big::menu_pages
{
	enum class vehicle_spawn_status
	{
		idle,
		loading,
		spawned,
		invalid_model,
		load_failed
	};

	struct vehicle_settings final
	{
		bool god_mode{};
		bool repair_loop{};
		bool seatbelt{};
		bool horn_boost{};
		bool rainbow_paint{};
		float acceleration_multiplier{1.0f};
		float gravity_multiplier{1.0f};

		std::array<char, 64> spawn_model{"adder"};
		bool spawn_inside{true};
		bool spawn_networked{true};
		std::atomic<vehicle_spawn_status> spawn_status{vehicle_spawn_status::idle};
	};

	inline vehicle_settings g_vehicle_settings;

	void draw_vehicle();
	void queue_vehicle_spawn(std::string model_name, bool put_player_inside, bool networked);
}
