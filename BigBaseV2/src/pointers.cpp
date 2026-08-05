#include "common.hpp"
#include "logger.hpp"
#include "pointers.hpp"
#include "memory/all.hpp"

namespace big
{
	pointers::pointers()
	{
		memory::pattern_batch main_batch;

		main_batch.add("Game state [Enhanced]", "83 3D ? ? ? ? ? 0F 85 ? ? ? ? BA ? 00", [this](memory::handle ptr)
		{
			m_game_state = ptr.add(2).rip().add(1).as<eGameState*>();
		});

		main_batch.add("Swapchain and command queue [Enhanced]", "72 C7 EB 02 31 C0 8B 0D", [this](memory::handle ptr)
		{
			m_command_queue = ptr.add(0x1A).add(3).rip().as<ID3D12CommandQueue**>();
			m_swapchain = ptr.add(0x21).add(3).rip().as<IDXGISwapChain**>();
		});

		main_batch.add("WndProc [Enhanced]", "3D 85 00 00 00 0F 87 2D 02 00 00", [this](memory::handle ptr)
		{
			m_wnd_proc = ptr.sub(0x4F).as<PVOID>();
		});

		main_batch.add("HWND [Enhanced]", "E8 ? ? ? ? 84 C0 74 25 48 8B 0D", [this](memory::handle ptr)
		{
			m_hwnd_ptr = ptr.add(9).add(3).rip().as<HWND*>();
		});

		main_batch.add("Screen resolution [Enhanced]", "75 39 0F 57 C0 F3 0F 2A 05", [this](memory::handle ptr)
		{
			m_screen_res_x = ptr.add(0x5).add(4).rip().as<std::uint32_t*>();
			m_screen_res_y = ptr.add(0x1E).add(4).rip().as<std::uint32_t*>();
		});

		main_batch.add("Game and online version [Enhanced]", "4C 8D 0D ? ? ? ? 48 8D 5C 24 ? 48 89 D9 48 89 FA", [this](memory::handle ptr)
		{
			m_game_version = ptr.add(3).rip().as<const char*>();
			m_online_version = ptr.add(0x47).add(3).rip().as<const char*>();
		});

		main_batch.add("Script threads [Enhanced]", "48 8B 05 ? ? ? ? 48 89 34 F8 48 FF C7 48 39 FB 75 97", [this](memory::handle ptr)
		{
			m_script_threads = ptr.add(3).rip().as<decltype(m_script_threads)>();
		});

		main_batch.add("Init native tables [Enhanced]", "EB 2A 0F 1F 40 00 48 8B 54 17 10", [this](memory::handle ptr)
		{
			m_init_native_tables = ptr.sub(0x2A).as<PVOID>();
		});

		main_batch.add("Run script threads [Enhanced]", "BE 40 5D C6 00", [this](memory::handle ptr)
		{
			m_run_script_threads = ptr.sub(0xA).as<functions::run_script_threads_t>();
		});

		main_batch.add("Handles and pointers [Enhanced]", "0F 1F 84 00 00 00 00 00 89 F8 0F 28 FE 41", [this](memory::handle ptr)
		{
			m_handle_to_ptr = ptr.add(0x21).add(1).rip().as<functions::handle_to_ptr_t>();
			m_ptr_to_handle = ptr.sub(0xB).add(1).rip().as<functions::ptr_to_handle_t>();
		});

		main_batch.add("Ped factory [Enhanced]", "C7 40 30 03 00 00 00 48 8B 0D", [this](memory::handle ptr)
		{
			m_ped_factory = ptr.add(7).add(3).rip().as<CPedFactory**>();
		});

		main_batch.add("Is session started [Enhanced]", "0F B6 05 ? ? ? ? 0A 05 ? ? ? ? 75 2A", [this](memory::handle ptr)
		{
			m_is_session_started = ptr.add(3).rip().as<bool*>();
		});

		main_batch.add("Script globals [Enhanced]", "48 8B 8E B8 00 00 00 48 8D 15 ? ? ? ? 49 89 D8", [this](memory::handle ptr)
		{
			m_script_globals = ptr.add(7).add(3).rip().as<std::int64_t**>();
		});

		main_batch.add("Script programs [Enhanced]", "48 C7 84 C8 D8 00 00 00 00 00 00 00", [this](memory::handle ptr)
		{
			m_script_programs = ptr.add(0x13).add(3).rip().add(0xD8).as<rage::scrProgram**>();
		});

		main_batch.add("Region code [Enhanced]", "4C 8D 05 ? ? ? ? 48 89 F1 48 89 FA E8 ? ? ? ? 84 C0 74 3D", [this](memory::handle ptr)
		{
			m_region_code = ptr.add(3).rip().as<int*>();
		});

		main_batch.add("Network object manager [Enhanced]", "41 83 7E FA 02 40 0F 9C C5 C1 E5 02", [this](memory::handle ptr)
		{
			m_network_object_mgr = ptr.add(0xC).add(3).rip().as<PVOID>();
		});

		main_batch.add("Network player manager [Enhanced]", "75 0E 48 8B 05 ? ? ? ? 48 8B 88 F0 00 00 00", [this](memory::handle ptr)
		{
			m_network_player_mgr = ptr.add(2).add(3).rip().as<CNetworkPlayerMgr**>();
		});

		main_batch.add("Queue dependency and scan memory [Enhanced]", "0F 29 46 50 48 8D 05", [this](memory::handle ptr)
		{
			m_queue_dependency = ptr.add(0x71).add(1).rip().as<PVOID>();
			m_sig_scan_memory = ptr.add(4).add(3).rip().as<PVOID>();
		});

		main_batch.add("Script VM [Enhanced]", "49 63 41 1C", [this](memory::handle ptr)
		{
			m_script_vm = ptr.sub(0x24).as<PVOID>();
		});

		main_batch.add("Stats manager [Enhanced]", "89 6C 24 28 48 8D 0D ? ? ? ? 48 8D", [this](memory::handle ptr)
		{
			m_stats_mgr = ptr.add(4).add(3).rip().as<PVOID>();
		});

		main_batch.add("Ped pool [Enhanced]", "80 79 4B 00 0F 84 F5 00 00 00 48 89 F1", [this](memory::handle ptr)
		{
			m_ped_pool = ptr.add(0x18).add(3).rip().as<PVOID>();
		});

		main_batch.add("Vehicle pool [Enhanced]", "48 83 78 18 0D", [this](memory::handle ptr)
		{
			m_vehicle_pool = ptr.sub(0xA).add(3).rip().as<PVOID>();
		});

		main_batch.add("Object pool [Enhanced]", "48 8B 04 0A C3 0F B6 05", [this](memory::handle ptr)
		{
			m_object_pool = ptr.add(5).add(3).rip().as<PVOID>();
		});

		main_batch.add("Network session [Enhanced]", "49 C7 86 F8 00 00 00 00 00 00 00 48 8B 05", [this](memory::handle ptr)
		{
			m_network_session = ptr.add(0x17).add(3).rip().as<PVOID>();
		});

		main_batch.add("Network time [Enhanced]", "89 05 ? ? ? ? 80 3D ? ? ? ? ? 0F 84 ? ? ? ? E9", [this](memory::handle ptr)
		{
			m_network_time = ptr.add(2).rip().as<std::uint32_t*>();
		});

		main_batch.add("Game timer [Enhanced]", "3B 2D ? ? ? ? 76 ? 89 D9", [this](memory::handle ptr)
		{
			m_game_timer = ptr.add(2).rip().as<std::uint32_t*>();
		});

		main_batch.add("Stats MP character mapping [Enhanced]", "48 8D 0D ? ? ? ? 89 F2 0F 28 74 24 ? 48 83 C4 38", [this](memory::handle ptr)
		{
			m_stats_mp_character_mapping_data = ptr.add(3).rip().as<PVOID>();
		});

		main_batch.add("GTA Plus membership flag [Enhanced]", "48 8D 15 ? ? ? ? 41 B8 18 02 00 00 E8", [this](memory::handle ptr)
		{
			m_has_gta_plus = ptr.add(3).rip().as<int*>();
		});

		main_batch.add("Game data hash [Enhanced]", "48 8D 3D ? ? ? ? 69 C9", [this](memory::handle ptr)
		{
			m_game_data_hash = ptr.add(3).rip().as<PVOID>();
		});

		main_batch.add("DLC manager and DLC hash [Enhanced]", "31 D2 E8 ? ? ? ? 3B 84", [this](memory::handle ptr)
		{
			m_dlc_manager = ptr.sub(4).rip().as<PVOID>();
			m_get_dlc_hash = ptr.add(3).rip().as<PVOID>();
		});

		main_batch.add("Game skeleton update [Enhanced]", "56 48 83 EC 20 48 8B 81 40 01 00 00 48 85 C0", [this](memory::handle ptr)
		{
			m_game_skeleton_update = ptr.as<PVOID>();
		});

		main_batch.run(memory::module(nullptr));

		if (!m_hwnd_ptr || !*m_hwnd_ptr)
			throw std::runtime_error("Failed to resolve the GTA V Enhanced HWND.");

		m_hwnd = *m_hwnd_ptr;

		if (!fully_ready())
			throw std::runtime_error("One or more required GTA V Enhanced pointers did not resolve.");

		if (!legacy_native_lookup_ready())
		{
			LOG_INFO("Enhanced pointers resolved. The legacy native registration-table invoker remains disabled until its per-program native-table port is complete.");
		}

		g_pointers = this;
	}

