#pragma once

#include <iostream>
#include <string>

#define LOG

namespace logger
{
    enum class LOG_LEVEL_t
    {
        Error,
        Warning,
        Info,
        Debug,
        None
    };

    const static std::string log_prefix[(size_t)LOG_LEVEL_t::None + 1] =
    {
        "[ERROR] ",
        "[Warning] ",
        "[Info] ",
        "[Debug] ",
        ""
    };

#   ifdef LOG_LEVEL_NONE
    const static LOG_LEVEL_t LOG_LEVEL = LOG_LEVEL_t::None;
#   elif defined LOG_LEVEL_ERROR
    const static LOG_LEVEL_t LOG_LEVEL = LOG_LEVEL_t::Error;
#   elif defined LOG_LEVEL_WARNING
    const static LOG_LEVEL_t LOG_LEVEL = LOG_LEVEL_t::Warning;
#   elif defined LOG_LEVEL_INFO
    const static LOG_LEVEL_t LOG_LEVEL = LOG_LEVEL_t::Info;
#   elif defined LOG_LEVEL_DEBUG
    const static LOG_LEVEL_t LOG_LEVEL = LOG_LEVEL_t::Debug;
#   elif defined LOG
    const static LOG_LEVEL_t LOG_LEVEL = LOG_LEVEL_t::Info;
#   endif // LOG_LEVEL_*

    inline void LOG_MSG(LOG_LEVEL_t level, const std::string & msg)
    {
#   ifdef LOG
        if (level <= LOG_LEVEL)
            std::cout << log_prefix[(size_t)level] << msg << '\n';
#   endif //LOG
    }
}