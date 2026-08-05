#pragma once

#include "fwddec.hpp"
#include "joaat.hpp"

#include <cstdint>

namespace rage
{
	class scriptIdBase
	{
	public:
		virtual ~scriptIdBase() = default; // 0x00
		virtual void assume_thread_identity(scrThread*) {} // 0x08
		virtual bool is_valid() { return false; } // 0x10
		virtual joaat_t* get_hash(joaat_t*) { return nullptr; } // 0x18
		virtual std::uint32_t* get_unique_id(std::uint32_t*) { return nullptr; } // 0x20
		virtual const char* get_debug_name() { return nullptr; } // 0x28
		virtual void deserialize(datBitBuffer*) {} // 0x30
		virtual void serialize(datBitBuffer*) {} // 0x38
		virtual std::uint32_t get_size() { return 0; } // 0x40
		virtual std::uint32_t get_size_with_header() { return 0; } // 0x48
		virtual void log_information(netLoggingInterface*) {} // 0x50
		virtual void copy_data(scriptIdBase*) {} // 0x58
		virtual bool operator==(scriptIdBase*) { return false; } // 0x60
		virtual void populate_script_name() {} // 0x68
		virtual bool is_handler_for_same_script(scriptIdBase*) { return false; } // 0x70
	};

	class scriptId : public scriptIdBase
	{
	public:
		joaat_t m_hash{};     // 0x08
		char m_name[0x20]{};  // 0x0C
	};

	static_assert(sizeof(scriptId) == 0x30);
}

class CGameScriptId : public rage::scriptId
{
public:
	std::int32_t m_timestamp{};     // 0x30
	std::int32_t m_position_hash{}; // 0x34
	std::int32_t m_instance_id{};   // 0x38
	std::int32_t m_unique_id{};     // 0x3C
};

static_assert(sizeof(CGameScriptId) == 0x40);
