#pragma once

#include "Framework/Common/Aliases.hpp"

#include <fmt/format.h>
#include <source_location>

// same as BindlessVK, w/out being inside a namespace
struct FrameworkException: std::exception
{
	FrameworkException(str const &what): msg(what)
	{
	}

	auto what() const noexcept -> c_str final
	{
		return msg.c_str();
	}

	str msg;
};

template<typename Expr, typename... Args>
void inline assert_true(const Expr &expr, fmt::format_string<Args...> fmt = "", Args &&...args)
{
	if (!static_cast<bool>(expr)) [[unlikely]]
		throw FrameworkException(fmt::format(fmt, std::forward<Args>(args)...));
}

template<typename T, typename... Args>
void inline assert_false(const T &expr, fmt::format_string<Args...> fmt = "", Args &&...args)
{
	assert_true(!static_cast<bool>(expr), fmt, std::forward<Args>(args)...);
}

template<typename Expr1, typename Expr2, typename... Args>
void inline assert_eq(const Expr1 &expr1, const Expr2 &expr2, Args &&...args)
{
	assert_true(expr1 == expr2, std::forward<Args>(args)...);
}

template<typename Expr1, typename Expr2, typename... Args>
void inline assert_nq(const Expr1 &expr1, const Expr2 &expr2, Args &&...args)
{
	assert_true(expr1 != expr2, std::forward<Args>(args)...);
}

template<typename... Args>
void inline assert_fail(fmt::format_string<Args...> fmt, Args &&...args)
{
	throw FrameworkException(fmt::format(fmt, std::forward<Args>(args)...));
}
