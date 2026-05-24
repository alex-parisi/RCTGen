/// SpriteRenderer.cpp

#include "SpriteRenderer.hpp"

#include <cmath>
#include <numbers>
#include <span>
#include <vector>

#include "Logging.hpp"

namespace rctgen
{
    namespace
    {
        using std::numbers::pi;
        using std::numbers::sqrt2;

        constexpr double kPi   = pi;
        constexpr double kPi_2 = pi / 2.0;
        constexpr double kPi_4 = pi / 4.0;
        constexpr double kPi_8 = pi / 8.0;
        constexpr double kPi_12 = pi / 12.0;
        constexpr double kSqrt1_2 = 1.0 / sqrt2;

        // Slope geometry: a track tile rises 1 unit over sqrt(6) of horizontal
        // run (so the "gentle" slope angle is atan(1/sqrt(6))).
        const double kTileSlope = 1.0 / std::sqrt(6.0);

        const double kFlat = 0.0;
        const double kGentle = std::atan(kTileSlope);
        const double kSteep = std::atan(4.0 * kTileSlope);
        const double kVertical = kPi_2;
        const double kFlatGentleTransition = (kFlat + kGentle) / 2.0;
        const double kGentleSteepTransition = (kGentle + kSteep) / 2.0;
        const double kSteepVerticalTransition = (kSteep + kVertical) / 2.0;

        const double kGentleDiagonal = std::atan(kTileSlope * kSqrt1_2);
        const double kSteepDiagonal = std::atan(4.0 * kTileSlope * kSqrt1_2);
        const double kFlatGentleTransitionDiagonal = (kFlat + kGentleDiagonal) / 2.0;

        constexpr double kBank = kPi_4;
        constexpr double kBankTransition = kBank / 2.0;

        // Corkscrew angle decomposition.
        constexpr double corkscrew_right_yaw(double a) {
            return std::atan2(0.5 * (1.0 - std::cos(a)), 1.0 - 0.5 * (1.0 - std::cos(a)));
        }
        constexpr double corkscrew_right_pitch(double a) {
            return -std::asin(-std::sin(a) / std::sqrt(2.0));
        }
        constexpr double corkscrew_right_roll(double a) {
            return -std::atan2(std::sin(a) / std::sqrt(2.0), std::cos(a));
        }

        struct Rotation
        {
            int    num_frames;
            double pitch;
            double roll;
            double yaw;
        };

        // Mirror the historical render path: the table angles round-trip through
        // float at the function-argument boundary, then promote back to double
        // for the per-frame yaw step. Skipping the round-trip shifts a handful
        // of antialiased edge pixels.
        void render_rotation(
            context_t* context, int num_frames,
            double pitch_d, double roll_d, double yaw_d,
            image_t* images)
        {
            const float pitch = static_cast<float>(pitch_d);
            const float roll  = static_cast<float>(roll_d);
            const float yaw   = static_cast<float>(yaw_d);
            for (int i = 0; i < num_frames; i++)
            {
                context_render_view(
                    context,
                    matrix_mult(
                        rotate_y(static_cast<float>(yaw + (2.0 * i * kPi) / num_frames)),
                        matrix_mult(rotate_z(pitch), rotate_x(roll))),
                    images + i);
            }
        }

        // --- Rotation tables for each sprite group --------------------------------

