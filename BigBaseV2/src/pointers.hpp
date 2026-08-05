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
		HWND* m_hwnd_ptr{};
		PVOID m_wnd_proc{};

		std::uint32_t* m_screen_res_x{};
		std::uint32_t* m_screen_res_y{};
		const char* m_game_version{};
		const char* m_online_version{};
		int* m_region_code{};

		eGameState* m_game_state{};
		bool* m_is_session_started{};
		std::uint32_t* m_network_time{};
		std::uint32_t* m_game_timer{};

		CPedFactory** m_ped_factory{};
		CNetworkPlayerMgr** m_network_player_mgr{};
		PVOID m_network_object_mgr{};
		PVOID m_network_session{};

		PVOID m_ped_pool{};
		PVOID m_vehicle_pool{};
		PVOID m_object_pool{};

		PVOID m_stats_mgr{};
		PVOID m_stats_mp_character_mapping_data{};
		int* m_has_gta_plus{};

		PVOID m_dlc_manager{};
		PVOID m_get_dlc_hash{};
		PVOID m_game_data_hash{};
		PVOID m_game_skeleton_update{};

		PVOID m_queue_dependency{};
		PVOID m_sig_scan_memory{};
		PVOID m_script_vm{};

		functions::handle_to_ptr_t m_handle_to_ptr{};
		functions::ptr_to_handle_t m_ptr_to_handle{};

		// GTA V Enhanced initializes native handlers through per-program native
		// tables. The old registration-table lookup remains declared only until
		// the legacy invoker is replaced.
		PVOID m_init_native_tables{};
		rage::scrNativeRegistrationTable* m_native_registration_table{};
		functions::get_native_handler_t m_get_native_handler{};
		functions::fix_vectors_t m_fix_vectors{};

		rage::atArray<GtaThread*>* m_script_threads{};
		functions::run_script_threads_t m_run_script_threads{};
		std::int64_t** m_script_globals{};

		// Enhanced exposes the program array directly. Keep the old table member
		// temporarily so existing legacy code still compiles during the port.
		rage::scrProgram** m_script_programs{};
		rage::scrProgramTable* m_script_program_table{};

		CGameScriptHandlerMgr** m_script_handler_mgr{};

		IDXGISwapChain** m_swapchain{};
		ID3D12CommandQueue** m_command_queue{};
	};

	inline pointers* g_pointers{};
}
