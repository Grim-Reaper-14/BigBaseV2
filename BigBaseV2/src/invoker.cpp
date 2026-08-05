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

	void native_invoker::cache_handlers()
	{
		for (const rage::scrNativeMapping &mapping : g_crossmap)
		{
			rage::scrNativeHandler handler = g_pointers->m_get_native_handler(
				g_pointers->m_native_registration_table, mapping.second);

			m_handler_cache.emplace(mapping.first, handler);
		}
	}

	void native_invoker::begin_call()
	{
		m_call_context.reset();
	}

	static bool invoke_native_handler_safe(rage::scrNativeHandler handler, native_call_context* context)
	{
		__try
		{
			handler(context);
			g_pointers->m_fix_vectors(context);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}

		return true;
	}

	void native_invoker::end_call(rage::scrNativeHash hash)
	{
		if (auto it = m_handler_cache.find(hash); it != m_handler_cache.end())
		{
			rage::scrNativeHandler handler = it->second;

			if (!invoke_native_handler_safe(handler, &m_call_context))
			{
				LOG_ERROR("Exception caught while trying to call 0x{:X} native.", hash);
			}
		}
		else
		{
			LOG_ERROR("Failed to find 0x{:X} native's handler.", hash);
		}
	}
}