        const Rotation kFlatSlopeRot[] = {
            {32, kFlat, 0, 0},
        };
        const Rotation kGentleSlopeRot[] = {
            {4,   kFlatGentleTransition, 0, 0}, {4,  -kFlatGentleTransition, 0, 0},
            {32,  kGentle,                0, 0}, {32, -kGentle,                0, 0},
        };
        const Rotation kSteepSlopeRot[] = {
            {8,   kGentleSteepTransition, 0, 0}, {8,  -kGentleSteepTransition, 0, 0},
            {32,  kSteep,                 0, 0}, {32, -kSteep,                 0, 0},
        };
        const Rotation kVerticalSlopeRot[] = {
            {4,  kSteepVerticalTransition, 0, 0}, {4, -kSteepVerticalTransition, 0, 0},
            {32, kVertical,                0, 0}, {32, -kVertical,                0, 0},
            {4,  kVertical + 1.0*kPi_12, 0, 0},   {4, -kVertical - 1.0*kPi_12, 0, 0},
            {4,  kVertical + 2.0*kPi_12, 0, 0},   {4, -kVertical - 2.0*kPi_12, 0, 0},
            {4,  kVertical + 3.0*kPi_12, 0, 0},   {4, -kVertical - 3.0*kPi_12, 0, 0},
            {4,  kVertical + 4.0*kPi_12, 0, 0},   {4, -kVertical - 4.0*kPi_12, 0, 0},
            {4,  kVertical + 5.0*kPi_12, 0, 0},   {4, -kVertical - 5.0*kPi_12, 0, 0},
            {4,  kPi,                      0, 0},
        };
        const Rotation kDiagonalSlopeRot[] = {
            {4,  kFlatGentleTransitionDiagonal, 0, kPi_4}, {4, -kFlatGentleTransitionDiagonal, 0, kPi_4},
            {4,  kGentleDiagonal,                0, kPi_4}, {4, -kGentleDiagonal,                0, kPi_4},
            {4,  kSteepDiagonal,                 0, kPi_4}, {4, -kSteepDiagonal,                 0, kPi_4},
        };
        const Rotation kBankingRot[] = {
            {8,  kFlat,  kBankTransition, 0}, {8,  kFlat, -kBankTransition, 0},
            {32, kFlat,  kBank,            0}, {32, kFlat, -kBank,            0},
        };
        const Rotation kInlineTwistRot[] = {
            {4, kFlat,  3.0*kPi_8,  0}, {4, kFlat, -3.0*kPi_8,  0},
            {4, kFlat,  kPi_2,       0}, {4, kFlat, -kPi_2,       0},
            {4, kFlat,  5.0*kPi_8,  0}, {4, kFlat, -5.0*kPi_8,  0},
            {4, kFlat,  3.0*kPi_4,  0}, {4, kFlat, -3.0*kPi_4,  0},
            {4, kFlat,  7.0*kPi_8,  0}, {4, kFlat, -7.0*kPi_8,  0},
        };
        const Rotation kSlopeBankTransitionRot[] = {
            {32,  kFlatGentleTransition,  kBankTransition, 0},
            {32,  kFlatGentleTransition, -kBankTransition, 0},
            {32, -kFlatGentleTransition,  kBankTransition, 0},
            {32, -kFlatGentleTransition, -kBankTransition, 0},
        };
        const Rotation kDiagonalBankTransitionRot[] = {
            {4,  kFlatGentleTransitionDiagonal,  kBankTransition, kPi_4},
            {4,  kFlatGentleTransitionDiagonal, -kBankTransition, kPi_4},
            {4, -kFlatGentleTransitionDiagonal,  kBankTransition, kPi_4},
            {4, -kFlatGentleTransitionDiagonal, -kBankTransition, kPi_4},
        };
        const Rotation kSlopedBankTransitionRot[] = {
            {4,  kGentle,  kBankTransition, 0}, {4,  kGentle, -kBankTransition, 0},
            {4, -kGentle,  kBankTransition, 0}, {4, -kGentle, -kBankTransition, 0},
        };
        const Rotation kDiagonalSlopedBankTransitionRot[] = {
            {4,  kFlatGentleTransitionDiagonal,  kBank,            kPi_4},
            {4,  kFlatGentleTransitionDiagonal, -kBank,            kPi_4},
            {4, -kFlatGentleTransitionDiagonal,  kBank,            kPi_4},
            {4, -kFlatGentleTransitionDiagonal, -kBank,            kPi_4},
            {4,  kGentleDiagonal,                kBankTransition, kPi_4},
            {4,  kGentleDiagonal,               -kBankTransition, kPi_4},
            {4, -kGentleDiagonal,                kBankTransition, kPi_4},
            {4, -kGentleDiagonal,               -kBankTransition, kPi_4},
            {4,  kGentleDiagonal,                kBank,            kPi_4},
            {4,  kGentleDiagonal,               -kBank,            kPi_4},
            {4, -kGentleDiagonal,                kBank,            kPi_4},
            {4, -kGentleDiagonal,               -kBank,            kPi_4},
        };
        const Rotation kSlopedBankedTurnRot[] = {
            {32,  kGentle,  kBank, 0}, {32,  kGentle, -kBank, 0},
            {32, -kGentle,  kBank, 0}, {32, -kGentle, -kBank, 0},
        };
        const Rotation kBankedSlopeTransitionRot[] = {
            {4,  kFlatGentleTransition,  kBank, 0}, {4,  kFlatGentleTransition, -kBank, 0},
            {4, -kFlatGentleTransition,  kBank, 0}, {4, -kFlatGentleTransition, -kBank, 0},
        };

