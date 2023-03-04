#pragma once

#include "BindlessVk/Common/Aliases.hpp"
#include "BindlessVk/Common/Types/Vec2.hpp"
#include "BindlessVk/Common/Types/Vec3.hpp"

namespace BINDLESSVK_NAMESPACE {

struct Vec4f
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

	Vec4f() = default;

	Vec4f(f32 const xyzw): x(xyzw), y(xyzw), z(xyzw), w(xyzw)
	{
	}

	Vec4f(f32 const x, f32 const y, f32 const z, f32 const w): x(x), y(y), z(z), w(w)
	{
	}

	Vec4f(Vec4f const &xy, Vec4f const &zw): x(xy.x), y(xy.y), z(zw.x), w(zw.y)
	{
	}

	Vec4f(Vec4f const &xyz, f32 const w): x(xyz.x), y(xyz.y), z(xyz.z), w(w)
	{
	}

	Vec4f(f32 const *const ptr)
	{
		memcpy(this, ptr, sizeof(Vec4f));
	}

	Vec4f(f64 const *const ptr)
	{
		x = ptr[0];
		y = ptr[1];
		z = ptr[2];
		w = ptr[3];
	}

	inline Vec4f unit() const
	{
		auto const mag = sqrt(x * x + y * y + z * z + w * w);
		return Vec4f(x / mag, y / mag, z / mag, w / mag);
	}

	inline f32 &operator[](usize const index)
	{
		return values[index];
	}

	inline f32 operator[](usize const index) const
	{
		return values[index];
	}

	inline Vec4f operator+(Vec4f const &rhs) const
	{
		return Vec4f(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
	}

	inline Vec4f operator*(Vec4f const &rhs) const
	{
		return Vec4f(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w);
	}

	inline Vec4f operator*(f32 const scalar) const
	{
		return Vec4f(x * scalar, y * scalar, z * scalar, w * scalar);
	}

	inline void operator+=(Vec4f const &rhs)
	{
		*this = *this + rhs;
	}

	inline void operator*=(Vec4f const &rhs)
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
