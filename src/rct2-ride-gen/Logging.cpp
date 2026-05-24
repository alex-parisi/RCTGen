/// Logging.cpp

#include "Logging.hpp"

#include <cstdio>

namespace RCTGen
{
    void logLine(const std::string_view message)
    {
        std::fwrite(message.data(), 1, message.size(), stdout);
        std::fputc('\n', stdout);
        std::fflush(stdout);
    }
}
