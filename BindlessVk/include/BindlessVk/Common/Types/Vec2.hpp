#pragma once

#include "BindlessVk/Common/Aliases.hpp"

namespace BINDLESSVK_NAMESPACE {

struct vec2
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

	vec2() = default;

	vec2(f32 const xy): x(xy), y(xy)
	{
	}

	vec2(f32 const x, f32 const y): x(x), y(y)
	{
	}

	vec2(f32 const *const ptr)
	{
		memcpy(this, ptr, sizeof(vec2));
	}

	vec2(f64 const *const ptr)
	{
		x = ptr[0];
		y = ptr[1];
	}

	inline vec2 unit() const
	{
		auto const mag = sqrt(x * x + y * y);
		return vec2(x / mag, y / mag);
	}

	inline f32 &operator[](usize const index)
	{
		return values[index];
	}

	inline f32 operator[](usize const index) const
	{
		return values[index];
	}

	inline vec2 operator+(vec2 const &rhs) const
	{
		return vec2(x + rhs.x, y + rhs.y);
	}

	inline vec2 operator*(vec2 const &rhs) const
	{
		return vec2(x * rhs.x, y * rhs.y);
	}

	inline vec2 operator*(f32 const scalar) const
	{
		return vec2(x * scalar, y * scalar);
	}

	inline void operator+=(vec2 const &rhs)
	{
		*this = *this + rhs;
	}

	inline void operator*=(vec2 const &rhs)
	{
		*this = *this * rhs;
	}

	inline void operator*=(f32 const scalar)
	{
		*this = *this * scalar;
	}

// Define additional constructors and implicit cast operators to convert back and forth between
// your math types and vec2
#ifdef BINDLESSVK_VEC2_EXTRA
	BINDLESSVK_VEC2_EXTRA
#endif
};

} // namespace BINDLESSVK_NAMESPACE
