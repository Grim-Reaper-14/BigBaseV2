#include "common.hpp"
#include "logger.hpp"
#include "memory/all.hpp"
#include "pointers.hpp"

namespace big
{
	namespace
	{
		std::string join_names(const std::vector<std::string>& names)
		{
			std::ostringstream stream;
			for (std::size_t index = 0; index < names.size(); ++index)
			{
				if (index != 0)
					stream << ", ";
				stream << names[index];
			}
			return stream.str();
		}
	}

	pointers::pointers()
	{
		const auto started = std::chrono::steady_clock::now();
		const memory::module game_module(nullptr);
		if (!game_module.valid())
			throw std::runtime_error("Failed to inspect GTA5_Enhanced.exe.");

		m_report.module_base = game_module.begin().value();
		m_report.module_size = game_module.size();

		memory::pattern_batch required_batch;

		required_batch.add("Game state", "83 3D ? ? ? ? ? 0F 85 ? ? ? ? BA ? 00", [this](memory::handle pointer)
		{
			m_game_state = pointer.add(2).rip().add(1).as<eGameState*>();
		});

		required_batch.add("Swapchain and command queue", "72 C7 EB 02 31 C0 8B 0D", [this](memory::handle pointer)
		{
			m_command_queue = pointer.add(0x1A).add(3).rip().as<ID3D12CommandQueue**>();
			m_swapchain = pointer.add(0x21).add(3).rip().as<IDXGISwapChain1**>();
		});

		required_batch.add("HWND", "E8 ? ? ? ? 84 C0 74 25 48 8B 0D", [this](memory::handle pointer)
		{
			m_hwnd_ptr = pointer.add(9).add(3).rip().as<HWND*>();
		});

		required_batch.add("Script threads", "48 8B 05 ? ? ? ? 48 89 34 F8 48 FF C7 48 39 FB 75 97", [this](memory::handle pointer)
		{
			m_script_threads = pointer.add(3).rip().as<decltype(m_script_threads)>();
		});

		required_batch.add("Enhanced InitNativeTables", "EB 2A 0F 1F 40 00 48 8B 54 17 10", [this](memory::handle pointer)
		{
			m_init_native_tables = pointer.sub(0x2A).as<functions::init_native_tables_t>();
		});

		required_batch.add("Run script threads", "BE 40 5D C6 00", [this](memory::handle pointer)
		{
			m_run_script_threads = pointer.sub(0xA).as<functions::run_script_threads_t>();
		});

		required_batch.add("Ped factory", "C7 40 30 03 00 00 00 48 8B 0D", [this](memory::handle pointer)
		{
			m_ped_factory = pointer.add(7).add(3).rip().as<CPedFactory**>();
		});

		required_batch.add("Is session started", "0F B6 05 ? ? ? ? 0A 05 ? ? ? ? 75 2A", [this](memory::handle pointer)
		{
			m_is_session_started = pointer.add(3).rip().as<bool*>();
		});

		required_batch.add("Script globals", "48 8B 8E B8 00 00 00 48 8D 15 ? ? ? ? 49 89 D8", [this](memory::handle pointer)
		{
			m_script_globals = pointer.add(7).add(3).rip().as<std::int64_t**>();
		});

		required_batch.add("Script programs", "48 C7 84 C8 D8 00 00 00 00 00 00 00", [this](memory::handle pointer)
		{
			m_script_programs = pointer.add(0x13).add(3).rip().add(0xD8).as<rage::scrProgram**>();
		});

		required_batch.add("Network player manager", "75 0E 48 8B 05 ? ? ? ? 48 8B 88 F0 00 00 00", [this](memory::handle pointer)
		{
			m_network_player_mgr = pointer.add(2).add(3).rip().as<CNetworkPlayerMgr**>();
		});

		const auto required_result = required_batch.run(game_module, false);
		m_report.required_total = required_result.total;
		m_report.required_found = required_result.found;

		memory::pattern_batch optional_batch;

		optional_batch.add("WndProc", "3D 85 00 00 00 0F 87 2D 02 00 00", [this](memory::handle pointer)
		{
			m_wnd_proc = pointer.sub(0x4F).as<PVOID>();
		});

		optional_batch.add("Screen resolution", "75 39 0F 57 C0 F3 0F 2A 05", [this](memory::handle pointer)
		{
			m_screen_res_x = pointer.add(0x5).add(4).rip().as<std::uint32_t*>();
			m_screen_res_y = pointer.add(0x1E).add(4).rip().as<std::uint32_t*>();
		});

		optional_batch.add("Game and online version", "4C 8D 0D ? ? ? ? 48 8D 5C 24 ? 48 89 D9 48 89 FA", [this](memory::handle pointer)
		{
			m_game_version = pointer.add(3).rip().as<const char*>();
			m_online_version = pointer.add(0x47).add(3).rip().as<const char*>();
		});

		optional_batch.add("Handles and pointers", "0F 1F 84 00 00 00 00 00 89 F8 0F 28 FE 41", [this](memory::handle pointer)
		{
			m_handle_to_ptr = pointer.add(0x21).add(1).rip().as<functions::handle_to_ptr_t>();
			m_ptr_to_handle = pointer.sub(0xB).add(1).rip().as<functions::ptr_to_handle_t>();
		});

		optional_batch.add("Region code", "4C 8D 05 ? ? ? ? 48 89 F1 48 89 FA E8 ? ? ? ? 84 C0 74 3D", [this](memory::handle pointer)
		{
			m_region_code = pointer.add(3).rip().as<int*>();
		});

		optional_batch.add("Network object manager", "41 83 7E FA 02 40 0F 9C C5 C1 E5 02", [this](memory::handle pointer)
		{
			m_network_object_mgr = pointer.add(0xC).add(3).rip().as<PVOID>();
		});

		optional_batch.add("Queue dependency and scan memory", "0F 29 46 50 48 8D 05", [this](memory::handle pointer)
		{
			m_queue_dependency = pointer.add(0x71).add(1).rip().as<PVOID>();
			m_sig_scan_memory = pointer.add(4).add(3).rip().as<PVOID>();
		});

		optional_batch.add("Script VM", "49 63 41 1C", [this](memory::handle pointer)
		{
			m_script_vm = pointer.sub(0x24).as<PVOID>();
		});

		optional_batch.add("Stats manager", "89 6C 24 28 48 8D 0D ? ? ? ? 48 8D", [this](memory::handle pointer)
		{
			m_stats_mgr = pointer.add(4).add(3).rip().as<PVOID>();
		});

		optional_batch.add("Ped pool", "80 79 4B 00 0F 84 F5 00 00 00 48 89 F1", [this](memory::handle pointer)
		{
			m_ped_pool = pointer.add(0x18).add(3).rip().as<PVOID>();
		});

		optional_batch.add("Vehicle pool", "48 83 78 18 0D", [this](memory::handle pointer)
		{
			m_vehicle_pool = pointer.sub(0xA).add(3).rip().as<PVOID>();
		});

		optional_batch.add("Object pool", "48 8B 04 0A C3 0F B6 05", [this](memory::handle pointer)
		{
			m_object_pool = pointer.add(5).add(3).rip().as<PVOID>();
		});

		optional_batch.add("Network session", "49 C7 86 F8 00 00 00 00 00 00 00 48 8B 05", [this](memory::handle pointer)
		{
			m_network_session = pointer.add(0x17).add(3).rip().as<PVOID>();
		});

		optional_batch.add("Network time", "89 05 ? ? ? ? 80 3D ? ? ? ? ? 0F 84 ? ? ? ? E9", [this](memory::handle pointer)
		{
			m_network_time = pointer.add(2).rip().as<std::uint32_t*>();
		});

		optional_batch.add("Game timer", "3B 2D ? ? ? ? 76 ? 89 D9", [this](memory::handle pointer)
		{
			m_game_timer = pointer.add(2).rip().as<std::uint32_t*>();
		});

		optional_batch.add("Stats MP character mapping", "48 8D 0D ? ? ? ? 89 F2 0F 28 74 24 ? 48 83 C4 38", [this](memory::handle pointer)
		{
			m_stats_mp_character_mapping_data = pointer.add(3).rip().as<PVOID>();
		});

		optional_batch.add("GTA Plus membership flag", "48 8D 15 ? ? ? ? 41 B8 18 02 00 00 E8", [this](memory::handle pointer)
		{
			m_has_gta_plus = pointer.add(3).rip().as<int*>();
		});

		optional_batch.add("Game data hash", "48 8D 3D ? ? ? ? 69 C9", [this](memory::handle pointer)
		{
			m_game_data_hash = pointer.add(3).rip().as<PVOID>();
		});

		optional_batch.add("DLC manager and DLC hash", "31 D2 E8 ? ? ? ? 3B 84", [this](memory::handle pointer)
		{
			m_dlc_manager = pointer.sub(4).rip().as<PVOID>();
			m_get_dlc_hash = pointer.add(3).rip().as<PVOID>();
		});

		optional_batch.add("Game skeleton update", "56 48 83 EC 20 48 8B 81 40 01 00 00 48 85 C0", [this](memory::handle pointer)
		{
			m_game_skeleton_update = pointer.as<PVOID>();
		});

		optional_batch.add("Fix vectors compatibility", "83 79 18 00 48 8B D1 74 4A FF 4A 18 48 63 4A 18 48 8D 41 04 48 8B 4C CA", [this](memory::handle pointer)
		{
			m_fix_vectors = pointer.as<functions::fix_vectors_t>();
		});

		optional_batch.add("Registration-table native lookup", "48 8D 0D ? ? ? ? 48 8B 14 FA E8 ? ? ? ? 48 85 C0 75 0A", [this](memory::handle pointer)
		{
			m_native_registration_table = pointer.add(3).rip().as<rage::scrNativeRegistrationTable*>();
			m_get_native_handler = pointer.add(12).rip().as<functions::get_native_handler_t>();
		});

		const auto optional_result = optional_batch.run(game_module, false);
		m_report.optional_total = optional_result.total;
		m_report.optional_found = optional_result.found;
		m_report.missing_optional = optional_result.missing;

		if (m_hwnd_ptr && *m_hwnd_ptr)
			m_hwnd = *m_hwnd_ptr;

		m_report.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - started);

