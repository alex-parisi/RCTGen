/// Json.cpp

#include "Json.hpp"

#include <cassert>
#include <format>

namespace RCTGen {

    using Json = json_t;

    namespace {
        void jsonDeleter(Json *p) noexcept {
            json_decref(p);
        }
    }

    JsonRef adoptJson(Json *raw) {
        if (raw == nullptr)
            return {};
        return {raw, &jsonDeleter};
    }

    JsonRef borrowJson(Json *raw) {
        if (raw == nullptr)
            return {};
        json_incref(raw);
        return adoptJson(raw);
    }

    Json *releaseJson(const JsonRef &ref) noexcept {
        Json *p = ref.get();
        if (p)
            json_incref(p);
        return p;
    }

    JsonResult<JsonRef> loadFile(const std::filesystem::path &path) {
        json_error_t err;
        Json *raw = json_load_file(path.string().c_str(), 0, &err);
        if (raw == nullptr) {
            return std::unexpected(std::format(
                "{} at line {} column {} ({})", err.text, err.line, err.column, err.source));
        }
        return adoptJson(raw);
    }

    JsonResult<std::int64_t> readInt(const Json *value, std::string_view property) {
        if (value == nullptr || !json_is_integer(value)) {
            return std::unexpected(std::format(
                "Property \"{}\" not found or is not an integer", property));
        }
        return json_integer_value(value);
    }

    JsonResult<std::uint32_t> readUint32(const Json *value, const std::string_view property) {
        auto v = readInt(value, property);
        if (!v)
            return std::unexpected(v.error());
        return static_cast<std::uint32_t>(*v);
    }

    JsonResult<std::string> readString(const Json *value, std::string_view property) {
        if (value == nullptr || !json_is_string(value)) {
            return std::unexpected(std::format(
                "Property \"{}\" not found or is not a string", property));
        }
        return std::string(json_string_value(value));
    }

    JsonResult<double> readNumber(const Json *value, std::string_view property) {
        if (value == nullptr || !json_is_number(value)) {
            return std::unexpected(std::format(
                "Property \"{}\" not found or is not a number", property));
        }
        return json_number_value(value);
    }

    JsonResult<vector3_t> readVector3(const Json *array) {
        if (array == nullptr || !json_is_array(array) || json_array_size(array) != 3) {
            return std::unexpected(std::string("Vector must be an array of 3 numbers"));
        }
        vector3_t v{};
        float *components[3] = {&v.x, &v.y, &v.z};
        for (std::size_t i = 0; i < 3; i++) {
            const Json *elem = json_array_get(array, i);
            if (!json_is_number(elem)) {
                return std::unexpected(std::string("Vector components must be numeric"));
            }
            *components[i] = static_cast<float>(json_number_value(elem));
        }
        return v;
    }

    JsonResult<std::uint32_t> readEnumIndex(
        const Json *value,
        const std::span<const std::string_view> names,
        std::string_view property,
        std::string_view itemLabel) {
        if (value == nullptr || !json_is_string(value)) {
            return std::unexpected(std::format(
                "Property \"{}\" not found or is not a string", property));
        }
        std::string_view tag = json_string_value(value);
        for (std::size_t i = 0; i < names.size(); i++) {
            if (names[i] == tag)
                return static_cast<std::uint32_t>(i);
        }
        return std::unexpected(std::format(
            "Unrecognized {} \"{}\"", itemLabel, tag));
    }

    JsonResult<std::uint32_t> readFlagBits(
        const Json *value,
        const std::span<const std::string_view> names,
        std::string_view property,
        std::string_view itemLabel) {
        if (value == nullptr || !json_is_array(value)) {
            return std::unexpected(std::format(
                "Property \"{}\" not found or is not an array", property));
        }
        std::uint32_t flags = 0;
        for (std::size_t i = 0; i < json_array_size(value); i++) {
            const Json *tag_json = json_array_get(value, i);
            if (!json_is_string(tag_json)) {
                return std::unexpected(std::format(
                    "Array \"{}\" contains non-string value", property));
            }
            std::string_view tag = json_string_value(tag_json);
            bool matched = false;
            for (std::size_t j = 0; j < names.size(); j++) {
                if (names[j] == tag) {
                    flags |= (1u << j);
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                return std::unexpected(std::format(
                    "Unrecognized {} \"{}\"", itemLabel, tag));
            }
        }
        return flags;
    }

    JsonResult<JsonRef> asArrayOrWrap(Json *value) {
        if (value == nullptr) {
            return std::unexpected(std::string("Missing value"));
        }
        if (json_is_array(value)) {
            if (json_array_size(value) == 0) {
                return std::unexpected(std::string("Empty array"));
            }
            return borrowJson(value);
        }
        // Wrap scalar in a 1-element array.
        Json *arr = json_array();
        json_array_append(arr, value);
        return adoptJson(arr);
    }

    JsonRef makeImageObject(
        const std::string_view path,
        const int x, const int y,
        const int srcX, const int srcY,
        const int srcWidth, const int srcHeight) {
        assert(srcWidth != 0 && srcHeight != 0);

        Json *image = json_object();
        json_object_set_new(image, "path", json_stringn(path.data(), path.size()));
        json_object_set_new(image, "x", json_integer(x));
        json_object_set_new(image, "y", json_integer(y));
        if (srcX >= 0)
            json_object_set_new(image, "src_x", json_integer(srcX));
        if (srcY >= 0)
            json_object_set_new(image, "src_y", json_integer(srcY));
        if (srcWidth > 0)
            json_object_set_new(image, "src_width", json_integer(srcWidth));
        if (srcHeight > 0)
            json_object_set_new(image, "src_height", json_integer(srcHeight));
        json_object_set_new(image, "palette", json_string("keep"));

        return adoptJson(image);
    }
}