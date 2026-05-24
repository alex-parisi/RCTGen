/// Json.hpp

#pragma once

#include <cstdint>
#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include <jansson.h>

#include "../iso-render/vectormath.h"

namespace rctgen
{
    using JsonError = std::string;

    template <class T>
    using JsonResult = std::expected<T, JsonError>;

    // Owning ref-counted wrapper. References are decremented on destruction.
    class JsonRef
    {
    public:
        JsonRef() = default;

        // Adopts an owning reference (e.g. from json_load_file / json_*_new).
        static JsonRef adopt(json_t* raw) noexcept
        {
            JsonRef r;
            r.ptr_ = raw;
            return r;
        }

        // Borrows a non-owning reference (e.g. from json_object_get) and
        // increments the refcount so we own a stake.
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

    // Loaders --------------------------------------------------------------------

    [[nodiscard]] JsonResult<JsonRef> load_file(const std::filesystem::path& path);

    // Read-only views over json_t* owned elsewhere (no refcount handling).

    [[nodiscard]] JsonResult<std::int64_t>   read_int(json_t* value, std::string_view property);
    [[nodiscard]] JsonResult<std::uint32_t>  read_uint32(json_t* value, std::string_view property);
    [[nodiscard]] JsonResult<std::string>    read_string(json_t* value, std::string_view property);
    [[nodiscard]] JsonResult<double>         read_number(json_t* value, std::string_view property);
    [[nodiscard]] JsonResult<vector3_t>      read_vector3(json_t* array);

    // Enumeration & flag parsing.

    [[nodiscard]] JsonResult<std::uint32_t> read_enum_index(
        json_t* value,
        std::span<const std::string_view> names,
        std::string_view property,
        std::string_view item_label);

    [[nodiscard]] JsonResult<std::uint32_t> read_flag_bits(
        json_t* value,
        std::span<const std::string_view> names,
        std::string_view property,
        std::string_view item_label);

    // Optional-array helper: returns a JSON array; if value is a scalar, wraps
    // it in a 1-element array. Returns adopted ownership.
    [[nodiscard]] JsonResult<JsonRef> as_array_or_wrap(json_t* value);

    // Build a json image entry (path, x, y, srcX/srcY/srcWidth/srcHeight,
    // palette=keep). Returns adopted ownership.
    [[nodiscard]] JsonRef make_image_object(
        std::string_view path,
        int x, int y,
        int src_x, int src_y,
        int src_width, int src_height);
}
