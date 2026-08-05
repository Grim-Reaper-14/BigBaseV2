#pragma once

#include "fwddec.hpp"
#include "tls_context.hpp"

#include <cstddef>
#include <cstdint>

namespace rage
{
	enum class eThreadState : std::uint32_t
	{
		idle,
		running,
		killed,
		paused,
		unknown_4,
	};

	struct scrThreadContext
	{
		std::uint32_t m_thread_id{};           // 0x00
		std::uint32_t m_padding0{};            // 0x04
		std::uint64_t m_script_hash{};         // 0x08
		eThreadState m_state{};                // 0x10
		std::uint32_t m_instruction_pointer{}; // 0x14
		std::uint32_t m_frame_pointer{};       // 0x18
		std::uint32_t m_stack_pointer{};       // 0x1C
		float m_timer_a{};                     // 0x20
		float m_timer_b{};                     // 0x24
		float m_wait_timer{};                  // 0x28
		std::byte m_padding1[0x2C]{};          // 0x2C
		std::uint32_t m_stack_size{};          // 0x58
		std::byte m_padding2[0x54]{};          // 0x5C
	};

	class scrThread
	{
	public:
		virtual ~scrThread() = default;
		virtual void reset(std::uint64_t script_hash, void* args, std::uint32_t arg_count) = 0;
		virtual eThreadState run_impl() = 0;
		virtual eThreadState run() = 0;
		virtual void kill() = 0;
		virtual void get_info(void* info) = 0;

		[[nodiscard]] static scrThread* get() noexcept
		{
			auto* context = tlsContext::get();
			return context ? context->m_script_thread : nullptr;
		}

		scrThreadContext m_context;         // 0x008
		void* m_stack{};                    // 0x0B8
		std::byte m_padding0[0x04]{};       // 0x0C0
		std::uint32_t m_parameter_size{};   // 0x0C4
		std::uint32_t m_parameter_location{};// 0x0C8
		std::byte m_padding1[0x04]{};       // 0x0CC
		char m_error_message[0x80]{};       // 0x0D0
		std::uint32_t m_script_hash{};      // 0x150
		char m_name[0x40]{};                // 0x154
	};

	static_assert(offsetof(scrThreadContext, m_script_hash) == 0x08);
	static_assert(offsetof(scrThreadContext, m_stack_size) == 0x58);
	static_assert(sizeof(scrThreadContext) == 0xB0);
	static_assert(offsetof(scrThread, m_context) == 0x08);
	static_assert(offsetof(scrThread, m_script_hash) == 0x150);
	static_assert(sizeof(scrThread) == 0x198);
}

class GtaThread : public rage::scrThread
{
public:
	scriptHandler* m_handler{};                   // 0x198
	scriptHandlerNetComponent* m_net_component{}; // 0x1A0
	std::uint32_t m_script_hash2{};               // 0x1A8
	std::uint32_t m_padding3{};                   // 0x1AC
};

static_assert(sizeof(GtaThread) == 0x1B0);
