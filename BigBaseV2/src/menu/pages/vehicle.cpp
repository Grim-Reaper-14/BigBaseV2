#include "vehicle.hpp"
#include "../widgets.hpp"

#include "../../fiber_pool.hpp"
#include "../../natives.hpp"
#include "../../script.hpp"

#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstring>

namespace big::menu_pages
{
	namespace
	{
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

		const char* status_text(vehicle_spawn_status status) noexcept
		{
			switch (status)
			{
			case vehicle_spawn_status::idle: return "Ready";
			case vehicle_spawn_status::loading: return "Loading model...";
			case vehicle_spawn_status::spawned: return "Vehicle spawned";
			case vehicle_spawn_status::invalid_model: return "Invalid vehicle model";
			case vehicle_spawn_status::load_failed: return "Model loading timed out";
			}
			return "Unknown";
		}

		void set_preset(const char* model)
		{
			std::fill(g_vehicle_settings.spawn_model.begin(), g_vehicle_settings.spawn_model.end(), '\0');
			std::copy_n(model,
				std::min<std::size_t>(std::strlen(model), g_vehicle_settings.spawn_model.size() - 1),
				g_vehicle_settings.spawn_model.begin());
		}
	}

	void queue_vehicle_spawn(std::string model_name, bool put_player_inside, bool networked)
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

		g_vehicle_settings.spawn_status.store(vehicle_spawn_status::loading, std::memory_order_relaxed);
		g_fiber_pool->queue_job([model_name = std::move(model_name), put_player_inside, networked]
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
				script::get_current()->yield();
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
				networked,
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
	}

	void draw_vehicle()
	{
		menu_ui::page_title("Vehicle", "Spawn vehicles and configure local protection, handling, and appearance options.");

		if (menu_ui::begin_section("VehicleSpawner", "Vehicle Spawner", 215.0f))
		{
			ImGui::SetNextItemWidth(-1.0f);
			ImGui::InputTextWithHint("##VehicleModel", "Vehicle model name...", g_vehicle_settings.spawn_model.data(), g_vehicle_settings.spawn_model.size());
			ImGui::Checkbox("Spawn Inside Vehicle", &g_vehicle_settings.spawn_inside);
			ImGui::SameLine();
			ImGui::Checkbox("Networked Vehicle", &g_vehicle_settings.spawn_networked);

			if (ImGui::Button("Adder")) set_preset("adder");
			ImGui::SameLine();
			if (ImGui::Button("Zentorno")) set_preset("zentorno");
			ImGui::SameLine();
			if (ImGui::Button("Kuruma")) set_preset("kuruma");
			ImGui::SameLine();
			if (ImGui::Button("Oppressor")) set_preset("oppressor");

			const auto status = g_vehicle_settings.spawn_status.load(std::memory_order_relaxed);
			const bool loading = status == vehicle_spawn_status::loading;
			ImGui::BeginDisabled(loading);
			if (ImGui::Button(loading ? "Loading..." : "Spawn Vehicle", ImVec2(190.0f, 0.0f)))
			{
				queue_vehicle_spawn(g_vehicle_settings.spawn_model.data(),
					g_vehicle_settings.spawn_inside,
					g_vehicle_settings.spawn_networked);
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			menu_ui::status_badge(status_text(status), status == vehicle_spawn_status::spawned);
		}
		menu_ui::end_section();

		ImGui::Spacing();
		if (menu_ui::begin_section("VehicleProtection", "Protection", 125.0f))
		{
			ImGui::Checkbox("Vehicle God Mode", &g_vehicle_settings.god_mode);
			ImGui::Checkbox("Repair Loop", &g_vehicle_settings.repair_loop);
			ImGui::Checkbox("Seatbelt", &g_vehicle_settings.seatbelt);
		}
		menu_ui::end_section();

		ImGui::Spacing();
		if (menu_ui::begin_section("VehicleDriving", "Driving & Handling", 145.0f))
		{
			ImGui::Checkbox("Horn Boost", &g_vehicle_settings.horn_boost);
			ImGui::SliderFloat("Acceleration", &g_vehicle_settings.acceleration_multiplier, 1.0f, 10.0f, "%.1fx");
			ImGui::SliderFloat("Gravity", &g_vehicle_settings.gravity_multiplier, 0.1f, 3.0f, "%.1fx");
		}
		menu_ui::end_section();

		ImGui::Spacing();
		if (menu_ui::begin_section("VehicleAppearance", "Appearance & Actions", 120.0f))
		{
			ImGui::Checkbox("Rainbow Paint", &g_vehicle_settings.rainbow_paint);
			if (ImGui::Button("Repair Current Vehicle", ImVec2(190.0f, 0.0f))) {}
			ImGui::SameLine();
			if (ImGui::Button("Clean Current Vehicle", ImVec2(190.0f, 0.0f))) {}
		}
		menu_ui::end_section();
	}
}
