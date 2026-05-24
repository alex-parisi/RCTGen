/// ProjectLoader.hpp

#pragma once

#include <expected>
#include <string>
#include <vector>

#include <jansson.h>

#include "../iso-render/renderer.h"
#include "Model.hpp"
#include "Project.hpp"

namespace rctgen
{
    using LoadError = std::string;

    template <class T>
    using LoadResult = std::expected<T, LoadError>;

    [[nodiscard]] LoadResult<void> load_model(Model& model, json_t* json, int num_meshes, int num_frames);
    [[nodiscard]] LoadResult<std::vector<light_t>> load_lights(json_t* json);
    [[nodiscard]] LoadResult<void> load_project(Project& project, json_t* json);
}
