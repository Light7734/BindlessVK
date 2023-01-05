#pragma once

#if !defined(BINDLESSVK_NAMESPACE)
	#define BINDLESSVK_NAMESPACE bvk
#endif

#if !defined(BVK_ASSERT)
	#define BVK_ASSERT(x, ...)                                                    \
		if (((bool)(x))) [[unlikely]]                                             \
		{                                                                         \
			BVK_LOG(LogLvl::eCritical, "" __VA_ARGS__);                           \
			BVK_LOG(LogLvl::eCritical, "Assertion failed({}): {}", (int)(x), #x); \
			throw BindlessVkException(#x, TKNIND(__FILE__), TKNIND(__LINE__));    \
		}
#endif

#if !defined(BVK_LOG)
	#define BVK_LOG_NOT_CONFIGURED

	#include <spdlog/spdlog.h>
	#include <spdlog/sinks/stdout_color_sinks.h>

// Token indirection (for __FILE__ and __LINE__)
	#define TOKEN_INDIRECTION(token) token
	#define TKNIND(token)            TOKEN_INDIRECTION(token)

// Concatinate tokens
	#define CAT_INDIRECTION(a, b) a##b
	#define CAT_TOKEN(a, b)       CAT_INDIRECTION(a, b)

// Logging
	#define BVK_LOG(logLevel, ...) \
		SPDLOG_LOGGER_CALL(Logger::get(), (spdlog::level::level_enum)logLevel, __VA_ARGS__)

namespace BINDLESSVK_NAMESPACE {
class Logger
{
private:
	static std::shared_ptr<spdlog::logger> s_logger;

public:
	static std::shared_ptr<spdlog::logger> get()
	{
		if (!s_logger)
		{
			spdlog::set_pattern("%^%n@%! ==> %v%$");
			spdlog::set_level(spdlog::level::trace);

			s_logger = spdlog::stdout_color_mt("BindlessVk");
		}
		return s_logger;
	}
};
} // namespace BINDLESSVK_NAMESPACE

#endif
