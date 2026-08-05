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
			throw std::runtime_error("Failed to inspect the GTA V executable module.");

		m_report.module_base = game_module.begin().value();
		m_report.module_size = game_module.size();

		memory::pattern_batch required_batch;

		required_batch.add("Game state", "83 3D ? ? ? ? ? 75 17 8B 42 20 25", [this](memory::handle pointer)
		{
			m_game_state = pointer.add(2).rip().as<eGameState*>();
		});

		required_batch.add("Is session started", "40 38 35 ? ? ? ? 75 0E 4C 8B C3 49 8B D7 49 8B CE", [this](memory::handle pointer)
		{
			m_is_session_started = pointer.add(3).rip().as<bool*>();
		});

		required_batch.add("Ped factory", "48 8B 05 ? ? ? ? 48 8B 48 08 48 85 C9 74 52 8B 81", [this](memory::handle pointer)
		{
			m_ped_factory = pointer.add(3).rip().as<CPedFactory**>();
		});

		required_batch.add("Network player manager", "48 8B 0D ? ? ? ? 8A D3 48 8B 01 FF 50 ? 4C 8B 07 48 8B CF", [this](memory::handle pointer)
		{
			m_network_player_mgr = pointer.add(3).rip().as<CNetworkPlayerMgr**>();
		});

		required_batch.add("Enhanced InitNativeTables", "EB 2A 0F 1F 40 00 48 8B 54 17 10", [this](memory::handle pointer)
		{
			m_init_native_tables = pointer.sub(0x2A).as<functions::init_native_tables_t>();
		});

		required_batch.add("Fix vectors", "83 79 18 00 48 8B D1 74 4A FF 4A 18 48 63 4A 18 48 8D 41 04 48 8B 4C CA", [this](memory::handle pointer)
		{
			m_fix_vectors = pointer.as<functions::fix_vectors_t>();
		});

		required_batch.add("Script threads", "45 33 F6 8B E9 85 C9 B8", [this](memory::handle pointer)
		{
			m_script_threads = pointer.sub(4).rip().sub(8).as<decltype(m_script_threads)>();
			m_run_script_threads = pointer.sub(0x1F).as<functions::run_script_threads_t>();
		});

		required_batch.add("Script programs", "44 8B 0D ? ? ? ? 4C 8B 1D ? ? ? ? 48 8B 1D ? ? ? ? 41 83 F8 FF 74 3F 49 63 C0 42 0F B6 0C 18 81 E1", [this](memory::handle pointer)
		{
			m_script_program_table = pointer.add(17).rip().as<decltype(m_script_program_table)>();
		});

		required_batch.add("Script globals", "48 8D 15 ? ? ? ? 4C 8B C0 E8 ? ? ? ? 48 85 FF 48 89 1D", [this](memory::handle pointer)
		{
			m_script_globals = pointer.add(3).rip().as<std::int64_t**>();
		});

		required_batch.add("CGameScriptHandlerMgr", "48 8B 0D ? ? ? ? 4C 8B CE E8 ? ? ? ? 48 85 C0 74 05 40 32 FF", [this](memory::handle pointer)
		{
			m_script_handler_mgr = pointer.add(3).rip().as<CGameScriptHandlerMgr**>();
		});

		required_batch.add("Swapchain", "48 8B 0D ? ? ? ? 48 8B 01 44 8D 43 01 33 D2 FF 50 40 8B C8", [this](memory::handle pointer)
		{
			m_swapchain = pointer.add(3).rip().as<IDXGISwapChain**>();
		});

		const auto required_result = required_batch.run(game_module, false);
		m_report.required_total = required_result.total;
		m_report.required_found = required_result.found;

		memory::pattern_batch optional_batch;
		optional_batch.add("Legacy native handlers", "48 8D 0D ? ? ? ? 48 8B 14 FA E8 ? ? ? ? 48 85 C0 75 0A", [this](memory::handle pointer)
		{
			m_native_registration_table = pointer.add(3).rip().as<rage::scrNativeRegistrationTable*>();
			m_get_native_handler = pointer.add(12).rip().as<functions::get_native_handler_t>();
		});

		const auto optional_result = optional_batch.run(game_module, false);
		m_report.optional_total = optional_result.total;
		m_report.optional_found = optional_result.found;
		m_report.missing_optional = optional_result.missing;

		m_hwnd = FindWindowW(L"grcWindow", nullptr);
		m_report.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - started);

		const auto missing = missing_required();
		if (!required_result.missing.empty())
		{
			for (const auto& name : required_result.missing)
			{
				if (std::find(missing.begin(), missing.end(), name) == missing.end())
					LOG_ERROR("Required pointer pattern was not found: {}.", name);
			}
		}

		if (!m_report.missing_optional.empty())
			LOG_WARNING("Optional pointer signatures unavailable: {}.", join_names(m_report.missing_optional));

		validate_required();

		g_pointers = this;
		LOG_INFO(
			"Pointer resolution completed in {} ms. Required: {}/{}, optional: {}/{}.",
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
		return m_hwnd && m_game_state && m_is_session_started && m_ped_factory &&
			m_init_native_tables && m_fix_vectors;
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

	bool pointers::native_ready() const noexcept
	{
		return m_init_native_tables && m_fix_vectors;
	}

	bool pointers::network_ready() const noexcept
	{
		return m_network_player_mgr != nullptr;
	}

	bool pointers::legacy_native_lookup_ready() const noexcept
	{
		return m_native_registration_table && m_get_native_handler;
	}

	bool pointers::fully_ready() const noexcept
	{
		return core_ready() && renderer_ready() && scripts_ready() && network_ready();
	}

	const pointers::resolution_report& pointers::report() const noexcept
	{
		return m_report;
	}

	std::vector<std::string> pointers::missing_required() const
	{
		std::vector<std::string> missing;
		if (!m_hwnd)
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
		if (!m_fix_vectors)
			missing.emplace_back("FixVectors");
		if (!m_swapchain || !*m_swapchain)
			missing.emplace_back("DXGI swapchain");
		if (!m_script_threads)
			missing.emplace_back("script threads");
		if (!m_script_program_table)
			missing.emplace_back("script program table");
		if (!m_run_script_threads)
			missing.emplace_back("script thread runner");
		if (!m_script_globals)
			missing.emplace_back("script globals");
		if (!m_script_handler_mgr)
			missing.emplace_back("script handler manager");
		return missing;
	}

	void pointers::validate_required() const
	{
		const auto missing = missing_required();
		if (missing.empty())
			return;

		throw std::runtime_error("Failed to resolve required runtime pointers: " + join_names(missing));
	}
}
