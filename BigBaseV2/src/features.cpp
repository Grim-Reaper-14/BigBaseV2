#include "common.hpp"
#include "features.hpp"
#include "logger.hpp"
#include "menu/pages/vehicle.hpp"
#include "script.hpp"

namespace big
{
	void features::run_tick()
	{
		menu_pages::tick_vehicle();
	}

	void features::script_func()
	{
		while (g_running)
		{
			run_tick();

			if (auto* current = script::get_current())
				current->yield();
			else
			{
				LOG_ERROR("Feature script lost its fiber context.");
				g_running = false;
				break;
			}
		}
	}
}
