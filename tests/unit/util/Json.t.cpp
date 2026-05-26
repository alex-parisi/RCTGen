#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <string_view>
#include <vector>

#include <jansson.h>

#include "Constants.hpp"
#include "Json.hpp"

namespace fs = std::filesystem;
using namespace RCTGen;

namespace {
    class TempJsonFile {
    public:
        explicit TempJsonFile(std::string_view contents) {
            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
            path_ = fs::temp_directory_path() /
                    ("rctgen_json_test_" + std::to_string(stamp) + "_" +
                     std::to_string(counter_++) + ".json");
            std::ofstream out(path_);
            out << contents;
        }

        ~TempJsonFile() {
            std::error_code ec;
            fs::remove(path_, ec);
        }

        TempJsonFile(const TempJsonFile &) = delete;

        TempJsonFile &operator=(const TempJsonFile &) = delete;

        const fs::path &path() const { return path_; }

    private:
        fs::path path_;
        static inline int counter_ = 0;
    };

    // Helper for terse JSON building in tests.
    JsonRef makeJson(const char *src) {
        json_error_t err;
        json_t *j = json_loads(src, 0, &err);
        return adoptJson(j);
    }
} // namespace

TEST(JsonRefcount, AdoptNullReturnsEmpty) {
    JsonRef r = adoptJson(nullptr);
    EXPECT_EQ(r.get(), nullptr);
}

TEST(JsonRefcount, BorrowNullReturnsEmpty) {
    JsonRef r = borrowJson(nullptr);
    EXPECT_EQ(r.get(), nullptr);
}

TEST(JsonRefcount, AdoptTakesOwnership) {
    json_t *raw = json_integer(42);
    ASSERT_NE(raw, nullptr);
    EXPECT_EQ(raw->refcount, 1);
    {
        JsonRef r = adoptJson(raw);
        EXPECT_EQ(r.get(), raw);
        EXPECT_EQ(raw->refcount, 1); // adopt does not bump
    }
    // r out of scope: refcount drops, jansson frees it. We can no longer touch raw.
}

TEST(JsonRefcount, BorrowIncrementsRefcount) {
    json_t *raw = json_integer(7);
    ASSERT_NE(raw, nullptr);
    ASSERT_EQ(raw->refcount, 1);
    {
        JsonRef r = borrowJson(raw);
        EXPECT_EQ(raw->refcount, 2);
    }
    EXPECT_EQ(raw->refcount, 1);
    json_decref(raw);
}

TEST(JsonRefcount, ReleaseNullReturnsNull) {
    JsonRef r;
    EXPECT_EQ(releaseJson(r), nullptr);
}

TEST(JsonRefcount, ReleaseIncrementsRefcount) {
    JsonRef r = adoptJson(json_integer(3));
    json_t *raw = r.get();
    ASSERT_EQ(raw->refcount, 1);
    json_t *released = releaseJson(r);
    EXPECT_EQ(released, raw);
    EXPECT_EQ(raw->refcount, 2);
    json_decref(released);
}

TEST(JsonLoadFile, MissingFileErrors) {
    auto r = loadFile("/definitely/not/a/real/path/__nope__.json");
    ASSERT_FALSE(r.has_value());
    EXPECT_FALSE(r.error().empty());
}

TEST(JsonLoadFile, ValidFileLoads) {
    TempJsonFile f(R"({"a": 1, "b": "two"})");
    auto r = loadFile(f.path());
    ASSERT_TRUE(r.has_value()) << r.error();
    json_t *root = r->get();
    ASSERT_TRUE(json_is_object(root));
    EXPECT_EQ(json_integer_value(json_object_get(root, "a")), 1);
}

TEST(JsonLoadFile, MalformedJsonErrors) {
    TempJsonFile f("{not valid json");
    auto r = loadFile(f.path());
    EXPECT_FALSE(r.has_value());
}

TEST(JsonReadInt, ValidInteger) {
    JsonRef v = adoptJson(json_integer(42));
    auto r = readInt(v.get(), "x");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 42);
}

TEST(JsonReadInt, NullErrors) {
    auto r = readInt(nullptr, "x");
    EXPECT_FALSE(r.has_value());
    EXPECT_NE(r.error().find("\"x\""), std::string::npos);
}

TEST(JsonReadInt, NonIntegerErrors) {
    JsonRef v = adoptJson(json_string("hello"));
    auto r = readInt(v.get(), "x");
    EXPECT_FALSE(r.has_value());
}

TEST(JsonReadInt, RealRejected) {
    JsonRef v = adoptJson(json_real(1.5));
    auto r = readInt(v.get(), "x");
    EXPECT_FALSE(r.has_value());
}

