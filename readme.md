# RCTGen

Tooling for generating RollerCoaster Tycoon 2 ride sprites.

## Prerequisites

- CMake 3.21 or newer
- A C++17 compiler (MSVC, Clang, or GCC)
- [vcpkg](https://github.com/microsoft/vcpkg) with the `VCPKG_ROOT` environment
  variable set to your vcpkg checkout
- Ninja (for the default presets) — already bundled with recent versions of
  Visual Studio and most package managers

All other dependencies (`zlib`, `libpng`, `assimp`, `embree`, `jansson`,
`libzip`) are listed in `vcpkg.json` and installed automatically by vcpkg
during configure.

## Building

The project ships with `CMakePresets.json`, so configure + build is a
two-liner on every platform:

```bash
cmake --preset release
cmake --build --preset release
```

For a debug build:

```bash
cmake --preset debug
cmake --build --preset debug
```

On Windows, if you would prefer a Visual Studio solution:

```bat
cmake --preset vs
cmake --build --preset vs-release
```

Binaries land in `build/<preset>/<Config>/`. The available executables are:

- `makevehicle` — generate RCT2 vehicle sprites
- `maketrack` — generate RCT2 track sprites
- `subposition` — generate subposition data
- `TestIsoRender` — sandbox for the isometric renderer

### Manual configure (no presets)

If you cannot use presets, the equivalent command is:

```bash
cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build
```
