#include <base/logger/logger.hpp>

#include <filesystem>
#include <iostream>

#include <stacktrace>

namespace sample_vk
{
    auto toString(std::source_location source)
    {
        auto filename = std::filesystem::path(source.file_name())
            .filename()
            .string();

        return std::format(
            "{}:{}:{}", 
            filename, 
            source.function_name(), 
            source.line()
        );
    }

    auto toString(Logger::Level level)
    {
        switch(level)
        {
            case Logger::Level::Info:
                return "Info";
            case Logger::Level::Warning:
                return "Warning";
            case Logger::Level::Error:
                return "Error";
        }

        return "Unknown level";
    }

    void Logger::log(
        const std::string_view  logger_name,
        Level                   level, 
        const std::string_view  msg
    )
    {
        if (level == Level::Error)
        {
            std::cerr 
                << std::format(
                    "[{}] [{}]\n{}\n{}", 
                    logger_name,
                    toString(level),
                    std::to_string(std::stacktrace::current()),
                    msg
                ) << std::endl;
            
            throw std::runtime_error(msg.data());    
        }

        std::cout 
            << std::format(
                "[{}] [{}] | {}", 
                logger_name,
                toString(level),
                msg
            ) << std::endl;
    }
}