        // Zero-G roll is the most elaborate group; the last sub-group conditionally
        // adopts an 8-frame variant when DiveLoop is also enabled.
        const Rotation kZeroGRollBaseRot[] = {
            // Gentle bank 67.5
            {4,  kGentle,  3.0*kPi_8, 0}, {4,  kGentle, -3.0*kPi_8, 0},
            {4, -kGentle,  3.0*kPi_8, 0}, {4, -kGentle, -3.0*kPi_8, 0},
            // Gentle bank 90
            {4,  kGentle,  kPi_2,    0}, {4,  kGentle, -kPi_2,    0},
            {4, -kGentle,  kPi_2,    0}, {4, -kGentle, -kPi_2,    0},
            // Gentle 112.5
            {4,  kGentle,  5.0*kPi_8, 0}, {4,  kGentle, -5.0*kPi_8, 0},
            {4, -kGentle,  5.0*kPi_8, 0}, {4, -kGentle, -5.0*kPi_8, 0},
            // Gentle bank 135
            {4,  kGentle,  3.0*kPi_4, 0}, {4,  kGentle, -3.0*kPi_4, 0},
            {4, -kGentle,  3.0*kPi_4, 0}, {4, -kGentle, -3.0*kPi_4, 0},
            // Gentle bank 157.5
            {4,  kGentle,  7.0*kPi_8, 0}, {4,  kGentle, -7.0*kPi_8, 0},
            {4, -kGentle,  7.0*kPi_8, 0}, {4, -kGentle, -7.0*kPi_8, 0},
            // Gentle-to-steep bank 22.5
            {4,  kGentleSteepTransition,  kPi_8,    0}, {4,  kGentleSteepTransition, -kPi_8,    0},
            {4, -kGentleSteepTransition,  kPi_8,    0}, {4, -kGentleSteepTransition, -kPi_8,    0},
            // Gentle-to-steep bank 45
            {4,  kGentleSteepTransition,  2.0*kPi_8, 0}, {4,  kGentleSteepTransition, -2.0*kPi_8, 0},
            {4, -kGentleSteepTransition,  2.0*kPi_8, 0}, {4, -kGentleSteepTransition, -2.0*kPi_8, 0},
            // Gentle-to-steep bank 67.5
            {4,  kGentleSteepTransition,  3.0*kPi_8, 0}, {4,  kGentleSteepTransition, -3.0*kPi_8, 0},
            {4, -kGentleSteepTransition,  3.0*kPi_8, 0}, {4, -kGentleSteepTransition, -3.0*kPi_8, 0},
            // Gentle-to-steep bank 90
            {4,  kGentleSteepTransition,  kPi_2,    0}, {4,  kGentleSteepTransition, -kPi_2,    0},
            {4, -kGentleSteepTransition,  kPi_2,    0}, {4, -kGentleSteepTransition, -kPi_2,    0},
        };
        // Steep bank 22.5 — 4 frames unless DiveLoop set, then 8 frames.
        const Rotation kZeroGRollSteepBank22_4[] = {
            {4,  kSteep,  kPi_8, 0}, {4,  kSteep, -kPi_8, 0},
            {4, -kSteep,  kPi_8, 0}, {4, -kSteep, -kPi_8, 0},
        };
        const Rotation kZeroGRollSteepBank22_8[] = {
            {8,  kSteep,  kPi_8, 0}, {8,  kSteep, -kPi_8, 0},
            {8, -kSteep,  kPi_8, 0}, {8, -kSteep, -kPi_8, 0},
        };

