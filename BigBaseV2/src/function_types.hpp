#pragma once
#include "common.hpp"
#include "gta/fwddec.hpp"
#include "gta/natives.hpp"

namespace big::functions
{
	using run_script_threads_t = bool(*)(std::uint32_t ops_to_execute);
	using get_native_handler_t = rage::scrNativeHandler(*)(rage::scrNativeRegistrationTable*, rage::scrNativeHash);
	using init_native_tables_t = void(*)(rage::scrProgram* program);
	using fix_vectors_t = void(*)(rage::scrNativeCallContext*);
	using handle_to_ptr_t = rage::fwEntity*(*)(std::int32_t handle);
	using ptr_to_handle_t = std::int32_t(*)(rage::fwEntity* entity);
}
