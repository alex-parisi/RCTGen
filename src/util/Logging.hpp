/// Logging.hpp

#pragma once

#include <format>
#include <string_view>
#include <utility>

namespace RCTGen {
    void logLine(std::string_view message);

    template<class... Args>
    void printMsg(std::format_string<Args...> fmt, Args &&... args) {
        logLine(std::format(fmt, std::forward<Args>(args)...));
    }

    inline void printMsg(const std::string_view message) {
        logLine(message);
    }
}