        const Rotation kDiveLoopRot[] = {
            // Steep bank 45
            {8,  kSteepDiagonal,  kPi_4,        kPi_8}, {8,  kSteepDiagonal, -kPi_4,        kPi_8},
            {8, -kSteepDiagonal,  kPi_4,        kPi_8}, {8, -kSteepDiagonal, -kPi_4,        kPi_8},
            // Steep bank 67.5
            {8,  kSteepDiagonal,  3.0*kPi_8,    kPi_8}, {8,  kSteepDiagonal, -3.0*kPi_8,    kPi_8},
            {8, -kSteepDiagonal,  3.0*kPi_8,    kPi_8}, {8, -kSteepDiagonal, -3.0*kPi_8,    kPi_8},
            // Diagonal steep bank 90
            {8,  kSteepDiagonal,  kPi_2,        kPi_8}, {8,  kSteepDiagonal, -kPi_2,        kPi_8},
            {8, -kSteepDiagonal,  kPi_2,        kPi_8}, {8, -kSteepDiagonal, -kPi_2,        kPi_8},
        };

        const std::array<double, 5> kCorkscrewAngles = {
            2.0 * kPi_12, 4.0 * kPi_12, kPi_2, 8.0 * kPi_12, 10.0 * kPi_12,
        };

        // Build the corkscrew rotation table once at static init.
        std::vector<Rotation> build_corkscrew_rotations()
        {
            std::vector<Rotation> v;
            v.reserve(20);
            // Right
            for (double a : kCorkscrewAngles)
                v.push_back({4, corkscrew_right_pitch(a), corkscrew_right_roll(a), corkscrew_right_yaw(a)});
            for (double a : kCorkscrewAngles)
                v.push_back({4, corkscrew_right_pitch(-a), corkscrew_right_roll(-a), corkscrew_right_yaw(-a)});
            // Left mirrors right
            for (double a : kCorkscrewAngles)
                v.push_back({4, -corkscrew_right_pitch(-a), -corkscrew_right_roll(a), -corkscrew_right_yaw(a)});
            for (double a : kCorkscrewAngles)
                v.push_back({4, -corkscrew_right_pitch(a), -corkscrew_right_roll(-a), -corkscrew_right_yaw(-a)});
            return v;
        }
        const std::vector<Rotation>& corkscrew_rotations()
        {
            static const std::vector<Rotation> v = build_corkscrew_rotations();
            return v;
        }

        int sum_frames(std::span<const Rotation> rots)
        {
            int n = 0;
            for (const auto& r : rots) n += r.num_frames;
            return n;
        }

        int render_group(
            context_t* context, std::span<const Rotation> rots, image_t* base)
        {
            int written = 0;
            for (const auto& r : rots)
            {
                render_rotation(context, r.num_frames, r.pitch, r.roll, r.yaw, base + written);
                written += r.num_frames;
            }
            return written;
        }

        // Restraint animation frames live outside the static groups: when
        // frame > 0 we emit 4 flat rotations.
        constexpr int kRestraintFrames = 12;       // count_sprites contribution
        constexpr int kRestraintPerFrame = 4;
    } // namespace

    int count_sprites(SpriteFlag sf, VehicleFlag vf)
    {
        int n = 0;
        if (has_flag(sf, SpriteFlag::FlatSlope))                    n +=  32;
        if (has_flag(sf, SpriteFlag::GentleSlope))                  n +=  72;
        if (has_flag(sf, SpriteFlag::SteepSlope))                   n +=  80;
        if (has_flag(sf, SpriteFlag::VerticalSlope))                n += 116;
        if (has_flag(sf, SpriteFlag::DiagonalSlope))                n +=  24;
        if (has_flag(sf, SpriteFlag::Banking))                      n +=  80;
        if (has_flag(sf, SpriteFlag::InlineTwist))                  n +=  40;
        if (has_flag(sf, SpriteFlag::SlopeBankTransition))          n += 128;
        if (has_flag(sf, SpriteFlag::DiagonalBankTransition))       n +=  16;
        if (has_flag(sf, SpriteFlag::SlopedBankTransition))         n +=  16;
        if (has_flag(sf, SpriteFlag::DiagonalSlopedBankTransition)) n +=  48;
        if (has_flag(sf, SpriteFlag::SlopedBankedTurn))             n += 128;
        if (has_flag(sf, SpriteFlag::BankedSlopeTransition))        n +=  16;
        if (has_flag(sf, SpriteFlag::Corkscrew))                    n +=  80;
        if (has_flag(sf, SpriteFlag::ZeroGRoll))                    n += 160;
        if (has_flag(sf, SpriteFlag::DiveLoop))                     n += 112;
        if (has_flag(vf, VehicleFlag::RestraintAnimation))          n +=  kRestraintFrames;
        return n;
    }

