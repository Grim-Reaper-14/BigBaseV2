#include "common.hpp"
#include "configuration.hpp"
#include "features.hpp"
#include "fiber_pool.hpp"
#include "gui.hpp"
#include "hooking.hpp"
#include "logger.hpp"
#include "lua/lua_manager.hpp"
#include "pointers.hpp"
#include "renderer.hpp"
#include "script_mgr.hpp"

namespace big
{
	namespace
	{
		void print_banner()
		{
			LOG_RAW(log_color::green | log_color::intensify,
	u8R"kek(                     ...
                   ;::::;
                 ;::::; :;
               ;:::::'   :;
              ;:::::;     ;.
             ,:::::'       ;           OOO\
             ::::::;       ;          OOOOO\
             ;:::::;       ;         OOOOOOOO
            ,;::::::;     ;'         / OOOOOOO
          ;:::::::::`. ,,,;.        /  / DOOOOOO
        .';:::::::::::::::::;,     /  /     DOOOO
       ,::::::;::::::;;;;::::;,   /  /        DOOO
      ;`::::::`'::::::;;;::::: ,#/  /          DOOO
      :`:::::::`;::::::;;::: ;::#  /            DOOO
      ::`:::::::`;:::::::: ;::::# /              DOO
      `:`:::::::`;:::::: ;::::::#/               DOO
       :::`:::::::`;; ;:::::::::##                OO
       ::::`:::::::`;::::::::;:::#                OO
       `:::::`::::::::::::;'`:;::#                O
        `:::::`::::::::;' /  / `:#
         ::::::`:::::;'  /  /   `#

)kek");
		}

		DWORD WINAPI module_thread(PVOID)
		{
			auto logger_instance = std::make_unique<logger>();
			std::unique_ptr<pointers> pointers_instance;
			std::unique_ptr<renderer> renderer_instance;
			std::unique_ptr<fiber_pool> fiber_pool_instance;
			std::unique_ptr<hooking> hooking_instance;

			try
			{
				print_banner();

				std::string configuration_status;
				if (!g_configuration.initialize(configuration_status))
					LOG_WARNING("{}", configuration_status);
				else
					LOG_INFO("{}", configuration_status);

				pointers_instance = std::make_unique<pointers>();
				LOG_INFO("Pointers initialized.");

				if (!g_pointers || !g_pointers->m_game_state)
					throw std::runtime_error("Game state pointer was not initialized.");

				if (*g_pointers->m_game_state != eGameState::Playing)
				{
					LOG_INFO("Waiting for the game to load.");
					while (g_running && *g_pointers->m_game_state != eGameState::Playing)
						std::this_thread::sleep_for(100ms);

					if (!g_running)
						throw std::runtime_error("Shutdown requested while waiting for the game.");

					LOG_INFO("The game has loaded.");
				}
				else
				{
					LOG_INFO("The game is already loaded.");
				}

				renderer_instance = std::make_unique<renderer>();
				LOG_INFO("Renderer initialized.");

				fiber_pool_instance = std::make_unique<fiber_pool>(10);
				LOG_INFO("Fiber pool initialized.");

				g_lua_manager = std::make_unique<lua_manager>();
				LOG_INFO("Sol2 Lua manager initialized.");

				hooking_instance = std::make_unique<hooking>();
				LOG_INFO("Hooking initialized.");

				g_script_mgr.add_script(std::make_unique<script>(&features::script_func));
				g_script_mgr.add_script(std::make_unique<script>(&gui::script_func));
				g_script_mgr.add_script(std::make_unique<script>(&lua_manager::script_func));
				LOG_INFO("Scripts registered.");

				g_hooking->enable();
				LOG_INFO("Hooking enabled.");

				while (g_running)
				{
					if (GetAsyncKeyState(g_configuration.values().unload_key) & 1)
						g_running = false;

					g_hooking->ensure_dynamic_hooks();
					std::this_thread::sleep_for(10ms);
				}
			}
			catch (const std::exception& exception)
			{
				g_running = false;
				LOG_ERROR("Fatal startup/runtime error: {}", exception.what());
				MessageBoxA(nullptr, exception.what(), "BigBaseV2", MB_OK | MB_ICONERROR);
			}
			catch (...)
			{
				g_running = false;
				LOG_ERROR("Fatal startup/runtime error: unknown exception.");
				MessageBoxA(nullptr, "An unknown fatal error occurred.", "BigBaseV2", MB_OK | MB_ICONERROR);
			}

			if (g_hooking)
			{
				g_hooking->disable();
				LOG_INFO("Hooking disabled.");
			}

			if (g_configuration.values().autosave)
			{
				std::string save_status;
				if (!g_configuration.save(g_configuration.active_profile(), save_status))
					LOG_WARNING("{}", save_status);
				else
					LOG_INFO("{}", save_status);
			}

			std::this_thread::sleep_for(250ms);
			g_script_mgr.remove_all_scripts();
			LOG_INFO("Scripts unregistered.");

			g_lua_manager.reset();
			LOG_INFO("Sol2 Lua manager uninitialized.");

			hooking_instance.reset();
			fiber_pool_instance.reset();
			renderer_instance.reset();
			pointers_instance.reset();

			LOG_INFO("Farewell!");
			logger_instance.reset();

			const auto module = g_hmodule;
			const auto thread_handle = g_main_thread;
			g_main_thread = nullptr;
			if (thread_handle)
				CloseHandle(thread_handle);

			FreeLibraryAndExitThread(module, 0);
		}
	}
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, PVOID)
{
	if (reason != DLL_PROCESS_ATTACH)
		return TRUE;

	DisableThreadLibraryCalls(module);
	big::g_hmodule = module;
	big::g_running = true;
	big::g_main_thread = CreateThread(nullptr, 0, &big::module_thread, nullptr, 0, &big::g_main_thread_id);

	if (!big::g_main_thread)
	{
		big::g_running = false;
		return FALSE;
	}

	return TRUE;
}
