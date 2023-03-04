#pragma once

#include "BindlessVk/Common/Aliases.hpp"

namespace BINDLESSVK_NAMESPACE {

struct Vec2f
{
	union {
		f32 values[2];

		struct
		{
			f32 x, y;
		};

		struct
		{
			f32 r, g;
		};
	};

	Vec2f() = default;

	Vec2f(f32 const xy): x(xy), y(xy)
	{
	}

	Vec2f(f32 const x, f32 const y): x(x), y(y)
	{
	}

	Vec2f(f32 const *const ptr)
	{
		memcpy(this, ptr, sizeof(Vec2f));
	}

	Vec2f(f64 const *const ptr)
	{
		x = ptr[0];
		y = ptr[1];
	}

	inline Vec2f unit() const
	{
		auto const mag = sqrt(x * x + y * y);
		return Vec2f(x / mag, y / mag);
	}

	inline f32 &operator[](usize const index)
	{
		return values[index];
	}

	inline f32 operator[](usize const index) const
	{
		return values[index];
	}

	inline Vec2f operator+(Vec2f const &rhs) const
	{
		return Vec2f(x + rhs.x, y + rhs.y);
	}

	inline Vec2f operator*(Vec2f const &rhs) const
	{
		return Vec2f(x * rhs.x, y * rhs.y);
	}

	inline Vec2f operator*(f32 const scalar) const
	{
		return Vec2f(x * scalar, y * scalar);
	}

	inline void operator+=(Vec2f const &rhs)
	{
		*this = *this + rhs;
	}

	inline void operator*=(Vec2f const &rhs)
	{
		*this = *this * rhs;
	}

	inline void operator*=(f32 const scalar)
	{
		*this = *this * scalar;
	}

// Define additional constructors and implicit cast operators to convert back and forth between
// your math types and Vec2f
#ifdef BINDLESSVK_Vec2f_EXTRA
	BINDLESSVK_Vec2f_EXTRA
#endif
};

} // namespace BINDLESSVK_NAMESPACE
