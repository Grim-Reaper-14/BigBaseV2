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
		bool pointer_core_ready{};
		bool pointer_renderer_ready{};
		bool pointer_scripts_ready{};
		bool pointer_network_ready{};
		bool registration_table_lookup_ready{};
		bool renderer_ready{};
		bool hooks_enabled{};
		bool native_cache_healthy{};
		bool lua_ready{};
		std::uintptr_t module_base{};
		std::size_t module_size{};
		std::size_t required_pointers_found{};
		std::size_t required_pointers_total{};
		std::size_t optional_pointers_found{};
		std::size_t optional_pointers_total{};
		std::int64_t pointer_scan_ms{};
		std::size_t native_cached{};
		std::size_t native_missing{};
		std::size_t scripts_registered{};
		std::size_t lua_scripts_loaded{};
		std::size_t fiber_jobs_queued{};
	};

	[[nodiscard]] inline runtime_diagnostics collect_runtime_diagnostics() noexcept
	{
		runtime_diagnostics result{};
		if (g_pointers)
		{
			result.pointer_core_ready = g_pointers->core_ready();
			result.pointer_renderer_ready = g_pointers->renderer_ready();
			result.pointer_scripts_ready = g_pointers->scripts_ready();
			result.pointer_network_ready = g_pointers->network_ready();
			result.registration_table_lookup_ready = g_pointers->registration_table_lookup_ready();
			result.pointers_ready = g_pointers->fully_ready();

			const auto& report = g_pointers->report();
			result.module_base = report.module_base;
			result.module_size = report.module_size;
			result.required_pointers_found = report.required_found;
			result.required_pointers_total = report.required_total;
			result.optional_pointers_found = report.optional_found;
			result.optional_pointers_total = report.optional_total;
			result.pointer_scan_ms = report.elapsed.count();
		}

		result.renderer_ready = g_renderer && g_renderer->ready();
		result.hooks_enabled = g_hooking && g_hooking->enabled();
		result.native_cache_healthy = g_native_invoker.ready();
		result.native_cached = g_native_invoker.cache_stats().cached;
		result.native_missing = g_native_invoker.cache_stats().missing;
		result.scripts_registered = g_script_mgr.size();
		result.lua_ready = static_cast<bool>(g_lua_manager);
		result.lua_scripts_loaded = g_lua_manager ? g_lua_manager->loaded_count() : 0;
		result.fiber_jobs_queued = g_fiber_pool ? g_fiber_pool->queued_jobs() : 0;
		return result;
	}
}
