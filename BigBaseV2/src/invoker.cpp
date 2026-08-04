#include "common.hpp"
#include "gta/scr_program.hpp"
#include "invoker.hpp"
#include "logger.hpp"
#include "pointers.hpp"

#if __has_include("crossmap_enhanced.hpp")
#include "crossmap_enhanced.hpp"
#define BIGBASEV2_HAS_ENHANCED_CROSSMAP 1
#else
#define BIGBASEV2_HAS_ENHANCED_CROSSMAP 0
#endif

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

#if BIGBASEV2_HAS_ENHANCED_CROSSMAP
		static_assert(sizeof(rage::scrNativeHandler) == sizeof(rage::scrNativeHash));

		m_cache_stats.mappings = g_enhanced_native_hashes.size();
		std::array<rage::scrNativeHandler, g_enhanced_native_hashes.size()> handlers{};

		// InitNativeTables expects the native hashes to be stored in the same
		// memory that will receive the resolved handler pointers.
		std::memcpy(
			handlers.data(),
			g_enhanced_native_hashes.data(),
			sizeof(g_enhanced_native_hashes));

		rage::scrProgram program{};
		program.m_native_count = static_cast<std::uint32_t>(handlers.size());
		program.m_native_entrypoints = handlers.data();
		g_pointers->m_init_native_tables(&program);

		for (std::size_t index = 0; index < handlers.size(); ++index)
		{
			const auto hash = g_enhanced_native_hashes[index];
			const auto handler = handlers[index];

			if (!m_handler_cache.emplace(hash, handler).second)
			{
				++m_cache_stats.duplicate_original_hashes;
				continue;
			}

			if (!handler)
			{
				m_handler_cache.erase(hash);
				++m_cache_stats.missing;
				continue;
			}

			++m_cache_stats.cached;
		}

		LOG_INFO(
			"Enhanced native cache: {}/{} handlers, {} missing, {} duplicate hashes. Source: {} ({})",
			m_cache_stats.cached,
			m_cache_stats.mappings,
			m_cache_stats.missing,
			m_cache_stats.duplicate_original_hashes,
			g_enhanced_crossmap_source,
			g_enhanced_crossmap_ref);
#else
		LOG_ERROR(
			"crossmap_enhanced.hpp is missing. Run: py tools/natives/sync_yimmenuv2_crossmap.py");
		m_cache_stats.missing = 1;
#endif

		if (!m_cache_stats.healthy())
		{
			LOG_ERROR(
				"Enhanced native handlers are not healthy. Do not execute gameplay features until the YimMenuV2 table is synchronized and validated.");
		}

		return m_cache_stats;
	}

	void native_invoker::begin_call()
	{
		m_call_context.reset();
	}

	void native_invoker::end_call(rage::scrNativeHash hash)
	{
		if (m_handler_cache.empty())
			cache_handlers();

		auto iterator = m_handler_cache.find(hash);
		if (iterator == m_handler_cache.end() || !iterator->second)
		{
			LOG_ERROR("Failed to find 0x{:X} Enhanced native handler.", hash);
			return;
		}

		__try
		{
			iterator->second(&m_call_context);
			if (g_pointers->m_fix_vectors)
				g_pointers->m_fix_vectors(&m_call_context);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			LOG_ERROR("Exception caught while trying to call 0x{:X} native.", hash);
		}
	}
}
