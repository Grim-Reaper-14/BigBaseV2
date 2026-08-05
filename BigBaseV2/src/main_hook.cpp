#include "common.hpp"
#include "function_types.hpp"
#include "logger.hpp"
#include "main_hook.hpp"
#include "pointers.hpp"
#include "script_mgr.hpp"

namespace big
{
	main_hook::main_hook() :
		m_run_script_threads_hook(
			"Main script hook",
			g_pointers ? reinterpret_cast<void*>(g_pointers->m_run_script_threads) : nullptr,
			reinterpret_cast<void*>(&run_script_threads))
	{
		if (!g_pointers || !g_pointers->m_run_script_threads)
			throw std::runtime_error("Cannot create the main hook without RunScriptThreads.");

		if (g_main_hook)
			throw std::runtime_error("A main hook instance is already active.");

		g_main_hook = this;
	}

	main_hook::~main_hook()
	{
		disable();
		if (g_main_hook == this)
			g_main_hook = nullptr;
	}

	void main_hook::enable()
	{
		bool expected = false;
		if (!m_enabled.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
			return;

		try
		{
			m_run_script_threads_hook.enable();
			LOG_INFO("Central main hook enabled.");
		}
		catch (...)
		{
			m_enabled.store(false, std::memory_order_release);
			throw;
		}
	}

	void main_hook::disable() noexcept
	{
		if (!m_enabled.exchange(false, std::memory_order_acq_rel))
			return;

		m_run_script_threads_hook.disable();
		LOG_INFO("Central main hook disabled after {} ticks.", tick_count());
	}

	bool main_hook::run_script_threads(std::uint32_t ops_to_execute)
	{
		auto* instance = g_main_hook;
		if (!instance)
			return false;

		const auto original = instance->m_run_script_threads_hook.get_original<functions::run_script_threads_t>();
		if (!original)
			return false;

		instance->tick(ops_to_execute);
		return original(ops_to_execute);
	}

	void main_hook::tick(std::uint32_t) noexcept
	{
		if (!enabled() || !g_running)
			return;

		if (m_in_tick.test_and_set(std::memory_order_acquire))
			return;

		struct tick_guard final
		{
			std::atomic_flag& flag;
			~tick_guard()
			{
				flag.clear(std::memory_order_release);
			}
		} guard{m_in_tick};

		m_tick_count.fetch_add(1, std::memory_order_relaxed);

		try
		{
			g_script_mgr.tick();
		}
		catch (const std::exception& exception)
		{
			LOG_ERROR("Main hook tick failed: {}", exception.what());
		}
		catch (...)
		{
			LOG_ERROR("Main hook tick failed with an unknown exception.");
		}
	}
}
