#pragma once

#include <string_view>
#include <format>

#include <source_location>

namespace vrts
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

namespace vrts::log
{
    constexpr void info(const std::string_view fmt, const auto&... args)
    {
        Logger::log("vrts", vrts::Logger::Level::Info, std::vformat(fmt, std::make_format_args(args...)));
    }

    constexpr void warning(const std::string_view fmt, const auto&... args)
    {
        Logger::log("vrts", vrts::Logger::Level::Warning, std::vformat(fmt, std::make_format_args(args...)));
    }

    constexpr void error(const std::string_view fmt, const auto&... args)
    {
        Logger::log("vrts", vrts::Logger::Level::Error, std::vformat(fmt, std::make_format_args(args...)));
    }
}