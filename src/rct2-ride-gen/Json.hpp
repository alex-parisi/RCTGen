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

namespace RCTGen
{
    using JsonError = std::string;

    template <class T>
    using JsonResult = std::expected<T, JsonError>;

    // Shared, refcounted handle to a json_t. Each JsonRef holds one jansson
    // reference; the count is released when the last copy is destroyed.
    using JsonRef = std::shared_ptr<json_t>;

    // Wraps a raw json_t* whose ownership is transferred from the caller.
    [[nodiscard]] JsonRef adoptJson(json_t* raw);

    // Wraps a borrowed json_t* by taking an additional reference.
    [[nodiscard]] JsonRef borrowJson(json_t* raw);

    // Transfers an owning raw pointer back to a jansson API that consumes a
    // reference (e.g. json_array_append_new, json_object_set_new).
    [[nodiscard]] json_t* releaseJson(const JsonRef& ref) noexcept;

    [[nodiscard]] JsonResult<JsonRef> loadFile(const std::filesystem::path& path);

    [[nodiscard]] JsonResult<std::int64_t>   readInt(const json_t* value, std::string_view property);
    [[nodiscard]] JsonResult<std::uint32_t>  readUint32(const json_t* value, std::string_view property);
    [[nodiscard]] JsonResult<std::string>    readString(const json_t* value, std::string_view property);
    [[nodiscard]] JsonResult<double>         readNumber(const json_t* value, std::string_view property);
    [[nodiscard]] JsonResult<vector3_t>      readVector3(const json_t* array);

    [[nodiscard]] JsonResult<std::uint32_t> readEnumIndex(
        const json_t* value,
        std::span<const std::string_view> names,
        std::string_view property,
        std::string_view itemLabel);

    [[nodiscard]] JsonResult<std::uint32_t> readFlagBits(
        const json_t* value,
        std::span<const std::string_view> names,
        std::string_view property,
        std::string_view itemLabel);

    [[nodiscard]] JsonResult<JsonRef> asArrayOrWrap(json_t* value);

    // Build a json image entry (path, x, y, srcX/srcY/srcWidth/srcHeight,
    // palette=keep).
    [[nodiscard]] JsonRef makeImageObject(
        std::string_view path,
        int x, int y,
        int srcX, int srcY,
        int srcWidth, int srcHeight);
}
