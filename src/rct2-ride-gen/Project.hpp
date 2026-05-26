/// Project.hpp

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "Image.hpp"
#include "Mesh.hpp"
#include "Vehicle.hpp"

namespace RCTGen {
    struct Project {
        std::string id;
        std::string original_id;
        std::string name;
        std::string description;
        std::string capacity;
        std::string author;
        std::string version = "1.0";
        std::string ride_type;

        std::array<std::uint8_t, 5> configuration{};

        std::uint32_t flags = 0;
        std::uint32_t zero_cars = 0;
        std::uint32_t min_cars_per_train = 0;
        std::uint32_t max_cars_per_train = 0;
        std::uint32_t category = 0;
        std::uint32_t build_menu_priority = 0;
        std::uint32_t tab_car = 0;
        std::uint32_t running_sound = 0;
        std::uint32_t secondary_sound = 0;
        std::uint32_t sprite_flags = 0;
        std::uint32_t num_sprites = 0;

        std::vector<std::array<std::uint8_t, 3> > colors;
        std::vector<Mesh> meshes;
        std::vector<Vehicle> vehicles;

        Image preview{};
    };
}