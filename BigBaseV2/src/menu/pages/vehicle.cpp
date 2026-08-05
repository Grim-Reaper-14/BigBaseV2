#include "vehicle.hpp"
#include "vehicle_catalog.hpp"
#include "../widgets.hpp"

#include "../../fiber_pool.hpp"
#include "../../natives.hpp"
#include "../../script.hpp"

#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstring>
#include <string>
#include <string_view>

namespace big::menu_pages
{
	namespace
	{
		enum class current_vehicle_action
		{
			repair,
			clean
		};

		std::uint32_t joaat(std::string_view value) noexcept
		{
			std::uint32_t hash{};
			for (char character : value)
			{
				const auto lowered = static_cast<std::uint8_t>(
					character >= 'A' && character <= 'Z' ? character + ('a' - 'A') : character);
				hash += lowered;
				hash += hash << 10;
				hash ^= hash >> 6;
			}
			hash += hash << 3;
			hash ^= hash >> 11;
			hash += hash << 15;
			return hash;
		}

		bool contains_case_insensitive(std::string_view value, std::string_view query)
		{
			if (query.empty())
				return true;

			return std::search(
				value.begin(),
				value.end(),
				query.begin(),
				query.end(),
				[](char left, char right)
				{
					return std::tolower(static_cast<unsigned char>(left)) ==
						std::tolower(static_cast<unsigned char>(right));
				}) != value.end();
		}

		const char* spawn_status_text(vehicle_spawn_status status) noexcept
		{
			switch (status)
			{
			case vehicle_spawn_status::idle: return "Ready";
			case vehicle_spawn_status::loading: return "Loading model...";
			case vehicle_spawn_status::spawned: return "Vehicle spawned";
			case vehicle_spawn_status::invalid_model: return "Invalid vehicle model";
			case vehicle_spawn_status::load_failed: return "Model loading failed";
			case vehicle_spawn_status::queue_failed: return "Feature queue unavailable";
			}
			return "Unknown";
		}

		const char* action_status_text(vehicle_action_status status) noexcept
		{
			switch (status)
			{
			case vehicle_action_status::idle: return "Ready";
			case vehicle_action_status::queued: return "Action queued";
			case vehicle_action_status::completed: return "Action completed";
			case vehicle_action_status::no_vehicle: return "Enter a vehicle first";
			case vehicle_action_status::queue_failed: return "Feature queue unavailable";
			}
			return "Unknown";
		}

		void set_model_name(const char* model)
		{
			std::fill(g_vehicle_settings.spawn_model.begin(), g_vehicle_settings.spawn_model.end(), '\0');
			if (!model)
				return;

			std::copy_n(
				model,
				std::min<std::size_t>(std::strlen(model), g_vehicle_settings.spawn_model.size() - 1),
				g_vehicle_settings.spawn_model.begin());
		}

		Vehicle get_current_vehicle() noexcept
		{
			const Ped player_ped = PLAYER::PLAYER_PED_ID();
			if (!ENTITY::DOES_ENTITY_EXIST(player_ped) || !PED::IS_PED_IN_ANY_VEHICLE(player_ped, false))
				return 0;

			return PED::GET_VEHICLE_PED_IS_IN(player_ped, false);
		}

		void repair_vehicle(Vehicle vehicle)
		{
			VEHICLE::SET_VEHICLE_FIXED(vehicle);
			VEHICLE::SET_VEHICLE_DEFORMATION_FIXED(vehicle);
			VEHICLE::SET_VEHICLE_ENGINE_HEALTH(vehicle, 1000.0f);
			VEHICLE::SET_VEHICLE_BODY_HEALTH(vehicle, 1000.0f);
		}

		void queue_current_vehicle_action(current_vehicle_action action)
		{
			if (!g_fiber_pool)
			{
				g_vehicle_settings.action_status.store(vehicle_action_status::queue_failed, std::memory_order_relaxed);
				return;
			}

			g_vehicle_settings.action_status.store(vehicle_action_status::queued, std::memory_order_relaxed);
			const bool queued = g_fiber_pool->queue_job([action]
			{
				const Vehicle vehicle = get_current_vehicle();
				if (!vehicle)
				{
					g_vehicle_settings.action_status.store(vehicle_action_status::no_vehicle, std::memory_order_relaxed);
					return;
				}

				switch (action)
				{
				case current_vehicle_action::repair:
					repair_vehicle(vehicle);
					break;
				case current_vehicle_action::clean:
					VEHICLE::SET_VEHICLE_DIRT_LEVEL(vehicle, 0.0f);
					break;
				}

				g_vehicle_settings.action_status.store(vehicle_action_status::completed, std::memory_order_relaxed);
			});

			if (!queued)
				g_vehicle_settings.action_status.store(vehicle_action_status::queue_failed, std::memory_order_relaxed);
		}

