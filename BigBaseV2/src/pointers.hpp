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
		[[nodiscard]] bool legacy_native_lookup_ready() const noexcept;
		[[nodiscard]] bool fully_ready() const noexcept;
		[[nodiscard]] const resolution_report& report() const noexcept;
		[[nodiscard]] std::vector<std::string> missing_required() const;
		void validate_required() const;

		HWND m_hwnd{};

		eGameState* m_game_state{};
		bool* m_is_session_started{};

		CPedFactory** m_ped_factory{};
		CNetworkPlayerMgr** m_network_player_mgr{};

		// Optional legacy registration lookup retained for diagnostics only.
		rage::scrNativeRegistrationTable* m_native_registration_table{};
		functions::get_native_handler_t m_get_native_handler{};

		// GTA V Enhanced populates an ordered hash array through this routine.
		functions::init_native_tables_t m_init_native_tables{};
		functions::fix_vectors_t m_fix_vectors{};

		rage::atArray<GtaThread*>* m_script_threads{};
		rage::scrProgramTable* m_script_program_table{};
		functions::run_script_threads_t m_run_script_threads{};
		std::int64_t** m_script_globals{};

		CGameScriptHandlerMgr** m_script_handler_mgr{};
		IDXGISwapChain** m_swapchain{};

	private:
		resolution_report m_report;
	};

	inline pointers* g_pointers{};
}
