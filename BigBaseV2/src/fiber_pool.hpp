#pragma once
#include "common.hpp"

namespace big
{
	class fiber_pool final
	{
	public:
		explicit fiber_pool(std::size_t num_fibers);
		~fiber_pool();

		fiber_pool(const fiber_pool&) = delete;
		fiber_pool(fiber_pool&&) = delete;
		fiber_pool& operator=(const fiber_pool&) = delete;
		fiber_pool& operator=(fiber_pool&&) = delete;

		bool queue_job(std::function<void()> job);
		void fiber_tick();

		[[nodiscard]] std::size_t pending_jobs() const;
		[[nodiscard]] bool stopping() const noexcept
		{
			return m_stopping.load(std::memory_order_acquire);
		}

		static void fiber_func();

	private:
		mutable std::mutex m_mutex;
		std::queue<std::function<void()>> m_jobs;
		std::atomic_bool m_stopping{};
	};

	inline fiber_pool* g_fiber_pool{};
}
