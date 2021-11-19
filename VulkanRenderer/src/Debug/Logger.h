#pragma once

#include <spdlog/spdlog.h>

#define LOG(logLevel, ...)   SPDLOG_LOGGER_CALL(Logger::GetRendererLogger(), spdlog::level::logLevel, __VA_ARGS__)
#define LOGVk(logLevel, ...) SPDLOG_LOGGER_CALL(Logger::GetVkLogger(), spdlog::level::logLevel, __VA_ARGS__)
// #define LOGVk(logLevel, ...) Logger::GetVkLogger()->log(spdlog::level::logLevel, __VA_ARGS__)

class Logger
{
private:
    static std::shared_ptr<spdlog::logger> s_RendererLogger;
    static std::shared_ptr<spdlog::logger> s_VkLogger;

public:
    static void Init();

    static std::shared_ptr<spdlog::logger> GetRendererLogger() { return s_RendererLogger; }
    static std::shared_ptr<spdlog::logger> GetVkLogger() { return s_VkLogger; }
};