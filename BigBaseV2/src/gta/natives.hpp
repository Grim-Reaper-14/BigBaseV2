#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#include "fwddec.hpp"
#include "vector.hpp"

namespace rage
{
	class scrNativeCallContext
	{
	public:
		void reset() noexcept
		{
			m_arg_count = 0;
			m_vector_ref_count = 0;
		}

		template <typename T>
		void push_arg(T&& value) noexcept
		{
			using value_type = std::remove_cv_t<std::remove_reference_t<T>>;
			static_assert(sizeof(value_type) <= sizeof(std::uint64_t), "Native arguments must fit in one script slot.");
			static_assert(std::is_trivially_copyable_v<value_type>, "Native arguments must be trivially copyable.");

			auto* slot = reinterpret_cast<std::uint64_t*>(m_args) + m_arg_count++;
			*reinterpret_cast<value_type*>(slot) = std::forward<T>(value);
		}

		template <typename T>
		[[nodiscard]] T& get_arg(std::size_t index) noexcept
		{
			static_assert(sizeof(T) <= sizeof(std::uint64_t), "Native arguments must fit in one script slot.");
			return *reinterpret_cast<T*>(reinterpret_cast<std::uint64_t*>(m_args) + index);
		}

		template <typename T>
		void set_arg(std::size_t index, T&& value) noexcept
		{
			using value_type = std::remove_cv_t<std::remove_reference_t<T>>;
			static_assert(sizeof(value_type) <= sizeof(std::uint64_t), "Native arguments must fit in one script slot.");
			static_assert(std::is_trivially_copyable_v<value_type>, "Native arguments must be trivially copyable.");

			auto* slot = reinterpret_cast<std::uint64_t*>(m_args) + index;
			*reinterpret_cast<value_type*>(slot) = std::forward<T>(value);
		}

		template <typename T>
		[[nodiscard]] T* get_return_value() noexcept
		{
			return reinterpret_cast<T*>(m_return_value);
		}

		template <typename T>
		void set_return_value(T&& value) noexcept
		{
			using value_type = std::remove_cv_t<std::remove_reference_t<T>>;
			static_assert(std::is_trivially_copyable_v<value_type>, "Native return values must be trivially copyable.");
			*reinterpret_cast<value_type*>(m_return_value) = std::forward<T>(value);
		}

		void fix_vectors() noexcept
		{
			for (std::int32_t index = 0; index < m_vector_ref_count && index < 4; ++index)
			{
				if (m_vector_ref_targets[index])
					*m_vector_ref_targets[index] = scrVector{m_vector_ref_sources[index]};
			}
			m_vector_ref_count = 0;
		}

	protected:
		void* m_return_value{};                    // 0x00
		std::uint32_t m_arg_count{};               // 0x08
		void* m_args{};                            // 0x10
		std::int32_t m_vector_ref_count{};         // 0x18
		scrVector* m_vector_ref_targets[4]{};       // 0x20
		fvector3 m_vector_ref_sources[4]{};        // 0x40
	};

	using scrNativeHash = std::uint64_t;
	using scrNativeMapping = std::pair<scrNativeHash, scrNativeHash>;
	using scrNativeHandler = void (*)(scrNativeCallContext*);

	class scrNativeRegistration;

#pragma pack(push, 1)
	class scrNativeRegistrationTable
	{
		scrNativeRegistration* m_entries[0xFF]{};
		std::uint32_t m_unk{};
		bool m_initialized{};
	};
#pragma pack(pop)

	static_assert(sizeof(scrNativeCallContext) == 0x80);
}

using Void = void;
using Any = std::uint32_t;
using Hash = std::uint32_t;
using Entity = std::int32_t;
using Player = std::int32_t;
using Ped = Entity;
using Vehicle = Entity;
using Cam = std::int32_t;
using Object = Entity;
using Pickup = Object;
using Blip = std::int32_t;
using Camera = Entity;
using ScrHandle = Entity;
using Vector3 = rage::scrVector;
