#pragma once

#include "fiber_pool.hpp"
#include "hooking.hpp"
#include "invoker.hpp"
#include "lua/lua_manager.hpp"
#include "pointers.hpp"
#include "renderer.hpp"
#include "script_mgr.hpp"

namespace big
{
	struct runtime_diagnostics final
	{
		bool pointers_ready{};
		bool renderer_ready{};
		bool hooks_enabled{};
		bool native_cache_healthy{};
		bool lua_ready{};
		std::size_t native_cached{};
		std::size_t native_missing{};
		std::size_t scripts_registered{};
		std::size_t lua_scripts_loaded{};
		std::size_t fiber_jobs_queued{};
	};

	[[nodiscard]] inline runtime_diagnostics collect_runtime_diagnostics() noexcept
	{
		runtime_diagnostics result{};
		result.pointers_ready = g_pointers && g_pointers->core_ready() && g_pointers->scripts_ready();
		result.renderer_ready = g_renderer && g_renderer->ready();
		result.hooks_enabled = g_hooking && g_hooking->enabled();
		result.native_cache_healthy = g_native_invoker.cache_healthy();
		result.native_cached = g_native_invoker.cache_stats().cached;
		result.native_missing = g_native_invoker.cache_stats().missing;
		result.scripts_registered = g_script_mgr.size();
		result.lua_ready = static_cast<bool>(g_lua_manager);
		result.lua_scripts_loaded = g_lua_manager ? g_lua_manager->loaded_count() : 0;
		result.fiber_jobs_queued = g_fiber_pool ? g_fiber_pool->queued_jobs() : 0;
		return result;
	}
}
