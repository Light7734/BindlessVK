#pragma once

#include "BindlessVk/Common/Aliases.hpp"
#include "BindlessVk/Common/Types/Vec2.hpp"

namespace BINDLESSVK_NAMESPACE {

struct Vec3f
{
	union {
		f32 values[3];

		struct
		{
			f32 x, y, z;
		};

		struct
		{
			f32 r, g, b;
		};
	};

	Vec3f() = default;

	Vec3f(f32 const xyz): x(xyz), y(xyz), z(xyz)
	{
	}

	Vec3f(f32 const x, f32 const y, f32 const z): x(x), y(y), z(z)
	{
	}

	Vec3f(Vec3f const xy, f32 const z): x(xy.x), y(xy.y), z(z)
	{
	}

	Vec3f(f32 const *const ptr)
	{
		memcpy(this, ptr, sizeof(Vec3f));
	}

	Vec3f(f64 const *const ptr)
	{
		x = ptr[0];
		y = ptr[1];
		z = ptr[2];
	}

	auto unit() const
	{
		auto const mag = sqrt(x * x + y * y + z * z);
		return Vec3f(x / mag, y / mag, z / mag);
	}

	auto &operator[](usize const index)
	{
		return values[index];
	}

	auto operator[](usize const index) const
	{
		return values[index];
	}

	auto operator+(Vec3f const &rhs)
	{
		return Vec3f(x + rhs.x, y + rhs.y, z + rhs.z);
	}

	auto operator*(Vec3f const &rhs)
	{
		return Vec3f(x * rhs.x, y * rhs.y, z * rhs.z);
	}

	auto operator*(f32 const scalar)
	{
		return Vec3f(x * scalar, y * scalar, z * scalar);
	}

	void operator+=(Vec3f const &rhs)
	{
		*this = *this + rhs;
	}

	void operator*=(Vec3f const &rhs)
	{
		*this = *this * rhs;
	}

	void operator*=(f32 const scalar)
	{
		*this = *this * scalar;
	}

// Define additional constructors and implicit cast operators to convert back and forth between
// your math types and vec3
#ifdef BINDLESSVK_VEC3_EXTRA
	BINDLESSVK_VEC3_EXTRA
#endif
};

} // namespace BINDLESSVK_NAMESPACE
