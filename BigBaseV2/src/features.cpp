#include "common.hpp"
#include "features.hpp"
#include "logger.hpp"
#include "script.hpp"

namespace big
{
	void features::run_tick()
	{
		// Feature modules register their per-frame work here. Keep this function
		// lightweight because it runs from the main script scheduler.
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
