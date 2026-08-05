#pragma once

#include "common.hpp"
#include "detour_hook.hpp"

namespace big
{
	// Owns the game's RunScriptThreads detour and provides the single execution
	// point for BigBaseV2's script scheduler.
	class main_hook final
	{
	public:
		main_hook();
		~main_hook();

		main_hook(const main_hook&) = delete;
		main_hook(main_hook&&) = delete;
		main_hook& operator=(const main_hook&) = delete;
		main_hook& operator=(main_hook&&) = delete;

		void enable();
		void disable() noexcept;

		[[nodiscard]] bool enabled() const noexcept
		{
			return m_enabled.load(std::memory_order_acquire);
		}

		[[nodiscard]] std::uint64_t tick_count() const noexcept
		{
			return m_tick_count.load(std::memory_order_relaxed);
		}

	private:
		static bool run_script_threads(std::uint32_t ops_to_execute);
		void tick(std::uint32_t ops_to_execute) noexcept;

		std::atomic_bool m_enabled{};
		std::atomic_flag m_in_tick = ATOMIC_FLAG_INIT;
		std::atomic<std::uint64_t> m_tick_count{};
		detour_hook m_run_script_threads_hook;
	};

	inline main_hook* g_main_hook{};
}
