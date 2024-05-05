#pragma once

#include <string_view>
#include <format>

#include <source_location>

namespace sample_vk
{
    struct Logger
    {
    public:
        enum class Level
        {
            Info,
            Warning,
            Error
        };

    public:
        static void log(
            const std::string_view  logger_name,
            Level                   level, 
            const std::string_view  msg
        );
    };
}

namespace sample_vk::log
{
    constexpr void appInfo(const std::string_view fmt, const auto&... args)
    {
        Logger::log("App", sample_vk::Logger::Level::Info, std::vformat(fmt, std::make_format_args(args...)));
    }

    constexpr void appWarning(const std::string_view fmt, const auto&... args)
    {
        Logger::log("App", sample_vk::Logger::Level::Warning, std::vformat(fmt, std::make_format_args(args...)));
    }

    constexpr void appError(const std::string_view fmt, const auto&... args)
    {
        Logger::log("App", sample_vk::Logger::Level::Error, std::vformat(fmt, std::make_format_args(args...)));
    }

    constexpr void vkInfo(const std::string_view fmt, const auto&... args)
    {
        Logger::log("Vulkan", sample_vk::Logger::Level::Info, std::vformat(fmt, std::make_format_args(args...)));
    }

    constexpr void vkWarning(const std::string_view fmt, const auto&... args)
    {
        Logger::log("Vulkan", sample_vk::Logger::Level::Warning, std::vformat(fmt, std::make_format_args(args...)));
    }

    constexpr void vkError(const std::string_view fmt, const auto&... args)
    {
        Logger::log("Vulkan", sample_vk::Logger::Level::Error, std::vformat(fmt, std::make_format_args(args...)));
    }

    constexpr void windowInfo(const std::string_view fmt, const auto&... args)
    {
        Logger::log("Window", sample_vk::Logger::Level::Info, std::vformat(fmt, std::make_format_args(args...)));
    }

    constexpr void windowWarning(const std::string_view fmt, const auto&... args)
    {
        Logger::log("Window", sample_vk::Logger::Level::Warning, std::vformat(fmt, std::make_format_args(args...)));
    }

    constexpr void windowError(const std::string_view fmt, const auto&... args)
    {
        Logger::log("Window", sample_vk::Logger::Level::Error, std::vformat(fmt, std::make_format_args(args...)));
    }
}