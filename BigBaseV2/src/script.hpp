#pragma once
#include "common.hpp"

namespace big
{
	class script final
	{
	public:
		using func_t = void(*)();
		using clock = std::chrono::steady_clock;

		explicit script(func_t func, std::optional<std::size_t> stack_size = std::nullopt);
		~script();

		script(const script&) = delete;
		script(script&&) = delete;
		script& operator=(const script&) = delete;
		script& operator=(script&&) = delete;

		void tick();
		void yield(std::optional<clock::duration> time = std::nullopt);

		[[nodiscard]] bool finished() const noexcept
		{
			return m_finished;
		}

		[[nodiscard]] bool faulted() const noexcept
		{
			return m_faulted;
		}

		[[nodiscard]] static script* get_current() noexcept;

	private:
		static VOID CALLBACK fiber_entry(void* parameter);
		void fiber_func() noexcept;

		void* m_script_fiber{};
		void* m_main_fiber{};
		func_t m_func{};
		std::optional<clock::time_point> m_wake_time;
		bool m_finished{};
		bool m_faulted{};
	};
}