TEST(JsonReadUint32, ValidValue) {
    JsonRef v = adoptJson(json_integer(7));
    auto r = readUint32(v.get(), "n");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 7u);
}

TEST(JsonReadUint32, ForwardsError) {
    auto r = readUint32(nullptr, "n");
    EXPECT_FALSE(r.has_value());
}

TEST(JsonReadString, ValidString) {
    JsonRef v = adoptJson(json_string("hello"));
    auto r = readString(v.get(), "s");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, "hello");
}

TEST(JsonReadString, EmptyString) {
    JsonRef v = adoptJson(json_string(""));
    auto r = readString(v.get(), "s");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, "");
}

TEST(JsonReadString, NullErrors) {
    auto r = readString(nullptr, "s");
    EXPECT_FALSE(r.has_value());
}

TEST(JsonReadString, NonStringErrors) {
    JsonRef v = adoptJson(json_integer(1));
    auto r = readString(v.get(), "s");
    EXPECT_FALSE(r.has_value());
}

TEST(JsonReadNumber, IntegerWorks) {
    JsonRef v = adoptJson(json_integer(5));
    auto r = readNumber(v.get(), "n");
    ASSERT_TRUE(r.has_value());
    EXPECT_DOUBLE_EQ(*r, 5.0);
}

TEST(JsonReadNumber, RealWorks) {
    JsonRef v = adoptJson(json_real(1.25));
    auto r = readNumber(v.get(), "n");
    ASSERT_TRUE(r.has_value());
    EXPECT_DOUBLE_EQ(*r, 1.25);
}

TEST(JsonReadNumber, StringErrors) {
    JsonRef v = adoptJson(json_string("3.14"));
    auto r = readNumber(v.get(), "n");
    EXPECT_FALSE(r.has_value());
}

TEST(JsonReadVector3, ValidArray) {
    JsonRef v = makeJson("[1.5, -2.0, 3.25]");
    auto r = readVector3(v.get());
    ASSERT_TRUE(r.has_value());
    EXPECT_FLOAT_EQ(r->x, 1.5f);
    EXPECT_FLOAT_EQ(r->y, -2.0f);
    EXPECT_FLOAT_EQ(r->z, 3.25f);
}

TEST(JsonReadVector3, WrongSizeErrors) {
    JsonRef two = makeJson("[1, 2]");
    EXPECT_FALSE(readVector3(two.get()).has_value());
    JsonRef four = makeJson("[1, 2, 3, 4]");
    EXPECT_FALSE(readVector3(four.get()).has_value());
}

TEST(JsonReadVector3, NotAnArrayErrors) {
    JsonRef v = adoptJson(json_string("not an array"));
    EXPECT_FALSE(readVector3(v.get()).has_value());
}

TEST(JsonReadVector3, NullErrors) {
    EXPECT_FALSE(readVector3(nullptr).has_value());
}

TEST(JsonReadVector3, NonNumericComponentErrors) {
    JsonRef v = makeJson(R"([1, "x", 3])");
    EXPECT_FALSE(readVector3(v.get()).has_value());
}

TEST(JsonReadEnumIndex, ValidName) {
    JsonRef v = adoptJson(json_string("gentle"));
    auto r = readEnumIndex(v.get(), std::span(kCategoryNames), "category", "category");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 1u);
}

TEST(JsonReadEnumIndex, FirstAndLast) {
    JsonRef first = adoptJson(json_string("transport"));
    auto rf = readEnumIndex(first.get(), std::span(kCategoryNames), "c", "cat");
    ASSERT_TRUE(rf.has_value());
    EXPECT_EQ(*rf, 0u);

    JsonRef last = adoptJson(json_string("rollercoaster"));
    auto rl = readEnumIndex(last.get(), std::span(kCategoryNames), "c", "cat");
    ASSERT_TRUE(rl.has_value());
    EXPECT_EQ(*rl, kCategoryNames.size() - 1u);
}

TEST(JsonReadEnumIndex, UnknownNameErrors) {
    JsonRef v = adoptJson(json_string("nope"));
    auto r = readEnumIndex(v.get(), std::span(kCategoryNames), "c", "category");
    EXPECT_FALSE(r.has_value());
    EXPECT_NE(r.error().find("nope"), std::string::npos);
}

TEST(JsonReadEnumIndex, NonStringErrors) {
    JsonRef v = adoptJson(json_integer(0));
    auto r = readEnumIndex(v.get(), std::span(kCategoryNames), "c", "category");
    EXPECT_FALSE(r.has_value());
}

TEST(JsonReadFlagBits, EmptyArrayProducesZero) {
    JsonRef v = makeJson("[]");
    auto r = readFlagBits(v.get(), std::span(kRideFlagNames), "flags", "flag");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 0u);
}

