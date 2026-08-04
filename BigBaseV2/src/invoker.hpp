#pragma once
#include "common.hpp"
#include "gta/natives.hpp"

namespace big
{
	class native_call_context : public rage::scrNativeCallContext
	{
	public:
		native_call_context();
	private:
		std::uint64_t m_return_stack[10];
		std::uint64_t m_arg_stack[100];
	};

	struct native_cache_stats final
	{
		std::size_t mappings{};
		std::size_t cached{};
		std::size_t missing{};
		std::size_t duplicate_original_hashes{};
		std::size_t direct_hash_fallbacks{};

		[[nodiscard]] bool healthy() const noexcept
		{
			return mappings > 0 && cached > 0 && missing == 0 && duplicate_original_hashes == 0;
		}
	};

	class native_invoker
	{
	public:
		explicit native_invoker() = default;
		~native_invoker() = default;

		native_cache_stats cache_handlers();

		void begin_call();
		void end_call(rage::scrNativeHash hash);

		[[nodiscard]] const native_cache_stats& cache_stats() const noexcept
		{
			return m_cache_stats;
		}

		template <typename T>
		void push_arg(T &&value)
		{
			m_call_context.push_arg(std::forward<T>(value));
		}

		template <typename T>
		T &get_return_value()
		{
			return *m_call_context.get_return_value<T>();
		}
	private:
		native_call_context m_call_context;
		std::unordered_map<rage::scrNativeHash, rage::scrNativeHandler> m_handler_cache;
		native_cache_stats m_cache_stats{};
	};

	inline native_invoker g_native_invoker;
}
