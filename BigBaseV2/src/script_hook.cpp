#include "common.hpp"
#include "gta/scr_program.hpp"
#include "logger.hpp"
#include "pointers.hpp"
#include "script_hook.hpp"

namespace big
{
	namespace
	{
		constexpr std::size_t scrprogram_vtable_size = 9;
		constexpr std::size_t scrprogram_destructor_index = 6;

		[[nodiscard]] rage::scrNativeHandler resolve_enhanced_handler(rage::scrNativeHash hash) noexcept
		{
			if (!g_pointers || !g_pointers->m_init_native_tables)
				return nullptr;

			static_assert(sizeof(rage::scrNativeHandler) == sizeof(rage::scrNativeHash));

			rage::scrNativeHandler handler{};
			std::memcpy(&handler, &hash, sizeof(hash));

			rage::scrProgram temporary_program{};
			temporary_program.m_native_count = 1;
			temporary_program.m_native_entrypoints = &handler;
			g_pointers->m_init_native_tables(&temporary_program);
			return handler;
		}
	}

	std::unordered_map<rage::scrProgram*, script_hook*> script_hook::s_map;

	script_hook::script_hook(
		rage::joaat_t script_hash,
		std::unordered_map<rage::scrNativeHash, rage::scrNativeHandler> native_replacements) :
		m_script_hash(script_hash),
		m_native_replacements(std::move(native_replacements))
	{
		ensure();
	}

	script_hook::~script_hook()
	{
		detach();
	}

	void script_hook::ensure()
	{
		if (m_vmt_hook || !g_pointers || !g_pointers->m_script_programs || !g_pointers->m_init_native_tables)
			return;

		for (std::size_t index = 0; index < rage::scr_program_capacity; ++index)
		{
			auto* program = g_pointers->m_script_programs[index];
			if (!program || !program->valid())
				continue;

			if (program->m_name_hash != m_script_hash && program->m_hash != m_script_hash)
				continue;

			hook_instance(program);
			if (m_program == program)
			{
				LOG_INFO(
					"Hooked Enhanced script {} ({}) with {} native replacement(s).",
					program->m_name ? program->m_name : "<unnamed>",
					static_cast<void*>(program),
					m_native_patches.size());
			}
			return;
		}
	}

	void script_hook::hook_instance(rage::scrProgram* program)
	{
		if (!program || !program->valid() || m_vmt_hook)
			return;

		if (const auto iterator = s_map.find(program); iterator != s_map.end() && iterator->second != this)
		{
			LOG_WARNING(
				"Enhanced script program {} is already owned by another script_hook.",
				static_cast<void*>(program));
			return;
		}

		auto program_hook = std::make_unique<vmt_hook>(program, scrprogram_vtable_size);
		program_hook->hook(scrprogram_destructor_index, reinterpret_cast<void*>(&scrprogram_dtor));

		m_native_patches.clear();
		m_native_patches.reserve(m_native_replacements.size());

		for (const auto& [hash, replacement] : m_native_replacements)
		{
			if (!replacement)
			{
				LOG_WARNING("Ignored null replacement for Enhanced native 0x{:X}.", hash);
				continue;
			}

			const auto original = resolve_enhanced_handler(hash);
			if (!original)
			{
				LOG_WARNING("Could not resolve Enhanced native handler 0x{:X}.", hash);
				continue;
			}

			auto* slot = program->native_entrypoint_slot(original);
			if (!slot)
			{
				LOG_WARNING(
					"Enhanced script {} does not contain native 0x{:X}.",
					program->m_name ? program->m_name : "<unnamed>",
					hash);
				continue;
			}

			const auto duplicate = std::find_if(
				m_native_patches.begin(),
				m_native_patches.end(),
				[slot](const native_patch& patch)
				{
					return patch.slot == slot;
				});
			if (duplicate != m_native_patches.end())
			{
				LOG_WARNING("Skipped duplicate Enhanced native patch for slot {}.", static_cast<void*>(slot));
				continue;
			}

			m_native_patches.push_back({hash, slot, *slot, replacement});
			*slot = replacement;
		}

		m_program = program;
		m_vmt_hook = std::move(program_hook);
		s_map[program] = this;
		m_vmt_hook->enable();
	}

	void script_hook::restore_handlers() noexcept
	{
		for (const auto& patch : m_native_patches)
		{
			if (patch.slot && *patch.slot == patch.replacement)
				*patch.slot = patch.original;
		}
		m_native_patches.clear();
	}

	void script_hook::detach() noexcept
	{
		restore_handlers();

		if (m_vmt_hook)
		{
			m_vmt_hook->disable();
			m_vmt_hook.reset();
		}

		if (m_program)
		{
			if (const auto iterator = s_map.find(m_program);
				iterator != s_map.end() && iterator->second == this)
			{
				s_map.erase(iterator);
			}
			m_program = nullptr;
		}
	}

	void script_hook::scrprogram_dtor(rage::scrProgram* this_, bool free_memory)
	{
		const auto iterator = s_map.find(this_);
		if (iterator == s_map.end() || !iterator->second)
			return;

		auto* hook = iterator->second;
		auto original = hook->m_vmt_hook
			? hook->m_vmt_hook->get_original<decltype(&scrprogram_dtor)>(scrprogram_destructor_index)
			: nullptr;

		hook->restore_handlers();
		if (hook->m_vmt_hook)
		{
			hook->m_vmt_hook->disable();
			hook->m_vmt_hook.reset();
		}

		hook->m_program = nullptr;
		s_map.erase(iterator);

		if (original)
			original(this_, free_memory);
	}
}
