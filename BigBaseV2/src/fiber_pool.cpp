#include "common.hpp"
#include "fiber_pool.hpp"
#include "logger.hpp"
#include "script.hpp"
#include "script_mgr.hpp"

namespace big
{
	fiber_pool::fiber_pool(std::size_t num_fibers)
	{
		if (num_fibers == 0)
			throw std::invalid_argument("Fiber pool requires at least one worker.");

		if (g_fiber_pool)
			throw std::runtime_error("Fiber pool is already initialized.");

		g_fiber_pool = this;

		try
		{
			for (std::size_t index = 0; index < num_fibers; ++index)
				g_script_mgr.add_script(std::make_unique<script>(&fiber_func));
		}
		catch (...)
		{
			g_fiber_pool = nullptr;
			throw;
		}
	}

	fiber_pool::~fiber_pool()
	{
		m_stopping.store(true, std::memory_order_release);

		{
			std::lock_guard lock(m_mutex);
			std::queue<std::function<void()>> empty;
			m_jobs.swap(empty);
		}

		if (g_fiber_pool == this)
			g_fiber_pool = nullptr;
	}

	bool fiber_pool::queue_job(std::function<void()> job)
	{
		if (!job || stopping())
			return false;

		std::lock_guard lock(m_mutex);
		if (stopping())
			return false;

		m_jobs.push(std::move(job));
		return true;
	}

	std::size_t fiber_pool::pending_jobs() const
	{
		std::lock_guard lock(m_mutex);
		return m_jobs.size();
	}

	void fiber_pool::fiber_tick()
	{
		std::function<void()> job;
		{
			std::lock_guard lock(m_mutex);
			if (m_jobs.empty())
				return;

			job = std::move(m_jobs.front());
			m_jobs.pop();
		}

		try
		{
			std::invoke(job);
		}
		catch (const std::exception& exception)
		{
			LOG_ERROR("Fiber pool job failed: {}", exception.what());
		}
		catch (...)
		{
			LOG_ERROR("Fiber pool job failed with an unknown exception.");
		}
	}

	void fiber_pool::fiber_func()
	{
		while (g_running)
		{
			auto* pool = g_fiber_pool;
			auto* current = script::get_current();
			if (!pool || !current || pool->stopping())
				break;

			pool->fiber_tick();
			current->yield();
		}
	}
}