	pointers::~pointers()
	{
		g_pointers = nullptr;
	}

	bool pointers::core_ready() const noexcept
	{
		return m_hwnd != nullptr
			&& m_hwnd_ptr != nullptr
			&& m_wnd_proc != nullptr
			&& m_game_state != nullptr
			&& m_is_session_started != nullptr
			&& m_ped_factory != nullptr
			&& m_network_player_mgr != nullptr;
	}

	bool pointers::scripts_ready() const noexcept
	{
		return m_script_threads != nullptr
			&& m_script_programs != nullptr
			&& m_run_script_threads != nullptr
			&& m_script_globals != nullptr;
	}

	bool pointers::renderer_ready() const noexcept
	{
		return m_swapchain != nullptr
			&& m_command_queue != nullptr
			&& m_screen_res_x != nullptr
			&& m_screen_res_y != nullptr;
	}

	bool pointers::native_tables_ready() const noexcept
	{
		return m_init_native_tables != nullptr;
	}

	bool pointers::legacy_native_lookup_ready() const noexcept
	{
		return m_native_registration_table != nullptr
			&& m_get_native_handler != nullptr
			&& m_fix_vectors != nullptr;
	}

	bool pointers::fully_ready() const noexcept
	{
		return core_ready()
			&& scripts_ready()
			&& renderer_ready()
			&& native_tables_ready();
	}
}
