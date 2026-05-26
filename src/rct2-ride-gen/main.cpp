/// main.cpp

#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

#include <jansson.h>

#include "Renderer.hpp"
#include "VectorMath.hpp"
#include "Constants.hpp"
#include "Json.hpp"
#include "Logging.hpp"
#include "Project.hpp"
#include "ProjectExporter.hpp"
#include "ProjectLoader.hpp"

namespace fs = std::filesystem;
using namespace RCTGen;

namespace {
    enum class Mode {
        export_,
        skipRender,
        test
    };

    struct CliArgs {
        Mode mode;
        fs::path input_file;
    };

    std::expected<CliArgs, std::string> parse_cli(const std::span<char * const> argv) {
        if (argv.size() == 3) {
            const std::string_view flag = argv[1];
            Mode mode;
            if (flag == "--test")
                mode = Mode::test;
            else if (flag == "--skip-render")
                mode = Mode::skipRender;
            else
                return std::unexpected(std::format("Unrecognized option {}", flag));
            return CliArgs{mode, argv[2]};
        }
        if (argv.size() == 2) {
            return CliArgs{Mode::export_, argv[1]};
        }
        return std::unexpected(std::string("Usage: makevehicle [--test|--skip-render] <file>"));
    }

    // The hand-tuned default lighting rig matches the historical setup.
    std::vector<Light> default_lights() {
        return {
            {LIGHT_DIFFUSE, 0, vector3_normalize(vector3(0.0f, -1.0f, 0.0f)), 0.1f},
            {LIGHT_DIFFUSE, 0, vector3_normalize(vector3(0.0f, 0.5f, -1.0f)), 0.8f},
            {LIGHT_SPECULAR, 1, vector3_normalize(vector3(1.0f, 1.65f, -1.0f)), 0.5f},
            {LIGHT_DIFFUSE, 1, vector3_normalize(vector3(1.0f, 1.7f, -1.0f)), 0.8f},
            {LIGHT_DIFFUSE, 0, vector3(0.0f, 1.0f, 0.0f), 0.45f},
            {LIGHT_DIFFUSE, 0, vector3_normalize(vector3(-1.0f, 0.85f, 1.0f)), 0.475f},
            {LIGHT_DIFFUSE, 0, vector3_normalize(vector3(0.75f, 0.4f, -1.0f)), 0.6f},
            {LIGHT_DIFFUSE, 0, vector3_normalize(vector3(1.0f, 0.25f, 0.0f)), 0.5f},
            {LIGHT_DIFFUSE, 0, vector3_normalize(vector3(-1.0f, -0.5f, 0.0f)), 0.1f},
        };
    }
}

int main(int argc, char **argv) {
    auto cli = parse_cli(std::span<char * const>(argv, static_cast<std::size_t>(argc)));
    if (!cli) {
        printMsg("Error: {}", cli.error());
        return 1;
    }

    auto project_json = loadFile(cli->input_file);
    if (!project_json) {
        printMsg("Error: {}", project_json.error());
        return 1;
    }
    json_t *root = project_json->get();

    fs::path output_directory = ".";
    if (json_t *od = json_object_get(root, "output_directory")) {
        auto od_str = readString(od, "output_directory");
        if (!od_str) {
            printMsg("Error: {}", od_str.error());
            return 1;
        }
        output_directory = *od_str;
    }

    std::vector<Light> lights = default_lights();
    if (json_t *lights_json = json_object_get(root, "lights")) {
        auto loaded = loadLights(lights_json);
        if (!loaded) {
            printMsg("Error: {}", loaded.error());
            return 1;
        }
        lights = std::move(*loaded);
    }

    Project project;
    if (auto r = loadProject(project, root); !r) {
        printMsg("Error: {}", r.error());
        return 1;
    }

    Context context;
    context_init(
        &context,
        lights.data(), static_cast<int>(lights.size()),
        1,
        palette_rct2(),
        (cli->mode == Mode::test) ? 0.125f * kTileSize : kTileSize);

    int exit_code = 0;
    if (cli->mode == Mode::test) {
        if (auto r = exportProjectTest(project, context); !r) {
            printMsg("Error: {}", r.error());
            exit_code = 1;
        }
    } else {
        auto r = exportProject(
            project, context, output_directory, cli->mode == Mode::skipRender);
        if (!r) {
            printMsg("Error: {}", r.error());
            exit_code = 1;
        }
    }

    context_destroy(&context);
    return exit_code;
}