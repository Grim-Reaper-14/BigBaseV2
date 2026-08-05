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

		main_batch.add("Is session started [Enhanced]", "0F B6 05 ? ? ? ? 0A 05 ? ? ? ? 75 2A", [this](memory::handle ptr)
		{
			m_is_session_started = ptr.add(3).rip().as<bool*>();
		});

		main_batch.add("Ped factory [Enhanced]", "C7 40 30 03 00 00 00 48 8B 0D", [this](memory::handle ptr)
		{
			m_ped_factory = ptr.add(7).add(3).rip().as<CPedFactory**>();
		});

		main_batch.add("Network player manager [Enhanced]", "75 0E 48 8B 05 ? ? ? ? 48 8B 88 F0 00 00 00", [this](memory::handle ptr)
		{
			m_network_player_mgr = ptr.add(2).add(3).rip().as<CNetworkPlayerMgr**>();
		});

		main_batch.add("Native tables [Enhanced]", "EB 2A 0F 1F 40 00 48 8B 54 17 10", [this](memory::handle ptr)
		{
			m_init_native_tables = ptr.sub(0x2A).as<PVOID>();
			m_native_registration_table = ptr.sub(0xE).rip().as<rage::scrNativeRegistrationTable*>();
		});

		main_batch.add("Script threads [Enhanced]", "BE 40 5D C6 00", [this](memory::handle ptr)
		{
			m_script_threads = ptr.add(0x1B).rip().as<decltype(m_script_threads)>();
			m_run_script_threads = ptr.sub(0xA).as<functions::run_script_threads_t>();
		});

		main_batch.add("Script programs [Enhanced]", "89 46 38 48 8B 0D ? ? ? ? 0F", [this](memory::handle ptr)
		{
			m_script_program_table = ptr.add(0x16).rip().as<decltype(m_script_program_table)>();
		});

		main_batch.add("Script globals [Enhanced]", "48 8B 8E B8 00 00 00 48 8D 15 ? ? ? ? 49 89 D8", [this](memory::handle ptr)
		{
			m_script_globals = ptr.add(10).rip().as<std::int64_t**>();
		});

		main_batch.add("Swapchain and command queue [Enhanced]", "72 C7 EB 02 31 C0 8B 0D", [this](memory::handle ptr)
		{
			m_command_queue = ptr.add(0x1A).add(3).rip().as<ID3D12CommandQueue**>();
			m_swapchain = ptr.add(0x21).add(3).rip().as<IDXGISwapChain**>();
		});

		main_batch.run(memory::module(nullptr));

		m_hwnd = FindWindowW(L"sgaWindow", nullptr);
		if (!m_hwnd)
			throw std::runtime_error("Failed to find the GTA V Enhanced window (sgaWindow).");

		if (!fully_ready())
			throw std::runtime_error("One or more required GTA V Enhanced pointers did not resolve.");

		if (!legacy_native_lookup_ready())
		{
			LOG_WARNING("Enhanced pointers resolved. Legacy native lookup is disabled; the native invoker still needs the Enhanced per-program native-table port.");
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
			&& m_game_state != nullptr
			&& m_is_session_started != nullptr
			&& m_ped_factory != nullptr
			&& m_network_player_mgr != nullptr;
	}

	bool pointers::scripts_ready() const noexcept
	{
		return m_script_threads != nullptr
			&& m_script_program_table != nullptr
			&& m_run_script_threads != nullptr
			&& m_script_globals != nullptr;
	}

	bool pointers::renderer_ready() const noexcept
	{
		return m_swapchain != nullptr
			&& m_command_queue != nullptr;
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
