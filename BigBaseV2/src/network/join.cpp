#include "join.hpp"

#include "fiber_pool.hpp"
#include "function_types.hpp"
#include "gta/array.hpp"
#include "gta/joaat.hpp"
#include "gta/scr_program.hpp"
#include "gta/script_thread.hpp"
#include "gta/tls_context.hpp"
#include "logger.hpp"
#include "pointers.hpp"
#include "script_global.hpp"

namespace big::network
{
	namespace
	{
		constexpr std::size_t script_program_count = 176;
		constexpr std::size_t join_type_global_index = 1575048;
		constexpr rage::joaat_t shop_controller_hash = RAGE_JOAAT("shop_controller");
		constexpr std::array<int, 11> send_to_clouds_pattern{
			0x2D, 0x00, 0x02, 0x00, 0x00, 0x72, 0x5D, -1, -1, -1, 0x72};

		std::mutex g_status_mutex;
		join_status g_status{true, false, "Session join service is ready."};

		void set_status(bool success, bool pending, std::string message)
		{
			std::lock_guard lock(g_status_mutex);
			g_status.success = success;
			g_status.pending = pending;
			g_status.message = std::move(message);
		}

		GtaThread* find_script_thread(rage::joaat_t hash) noexcept
		{
			if (!g_pointers || !g_pointers->m_script_threads)
				return nullptr;

			for (auto* thread : *g_pointers->m_script_threads)
			{
				if (!thread || !thread->m_context.m_thread_id)
					continue;

				if (thread->m_script_hash == hash ||
					static_cast<rage::joaat_t>(thread->m_context.m_script_hash) == hash)
				{
					return thread;
				}
			}

			return nullptr;
		}

		rage::scrProgram* find_script_program(rage::joaat_t hash) noexcept
		{
			if (!g_pointers || !g_pointers->m_script_programs)
				return nullptr;

			for (std::size_t index = 0; index < script_program_count; ++index)
			{
				auto* program = g_pointers->m_script_programs[index];
				if (program && program->m_name_hash == hash)
					return program;
			}

			return nullptr;
		}

		std::optional<std::uint32_t> find_send_to_clouds_pc(rage::scrProgram* program) noexcept
		{
			static rage::scrProgram* cached_program{};
			static std::uint32_t cached_pc{};

			if (!program || !program->valid())
				return std::nullopt;

			if (cached_program == program && cached_pc < program->m_code_size)
				return cached_pc;

			if (program->m_code_size < send_to_clouds_pattern.size())
				return std::nullopt;

			const auto last_start = program->m_code_size -
				static_cast<std::uint32_t>(send_to_clouds_pattern.size());

			for (std::uint32_t start = 0; start <= last_start; ++start)
			{
				bool matches = true;
				for (std::size_t offset = 0; offset < send_to_clouds_pattern.size(); ++offset)
				{
					const int expected = send_to_clouds_pattern[offset];
					if (expected < 0)
						continue;

					const auto* byte = program->code_address(start + static_cast<std::uint32_t>(offset));
					if (!byte || *byte != static_cast<std::uint8_t>(expected))
					{
						matches = false;
						break;
					}
				}

				if (matches)
				{
					cached_program = program;
					cached_pc = start;
					return start;
				}
			}

			return std::nullopt;
		}

		bool call_send_to_clouds()
		{
			if (!g_pointers || !g_pointers->m_script_vm || !g_pointers->m_script_globals)
			{
				set_status(false, false, "Enhanced ScriptVM or script globals are unavailable.");
				return false;
			}

			auto* thread = find_script_thread(shop_controller_hash);
			auto* program = find_script_program(shop_controller_hash);
			if (!thread || !program)
			{
				set_status(false, false, "shop_controller is not active yet.");
				return false;
			}

			const auto program_counter = find_send_to_clouds_pc(program);
			if (!program_counter)
			{
				set_status(false, false, "The Enhanced SendToClouds script function was not found.");
				return false;
			}

			auto* tls = rage::tlsContext::get();
			auto* stack = static_cast<std::uint64_t*>(thread->m_stack);
			if (!tls || !stack)
			{
				set_status(false, false, "The Enhanced script TLS or stack is unavailable.");
				return false;
			}

			auto context = thread->m_context;
			if (context.m_stack_size && context.m_stack_pointer >= context.m_stack_size)
			{
				set_status(false, false, "shop_controller does not have enough script-stack space.");
				return false;
			}

			const auto original_thread = tls->m_script_thread;
			const bool original_active = tls->m_is_script_thread_active;
			struct tls_restore final
			{
				rage::tlsContext* tls;
				rage::scrThread* thread;
				bool active;
				~tls_restore()
				{
					tls->m_script_thread = thread;
					tls->m_is_script_thread_active = active;
				}
			} restore{tls, original_thread, original_active};

			tls->m_script_thread = thread;
			tls->m_is_script_thread_active = true;

			stack[context.m_stack_pointer++] = 0;
			context.m_instruction_pointer = *program_counter;
			context.m_state = rage::eThreadState::idle;

			const auto script_vm = reinterpret_cast<functions::script_vm_t>(g_pointers->m_script_vm);
			script_vm(stack, g_pointers->m_script_globals, program, &context);
			return true;
		}

		bool launch_join_type(join_type type)
		{
			if (!call_send_to_clouds())
				return false;

			script_global join_global(join_type_global_index);
			if (!join_global.can_access())
			{
				set_status(false, false, "The Enhanced session-type global is unavailable.");
				return false;
			}

			*join_global.as<std::int32_t*>() = static_cast<std::int32_t>(type);
			set_status(true, false, "Session transition was requested.");
			LOG_INFO("Queued GTA V Enhanced session transition type {}.", static_cast<std::int32_t>(type));
			return true;
		}
	}

	bool join_service_ready() noexcept
	{
		return g_fiber_pool &&
			g_pointers &&
			g_pointers->m_script_vm &&
			g_pointers->m_script_globals &&
			g_pointers->m_script_threads &&
			g_pointers->m_script_programs;
	}

	bool queue_join_type(join_type type)
	{
		if (!join_service_ready())
		{
			set_status(false, false, "Session join service is not ready.");
			return false;
		}

		if (!g_fiber_pool->queue_job([type]
		{
			launch_join_type(type);
		}))
		{
			set_status(false, false, "Failed to queue the session transition.");
			return false;
		}

		set_status(true, true, "Session transition is queued.");
		return true;
	}

	join_status current_join_status()
	{
		std::lock_guard lock(g_status_mutex);
		return g_status;
	}
}
