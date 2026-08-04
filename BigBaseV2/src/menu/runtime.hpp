#pragma once

#include <atomic>

namespace big::menu_runtime
{
	inline std::atomic_bool disable_game_controls{true};
	inline std::atomic_bool unload_requested{false};

	inline void request_unload() noexcept
	{
		unload_requested.store(true, std::memory_order_relaxed);
	}

	inline bool consume_unload_request() noexcept
	{
		return unload_requested.exchange(false, std::memory_order_relaxed);
	}
}