		void draw_vehicle_catalog()
		{
			ImGui::SetNextItemWidth(210.0f);
			if (ImGui::BeginCombo("Category", g_vehicle_categories[g_vehicle_settings.category_index]))
			{
				for (int index = 0; index < static_cast<int>(std::size(g_vehicle_categories)); ++index)
				{
					const bool selected = g_vehicle_settings.category_index == index;
					if (ImGui::Selectable(g_vehicle_categories[index], selected))
						g_vehicle_settings.category_index = index;
					if (selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			ImGui::SetNextItemWidth(-1.0f);
			ImGui::InputTextWithHint(
				"##VehicleCatalogSearch",
				"Search display name or model...",
				g_vehicle_settings.catalog_search.data(),
				g_vehicle_settings.catalog_search.size());

			const std::string_view query{g_vehicle_settings.catalog_search.data()};
			const char* selected_category = g_vehicle_categories[g_vehicle_settings.category_index];
			ImGui::BeginChild("VehicleCatalogList", ImVec2(0.0f, 145.0f), true);
			for (const auto& entry : g_vehicle_catalog)
			{
				if (g_vehicle_settings.category_index != 0 && std::strcmp(entry.category, selected_category) != 0)
					continue;
				if (!contains_case_insensitive(entry.display_name, query) &&
					!contains_case_insensitive(entry.model_name, query))
				{
					continue;
				}

				const bool selected = std::strcmp(g_vehicle_settings.spawn_model.data(), entry.model_name) == 0;
				const std::string label = std::string(entry.display_name) + "  [" + entry.model_name + "]##" + entry.model_name;
				if (ImGui::Selectable(label.c_str(), selected))
					set_model_name(entry.model_name);
			}
			ImGui::EndChild();
		}
	}

	void queue_vehicle_spawn(std::string model_name, bool put_player_inside)
	{
		model_name.erase(std::remove_if(model_name.begin(), model_name.end(), [](unsigned char character)
		{
			return std::isspace(character) != 0;
		}), model_name.end());

		if (model_name.empty())
		{
			g_vehicle_settings.spawn_status.store(vehicle_spawn_status::invalid_model, std::memory_order_relaxed);
			return;
		}

		if (!g_fiber_pool)
		{
			g_vehicle_settings.spawn_status.store(vehicle_spawn_status::queue_failed, std::memory_order_relaxed);
			return;
		}

		g_vehicle_settings.spawn_status.store(vehicle_spawn_status::loading, std::memory_order_relaxed);
		const bool queued = g_fiber_pool->queue_job([model_name = std::move(model_name), put_player_inside]
		{
			const auto model_hash = static_cast<Hash>(joaat(model_name));
			if (!STREAMING::IS_MODEL_IN_CDIMAGE(model_hash) ||
				!STREAMING::IS_MODEL_VALID(model_hash) ||
				!STREAMING::IS_MODEL_A_VEHICLE(model_hash))
			{
				g_vehicle_settings.spawn_status.store(vehicle_spawn_status::invalid_model, std::memory_order_relaxed);
				return;
			}

			STREAMING::REQUEST_MODEL(model_hash);
			const auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(8);
			while (!STREAMING::HAS_MODEL_LOADED(model_hash))
			{
				if (std::chrono::steady_clock::now() >= timeout)
				{
					STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model_hash);
					g_vehicle_settings.spawn_status.store(vehicle_spawn_status::load_failed, std::memory_order_relaxed);
					return;
				}

				STREAMING::REQUEST_MODEL(model_hash);
				if (auto* current = script::get_current())
					current->yield();
				else
				{
					STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model_hash);
					g_vehicle_settings.spawn_status.store(vehicle_spawn_status::load_failed, std::memory_order_relaxed);
					return;
				}
			}

			const Ped player_ped = PLAYER::PLAYER_PED_ID();
			const auto spawn_position = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(player_ped, 0.0f, 5.0f, 0.0f);
			const float heading = ENTITY::GET_ENTITY_HEADING(player_ped);
			const Vehicle vehicle = VEHICLE::CREATE_VEHICLE(
				model_hash,
				spawn_position.x,
				spawn_position.y,
				spawn_position.z,
				heading,
				false,
				true);

			if (vehicle != 0)
			{
				VEHICLE::SET_VEHICLE_ON_GROUND_PROPERLY(vehicle);
				if (put_player_inside)
					PED::SET_PED_INTO_VEHICLE(player_ped, vehicle, -1);
				g_vehicle_settings.spawn_status.store(vehicle_spawn_status::spawned, std::memory_order_relaxed);
			}
			else
			{
				g_vehicle_settings.spawn_status.store(vehicle_spawn_status::load_failed, std::memory_order_relaxed);
			}

			STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model_hash);
		});

