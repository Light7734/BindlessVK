#pragma once

//
//
#include "Framework/Common/Common.hpp"

#include <BindlessVk/Common/Common.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

class Logger
{
public:
	Logger();
	~Logger();

	template<typename... Args>
	inline void log(spdlog::level::level_enum level, fmt::format_string<Args...> fmt = "", Args&&... args)
	  const
	{
		logger->log(level, fmt, std::forward<Args>(args)...);
	}

private:
	ref<spdlog::logger> logger;
};
