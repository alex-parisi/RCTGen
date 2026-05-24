/// Json.hpp

#pragma once

#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include <jansson.h>

#include "vectormath.h"

namespace RCTGen
{
    using JsonError = std::string;

    template <class T>
    using JsonResult = std::expected<T, JsonError>;

    // Owning ref-counted wrapper. References are decremented on destruction.
    class JsonRef
    {
    public:
        JsonRef() = default;

        // Adopts an owning reference
        static JsonRef adopt(json_t* raw) noexcept
        {
            JsonRef r;
            r.ptr_ = raw;
            return r;
        }

        // Borrows a non-owning reference and increments the refcount so we own a stake.
        static JsonRef borrow(json_t* raw) noexcept
        {
            JsonRef r;
            if (raw) json_incref(raw);
            r.ptr_ = raw;
            return r;
        }

        JsonRef(const JsonRef& other) noexcept
        {
            if (other.ptr_) json_incref(other.ptr_);
            ptr_ = other.ptr_;
        }

        JsonRef(JsonRef&& other) noexcept : ptr_(std::exchange(other.ptr_, nullptr)) {}

        JsonRef& operator=(JsonRef other) noexcept
        {
            std::swap(ptr_, other.ptr_);
            return *this;
        }

        ~JsonRef()
        {
            if (ptr_) json_decref(ptr_);
        }

        [[nodiscard]] json_t* get() const noexcept { return ptr_; }
        [[nodiscard]] json_t* release() noexcept { return std::exchange(ptr_, nullptr); }
        [[nodiscard]] explicit operator bool() const noexcept { return ptr_ != nullptr; }

    private:
        json_t* ptr_ = nullptr;
    };

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
    // palette=keep). Returns adopted ownership.
    [[nodiscard]] JsonRef makeImageObject(
        std::string_view path,
        int x, int y,
        int src_x, int src_y,
        int src_width, int src_height);
}