		if (!queued)
			g_vehicle_settings.spawn_status.store(vehicle_spawn_status::queue_failed, std::memory_order_relaxed);
	}

	void tick_vehicle()
	{
		static Vehicle previous_vehicle{};
		static auto next_repair = std::chrono::steady_clock::now();

		const Ped player_ped = PLAYER::PLAYER_PED_ID();
		const Vehicle vehicle = get_current_vehicle();
		if (!vehicle)
		{
			if (previous_vehicle && ENTITY::DOES_ENTITY_EXIST(previous_vehicle))
			{
				ENTITY::SET_ENTITY_INVINCIBLE(previous_vehicle, false);
				VEHICLE::SET_VEHICLE_GRAVITY(previous_vehicle, true);
			}
			previous_vehicle = 0;
			return;
		}

		if (previous_vehicle && previous_vehicle != vehicle && ENTITY::DOES_ENTITY_EXIST(previous_vehicle))
		{
			ENTITY::SET_ENTITY_INVINCIBLE(previous_vehicle, false);
			VEHICLE::SET_VEHICLE_GRAVITY(previous_vehicle, true);
		}
		previous_vehicle = vehicle;

		const bool god_mode = g_vehicle_settings.god_mode.load(std::memory_order_relaxed);
		ENTITY::SET_ENTITY_INVINCIBLE(vehicle, god_mode);
		VEHICLE::SET_DISABLE_VEHICLE_PETROL_TANK_DAMAGE(vehicle, god_mode);
		VEHICLE::SET_DISABLE_VEHICLE_PETROL_TANK_FIRES(vehicle, god_mode);

		const bool seatbelt = g_vehicle_settings.seatbelt.load(std::memory_order_relaxed);
		PED::SET_PED_CONFIG_FLAG(player_ped, 32, !seatbelt);

		const bool disable_gravity = g_vehicle_settings.disable_gravity.load(std::memory_order_relaxed);
		VEHICLE::SET_VEHICLE_GRAVITY(vehicle, !disable_gravity);

		const float acceleration = std::clamp(
			g_vehicle_settings.acceleration_multiplier.load(std::memory_order_relaxed),
			1.0f,
			10.0f);
		VEHICLE::SET_VEHICLE_ENGINE_POWER_MULTIPLIER(vehicle, (acceleration - 1.0f) * 100.0f);
		VEHICLE::SET_VEHICLE_ENGINE_TORQUE_MULTIPLIER(vehicle, acceleration);

		const auto now = std::chrono::steady_clock::now();
		if (g_vehicle_settings.repair_loop.load(std::memory_order_relaxed) && now >= next_repair)
		{
			repair_vehicle(vehicle);
			next_repair = now + std::chrono::milliseconds(500);
		}

		if (g_vehicle_settings.horn_boost.load(std::memory_order_relaxed) && AUDIO::IS_HORN_ACTIVE(vehicle))
		{
			const float speed = ENTITY::GET_ENTITY_SPEED(vehicle);
			VEHICLE::SET_VEHICLE_FORWARD_SPEED(vehicle, std::max(speed + 0.75f, 8.0f));
		}

		if (g_vehicle_settings.rainbow_paint.load(std::memory_order_relaxed))
		{
			const float seconds = std::chrono::duration<float>(now.time_since_epoch()).count();
			const int red = static_cast<int>((std::sin(seconds * 1.7f) + 1.0f) * 127.5f);
			const int green = static_cast<int>((std::sin(seconds * 1.7f + 2.0943951f) + 1.0f) * 127.5f);
			const int blue = static_cast<int>((std::sin(seconds * 1.7f + 4.1887902f) + 1.0f) * 127.5f);
			VEHICLE::SET_VEHICLE_CUSTOM_PRIMARY_COLOUR(vehicle, red, green, blue);
			VEHICLE::SET_VEHICLE_CUSTOM_SECONDARY_COLOUR(vehicle, blue, red, green);
		}
	}

