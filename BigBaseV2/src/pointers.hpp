#pragma once

#include "common.hpp"
#include "function_types.hpp"
#include "gta/enums.hpp"
#include "gta/fwddec.hpp"

namespace big
{
	class pointers final
	{
	public:
		struct resolution_report final
		{
			std::uintptr_t module_base{};
			std::size_t module_size{};
			std::size_t required_total{};
			std::size_t required_found{};
			std::size_t optional_total{};
			std::size_t optional_found{};
			std::chrono::milliseconds elapsed{};
			std::vector<std::string> missing_optional;

			[[nodiscard]] bool required_complete() const noexcept
			{
				return required_total > 0 && required_found == required_total;
			}
		};

		pointers();
		~pointers();

		pointers(const pointers&) = delete;
		pointers(pointers&&) = delete;
		pointers& operator=(const pointers&) = delete;
		pointers& operator=(pointers&&) = delete;

		[[nodiscard]] bool core_ready() const noexcept;
		[[nodiscard]] bool renderer_ready() const noexcept;
		[[nodiscard]] bool scripts_ready() const noexcept;
		[[nodiscard]] bool native_ready() const noexcept;
		[[nodiscard]] bool network_ready() const noexcept;
		[[nodiscard]] bool registration_table_lookup_ready() const noexcept;
		[[nodiscard]] bool fully_ready() const noexcept;
		[[nodiscard]] const resolution_report& report() const noexcept;
		[[nodiscard]] std::vector<std::string> missing_required() const;
		void validate_required() const;

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
		functions::script_vm_t m_script_vm{};

		functions::handle_to_ptr_t m_handle_to_ptr{};
		functions::ptr_to_handle_t m_ptr_to_handle{};

		// Enhanced populates per-program native tables with this routine.
		functions::init_native_tables_t m_init_native_tables{};

		// These are optional compatibility pointers. Enhanced native execution
		// must not require the Legacy registration-table path.
		rage::scrNativeRegistrationTable* m_native_registration_table{};
		functions::get_native_handler_t m_get_native_handler{};
		functions::fix_vectors_t m_fix_vectors{};

		rage::atArray<GtaThread*>* m_script_threads{};
		rage::scrProgram** m_script_programs{};
		functions::run_script_threads_t m_run_script_threads{};
		std::int64_t** m_script_globals{};

		// Retained for older code while the direct Enhanced program array is
		// adopted throughout the project.
		rage::scrProgramTable* m_script_program_table{};
		CGameScriptHandlerMgr** m_script_handler_mgr{};

		IDXGISwapChain1** m_swapchain{};
		ID3D12CommandQueue** m_command_queue{};

	private:
		resolution_report m_report;
	};

	inline pointers* g_pointers{};
}
