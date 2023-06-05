#pragma once

#include "Amender/Aliases.hpp"

#include <any>
#include <format>
#include <memory>

/** @brief Severity of a log message.
 *
 * @note Values reflect spdlog::lvl
 */
enum class LogLvl : u8
{
	/** Lowest and most vebose log level, for tracing execution paths and events */
	eTrace = 0,

	/** Vebose log level, for enabling temporarily to debug */
	eDebug = 1,

	/** General information */
	eInfo = 2,

	/** Things we should to be aware of and edge cases */
	eWarn = 3,

	/** Defects, bugs and undesired behaviour */
	eError = 4,

	/** Unrecoverable errors */
	eCritical = 5,

	/** No logging */
	eOff = 6,

	nCount,
};

namespace spdlog {
class logger;
}

/** Responsible for logging */
class Logger
{
public:
	/** Data required for logging */
	struct Callback
	{
		fn<void(LogLvl, str const &, std::any)> log_callback;
		std::any user_data;
	};

public:
	void static set_callbacks(Callback const &callback);

	void static show_imgui_window();

	template<typename... Args>
	void static log(LogLvl lvl, std::format_string<Args...> fmt, Args &&...args)
	{
		instance().callback.log_callback(
		    lvl,
		    std::format(fmt, std::forward<Args>(args)...),
		    instance().callback.user_data
		);
	}

private:
	Logger();
	~Logger();

	auto static instance() -> Logger &;
	void static default_log_callback(LogLvl lvl, str const &message, std::any const user_data);

private:
	Callback callback = Callback { &Logger::default_log_callback };
	std::shared_ptr<spdlog::logger> spd_logger;
};

template<typename... Args>
void inline log_trc(std::format_string<Args...> fmt, Args &&...args)
{
	Logger::log(LogLvl::eTrace, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void inline log_dbg(std::format_string<Args...> fmt, Args &&...args)
{
	Logger::log(LogLvl::eDebug, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void inline log_inf(std::format_string<Args...> fmt, Args &&...args)
{
	Logger::log(LogLvl::eInfo, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void inline log_wrn(std::format_string<Args...> fmt, Args &&...args)
{
	Logger::log(LogLvl::eWarn, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void inline log_err(std::format_string<Args...> fmt, Args &&...args)
{
	Logger::log(LogLvl::eError, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void inline log_crt(std::format_string<Args...> fmt, Args &&...args)
{
	Logger::log(LogLvl::eCritical, fmt, std::forward<Args>(args)...);
}
