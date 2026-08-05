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
		load_failed,
		queue_failed
	};

	enum class vehicle_action_status
	{
		idle,
		queued,
		completed,
		no_vehicle,
		queue_failed
	};

	struct vehicle_settings final
	{
		std::atomic_bool god_mode{};
		std::atomic_bool repair_loop{};
		std::atomic_bool seatbelt{true};
		std::atomic_bool horn_boost{};
		std::atomic_bool disable_gravity{};
		std::atomic_bool rainbow_paint{};
		std::atomic<float> acceleration_multiplier{1.0f};

		std::array<char, 64> spawn_model{'a', 'd', 'd', 'e', 'r', '\0'};
		std::array<char, 64> catalog_search{};
		bool spawn_inside{true};
		int category_index{};
		std::atomic<vehicle_spawn_status> spawn_status{vehicle_spawn_status::idle};
		std::atomic<vehicle_action_status> action_status{vehicle_action_status::idle};
	};

	inline vehicle_settings g_vehicle_settings;

	void draw_vehicle();
	void tick_vehicle();
	void queue_vehicle_spawn(std::string model_name, bool put_player_inside);
}
