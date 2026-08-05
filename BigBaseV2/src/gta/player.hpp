#pragma once

#include "extensible.hpp"
#include "fwddec.hpp"
#include "vector.hpp"

#include <cstddef>
#include <cstdint>

#pragma pack(push, 1)
namespace rage
{
#	pragma warning(push)
#	pragma warning(disable : 4201)
	union netAddress
	{
		std::uint32_t m_raw;
		struct
		{
			std::uint8_t m_field4;
			std::uint8_t m_field3;
			std::uint8_t m_field2;
			std::uint8_t m_field1;
		};
	};
#	pragma warning(pop)

	// Compatibility views retained for the existing feature layer. The public
	// Enhanced reference does not currently provide stable field offsets for
	// the full player objects, so code should prefer virtual accessors/natives.
	class netPlayerData
	{
	public:
		std::uint64_t m_unk1;
		std::uint64_t m_unk2;
		std::uint32_t m_sec_key_time;
		netAddress m_lan_ip;
		std::uint16_t m_lan_port;
		char m_pad1[0x02];
		netAddress m_relay_ip;
		std::uint16_t m_relay_port;
		char m_pad2[0x02];
		netAddress m_online_ip;
		std::uint16_t m_online_port;
		char m_pad3[0x1E];
		std::uint64_t m_rockstar_id;
		bool m_id_flag;
		char m_pad4[0x0B];
		char m_name[0x14];
	};

	class nonPhysicalPlayerDataBase
	{
	public:
		virtual ~nonPhysicalPlayerDataBase() = default;
		virtual void unknown_0x08() = 0;
		virtual void unknown_0x10() = 0;
		virtual void unknown_0x18() = 0;
		virtual void log(netLoggingInterface* logger) = 0;
	};

	class netPlayer
	{
	public:
		virtual ~netPlayer() = default;
		virtual void reset() = 0;
		virtual bool is_valid() const = 0;
		virtual const char* get_name() const = 0;
		virtual void unknown_0x20() = 0;
		virtual bool is_host() = 0;
		virtual netPlayerData* get_net_data() = 0;
		virtual void unknown_0x38() = 0;
	};

	class netPlayerMgrBase
	{
	public:
		virtual ~netPlayerMgrBase() = default;
	};
}

namespace gta
{
	inline constexpr std::size_t num_players = 32;
}

class CNonPhysicalPlayerData : public rage::nonPhysicalPlayerDataBase
{
public:
	std::int32_t m_bubble_id{};
	std::int32_t m_player_id{};
	rage::vector3 m_position{};
};

class CNetGamePlayer : public rage::netPlayer
{
};

class CWantedData
{
public:
	char m_padding[0x98]{};
	std::int32_t m_wanted_level{};
};

class CPlayerInfo : public rage::fwExtensibleBase
{
public:
	char m_padding1[0x1D8]{};
	std::uint32_t m_frame_flags{};
	char m_padding2[0x584]{};
	CWantedData m_wanted_data{};
};

static_assert(sizeof(rage::netAddress) == 0x04);
static_assert(sizeof(CNonPhysicalPlayerData) == 0x20);
#pragma pack(pop)
