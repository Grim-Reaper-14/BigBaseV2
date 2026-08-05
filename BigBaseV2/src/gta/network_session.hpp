#pragma once

#include <cstddef>
#include <cstdint>

namespace rage
{
	struct netSocketAddress final
	{
		union
		{
			std::uint32_t m_packed;
			struct
			{
				std::uint8_t m_field4;
				std::uint8_t m_field3;
				std::uint8_t m_field2;
				std::uint8_t m_field1;
			};
		} m_ip_address{};
		std::uint16_t m_port{};
	};
	static_assert(sizeof(netSocketAddress) == 0x08);

	struct netAddress final
	{
		netSocketAddress m_internal_ip{};
		netSocketAddress m_external_ip{};
		std::uint64_t m_peer_id{};
		std::byte m_pad[6]{};
		std::uint8_t m_connection_type{};
	};
	static_assert(sizeof(netAddress) == 0x20);

	enum class rlPlatform : std::uint8_t
	{
		unknown,
		xbox,
		playstation,
		pc
	};

	class rlGamerHandle final
	{
	public:
		rlGamerHandle() = default;
		explicit rlGamerHandle(std::uint64_t rockstar_id) :
			m_rockstar_id(static_cast<std::int64_t>(rockstar_id)),
			m_platform(static_cast<std::uint8_t>(rlPlatform::pc))
		{
		}

		std::int64_t m_rockstar_id{};
		std::uint8_t m_platform{};
		std::uint8_t m_profile_index{};
		std::byte m_pad[6]{};
	};
	static_assert(sizeof(rlGamerHandle) == 0x10);

	class rlGamerInfoBase final
	{
	public:
		bool m_security_enabled{};
		std::byte m_pad0[7]{};
		std::uint64_t m_peer_id{};
		rlGamerHandle m_gamer_handle{};
		std::byte m_aes_key[0x28]{};
		netAddress m_relay_address{};
		std::byte m_relay_signature[0x40]{};
		netSocketAddress m_external_address{};
		netSocketAddress m_internal_address{};
		std::uint32_t m_nat_type{};
		bool m_force_relays{};
		std::byte m_pad1[3]{};
	};
	static_assert(sizeof(rlGamerInfoBase) == 0xC0);

	class rlSessionInfo final
	{
	public:
		std::uint64_t m_session_id{};
		std::uint64_t m_session_token{};
		rlGamerInfoBase m_host_info{};
	};
	static_assert(sizeof(rlSessionInfo) == 0xD0);

	struct rlTaskStatus final
	{
		int m_status{};
		int m_error_code{};
	};

	class rlSessionByGamerTaskResult final
	{
	public:
		rlGamerHandle m_gamer_handle{};
		rlSessionInfo m_session_info{};
	};

	struct rlScTaskStatus final
	{
		void* m_pad{};
		int m_status{};
		int m_error_code{};
	};

	struct rlGetAvatarsPlayerList final
	{
		union
		{
			char m_player_names[51][250];
			char m_player_handles[51][250];
		};
		int m_num_entries{};
	};
	static_assert(sizeof(rlGetAvatarsPlayerList) == 0x31D4);

	struct rlGetAvatarsResult final
	{
		std::uint64_t m_rockstar_id{};
		char m_nickname[51]{};
		char m_avatar_url[127]{};
	};
	static_assert(sizeof(rlGetAvatarsResult) == 0xC0);

	struct rlGetAvatarsResults final
	{
		rlGetAvatarsResult m_results[250]{};
		int m_num_results{};
	};
	static_assert(sizeof(rlGetAvatarsResults) == 0xBB88);

	class rlGetAvatarsContext
	{
	public:
		enum class type : std::uint8_t
		{
			by_nickname,
			by_rockstar_id
		};

		enum class avatar_size : std::uint32_t
		{
			small = 1,
			medium = 2,
			large = 4
		};

		virtual ~rlGetAvatarsContext() = default;

		std::byte m_pad0[80]{};
		void* m_result{};
		std::byte m_pad1[4]{};
		type m_type{};
		avatar_size m_avatar_size{};
		rlScTaskStatus m_status{};
	};
	static_assert(sizeof(rlGetAvatarsContext) == 0x80);
}
