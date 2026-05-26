/// ProjectLoader.hpp

#pragma once

#include <expected>
#include <string>
#include <vector>

#include <jansson.h>

#include "Renderer.hpp"
#include "Model.hpp"
#include "Project.hpp"

namespace RCTGen {
    using LoadError = std::string;
    using Json = json_t;

    template<class T>
    using LoadResult = std::expected<T, LoadError>;

    [[nodiscard]] LoadResult<void> loadModel(Model &model, Json *json, int numMeshes, int numFrames);

    [[nodiscard]] LoadResult<std::vector<Light> > loadLights(Json *json);

    [[nodiscard]] LoadResult<void> loadProject(Project & project, Json * json);
}