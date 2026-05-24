/// project.h

#pragma once

#include "../iso-render/renderer.h"
#include "Project.hpp"

inline constexpr float TILE_SIZE = 3.3f;

// Sprite flags
inline constexpr std::uint32_t SPRITE_FLAT_SLOPE                       = 0x0001;
inline constexpr std::uint32_t SPRITE_GENTLE_SLOPE                     = 0x0002;
inline constexpr std::uint32_t SPRITE_STEEP_SLOPE                      = 0x0004;
inline constexpr std::uint32_t SPRITE_VERTICAL_SLOPE                   = 0x0008;
inline constexpr std::uint32_t SPRITE_DIAGONAL_SLOPE                   = 0x0010;
inline constexpr std::uint32_t SPRITE_BANKING                          = 0x0020;
inline constexpr std::uint32_t SPRITE_INLINE_TWIST                     = 0x0040;
inline constexpr std::uint32_t SPRITE_SLOPE_BANK_TRANSITION            = 0x0080;
inline constexpr std::uint32_t SPRITE_DIAGONAL_BANK_TRANSITION         = 0x0100;
inline constexpr std::uint32_t SPRITE_SLOPED_BANK_TRANSITION           = 0x0200;
inline constexpr std::uint32_t SPRITE_SLOPED_BANKED_TURN               = 0x0400;
inline constexpr std::uint32_t SPRITE_BANKED_SLOPE_TRANSITION          = 0x0800;
inline constexpr std::uint32_t SPRITE_CORKSCREW                        = 0x1000;
inline constexpr std::uint32_t SPRITE_ZERO_G_ROLL                      = 0x2000;
inline constexpr std::uint32_t SPRITE_DIAGONAL_SLOPED_BANK_TRANSITION  = 0x4000;
inline constexpr std::uint32_t SPRITE_DIVE_LOOP                        = 0x8000;

// Ride flags
inline constexpr std::uint32_t RIDE_NO_COLLISION_CRASHES = 1;
inline constexpr std::uint32_t RIDE_RIDER_CONTROLS_SPEED = 2;

// Vehicle flags
inline constexpr std::uint32_t VEHICLE_SECONDARY_REMAP     = 1;
inline constexpr std::uint32_t VEHICLE_TERTIARY_REMAP      = 2;
inline constexpr std::uint32_t VEHICLE_RIDERS_SCREAM       = 4;
inline constexpr std::uint32_t VEHICLE_RESTRAINT_ANIMATION = 8;

// Running sounds
inline constexpr std::uint32_t RUNNING_SOUND_WOODEN_OLD    = 1;
inline constexpr std::uint32_t RUNNING_SOUND_WOODEN_MODERN = 54;
inline constexpr std::uint32_t RUNNING_SOUND_STEEL         = 2;
inline constexpr std::uint32_t RUNNING_SOUND_STEEL_SMOOTH  = 57;
inline constexpr std::uint32_t RUNNING_SOUND_WATERSLIDE    = 32;
inline constexpr std::uint32_t RUNNING_SOUND_TRAIN         = 31;
inline constexpr std::uint32_t RUNNING_SOUND_ENGINE        = 21;
inline constexpr std::uint32_t RUNNING_SOUND_NONE          = 255;

// Secondary sounds
inline constexpr std::uint32_t SECONDARY_SOUND_SCREAMS_1 = 0;
inline constexpr std::uint32_t SECONDARY_SOUND_SCREAMS_2 = 1;
inline constexpr std::uint32_t SECONDARY_SOUND_SCREAMS_3 = 2;
inline constexpr std::uint32_t SECONDARY_SOUND_WHISTLE   = 3;
inline constexpr std::uint32_t SECONDARY_SOUND_BELL      = 4;
inline constexpr std::uint32_t SECONDARY_SOUND_NONE      = 255;

// Car indices
inline constexpr std::uint32_t CAR_INDEX_DEFAULT = 0;
inline constexpr std::uint32_t CAR_INDEX_FRONT   = 1;
inline constexpr std::uint32_t CAR_INDEX_SECOND  = 2;
inline constexpr std::uint32_t CAR_INDEX_THIRD   = 4;
inline constexpr std::uint32_t CAR_INDEX_REAR    = 3;

// Categories
inline constexpr std::uint32_t CATEGORY_TRANSPORT_RIDE = 0;
inline constexpr std::uint32_t CATEGORY_GENTLE_RIDE    = 1;
inline constexpr std::uint32_t CATEGORY_ROLLERCOASTER  = 2;
inline constexpr std::uint32_t CATEGORY_THRILL_RIDE    = 3;
inline constexpr std::uint32_t CATEGORY_WATER_RIDE     = 4;
inline constexpr std::uint32_t CATEGORY_SHOP           = 5;

int count_animation_frames(std::uint16_t sprites);
int count_sprites_from_flags(std::uint16_t sprites, int flags);
int project_export(Project* project, context_t* context, const char* output_directory, int skip_render);
int project_export_test(Project* project, context_t* context);

inline constexpr int NUM_SPRITE_GROUPS    = 16;
inline constexpr int NUM_FLAGS            = 2;
inline constexpr int NUM_VEHICLE_FLAGS    = 4;
inline constexpr int NUM_RUNNING_SOUNDS   = 6;
inline constexpr int NUM_SECONDARY_SOUNDS = 4;
inline constexpr int NUM_COLORS           = 32;
inline constexpr int NUM_CATEGORIES       = 4;

extern const char* sprite_group_names[NUM_SPRITE_GROUPS];
extern const char* flag_names[NUM_FLAGS];
extern const char* vehicle_flag_names[NUM_VEHICLE_FLAGS];
extern const char* running_sounds[NUM_RUNNING_SOUNDS];
extern const char* secondary_sounds[NUM_SECONDARY_SOUNDS];
extern const char* color_names[NUM_COLORS];
extern const char* category_names[NUM_CATEGORIES];
