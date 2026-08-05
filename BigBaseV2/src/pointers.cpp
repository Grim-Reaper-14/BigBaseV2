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

		main_batch.add("Network player manager [Enhanced]", "75 0E 48 8B 05 ? ? ? ? 48 8B 88 F0 00 00 00", [this](memory::handle ptr)
		{
			m_network_player_mgr = ptr.add(2).add(3).rip().as<CNetworkPlayerMgr**>();
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
