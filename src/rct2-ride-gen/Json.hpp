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
    [[nodiscard]] json_t* releaseJson(JsonRef ref) noexcept;

    [[nodiscard]] JsonResult<JsonRef> loadFile(const std::filesystem::path& path);

    [[nodiscard]] JsonResult<std::int64_t>   readInt(json_t* value, std::string_view property);
    [[nodiscard]] JsonResult<std::uint32_t>  readUint32(json_t* value, std::string_view property);
    [[nodiscard]] JsonResult<std::string>    readString(json_t* value, std::string_view property);
    [[nodiscard]] JsonResult<double>         readNumber(json_t* value, std::string_view property);
    [[nodiscard]] JsonResult<vector3_t>      readVector3(json_t* array);

    [[nodiscard]] JsonResult<std::uint32_t> readEnumIndex(
        json_t* value,
        std::span<const std::string_view> names,
        std::string_view property,
        std::string_view item_label);

    [[nodiscard]] JsonResult<std::uint32_t> readFlagBits(
        json_t* value,
        std::span<const std::string_view> names,
        std::string_view property,
        std::string_view item_label);

    [[nodiscard]] JsonResult<JsonRef> asArrayOrWrap(json_t* value);

    // Build a json image entry (path, x, y, srcX/srcY/srcWidth/srcHeight,
    // palette=keep).
    [[nodiscard]] JsonRef makeImageObject(
        std::string_view path,
        int x, int y,
        int src_x, int src_y,
        int src_width, int src_height);
}
