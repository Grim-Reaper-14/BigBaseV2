#pragma once

#include "extensible.hpp"
#include "fwddec.hpp"
#include "vector.hpp"

#include <cstddef>
#include <cstdint>

namespace rage
{
#	pragma warning(push)
#	pragma warning(disable : 4201)
	union netAddress
	{
		std::uint32_t m_raw{};
		struct
		{
			std::uint8_t m_field4;
			std::uint8_t m_field3;
			std::uint8_t m_field2;
			std::uint8_t m_field1;
		};
	};
#	pragma warning(pop)

	class netPlayerData
	{
	public:
		std::uint64_t m_unknown_00{};
		std::uint64_t m_unknown_08{};
		std::uint32_t m_security_key_time{};
		netAddress m_lan_ip{};
		std::uint16_t m_lan_port{};
		std::byte m_padding_1A[0x02]{};
		netAddress m_relay_ip{};
		std::uint16_t m_relay_port{};
		std::byte m_padding_22[0x02]{};
		netAddress m_online_ip{};
		std::uint16_t m_online_port{};
		std::byte m_padding_2A[0x1E]{};
		std::uint64_t m_rockstar_id{};
		bool m_id_flag{};
		std::byte m_padding_51[0x0B]{};
		char m_name[0x14]{};
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

#pragma pack(push, 8)
	class netPlayer
	{
	private:
		// Verified RAGE RTTI vtable prefix used by Enhanced network objects.
		virtual void* rtti_0x00() = 0;
		virtual void* rtti_0x08() = 0;
		virtual std::uint32_t rtti_0x10() = 0;
		virtual netPlayer* rtti_0x18(void*) = 0;
		virtual bool rtti_0x20(void*) = 0;
		virtual bool rtti_0x28(void**) = 0;
		virtual void destroy() = 0;

	public:
		virtual void reset() = 0;
		virtual bool is_physical() = 0;
		virtual const char* get_name() = 0;
		virtual std::uint64_t get_host_token() = 0;
		virtual void update_permissions() = 0;
		virtual bool is_host() = 0;
		virtual rlGamerInfo* get_gamer_info() = 0;
		virtual void update_unknown() = 0;

		[[nodiscard]] bool is_local() const noexcept
		{
			return (m_flags & 1u) != 0;
		}

		std::int32_t m_account_id{};                     // 0x008
		std::byte m_padding_0C[0x04]{};
		std::int64_t m_rockstar_id{};                    // 0x010
		std::byte m_platform_account_id[0x30]{};          // 0x018
		std::uint32_t m_unknown_48{};                    // 0x048
		std::byte m_padding_4C[0x04]{};
		CNonPhysicalPlayerData* m_non_physical_player{}; // 0x050
		std::uint32_t m_message_id{};                    // 0x058
		std::byte m_padding_5C[0x04]{};
		std::uint8_t m_active_index{};                   // 0x060
		std::uint8_t m_player_index{};                   // 0x061
		std::byte m_padding_62[0x6E]{};
		std::uint8_t m_flags{};                          // 0x0D0
		std::byte m_padding_D1[0x0F]{};
	};
#pragma pack(pop)

	class netPlayerMgrBase
	{
	public:
		virtual ~netPlayerMgrBase() = default;
		virtual void initialize() = 0;
		virtual void shutdown() = 0;
		virtual void update() = 0;
		virtual CNetGamePlayer* add_player(
			rlGamerInfo* gamer_info,
			std::uint32_t flags,
			CNetGamePlayerDataMsg* player_data,
			CNonPhysicalPlayerData* non_physical_player_data) = 0;
		virtual void remove_player(CNetGamePlayer* player) = 0;
		virtual void update_player_lists_for_player(CNetGamePlayer* player) = 0;
		virtual CNetGamePlayer* add_player_secondary(
			rlGamerInfo* gamer_info,
			std::uint32_t flags,
			CNetGamePlayerDataMsg* player_data,
			CNonPhysicalPlayerData* non_physical_player_data) = 0;

		netConnectionManager* m_connection_manager{}; // 0x008
		void* m_bandwidth_manager{};                  // 0x010
		std::byte m_padding_018[0xD8]{};
		CNetGamePlayer* m_local_player{};             // 0x0F0
		std::byte m_padding_0F8[0x90]{};
		CNetGamePlayer* m_players[32]{};              // 0x188
		std::uint32_t m_max_players{};                // 0x288
		std::byte m_padding_28C[0x04]{};
		std::int32_t m_unloaded_player_count{};       // 0x290
		std::int32_t m_loaded_player_count{};         // 0x294
		std::int32_t m_loaded_non_local_count{};      // 0x298
		std::int32_t m_physical_player_count{};       // 0x29C
		std::int32_t m_local_physical_count{};        // 0x2A0
		std::int32_t m_non_local_physical_count{};    // 0x2A4
		std::byte m_padding_2A8[0x648]{};
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

class CPlayerInfo : public rage::fwExtensibleBase
{
};

class CNetGamePlayer : public rage::netPlayer
{
public:
	void* m_unknown_E0{};
	CPlayerInfo* m_player_info{};
	std::byte m_padding_F0[0x280]{};
};

class CNetworkPlayerMgr : public rage::netPlayerMgrBase
{
};

static_assert(sizeof(rage::netAddress) == 0x04);
static_assert(sizeof(CNonPhysicalPlayerData) == 0x20);
static_assert(sizeof(rage::netPlayer) == 0xE0);
static_assert(sizeof(CNetGamePlayer) == 0x370);
static_assert(sizeof(rage::netPlayerMgrBase) == 0x8F0);
static_assert(sizeof(CNetworkPlayerMgr) == 0x8F0);
