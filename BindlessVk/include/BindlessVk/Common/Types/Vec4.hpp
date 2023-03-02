#pragma once

#include "BindlessVk/Common/Aliases.hpp"
#include "BindlessVk/Common/Types/Vec2.hpp"
#include "BindlessVk/Common/Types/Vec3.hpp"

namespace BINDLESSVK_NAMESPACE {

struct vec4
{
	union {
		f32 values[4];

		struct
		{
			f32 x, y, z, w;
		};

		struct
		{
			f32 r, g, b, a;
		};
	};

	vec4() = default;

	vec4(f32 const xyzw): x(xyzw), y(xyzw), z(xyzw), w(xyzw)
	{
	}

	vec4(f32 const x, f32 const y, f32 const z, f32 const w): x(x), y(y), z(z), w(w)
	{
	}

	vec4(vec2 const &xy, vec2 const &zw): x(xy.x), y(xy.y), z(zw.x), w(zw.y)
	{
	}

	vec4(vec3 const &xyz, f32 const w): x(xyz.x), y(xyz.y), z(xyz.z), w(w)
	{
	}

	vec4(f32 const *const ptr)
	{
		memcpy(this, ptr, sizeof(vec4));
	}

	vec4(f64 const *const ptr)
	{
		x = ptr[0];
		y = ptr[1];
		z = ptr[2];
		w = ptr[3];
	}

	inline vec4 unit() const
	{
		auto const mag = sqrt(x * x + y * y + z * z + w * w);
		return vec4(x / mag, y / mag, z / mag, w / mag);
	}

	inline f32 &operator[](usize const index)
	{
		return values[index];
	}

	inline f32 operator[](usize const index) const
	{
		return values[index];
	}

	inline vec4 operator+(vec4 const &rhs) const
	{
		return vec4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
	}

	inline vec4 operator*(vec4 const &rhs) const
	{
		return vec4(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w);
	}

	inline vec4 operator*(f32 const scalar) const
	{
		return vec4(x * scalar, y * scalar, z * scalar, w * scalar);
	}

	inline void operator+=(vec4 const &rhs)
	{
		*this = *this + rhs;
	}

	inline void operator*=(vec4 const &rhs)
	{
		*this = *this * rhs;
	}

	inline void operator*=(f32 const scalar)
	{
		*this = *this * scalar;
	}

// Define additional constructors and implicit cast operators to convert back and forth between
// your math types and vec4
#ifdef BINDLESSVK_VEC4_EXTRA
	BINDLESSVK_VEC4_EXTRA
#endif
};

} // namespace BINDLESSVK_NAMESPACE
