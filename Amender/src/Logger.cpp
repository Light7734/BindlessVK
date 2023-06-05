#include "Amender/Logger.hpp"

#include <imgui.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

Logger::Logger(): spd_logger(spdlog::stdout_color_mt("Logger"))
{
	spd_logger->set_pattern("%^%v%$");
	spd_logger->set_level(spdlog::level::level_enum::trace);
}

Logger::~Logger()
{
	spdlog::drop_all();
}

auto Logger::instance() -> Logger &
{
	auto static logger = Logger {};
	return logger;
}

void Logger::default_log_callback(LogLvl lvl, str const &message, std::any const user_data)
{
	instance().spd_logger->log((spdlog::level::level_enum)lvl, message);
}

void Logger::set_callbacks(Callback const &callback)
{
	instance().callback = callback;
}

void Logger::show_imgui_window()
{
	ImGui::Begin("Logger");

	// ...

	ImGui::End();
}
