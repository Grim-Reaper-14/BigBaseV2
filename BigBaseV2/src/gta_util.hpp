#pragma once

#include "common.hpp"
#include "gta/array.hpp"
#include "gta/ped_factory.hpp"
#include "gta/player.hpp"
#include "gta/script_thread.hpp"
#include "gta/tls_context.hpp"
#include "pointers.hpp"

namespace big::gta_util
{
	[[nodiscard]] inline CPed* get_local_ped() noexcept
	{
		if (!g_pointers || !g_pointers->m_ped_factory || !*g_pointers->m_ped_factory)
			return nullptr;

		return (*g_pointers->m_ped_factory)->m_local_ped;
	}

	[[nodiscard]] inline CPlayerInfo* get_local_playerinfo() noexcept
	{
		if (auto* ped = get_local_ped())
			return ped->m_playerinfo;

		return nullptr;
	}

	template <typename F, typename... Args>
	bool execute_as_script(rage::joaat_t script_hash, F&& callback, Args&&... args)
	{
		if (!g_pointers || !g_pointers->m_script_threads)
			return false;

		auto* tls_context = rage::tlsContext::get();
		if (!tls_context)
			return false;

		for (auto* thread : *g_pointers->m_script_threads)
		{
			if (!thread || !thread->m_context.m_thread_id || thread->m_context.m_script_hash != script_hash)
				continue;

			struct tls_restore final
			{
				rage::tlsContext* context;
				GtaThread* original_thread;
				bool original_active;

				~tls_restore()
				{
					context->m_script_thread = original_thread;
					context->m_is_script_thread_active = original_active;
				}
			} restore{tls_context, tls_context->m_script_thread, tls_context->m_is_script_thread_active};

			tls_context->m_script_thread = thread;
			tls_context->m_is_script_thread_active = true;
			std::invoke(std::forward<F>(callback), std::forward<Args>(args)...);
			return true;
		}

		return false;
	}
}
