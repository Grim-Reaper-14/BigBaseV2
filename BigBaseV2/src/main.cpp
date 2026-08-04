#include "common.hpp"
#include "features.hpp"
#include "fiber_pool.hpp"
#include "gui.hpp"
#include "logger.hpp"
#include "hooking.hpp"
#include "pointers.hpp"
#include "renderer.hpp"
#include "script_mgr.hpp"

namespace
{
	DWORD WINAPI main_thread(PVOID)
	{
		using namespace big;

		auto logger_instance = std::make_unique<logger>();
		std::unique_ptr<pointers> pointers_instance;
		std::unique_ptr<renderer> renderer_instance;
		std::unique_ptr<fiber_pool> fiber_pool_instance;
		std::unique_ptr<hooking> hooking_instance;
		bool hooks_enabled = false;
		bool scripts_registered = false;

		try
		{
			LOG_RAW(log_color::green | log_color::intensify,
R"logo(                     ...
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
)logo");

			pointers_instance = std::make_unique<pointers>();
			LOG_INFO("Pointers initialized.");

			if (g_pointers == nullptr || g_pointers->m_game_state == nullptr)
				throw std::runtime_error("Game state pointer is unavailable.");

			if (*g_pointers->m_game_state != eGameState::Playing)
			{
				LOG_INFO("Waiting for the game to load.");
				while (g_running && *g_pointers->m_game_state != eGameState::Playing)
					std::this_thread::sleep_for(100ms);

				if (!g_running)
					throw std::runtime_error("Startup cancelled while waiting for the game.");

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

			hooking_instance = std::make_unique<hooking>();
			LOG_INFO("Hooking initialized.");

			g_script_mgr.add_script(std::make_unique<script>(&features::script_func));
			g_script_mgr.add_script(std::make_unique<script>(&gui::script_func));
			scripts_registered = true;
			LOG_INFO("Scripts registered.");

			g_hooking->enable();
			hooks_enabled = true;
			LOG_INFO("Hooking enabled.");

			while (g_running)
			{
				if ((GetAsyncKeyState(VK_END) & 1) != 0)
					g_running = false;

				g_hooking->ensure_dynamic_hooks();
				std::this_thread::sleep_for(10ms);
			}
		}
		catch (const std::exception& ex)
		{
			LOG_ERROR("{}", ex.what());
			MessageBoxA(nullptr, ex.what(), "BigBaseV2", MB_OK | MB_ICONERROR);
			g_running = false;
		}
		catch (...)
		{
			LOG_ERROR("An unknown fatal error occurred.");
			MessageBoxA(nullptr, "An unknown fatal error occurred.", "BigBaseV2", MB_OK | MB_ICONERROR);
			g_running = false;
		}

		if (hooks_enabled && g_hooking != nullptr)
		{
			g_hooking->disable();
			LOG_INFO("Hooking disabled.");
		}

		std::this_thread::sleep_for(250ms);

		if (scripts_registered)
		{
			g_script_mgr.remove_all_scripts();
			LOG_INFO("Scripts unregistered.");
		}

		hooking_instance.reset();
		fiber_pool_instance.reset();
		renderer_instance.reset();
		pointers_instance.reset();

		LOG_INFO("Farewell!");
		logger_instance.reset();

		const HMODULE module = g_hmodule;
		g_main_thread = nullptr;
		g_main_thread_id = 0;
		FreeLibraryAndExitThread(module, 0);
	}
}

BOOL APIENTRY DllMain(HMODULE hmod, DWORD reason, PVOID)
{
	using namespace big;

	if (reason != DLL_PROCESS_ATTACH)
		return TRUE;

	DisableThreadLibraryCalls(hmod);
	g_hmodule = hmod;
	g_running = true;

	g_main_thread = CreateThread(nullptr, 0, main_thread, nullptr, 0, &g_main_thread_id);
	if (g_main_thread == nullptr)
		return FALSE;

	CloseHandle(g_main_thread);
	g_main_thread = nullptr;
	return TRUE;
}
