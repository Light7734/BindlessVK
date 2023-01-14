#pragma once


#include <exception>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

//
#include "Framework/Common/Aliases.hpp"
#include "Framework/Common/Assertions.hpp"

#include <BindlessVk/Common/Common.hpp>

#define LOG(logLevel, ...) Logger::Get()->log(spdlog::level::logLevel, __VA_ARGS__)

class Logger
{
private:
	static std::shared_ptr<spdlog::logger> s_logger;

public:
	static std::shared_ptr<spdlog::logger> Get()
	{
		if (!s_logger) {
			spdlog::set_pattern("%^%n@%! ==> %v%$");
			spdlog::set_level(spdlog::level::trace);

			s_logger = spdlog::stdout_color_mt("Framework");
		}

		return s_logger;
	}
};
