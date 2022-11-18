#pragma once

#include <exception>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#define LOG(logLevel, ...) SPDLOG_LOGGER_CALL(Logger::Get(), spdlog::level::logLevel, __VA_ARGS__)

// Token indirection (for __FILE and __LINE__)
#define TOKEN_INDIRECTION(token) token
#define TKNIND(token)            TOKEN_INDIRECTION(token)

// Concatinate tokens
#define CAT_INDIRECTION(a, b) a##b
#define CAT_TOKEN(a, b)       CAT_INDIRECTION(a, b)

// Assertions
#define ASSERT(x, ...)                                                    \
	if (!(x))                                                             \
	{                                                                     \
		LOG(critical, __VA_ARGS__);                                       \
		throw FrameworkException(#x, TKNIND(__FILE__), TKNIND(__LINE__)); \
	}

class Logger
{
private:
	static std::shared_ptr<spdlog::logger> s_Logger;

public:
	static std::shared_ptr<spdlog::logger> Get()
	{
		if (!s_Logger)
		{
			spdlog::set_pattern("%^%n@%! ==> %v%$");
			spdlog::set_level(spdlog::level::trace);

			s_Logger = spdlog::stdout_color_mt("Framework");
		}

		return s_Logger;
	}
};

struct FrameworkException: std::exception
{
	FrameworkException(const char* statement, const char* file, int line)
	    : statement(statement)
	    , file(file)
	    , line(line)
	{
	}

	const char* statement;
	const char* file;
	int line;
};
