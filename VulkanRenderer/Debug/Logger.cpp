#include "Debug/Logger.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>

std::shared_ptr<spdlog::logger> Logger::s_RendererLogger = {};
std::shared_ptr<spdlog::logger> Logger::s_VkLogger       = {};

void Logger::Init()
{
	spdlog::set_pattern("%^%n@%! ==> %v%$");
	spdlog::set_level(spdlog::level::trace);

	s_RendererLogger = spdlog::stdout_color_mt("Renderer");
	s_VkLogger       = spdlog::stdout_color_mt("Vulkan");
}