TEST(JsonReadFlagBits, SingleFlag) {
    JsonRef v = makeJson(R"(["no_collision_crashes"])");
    auto r = readFlagBits(v.get(), std::span(kRideFlagNames), "flags", "flag");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 1u << 0);
}

TEST(JsonReadFlagBits, MultipleFlags) {
    JsonRef v = makeJson(R"(["no_collision_crashes", "rider_controls_speed"])");
    auto r = readFlagBits(v.get(), std::span(kRideFlagNames), "flags", "flag");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, (1u << 0) | (1u << 1));
}

TEST(JsonReadFlagBits, AllSpriteGroupNames) {
    json_t *arr = json_array();
    for (auto n: kSpriteGroupNames) {
        json_array_append_new(arr, json_stringn(n.data(), n.size()));
    }
    JsonRef v = adoptJson(arr);
    auto r = readFlagBits(v.get(), std::span(kSpriteGroupNames), "sprites", "sprite group");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, (1u << kSpriteGroupNames.size()) - 1u);
}

TEST(JsonReadFlagBits, UnknownFlagErrors) {
    JsonRef v = makeJson(R"(["bogus"])");
    auto r = readFlagBits(v.get(), std::span(kRideFlagNames), "flags", "flag");
    EXPECT_FALSE(r.has_value());
    EXPECT_NE(r.error().find("bogus"), std::string::npos);
}

TEST(JsonReadFlagBits, NonArrayErrors) {
    JsonRef v = adoptJson(json_string("nope"));
    auto r = readFlagBits(v.get(), std::span(kRideFlagNames), "flags", "flag");
    EXPECT_FALSE(r.has_value());
}

TEST(JsonReadFlagBits, NonStringElementErrors) {
    JsonRef v = makeJson("[1]");
    auto r = readFlagBits(v.get(), std::span(kRideFlagNames), "flags", "flag");
    EXPECT_FALSE(r.has_value());
}

TEST(JsonAsArrayOrWrap, NullErrors) {
    auto r = asArrayOrWrap(nullptr);
    EXPECT_FALSE(r.has_value());
}

TEST(JsonAsArrayOrWrap, EmptyArrayErrors) {
    JsonRef arr = adoptJson(json_array());
    auto r = asArrayOrWrap(arr.get());
    EXPECT_FALSE(r.has_value());
}

TEST(JsonAsArrayOrWrap, ArrayPassesThrough) {
    JsonRef arr = makeJson("[1, 2, 3]");
    auto r = asArrayOrWrap(arr.get());
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->get(), arr.get());
    EXPECT_EQ(json_array_size(r->get()), 3u);
}

TEST(JsonAsArrayOrWrap, ScalarIsWrapped) {
    JsonRef scalar = adoptJson(json_integer(7));
    auto r = asArrayOrWrap(scalar.get());
    ASSERT_TRUE(r.has_value());
    ASSERT_TRUE(json_is_array(r->get()));
    EXPECT_EQ(json_array_size(r->get()), 1u);
    EXPECT_EQ(json_integer_value(json_array_get(r->get(), 0)), 7);
}

TEST(JsonMakeImageObject, FullObject) {
    JsonRef img = makeImageObject("images/car_0.png", 10, -5, 100, 200, 50, 25);
    json_t *j = img.get();
    ASSERT_TRUE(json_is_object(j));
    EXPECT_STREQ(json_string_value(json_object_get(j, "path")), "images/car_0.png");
    EXPECT_EQ(json_integer_value(json_object_get(j, "x")), 10);
    EXPECT_EQ(json_integer_value(json_object_get(j, "y")), -5);
    EXPECT_EQ(json_integer_value(json_object_get(j, "src_x")), 100);
    EXPECT_EQ(json_integer_value(json_object_get(j, "src_y")), 200);
    EXPECT_EQ(json_integer_value(json_object_get(j, "src_width")), 50);
    EXPECT_EQ(json_integer_value(json_object_get(j, "src_height")), 25);
    EXPECT_STREQ(json_string_value(json_object_get(j, "palette")), "keep");
}

TEST(JsonMakeImageObject, OmitsNegativeSourceCoords) {
    JsonRef img = makeImageObject("images/preview.png", 0, 0, -1, -1, -1, -1);
    json_t *j = img.get();
    EXPECT_EQ(json_object_get(j, "src_x"), nullptr);
    EXPECT_EQ(json_object_get(j, "src_y"), nullptr);
    EXPECT_EQ(json_object_get(j, "src_width"), nullptr);
    EXPECT_EQ(json_object_get(j, "src_height"), nullptr);
    EXPECT_STREQ(json_string_value(json_object_get(j, "path")), "images/preview.png");
}