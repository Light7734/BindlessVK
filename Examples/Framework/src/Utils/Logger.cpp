#include "Framework/Utils/Logger.hpp"

Logger::Logger(): logger(spdlog::stdout_color_mt("Framework"))
{
	logger->set_pattern("%^%n@%! ==> %v%$");
	logger->set_level(spdlog::level::level_enum::trace);
}

Logger::~Logger()
{
	spdlog::drop_all();
}
