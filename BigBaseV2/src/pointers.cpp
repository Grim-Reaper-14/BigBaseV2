#include "common.hpp"
#include "logger.hpp"
#include "memory/all.hpp"
#include "pointers.hpp"

namespace big
{
	namespace
	{
		[[noreturn]] void throw_missing_pointer(const char* name)
		{
			throw std::runtime_error(std::string("Failed to resolve required pointer: ") + name);
		}
	}

	pointers::pointers()
	{
		memory::pattern_batch main_batch;

		main_batch.add("Game state", "83 3D ? ? ? ? ? 75 17 8B 42 20 25", [this](memory::handle pointer)
		{
			m_game_state = pointer.add(2).rip().as<eGameState*>();
		});

		main_batch.add("Is session started", "40 38 35 ? ? ? ? 75 0E 4C 8B C3 49 8B D7 49 8B CE", [this](memory::handle pointer)
		{
			m_is_session_started = pointer.add(3).rip().as<bool*>();
		});

		main_batch.add("Ped factory", "48 8B 05 ? ? ? ? 48 8B 48 08 48 85 C9 74 52 8B 81", [this](memory::handle pointer)
		{
			m_ped_factory = pointer.add(3).rip().as<CPedFactory**>();
		});

		main_batch.add("Network player manager", "48 8B 0D ? ? ? ? 8A D3 48 8B 01 FF 50 ? 4C 8B 07 48 8B CF", [this](memory::handle pointer)
		{
			m_network_player_mgr = pointer.add(3).rip().as<CNetworkPlayerMgr**>();
		});

		main_batch.add("Legacy native handlers", "48 8D 0D ? ? ? ? 48 8B 14 FA E8 ? ? ? ? 48 85 C0 75 0A", [this](memory::handle pointer)
		{
			m_native_registration_table = pointer.add(3).rip().as<rage::scrNativeRegistrationTable*>();
			m_get_native_handler = pointer.add(12).rip().as<functions::get_native_handler_t>();
		});

		main_batch.add("Enhanced InitNativeTables", "EB 2A 0F 1F 40 00 48 8B 54 17 10", [this](memory::handle pointer)
		{
			m_init_native_tables = pointer.sub(0x2A).as<functions::init_native_tables_t>();
		});

		main_batch.add("Fix vectors", "83 79 18 00 48 8B D1 74 4A FF 4A 18 48 63 4A 18 48 8D 41 04 48 8B 4C CA", [this](memory::handle pointer)
		{
			m_fix_vectors = pointer.as<functions::fix_vectors_t>();
		});

		main_batch.add("Script threads", "45 33 F6 8B E9 85 C9 B8", [this](memory::handle pointer)
		{
			m_script_threads = pointer.sub(4).rip().sub(8).as<decltype(m_script_threads)>();
			m_run_script_threads = pointer.sub(0x1F).as<functions::run_script_threads_t>();
		});

		main_batch.add("Script programs", "44 8B 0D ? ? ? ? 4C 8B 1D ? ? ? ? 48 8B 1D ? ? ? ? 41 83 F8 FF 74 3F 49 63 C0 42 0F B6 0C 18 81 E1", [this](memory::handle pointer)
		{
			m_script_program_table = pointer.add(17).rip().as<decltype(m_script_program_table)>();
		});

		main_batch.add("Script globals", "48 8D 15 ? ? ? ? 4C 8B C0 E8 ? ? ? ? 48 85 FF 48 89 1D", [this](memory::handle pointer)
		{
			m_script_globals = pointer.add(3).rip().as<std::int64_t**>();
		});

		main_batch.add("CGameScriptHandlerMgr", "48 8B 0D ? ? ? ? 4C 8B CE E8 ? ? ? ? 48 85 C0 74 05 40 32 FF", [this](memory::handle pointer)
		{
			m_script_handler_mgr = pointer.add(3).rip().as<CGameScriptHandlerMgr**>();
		});

		main_batch.add("Swapchain", "48 8B 0D ? ? ? ? 48 8B 01 44 8D 43 01 33 D2 FF 50 40 8B C8", [this](memory::handle pointer)
		{
			m_swapchain = pointer.add(3).rip().as<IDXGISwapChain**>();
		});

		main_batch.run(memory::module(nullptr));

		m_hwnd = FindWindowW(L"grcWindow", nullptr);
		validate_required();

		g_pointers = this;
		LOG_INFO(
			"Pointer validation complete. Core: {}, renderer: {}, scripts: {}.",
			core_ready() ? "ready" : "missing",
			renderer_ready() ? "ready" : "missing",
			scripts_ready() ? "ready" : "missing");
	}

	pointers::~pointers()
	{
		if (g_pointers == this)
			g_pointers = nullptr;
	}

	bool pointers::core_ready() const noexcept
	{
		return m_hwnd && m_game_state && m_is_session_started && m_ped_factory &&
			m_network_player_mgr && m_init_native_tables && m_fix_vectors;
	}

	bool pointers::renderer_ready() const noexcept
	{
		return m_hwnd && m_swapchain && *m_swapchain;
	}

	bool pointers::scripts_ready() const noexcept
	{
		return m_script_threads && m_script_program_table && m_run_script_threads &&
			m_script_globals && m_script_handler_mgr;
	}

	void pointers::validate_required() const
	{
		if (!m_hwnd)
			throw_missing_pointer("game window");
		if (!m_game_state)
			throw_missing_pointer("game state");
		if (!m_is_session_started)
			throw_missing_pointer("session state");
		if (!m_ped_factory)
			throw_missing_pointer("ped factory");
		if (!m_network_player_mgr)
			throw_missing_pointer("network player manager");
		if (!m_init_native_tables)
			throw_missing_pointer("Enhanced InitNativeTables");
		if (!m_fix_vectors)
			throw_missing_pointer("FixVectors");
		if (!m_swapchain || !*m_swapchain)
			throw_missing_pointer("DXGI swapchain");
		if (!scripts_ready())
			throw_missing_pointer("script runtime");
	}
}
