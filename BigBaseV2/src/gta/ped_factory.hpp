#pragma once

#include "fwddec.hpp"

#include <cstddef>
#include <cstdint>

class CPed
{
public:
	// Enhanced field offsets are isolated behind accessors so this header does
	// not pretend to describe the complete CPed object.
	inline static constexpr std::ptrdiff_t player_info_offset = 0x10B8;

	[[nodiscard]] CPlayerInfo* player_info() noexcept
	{
		return *reinterpret_cast<CPlayerInfo**>(
			reinterpret_cast<std::byte*>(this) + player_info_offset);
	}

	[[nodiscard]] const CPlayerInfo* player_info() const noexcept
	{
		return *reinterpret_cast<CPlayerInfo* const*>(
			reinterpret_cast<const std::byte*>(this) + player_info_offset);
	}
};

class CPedFactory
{
public:
	enum class ped_create_flags : std::uint8_t
	{
		is_networked = 1u << 0,
		is_player = 1u << 1,
	};

	virtual ~CPedFactory() = default;
	virtual CPed* create_ped(
		std::uint8_t* flags,
		std::uint16_t* model_index,
		void* matrix,
		bool default_component_variation,
		bool register_network_object,
		bool give_default_loadout,
		bool,
		bool) = 0;
	virtual CPed* create_clone(
		std::uint8_t* flags,
		std::uint16_t* model_index,
		void* matrix,
		bool default_component_variation,
		bool,
		bool register_network_object,
		bool) = 0;
	virtual CPed* clone_ped(
		CPed* ped,
		bool register_network_object,
		bool link_blends,
		bool clone_compressed_damage) = 0;
	virtual CPed* clone_ped_to_target(
		CPed* source,
		CPed* target,
		bool clone_compressed_damage) = 0;
	virtual CPed* create_player(
		std::uint8_t* flags,
		std::uint16_t model_index,
		void* matrix,
		CPlayerInfo* player_info) = 0;
	virtual void destroy_ped(CPed* ped) = 0;

	CPed* m_local_ped{}; // 0x08
};

static_assert(sizeof(CPedFactory) == 0x10);
