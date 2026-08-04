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
		m_return_value = m_return_stack;
		m_args = m_arg_stack;
	}

	native_cache_stats native_invoker::cache_handlers()
	{
		invalidate_cache();

		if (!g_pointers || !g_pointers->m_init_native_tables)
		{
			LOG_ERROR("Cannot initialize Enhanced native cache: pointer table is not ready.");
			m_cache_stats.missing = 1;
			return m_cache_stats;
		}

#if BIGBASEV2_HAS_ENHANCED_CROSSMAP
		static_assert(sizeof(rage::scrNativeHandler) == sizeof(rage::scrNativeHash));

		m_cache_stats.mappings = g_enhanced_native_hashes.size();
		std::array<rage::scrNativeHandler, g_enhanced_native_hashes.size()> handlers{};

		std::memcpy(
			handlers.data(),
			g_enhanced_native_hashes.data(),
			sizeof(g_enhanced_native_hashes));

		rage::scrProgram program{};
		program.m_native_count = static_cast<std::uint32_t>(handlers.size());
		program.m_native_entrypoints = handlers.data();
		g_pointers->m_init_native_tables(&program);

		m_handler_cache.reserve(handlers.size());

		for (std::size_t index = 0; index < handlers.size(); ++index)
		{
			const auto hash = g_enhanced_native_hashes[index];
			const auto handler = handlers[index];

			if (!handler)
			{
				++m_cache_stats.missing;
				continue;
			}

			const auto [iterator, inserted] = m_handler_cache.emplace(hash, handler);
			if (!inserted)
			{
				++m_cache_stats.duplicate_original_hashes;
				continue;
			}

			++m_cache_stats.cached;
		}

		m_cache_initialized = true;

		LOG_INFO(
			"Enhanced native cache: {}/{} handlers, {} missing, {} duplicate hashes. Source: {} ({})",
			m_cache_stats.cached,
			m_cache_stats.mappings,
			m_cache_stats.missing,
			m_cache_stats.duplicate_original_hashes,
			g_enhanced_crossmap_source,
			g_enhanced_crossmap_ref);
#else
		LOG_ERROR("crossmap_enhanced.hpp is missing. Run tools/natives/sync_yimmenuv2_crossmap.py.");
		m_cache_stats.missing = 1;
#endif

		if (!m_cache_stats.healthy())
		{
			LOG_ERROR(
				"Enhanced native cache is unhealthy. Native execution will remain blocked until the table is synchronized and validated.");
		}

		return m_cache_stats;
	}

	void native_invoker::invalidate_cache() noexcept
	{
		m_handler_cache.clear();
		m_cache_stats = {};
		m_cache_initialized = false;
	}

	void native_invoker::begin_call()
	{
		m_call_context.reset();
	}

	void native_invoker::end_call(rage::scrNativeHash hash)
	{
		if (!ready())
			cache_handlers();

		if (!ready())
		{
			LOG_ERROR("Blocked native 0x{:X}: Enhanced native cache is not ready.", hash);
			return;
		}

		const auto iterator = m_handler_cache.find(hash);
		if (iterator == m_handler_cache.end() || !iterator->second)
		{
			LOG_ERROR("Failed to resolve Enhanced native handler 0x{:X}.", hash);
			return;
		}

		__try
		{
			iterator->second(&m_call_context);
			g_pointers->m_fix_vectors(&m_call_context);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			LOG_ERROR("Structured exception while executing Enhanced native 0x{:X}.", hash);
		}
	}
}
