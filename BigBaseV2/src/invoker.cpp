#include "common.hpp"
#include "crossmap.hpp"
#include "invoker.hpp"
#include "logger.hpp"
#include "pointers.hpp"

namespace big
{
	native_call_context::native_call_context()
	{
		m_return_value = &m_return_stack[0];
		m_args = &m_arg_stack[0];
	}

	native_cache_stats native_invoker::cache_handlers()
	{
		m_handler_cache.clear();
		m_cache_stats = {};
		m_cache_stats.mappings = std::size(g_crossmap);

		for (const rage::scrNativeMapping& mapping : g_crossmap)
		{
			if (m_handler_cache.find(mapping.first) != m_handler_cache.end())
			{
				++m_cache_stats.duplicate_original_hashes;
				continue;
			}

			auto handler = g_pointers->m_get_native_handler(
				g_pointers->m_native_registration_table,
				mapping.second);

			if (!handler && mapping.first != mapping.second)
			{
				handler = g_pointers->m_get_native_handler(
					g_pointers->m_native_registration_table,
					mapping.first);

				if (handler)
					++m_cache_stats.direct_hash_fallbacks;
			}

			if (!handler)
			{
				++m_cache_stats.missing;
				continue;
			}

			m_handler_cache.emplace(mapping.first, handler);
			++m_cache_stats.cached;
		}

		LOG_INFO(
			"Native cache: {}/{} mapped, {} missing, {} duplicates, {} direct fallbacks.",
			m_cache_stats.cached,
			m_cache_stats.mappings,
			m_cache_stats.missing,
			m_cache_stats.duplicate_original_hashes,
			m_cache_stats.direct_hash_fallbacks);

		if (!m_cache_stats.healthy())
		{
			LOG_ERROR(
				"Native mapping database is incomplete or stale. Update crossmap.hpp for the current game build.");
		}

		return m_cache_stats;
	}

	void native_invoker::begin_call()
	{
		m_call_context.reset();
	}

	void native_invoker::end_call(rage::scrNativeHash hash)
	{
		auto iterator = m_handler_cache.find(hash);
		if (iterator == m_handler_cache.end() || !iterator->second)
		{
			LOG_ERROR("Failed to find 0x{:X} native's handler.", hash);
			return;
		}

		__try
		{
			iterator->second(&m_call_context);
			g_pointers->m_fix_vectors(&m_call_context);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			LOG_ERROR("Exception caught while trying to call 0x{:X} native.", hash);
		}
	}
}
