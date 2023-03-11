#include "BindlessVk/Common/Aliases.hpp"

#include <exception>
#include <fmt/format.h>

namespace BINDLESSVK_NAMESPACE {

struct BindlessVkException: std::exception
{
	BindlessVkException(str const &what): msg(what)
	{
	}

	auto virtual what() const noexcept -> c_str
	{
		return msg.c_str();
	}

	str msg;
};

template<typename Expr, typename... Args>
void inline assert_true(const Expr &expr, fmt::format_string<Args...> fmt = "", Args &&...args)
{
	if (!static_cast<bool>(expr)) [[unlikely]]
	{
		throw BindlessVkException(fmt::format(fmt, std::forward<Args>(args)...));
	}
}

template<typename T, typename... Args>
void inline assert_false(const T &expr, fmt::format_string<Args...> fmt = "", Args &&...args)
{
	assert_true(!static_cast<bool>(expr), fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void inline assert_fail(fmt::format_string<Args...> fmt = "", Args &&...args)
{
	throw BindlessVkException(fmt::format(fmt, std::forward<Args>(args)...));
}

} // namespace BINDLESSVK_NAMESPACE
