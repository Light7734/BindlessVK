#pragma once

#include "Framework/Common/Common.hpp"

#include <BindlessVk/Common/Common.hpp>
#include <BindlessVk/Context/VkContext.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

class Logger
{
public:
	Logger();
	~Logger();

	template<typename... Args>
	void log(spdlog::level::level_enum level, fmt::format_string<Args...> fmt = "", Args &&...args)
	    const
	{
		spd_logger->log(level, fmt, std::forward<Args>(args)...);
	}

	void static bindlessvk_callback(
	    bvk::DebugCallbackSource const source,
	    bvk::LogLvl const severity,
	    str const &message,
	    std::any const user_data
	);

private:
	ref<spdlog::logger> spd_logger;
};