    int render_vehicle_frame(
        context_t* context, SpriteFlag sprite_flags, int frame, image_t* out)
    {
        if (frame > 0)
        {
            print_msg("Rendering restraint animation");
            render_rotation(context, kRestraintPerFrame, 0, 0, 0, out);
            return kRestraintPerFrame;
        }

        int base = 0;

        auto emit_if = [&](SpriteFlag f, const char* msg, std::span<const Rotation> rots) {
            if (has_flag(sprite_flags, f))
            {
                print_msg(msg);
                base += render_group(context, rots, out + base);
            }
        };

        emit_if(SpriteFlag::FlatSlope,              "Rendering flat sprites",                          kFlatSlopeRot);
        emit_if(SpriteFlag::GentleSlope,            "Rendering gentle sprites",                        kGentleSlopeRot);
        emit_if(SpriteFlag::SteepSlope,             "Rendering steep sprites",                         kSteepSlopeRot);
        emit_if(SpriteFlag::VerticalSlope,          "Rendering vertical sprites",                      kVerticalSlopeRot);
        emit_if(SpriteFlag::DiagonalSlope,          "Rendering diagonal sprites",                      kDiagonalSlopeRot);
        emit_if(SpriteFlag::Banking,                "Rendering banked sprites",                        kBankingRot);
        emit_if(SpriteFlag::InlineTwist,            "Rendering inline twist sprites",                  kInlineTwistRot);
        emit_if(SpriteFlag::SlopeBankTransition,    "Rendering slope-bank transition sprites",         kSlopeBankTransitionRot);
        emit_if(SpriteFlag::DiagonalBankTransition, "Rendering diagonal slope-bank transition sprites",kDiagonalBankTransitionRot);
        emit_if(SpriteFlag::SlopedBankTransition,   "Rendering sloped bank transition sprites",        kSlopedBankTransitionRot);
        emit_if(SpriteFlag::DiagonalSlopedBankTransition,
                "Rendering diagonal sloped bank transition sprites",                                   kDiagonalSlopedBankTransitionRot);
        emit_if(SpriteFlag::SlopedBankedTurn,       "Rendering sloped banked sprites",                 kSlopedBankedTurnRot);
        emit_if(SpriteFlag::BankedSlopeTransition,  "Rendering banked slope transition sprites",       kBankedSlopeTransitionRot);

        if (has_flag(sprite_flags, SpriteFlag::ZeroGRoll))
        {
            print_msg("Rendering zero G roll sprites");
            base += render_group(context, kZeroGRollBaseRot, out + base);
            std::span<const Rotation> sb22 = has_flag(sprite_flags, SpriteFlag::DiveLoop)
                ? std::span<const Rotation>(kZeroGRollSteepBank22_8)
                : std::span<const Rotation>(kZeroGRollSteepBank22_4);
            base += render_group(context, sb22, out + base);
        }
        if (has_flag(sprite_flags, SpriteFlag::DiveLoop))
        {
            print_msg("Rendering dive loop sprites");
            base += render_group(context, kDiveLoopRot, out + base);
        }
        if (has_flag(sprite_flags, SpriteFlag::Corkscrew))
        {
            print_msg("Rendering corkscrew sprites");
            const auto& rots = corkscrew_rotations();
            base += render_group(context, rots, out + base);
        }

        return base;
    }
}
