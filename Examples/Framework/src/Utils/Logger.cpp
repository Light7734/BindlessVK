#include "Framework/Utils/Logger.hpp"

Logger::Logger(): spd_logger(spdlog::stdout_color_mt("Framework"))
{
	spd_logger->set_pattern("%^%n@%! ==> %v%$");
	spd_logger->set_level(spdlog::level::level_enum::trace);
}

Logger::~Logger()
{
	spdlog::drop_all();
}


void Logger::bindlessvk_callback(
  bvk::DebugCallbackSource const source,
  bvk::LogLvl const severity,
  str const &message,
  std::any const user_data
)
{
	auto const *const logger = std::any_cast<Logger const *const>(user_data);

	auto const source_str = //
	  source == bvk::DebugCallbackSource::eBindlessVk       ? "BindlessVk" :
	  source == bvk::DebugCallbackSource::eValidationLayers ? "Validation Layers" :
	                                                          "Memory Allocator";


	logger->log((spdlog::level::level_enum)(int)severity, "[{}]: {}", source_str, message);
}
