#include "common.hpp"
#include "gta/array.hpp"
#include "gta/script_thread.hpp"
#include "gta/tls_context.hpp"
#include "gta_util.hpp"
#include "invoker.hpp"
#include "logger.hpp"
#include "pointers.hpp"
#include "script_mgr.hpp"

namespace big
{
	void script_mgr::add_script(std::unique_ptr<script> instance)
	{
		if (!instance)
			throw std::invalid_argument("Cannot add a null script.");

		std::lock_guard lock(m_mutex);
		m_scripts.push_back(std::move(instance));
	}

	void script_mgr::remove_all_scripts()
	{
		std::lock_guard lock(m_mutex);
		m_scripts.clear();
		m_runtime_ready = false;
	}

	std::size_t script_mgr::size() const
	{
		std::lock_guard lock(m_mutex);
		return m_scripts.size();
	}

	void script_mgr::tick()
	{
		if (!g_running || !g_pointers || !g_pointers->scripts_ready())
			return;

		gta_util::execute_as_script(
			RAGE_JOAAT("main_persistent"),
			std::mem_fn(&script_mgr::tick_internal),
			this);
	}

	void script_mgr::ensure_runtime_ready()
	{
		if (m_runtime_ready)
			return;

		if (!IsThreadAFiber() && !ConvertThreadToFiber(nullptr))
			throw std::runtime_error("ConvertThreadToFiber failed for the script scheduler.");

		const auto stats = g_native_invoker.cache_handlers();
		if (!stats.healthy())
			throw std::runtime_error("Enhanced native handler cache is unhealthy.");

		m_runtime_ready = true;
		LOG_INFO("Script runtime initialized with {} native handlers.", stats.cached);
	}

	void script_mgr::tick_internal()
	{
		ensure_runtime_ready();

		std::lock_guard lock(m_mutex);
		for (auto& instance : m_scripts)
		{
			if (!instance || instance->finished())
				continue;

			try
			{
				instance->tick();
			}
			catch (const std::exception& exception)
			{
				LOG_ERROR("Script scheduler error: {}", exception.what());
			}
			catch (...)
			{
				LOG_ERROR("Script scheduler encountered an unknown error.");
			}
		}

		m_scripts.erase(
			std::remove_if(
				m_scripts.begin(),
				m_scripts.end(),
				[](const auto& instance)
				{
					return !instance || instance->finished();
				}),
			m_scripts.end());
	}
}
