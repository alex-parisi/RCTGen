#include <gtest/gtest.h>

#include <string>

#include "Logging.hpp"

TEST(Logging, LogLineWritesMessageAndNewline) {
    testing::internal::CaptureStdout();
    RCTGen::logLine("hello world");
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_EQ(out, "hello world\n");
}

TEST(Logging, LogLineHandlesEmptyMessage) {
    testing::internal::CaptureStdout();
    RCTGen::logLine("");
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_EQ(out, "\n");
}

TEST(Logging, LogLinePreservesEmbeddedNewlines) {
    testing::internal::CaptureStdout();
    RCTGen::logLine("a\nb");
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_EQ(out, "a\nb\n");
}

TEST(Logging, PrintMsgFormatsArguments) {
    testing::internal::CaptureStdout();
    RCTGen::printMsg("number={}, name={}", 42, "alpine");
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_EQ(out, "number=42, name=alpine\n");
}

TEST(Logging, PrintMsgStringViewOverload) {
    testing::internal::CaptureStdout();
    std::string_view msg = "plain message";
    RCTGen::printMsg(msg);
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_EQ(out, "plain message\n");
}

TEST(Logging, PrintMsgNoArgsHonorsFormatBraces) {
    testing::internal::CaptureStdout();
    RCTGen::printMsg("no placeholders here");
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_EQ(out, "no placeholders here\n");
}