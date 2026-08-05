#pragma once

#include <cmath>
#include <cstddef>

namespace rage
{
	struct vector2
	{
		float x{};
		float y{};
	};

	// GTA V Enhanced aligns three-component engine vectors to 16 bytes.
	struct alignas(16) vector3
	{
		float x{};
		float y{};
		float z{};

		[[nodiscard]] constexpr bool operator==(const vector3& other) const noexcept
		{
			return x == other.x && y == other.y && z == other.z;
		}

		[[nodiscard]] constexpr bool operator!=(const vector3& other) const noexcept
		{
			return !(*this == other);
		}

		[[nodiscard]] constexpr vector3 operator+(const vector3& other) const noexcept
		{
			return {x + other.x, y + other.y, z + other.z};
		}

		[[nodiscard]] constexpr vector3 operator-(const vector3& other) const noexcept
		{
			return {x - other.x, y - other.y, z - other.z};
		}

		[[nodiscard]] constexpr vector3 operator*(float scalar) const noexcept
		{
			return {x * scalar, y * scalar, z * scalar};
		}

		[[nodiscard]] float magnitude() const noexcept
		{
			return std::sqrt((x * x) + (y * y) + (z * z));
		}

		[[nodiscard]] float distance(const vector3& other) const noexcept
		{
			return (*this - other).magnitude();
		}
	};

	struct vector4
	{
		float x{};
		float y{};
		float z{};
		float w{};
	};

	using fvector2 = vector2;
	using fvector3 = vector3;
	using fvector4 = vector4;

	// Script vectors use one eight-byte slot per component in native calls.
	class scrVector final
	{
	public:
		constexpr scrVector() = default;
		constexpr scrVector(float x_value, float y_value, float z_value) noexcept :
			x(x_value),
			y(y_value),
			z(z_value)
		{
		}

		constexpr scrVector(const fvector3& value) noexcept :
			x(value.x),
			y(value.y),
			z(value.z)
		{
		}

		[[nodiscard]] constexpr operator fvector3() const noexcept
		{
			return {x, y, z};
		}

		[[nodiscard]] constexpr explicit operator bool() const noexcept
		{
			return x != 0.0f || y != 0.0f || z != 0.0f;
		}

		alignas(8) float x{};
		alignas(8) float y{};
		alignas(8) float z{};
	};

	static_assert(sizeof(vector2) == 0x08);
	static_assert(sizeof(vector3) == 0x10);
	static_assert(sizeof(vector4) == 0x10);
	static_assert(offsetof(scrVector, x) == 0x00);
	static_assert(offsetof(scrVector, y) == 0x08);
	static_assert(offsetof(scrVector, z) == 0x10);
	static_assert(sizeof(scrVector) == 0x18);
}
