#include "Debug/Logger.h"

#include <spdlog/sinks/stdout_color_sinks.h>

std::shared_ptr<spdlog::logger> Logger::s_RendererLogger = nullptr;
std::shared_ptr<spdlog::logger> Logger::s_VkLogger       = nullptr;

void Logger::Init()
{
	spdlog::set_pattern("%^%M:%S:%e - %n - %! ==> %v%$");
	spdlog::set_level(spdlog::level::trace);

	s_RendererLogger = spdlog::stdout_color_mt("Renderer");
	s_VkLogger       = spdlog::stdout_color_mt("Vulkan");
}