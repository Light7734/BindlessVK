#pragma once

#include "BindlessVk/Common/Aliases.hpp"
#include "BindlessVk/Common/Types/Vec2.hpp"

namespace BINDLESSVK_NAMESPACE {

struct vec3
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

	vec3() = default;

	vec3(f32 const xyz): x(xyz), y(xyz), z(xyz)
	{
	}

	vec3(f32 const x, f32 const y, f32 const z): x(x), y(y), z(z)
	{
	}

	vec3(vec2 const xy, f32 const z): x(xy.x), y(xy.y), z(z)
	{
	}

	vec3(f32 const *const ptr)
	{
		memcpy(this, ptr, sizeof(vec3));
	}

	vec3(f64 const *const ptr)
	{
		x = ptr[0];
		y = ptr[1];
		z = ptr[2];
	}

	inline vec3 unit() const
	{
		auto const mag = sqrt(x * x + y * y + z * z);
		return vec3(x / mag, y / mag, z / mag);
	}

	inline f32 &operator[](usize const index)
	{
		return values[index];
	}

	inline f32 operator[](usize const index) const
	{
		return values[index];
	}

	inline vec3 operator+(vec3 const &rhs)
	{
		return vec3(x + rhs.x, y + rhs.y, z + rhs.z);
	}

	inline vec3 operator*(vec3 const &rhs)
	{
		return vec3(x * rhs.x, y * rhs.y, z * rhs.z);
	}

	inline vec3 operator*(f32 const scalar)
	{
		return vec3(x * scalar, y * scalar, z * scalar);
	}

	inline void operator+=(vec3 const &rhs)
	{
		*this = *this + rhs;
	}

	inline void operator*=(vec3 const &rhs)
	{
		*this = *this * rhs;
	}

	inline void operator*=(f32 const scalar)
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
