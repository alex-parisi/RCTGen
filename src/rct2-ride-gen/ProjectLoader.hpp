/// ProjectLoader.hpp

#pragma once

#include <expected>
#include <string>
#include <vector>

#include <jansson.h>

#include "../iso-render/renderer.h"
#include "Model.hpp"
#include "Project.hpp"

namespace RCTGen
{
    using LoadError = std::string;

    template <class T>
    using LoadResult = std::expected<T, LoadError>;

    [[nodiscard]] LoadResult<void> loadModel(Model& model, json_t* json, int numMeshes, int numFrames);
    [[nodiscard]] LoadResult<std::vector<light_t>> loadLights(json_t* json);
    [[nodiscard]] LoadResult<void> loadProject(Project& project, json_t* json);
}