	void draw_vehicle()
	{
		menu_ui::page_title(
			"Vehicle",
			"Browse local vehicle data, spawn by model, and control the vehicle you are currently driving.");

		if (menu_ui::begin_section("VehicleSpawner", "Local Vehicle Spawner", 385.0f))
		{
			draw_vehicle_catalog();

			ImGui::SetNextItemWidth(-1.0f);
			ImGui::InputTextWithHint(
				"##VehicleModel",
				"Raw vehicle model name...",
				g_vehicle_settings.spawn_model.data(),
				g_vehicle_settings.spawn_model.size());
			ImGui::Checkbox("Spawn Inside Vehicle", &g_vehicle_settings.spawn_inside);
			ImGui::SameLine();
			ImGui::TextDisabled("Local spawn only");

			const auto status = g_vehicle_settings.spawn_status.load(std::memory_order_relaxed);
			const bool loading = status == vehicle_spawn_status::loading;
			ImGui::BeginDisabled(loading);
			if (ImGui::Button(loading ? "Loading..." : "Spawn Selected Vehicle", ImVec2(205.0f, 0.0f)))
				queue_vehicle_spawn(g_vehicle_settings.spawn_model.data(), g_vehicle_settings.spawn_inside);
			ImGui::EndDisabled();
			ImGui::SameLine();
			menu_ui::status_badge(spawn_status_text(status), status == vehicle_spawn_status::spawned);
		}
		menu_ui::end_section();

		ImGui::Spacing();
		if (menu_ui::begin_section("VehicleProtection", "Protection", 135.0f))
		{
			menu_ui::toggle("Vehicle God Mode", g_vehicle_settings.god_mode, "Protects only the vehicle you are currently using.");
			menu_ui::toggle("Repair Loop", g_vehicle_settings.repair_loop, "Repairs the current vehicle twice per second.");
			menu_ui::toggle("Seatbelt", g_vehicle_settings.seatbelt, "Prevents the local player from being thrown through the windscreen.");
		}
		menu_ui::end_section();

		ImGui::Spacing();
		if (menu_ui::begin_section("VehicleDriving", "Driving & Handling", 145.0f))
		{
			menu_ui::toggle("Horn Boost", g_vehicle_settings.horn_boost, "Accelerates the current vehicle while the horn is active.");
			menu_ui::toggle("Disable Gravity", g_vehicle_settings.disable_gravity, "Disables gravity for the current vehicle until the option is turned off.");
			menu_ui::slider_float("Acceleration", g_vehicle_settings.acceleration_multiplier, 1.0f, 10.0f, "%.1fx");
		}
		menu_ui::end_section();

		ImGui::Spacing();
		if (menu_ui::begin_section("VehicleAppearance", "Appearance & Actions", 140.0f))
		{
			menu_ui::toggle("Rainbow Paint", g_vehicle_settings.rainbow_paint, "Cycles the local vehicle's primary and secondary colors.");
			if (ImGui::Button("Repair Current Vehicle", ImVec2(190.0f, 0.0f)))
				queue_current_vehicle_action(current_vehicle_action::repair);
			ImGui::SameLine();
			if (ImGui::Button("Clean Current Vehicle", ImVec2(190.0f, 0.0f)))
				queue_current_vehicle_action(current_vehicle_action::clean);

			const auto action_status = g_vehicle_settings.action_status.load(std::memory_order_relaxed);
			menu_ui::status_badge(action_status_text(action_status), action_status == vehicle_action_status::completed);
		}
		menu_ui::end_section();
	}
}
