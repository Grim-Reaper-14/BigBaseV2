#pragma once
#include "common.hpp"
#include "gta/fwddec.hpp"
#include "gta/enums.hpp"
#include "function_types.hpp"

namespace big
{
	class pointers
	{
	public:
		explicit pointers();
		~pointers();

		[[nodiscard]] bool core_ready() const noexcept;
		[[nodiscard]] bool scripts_ready() const noexcept;
		[[nodiscard]] bool renderer_ready() const noexcept;
		[[nodiscard]] bool native_tables_ready() const noexcept;
		[[nodiscard]] bool legacy_native_lookup_ready() const noexcept;
		[[nodiscard]] bool fully_ready() const noexcept;

	public:
		HWND m_hwnd{};

		eGameState* m_game_state{};
		bool* m_is_session_started{};

		CPedFactory** m_ped_factory{};
		CNetworkPlayerMgr** m_network_player_mgr{};

		// Enhanced initializes native handlers through the per-program native
		// tables. The old registration-table lookup remains for diagnostics and
		// legacy invoker compatibility only.
		PVOID m_init_native_tables{};
		rage::scrNativeRegistrationTable* m_native_registration_table{};
		functions::get_native_handler_t m_get_native_handler{};
		functions::fix_vectors_t m_fix_vectors{};

		rage::atArray<GtaThread*>* m_script_threads{};
		rage::scrProgramTable* m_script_program_table{};
		functions::run_script_threads_t m_run_script_threads{};
		std::int64_t** m_script_globals{};

		CGameScriptHandlerMgr** m_script_handler_mgr{};

		IDXGISwapChain** m_swapchain{};
		ID3D12CommandQueue** m_command_queue{};
	};

	inline pointers* g_pointers{};
}
