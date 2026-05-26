/// Json.cpp

#include "Json.hpp"

#include <format>

namespace RCTGen {
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

    JsonResult<JsonRef> loadFile(const std::filesystem::path &path) {
        json_error_t err;
        Json *raw = json_load_file(path.string().c_str(), 0, &err);
        if (raw == nullptr) {
            return std::unexpected(std::format(
                "{} at line {} column {} ({})", err.text, err.line, err.column, err.source));
        }
        return adoptJson(raw);
    }

    int dumpFile(const Json *json, const std::filesystem::path &path, std::size_t flags) {
        return json_dump_file(json, path.string().c_str(), flags);
    }

    JsonResult<std::int64_t> readInt(const Json *value, std::string_view property) {
        if (value == nullptr || !json_is_integer(value)) {
            return std::unexpected(std::format(
                "Property \"{}\" not found or is not an integer", property));
        }
        return json_integer_value(value);
    }

    JsonResult<bool> readBool(const Json *value, std::string_view property) {
        if (value == nullptr || !json_is_boolean(value)) {
            return std::unexpected(std::format(
                "Property \"{}\" not found or is not a boolean", property));
        }
        return json_boolean_value(value) != 0;
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
}