/// Json.hpp

#pragma once

#include <expected>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

#include <jansson.h>

#include "vectormath.h"

namespace RCTGen {
    using JsonError = std::string;
    using Json = json_t;

    template<class T>
    using JsonResult = std::expected<T, JsonError>;

    // Shared, refcounted handle to a json_t. Each JsonRef holds one jansson
    // reference; the count is released when the last copy is destroyed.
    using JsonRef = std::shared_ptr<Json>;

    [[nodiscard]] JsonRef adoptJson(Json *raw);

    [[nodiscard]] JsonRef borrowJson(Json *raw);

    [[nodiscard]] JsonResult<JsonRef> loadFile(const std::filesystem::path &path);

    int dumpFile(const Json *json, const std::filesystem::path &path, std::size_t flags);

    [[nodiscard]] JsonResult<std::int64_t> readInt(const Json *value, std::string_view property);

    [[nodiscard]] JsonResult<bool> readBool(const Json *value, std::string_view property);

    [[nodiscard]] JsonResult<std::string> readString(const Json *value, std::string_view property);

    [[nodiscard]] JsonResult<double> readNumber(const Json *value, std::string_view property);

    [[nodiscard]] JsonResult<vector3_t> readVector3(const Json *array);
}