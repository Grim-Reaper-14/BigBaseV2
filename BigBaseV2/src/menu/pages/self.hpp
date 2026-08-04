#pragma once

#include <atomic>

namespace big::menu_pages
{
	struct self_settings final
	{
		std::atomic_bool god_mode{false};
		std::atomic_bool never_wanted{false};
		std::atomic_bool no_ragdoll{false};
		std::atomic_bool no_critical_hits{false};
		std::atomic_bool unlimited_stamina{false};
		std::atomic_bool super_jump{false};
		std::atomic_bool invisible{false};
		std::atomic_bool keep_clean{false};
		std::atomic_bool fast_run{false};
		std::atomic<float> run_multiplier{1.25f};
	};

	inline self_settings g_self_settings;

	void draw_self();
	void tick_self();
	void reset_self();
}
