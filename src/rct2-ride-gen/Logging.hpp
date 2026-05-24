/// Logging.hpp

#pragma once

#include <format>
#include <string_view>
#include <utility>

namespace rctgen
{
    void log_line(std::string_view message);

    template <class... Args>
    void print_msg(std::format_string<Args...> fmt, Args&&... args)
    {
        log_line(std::format(fmt, std::forward<Args>(args)...));
    }

    inline void print_msg(std::string_view message)
    {
        log_line(message);
    }
}