		if (!required_result.missing.empty())
		{
			for (const auto& name : required_result.missing)
				LOG_ERROR("Required Enhanced pointer pattern was not found: {}.", name);
		}

		if (!m_report.missing_optional.empty())
			LOG_WARNING("Optional Enhanced pointer signatures unavailable: {}.", join_names(m_report.missing_optional));

		if (!m_fix_vectors)
			LOG_WARNING("FixVectors compatibility pointer was not found; native vector-result fixups will be skipped.");

		validate_required();

		g_pointers = this;
		LOG_INFO(
			"Enhanced pointer resolution completed in {} ms. Required: {}/{}, optional: {}/{}.",
			m_report.elapsed.count(),
			m_report.required_found,
			m_report.required_total,
			m_report.optional_found,
			m_report.optional_total);
	}

	pointers::~pointers()
	{
		if (g_pointers == this)
			g_pointers = nullptr;
	}

	bool pointers::core_ready() const noexcept
	{
		return m_hwnd && m_hwnd_ptr && m_game_state && m_is_session_started && m_ped_factory;
	}

	bool pointers::renderer_ready() const noexcept
	{
		return m_hwnd && m_swapchain && *m_swapchain && m_command_queue && *m_command_queue;
	}

	bool pointers::scripts_ready() const noexcept
	{
		return m_script_threads && m_script_programs && m_run_script_threads && m_script_globals;
	}

	bool pointers::native_ready() const noexcept
	{
		return m_init_native_tables != nullptr;
	}

	bool pointers::network_ready() const noexcept
	{
		return m_network_player_mgr != nullptr;
	}

	bool pointers::registration_table_lookup_ready() const noexcept
	{
		return m_native_registration_table && m_get_native_handler;
	}

	bool pointers::fully_ready() const noexcept
	{
		return core_ready() && renderer_ready() && scripts_ready() && native_ready() && network_ready();
	}

	const pointers::resolution_report& pointers::report() const noexcept
	{
		return m_report;
	}

	std::vector<std::string> pointers::missing_required() const
	{
		std::vector<std::string> missing;
		if (!m_hwnd_ptr || !m_hwnd)
			missing.emplace_back("game window");
		if (!m_game_state)
			missing.emplace_back("game state");
		if (!m_is_session_started)
			missing.emplace_back("session state");
		if (!m_ped_factory)
			missing.emplace_back("ped factory");
		if (!m_network_player_mgr)
			missing.emplace_back("network player manager");
		if (!m_init_native_tables)
			missing.emplace_back("Enhanced InitNativeTables");
		if (!m_swapchain || !*m_swapchain)
			missing.emplace_back("DXGI swapchain");
		if (!m_command_queue || !*m_command_queue)
			missing.emplace_back("D3D12 command queue");
		if (!m_script_threads)
			missing.emplace_back("script threads");
		if (!m_script_programs)
			missing.emplace_back("script programs");
		if (!m_run_script_threads)
			missing.emplace_back("script thread runner");
		if (!m_script_globals)
			missing.emplace_back("script globals");
		return missing;
	}

	void pointers::validate_required() const
	{
		const auto missing = missing_required();
		if (missing.empty())
			return;

		throw std::runtime_error("Failed to resolve required GTA V Enhanced runtime pointers: " + join_names(missing));
	}
}
