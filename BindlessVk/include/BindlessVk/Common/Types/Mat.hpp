#pragma once

#include "BindlessVk/Common/Aliases.hpp"
#include "BindlessVk/Common/Types/Vec4.hpp"

namespace BINDLESSVK_NAMESPACE {

struct mat4f
{
	union {
		struct
		{
			f32 elements[4 * 4];
		};

		struct
		{
			vec4 rows[4];
		};
	};

	mat4f() = default;

	mat4f(f32 const *const ptr)
	{
		memcpy(this, ptr, sizeof(mat4f));
	}

	mat4f(f64 const *const ptr)
	{
		for (usize i = 0; i < 4 * 4; ++i)
			elements[i] = ptr[i];
	}

	mat4f(vec4 const &q)
	{
		*this = mat4f(1.0f);
		f32 qxx(q.x * q.x);
		f32 qyy(q.y * q.y);
		f32 qzz(q.z * q.z);
		f32 qxz(q.x * q.z);
		f32 qxy(q.x * q.y);
		f32 qyz(q.y * q.z);
		f32 qwx(q.w * q.x);
		f32 qwy(q.w * q.y);
		f32 qwz(q.w * q.z);

		rows[0][0] = f32(1) - f32(2) * (qyy + qzz);
		rows[0][1] = f32(2) * (qxy + qwz);
		rows[0][2] = f32(2) * (qxz - qwy);

		rows[1][0] = f32(2) * (qxy - qwz);
		rows[1][1] = f32(1) - f32(2) * (qxx + qzz);
		rows[1][2] = f32(2) * (qyz + qwx);

		rows[2][0] = f32(2) * (qxz + qwy);
		rows[2][1] = f32(2) * (qyz - qwx);
		rows[2][2] = f32(1) - f32(2) * (qxx + qyy);
	}

	inline void translate(vec3 const &v)
	{
		rows[3] += ((rows[0]) * (v[0])) + (rows[1] * v[1]) + (rows[2] * v[2]);
	}

	inline void rotate(vec3 const &v)
	{
	}

	inline void scale(vec3 const &v)
	{
		rows[0] *= v[0];
		rows[1] *= v[1];
		rows[2] *= v[2];
	}

	inline void operator*=(mat4f const &rhs)
	{
		auto const SrcA0 = rows[0];
		auto const SrcA1 = rows[1];
		auto const SrcA2 = rows[2];
		auto const SrcA3 = rows[3];

		auto const SrcB0 = rhs.rows[0];
		auto const SrcB1 = rhs.rows[1];
		auto const SrcB2 = rhs.rows[2];
		auto const SrcB3 = rhs.rows[3];

		rows[0] = SrcA0 * SrcB0[0] + SrcA1 * SrcB0[1] + SrcA2 * SrcB0[2] + SrcA3 * SrcB0[3];
		rows[1] = SrcA0 * SrcB1[0] + SrcA1 * SrcB1[1] + SrcA2 * SrcB1[2] + SrcA3 * SrcB1[3];
		rows[2] = SrcA0 * SrcB2[0] + SrcA1 * SrcB2[1] + SrcA2 * SrcB2[2] + SrcA3 * SrcB2[3];
		rows[3] = SrcA0 * SrcB3[0] + SrcA1 * SrcB3[1] + SrcA2 * SrcB3[2] + SrcA3 * SrcB3[3];
	}
};

} // namespace BINDLESSVK_NAMESPACE
