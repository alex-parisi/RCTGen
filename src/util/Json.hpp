/// Json.hpp

#pragma once

#include <expected>
#include <filesystem>
#include <memory>
#include <span>
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

    // Wraps a raw json_t* whose ownership is transferred from the caller.
    [[nodiscard]] JsonRef adoptJson(Json *raw);

    // Wraps a borrowed json_t* by taking an additional reference.
    [[nodiscard]] JsonRef borrowJson(Json *raw);

    // Transfers an owning raw pointer back to a jansson API that consumes a
    // reference (e.g. json_array_append_new, json_object_set_new).
    [[nodiscard]] Json *releaseJson(const JsonRef &ref) noexcept;

    [[nodiscard]] JsonResult<JsonRef> loadFile(const std::filesystem::path &path);

    int dumpFile(const Json *json, const std::filesystem::path &path, std::size_t flags);

    [[nodiscard]] JsonResult<std::int64_t> readInt(const Json *value, std::string_view property);

    [[nodiscard]] JsonResult<std::uint32_t> readUint32(const Json *value, std::string_view property);

    [[nodiscard]] JsonResult<bool> readBool(const Json *value, std::string_view property);

    [[nodiscard]] JsonResult<std::string> readString(const Json *value, std::string_view property);

    [[nodiscard]] JsonResult<double> readNumber(const Json *value, std::string_view property);

    [[nodiscard]] JsonResult<vector3_t> readVector3(const Json *array);

    [[nodiscard]] JsonResult<std::uint32_t> readEnumIndex(
        const Json *value,
        std::span<const std::string_view> names,
        std::string_view property,
        std::string_view itemLabel);

    [[nodiscard]] JsonResult<std::uint32_t> readFlagBits(
        const Json *value,
        std::span<const std::string_view> names,
        std::string_view property,
        std::string_view itemLabel);

    [[nodiscard]] JsonResult<JsonRef> asArrayOrWrap(Json *value);

    // Build a json image entry (path, x, y, srcX/srcY/srcWidth/srcHeight,
    // palette=keep).
    [[nodiscard]] JsonRef makeImageObject(
        std::string_view path,
        int x, int y,
        int srcX, int srcY,
        int srcWidth, int srcHeight);
}
