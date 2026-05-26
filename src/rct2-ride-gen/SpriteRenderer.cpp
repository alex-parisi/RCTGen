/// SpriteRenderer.cpp

#include "SpriteRenderer.hpp"

#include <cmath>
#include <numbers>
#include <span>
#include <vector>

#include "Logging.hpp"

namespace RCTGen {

    namespace {
        using std::numbers::pi;
        using std::numbers::sqrt2;

        constexpr double kPi = pi;
        constexpr double kPi2 = pi / 2.0;
        constexpr double kPi4 = pi / 4.0;
        constexpr double kPi8 = pi / 8.0;
        constexpr double kPi12 = pi / 12.0;
        constexpr double kSqrt12 = 1.0 / sqrt2;

        // Slope geometry: a track tile rises 1 unit over sqrt(6) of horizontal
        // run (so the "gentle" slope angle is atan(1/sqrt(6))).
        const double kTileSlope = 1.0 / std::sqrt(6.0);

        constexpr double kFlat = 0.0;
        const double kGentle = std::atan(kTileSlope);
        const double kSteep = std::atan(4.0 * kTileSlope);
        constexpr double kVertical = kPi2;
        const double kFlatGentleTransition = (kFlat + kGentle) / 2.0;
        const double kGentleSteepTransition = (kGentle + kSteep) / 2.0;
        const double kSteepVerticalTransition = (kSteep + kVertical) / 2.0;

        const double kGentleDiagonal = std::atan(kTileSlope * kSqrt12);
        const double kSteepDiagonal = std::atan(4.0 * kTileSlope * kSqrt12);
        const double kFlatGentleTransitionDiagonal = (kFlat + kGentleDiagonal) / 2.0;

        constexpr double kBank = kPi4;
        constexpr double kBankTransition = kBank / 2.0;

        // Corkscrew angle decomposition.
        constexpr double corkscrewRightYaw(const double a) {
            return std::atan2(0.5 * (1.0 - std::cos(a)), 1.0 - 0.5 * (1.0 - std::cos(a)));
        }

        constexpr double corkscrewRightPitch(const double a) {
            return -std::asin(-std::sin(a) / std::sqrt(2.0));
        }

        constexpr double corkscrewRightRoll(const double a) {
            return -std::atan2(std::sin(a) / std::sqrt(2.0), std::cos(a));
        }

        struct Rotation {
            int num_frames;
            double pitch;
            double roll;
            double yaw;
        };

        // Mirror the historical render path: the table angles round-trip through
        // float at the function-argument boundary, then promote back to double
        // for the per-frame yaw step. Skipping the round-trip shifts a handful
        // of antialiased edge pixels.
        void renderRotation(
            Context *context, const int numFrames,
            const double pitchD, const double rollD, const double yawD,
            Image *images) {
            const auto pitch = static_cast<float>(pitchD);
            const auto roll = static_cast<float>(rollD);
            const auto yaw = static_cast<float>(yawD);
            for (int i = 0; i < numFrames; i++) {
                context_render_view(
                    context,
                    matrix_mult(
                        rotate_y(static_cast<float>(yaw + (2.0 * i * kPi) / numFrames)),
                        matrix_mult(rotate_z(pitch), rotate_x(roll))),
                    images + i);
            }
        }

        // --- Rotation tables for each sprite group --------------------------------

        constexpr Rotation kFlatSlopeRot[] = {
            {32, kFlat, 0, 0},
        };
        const Rotation kGentleSlopeRot[] = {
            {4, kFlatGentleTransition, 0, 0}, {4, -kFlatGentleTransition, 0, 0},
            {32, kGentle, 0, 0}, {32, -kGentle, 0, 0},
        };
        const Rotation kSteepSlopeRot[] = {
            {8, kGentleSteepTransition, 0, 0}, {8, -kGentleSteepTransition, 0, 0},
            {32, kSteep, 0, 0}, {32, -kSteep, 0, 0},
        };
        const Rotation kVerticalSlopeRot[] = {
            {4, kSteepVerticalTransition, 0, 0}, {4, -kSteepVerticalTransition, 0, 0},
            {32, kVertical, 0, 0}, {32, -kVertical, 0, 0},
            {4, kVertical + 1.0 * kPi12, 0, 0}, {4, -kVertical - 1.0 * kPi12, 0, 0},
            {4, kVertical + 2.0 * kPi12, 0, 0}, {4, -kVertical - 2.0 * kPi12, 0, 0},
            {4, kVertical + 3.0 * kPi12, 0, 0}, {4, -kVertical - 3.0 * kPi12, 0, 0},
            {4, kVertical + 4.0 * kPi12, 0, 0}, {4, -kVertical - 4.0 * kPi12, 0, 0},
            {4, kVertical + 5.0 * kPi12, 0, 0}, {4, -kVertical - 5.0 * kPi12, 0, 0},
            {4, kPi, 0, 0},
        };
        const Rotation kDiagonalSlopeRot[] = {
            {4, kFlatGentleTransitionDiagonal, 0, kPi4}, {4, -kFlatGentleTransitionDiagonal, 0, kPi4},
            {4, kGentleDiagonal, 0, kPi4}, {4, -kGentleDiagonal, 0, kPi4},
            {4, kSteepDiagonal, 0, kPi4}, {4, -kSteepDiagonal, 0, kPi4},
        };
        constexpr Rotation kBankingRot[] = {
            {8, kFlat, kBankTransition, 0}, {8, kFlat, -kBankTransition, 0},
            {32, kFlat, kBank, 0}, {32, kFlat, -kBank, 0},
        };
        const Rotation kInlineTwistRot[] = {
            {4, kFlat, 3.0 * kPi8, 0}, {4, kFlat, -3.0 * kPi8, 0},
            {4, kFlat, kPi2, 0}, {4, kFlat, -kPi2, 0},
            {4, kFlat, 5.0 * kPi8, 0}, {4, kFlat, -5.0 * kPi8, 0},
            {4, kFlat, 3.0 * kPi4, 0}, {4, kFlat, -3.0 * kPi4, 0},
            {4, kFlat, 7.0 * kPi8, 0}, {4, kFlat, -7.0 * kPi8, 0},
        };
        const Rotation kSlopeBankTransitionRot[] = {
            {32, kFlatGentleTransition, kBankTransition, 0},
            {32, kFlatGentleTransition, -kBankTransition, 0},
            {32, -kFlatGentleTransition, kBankTransition, 0},
            {32, -kFlatGentleTransition, -kBankTransition, 0},
        };
        const Rotation kDiagonalBankTransitionRot[] = {
            {4, kFlatGentleTransitionDiagonal, kBankTransition, kPi4},
            {4, kFlatGentleTransitionDiagonal, -kBankTransition, kPi4},
            {4, -kFlatGentleTransitionDiagonal, kBankTransition, kPi4},
            {4, -kFlatGentleTransitionDiagonal, -kBankTransition, kPi4},
        };
        const Rotation kSlopedBankTransitionRot[] = {
            {4, kGentle, kBankTransition, 0}, {4, kGentle, -kBankTransition, 0},
            {4, -kGentle, kBankTransition, 0}, {4, -kGentle, -kBankTransition, 0},
        };
        const Rotation kDiagonalSlopedBankTransitionRot[] = {
            {4, kFlatGentleTransitionDiagonal, kBank, kPi4},
            {4, kFlatGentleTransitionDiagonal, -kBank, kPi4},
            {4, -kFlatGentleTransitionDiagonal, kBank, kPi4},
            {4, -kFlatGentleTransitionDiagonal, -kBank, kPi4},
            {4, kGentleDiagonal, kBankTransition, kPi4},
            {4, kGentleDiagonal, -kBankTransition, kPi4},
            {4, -kGentleDiagonal, kBankTransition, kPi4},
            {4, -kGentleDiagonal, -kBankTransition, kPi4},
            {4, kGentleDiagonal, kBank, kPi4},
            {4, kGentleDiagonal, -kBank, kPi4},
            {4, -kGentleDiagonal, kBank, kPi4},
            {4, -kGentleDiagonal, -kBank, kPi4},
        };
        const Rotation kSlopedBankedTurnRot[] = {
            {32, kGentle, kBank, 0}, {32, kGentle, -kBank, 0},
            {32, -kGentle, kBank, 0}, {32, -kGentle, -kBank, 0},
        };
        const Rotation kBankedSlopeTransitionRot[] = {
            {4, kFlatGentleTransition, kBank, 0}, {4, kFlatGentleTransition, -kBank, 0},
            {4, -kFlatGentleTransition, kBank, 0}, {4, -kFlatGentleTransition, -kBank, 0},
        };

        // Zero-G roll is the most elaborate group; the last subgroup conditionally
        // adopts an 8-frame variant when DiveLoop is also enabled.
        const Rotation kZeroGRollBaseRot[] = {
            // Gentle bank 67.5
            {4, kGentle, 3.0 * kPi8, 0}, {4, kGentle, -3.0 * kPi8, 0},
            {4, -kGentle, 3.0 * kPi8, 0}, {4, -kGentle, -3.0 * kPi8, 0},
            // Gentle bank 90
            {4, kGentle, kPi2, 0}, {4, kGentle, -kPi2, 0},
            {4, -kGentle, kPi2, 0}, {4, -kGentle, -kPi2, 0},
            // Gentle 112.5
            {4, kGentle, 5.0 * kPi8, 0}, {4, kGentle, -5.0 * kPi8, 0},
            {4, -kGentle, 5.0 * kPi8, 0}, {4, -kGentle, -5.0 * kPi8, 0},
            // Gentle bank 135
            {4, kGentle, 3.0 * kPi4, 0}, {4, kGentle, -3.0 * kPi4, 0},
            {4, -kGentle, 3.0 * kPi4, 0}, {4, -kGentle, -3.0 * kPi4, 0},
            // Gentle bank 157.5
            {4, kGentle, 7.0 * kPi8, 0}, {4, kGentle, -7.0 * kPi8, 0},
            {4, -kGentle, 7.0 * kPi8, 0}, {4, -kGentle, -7.0 * kPi8, 0},
            // Gentle-to-steep bank 22.5
            {4, kGentleSteepTransition, kPi8, 0}, {4, kGentleSteepTransition, -kPi8, 0},
            {4, -kGentleSteepTransition, kPi8, 0}, {4, -kGentleSteepTransition, -kPi8, 0},
            // Gentle-to-steep bank 45
            {4, kGentleSteepTransition, 2.0 * kPi8, 0}, {4, kGentleSteepTransition, -2.0 * kPi8, 0},
            {4, -kGentleSteepTransition, 2.0 * kPi8, 0}, {4, -kGentleSteepTransition, -2.0 * kPi8, 0},
            // Gentle-to-steep bank 67.5
            {4, kGentleSteepTransition, 3.0 * kPi8, 0}, {4, kGentleSteepTransition, -3.0 * kPi8, 0},
            {4, -kGentleSteepTransition, 3.0 * kPi8, 0}, {4, -kGentleSteepTransition, -3.0 * kPi8, 0},
            // Gentle-to-steep bank 90
            {4, kGentleSteepTransition, kPi2, 0}, {4, kGentleSteepTransition, -kPi2, 0},
            {4, -kGentleSteepTransition, kPi2, 0}, {4, -kGentleSteepTransition, -kPi2, 0},
        };
        // Steep bank 22.5 — 4 frames unless DiveLoop set, then 8 frames.
        const Rotation kZeroGRollSteepBank22_4[] = {
            {4, kSteep, kPi8, 0}, {4, kSteep, -kPi8, 0},
            {4, -kSteep, kPi8, 0}, {4, -kSteep, -kPi8, 0},
        };
        const Rotation kZeroGRollSteepBank22_8[] = {
            {8, kSteep, kPi8, 0}, {8, kSteep, -kPi8, 0},
            {8, -kSteep, kPi8, 0}, {8, -kSteep, -kPi8, 0},
        };

        const Rotation kDiveLoopRot[] = {
            // Steep bank 45
            {8, kSteepDiagonal, kPi4, kPi8}, {8, kSteepDiagonal, -kPi4, kPi8},
            {8, -kSteepDiagonal, kPi4, kPi8}, {8, -kSteepDiagonal, -kPi4, kPi8},
            // Steep bank 67.5
            {8, kSteepDiagonal, 3.0 * kPi8, kPi8}, {8, kSteepDiagonal, -3.0 * kPi8, kPi8},
            {8, -kSteepDiagonal, 3.0 * kPi8, kPi8}, {8, -kSteepDiagonal, -3.0 * kPi8, kPi8},
            // Diagonal steep bank 90
            {8, kSteepDiagonal, kPi2, kPi8}, {8, kSteepDiagonal, -kPi2, kPi8},
            {8, -kSteepDiagonal, kPi2, kPi8}, {8, -kSteepDiagonal, -kPi2, kPi8},
        };

        constexpr std::array kCorkscrewAngles = {
            2.0 * kPi12, 4.0 * kPi12, kPi2, 8.0 * kPi12, 10.0 * kPi12,
        };

        // Build the corkscrew rotation table once at static init.
        std::vector<Rotation> buildCorkscrewRotations() {
            std::vector<Rotation> v;
            v.reserve(20);
            // Right
            for (const double a: kCorkscrewAngles)
                v.push_back({4, corkscrewRightPitch(a), corkscrewRightRoll(a), corkscrewRightYaw(a)});
            for (const double a: kCorkscrewAngles)
                v.push_back({4, corkscrewRightPitch(-a), corkscrewRightRoll(-a), corkscrewRightYaw(-a)});
            // Left mirrors right
            for (const double a: kCorkscrewAngles)
                v.push_back({4, -corkscrewRightPitch(-a), -corkscrewRightRoll(a), -corkscrewRightYaw(a)});
            for (const double a: kCorkscrewAngles)
                v.push_back({4, -corkscrewRightPitch(a), -corkscrewRightRoll(-a), -corkscrewRightYaw(-a)});
            return v;
        }

        const std::vector<Rotation> &corkscrewRotations() {
            static const std::vector<Rotation> v = buildCorkscrewRotations();
            return v;
        }

        int renderGroup(
            Context *context, const std::span<const Rotation> rots, Image *base) {
            int written = 0;
            for (const auto &[num_frames, pitch, roll, yaw]: rots) {
                renderRotation(context, num_frames, pitch, roll, yaw, base + written);
                written += num_frames;
            }
            return written;
        }

        // Restraint animation frames live outside the static groups: when
        // frame > 0 we emit 4 flat rotations.
        constexpr int kRestraintFrames = 12; // count_sprites contribution
        constexpr int kRestraintPerFrame = 4;
    } // namespace

    int countSprites(const SpriteFlag spriteFlags, const VehicleFlag vehicleFlags) {
        int n = 0;
        if (has_flag(spriteFlags, SpriteFlag::flatSlope)) n += 32;
        if (has_flag(spriteFlags, SpriteFlag::gentleSlope)) n += 72;
        if (has_flag(spriteFlags, SpriteFlag::steepSlope)) n += 80;
        if (has_flag(spriteFlags, SpriteFlag::verticalSlope)) n += 116;
        if (has_flag(spriteFlags, SpriteFlag::diagonalSlope)) n += 24;
        if (has_flag(spriteFlags, SpriteFlag::banking)) n += 80;
        if (has_flag(spriteFlags, SpriteFlag::inlineTwist)) n += 40;
        if (has_flag(spriteFlags, SpriteFlag::slopeBankTransition)) n += 128;
        if (has_flag(spriteFlags, SpriteFlag::diagonalBankTransition)) n += 16;
        if (has_flag(spriteFlags, SpriteFlag::slopedBankTransition)) n += 16;
        if (has_flag(spriteFlags, SpriteFlag::diagonalSlopedBankTransition)) n += 48;
        if (has_flag(spriteFlags, SpriteFlag::slopedBankedTurn)) n += 128;
        if (has_flag(spriteFlags, SpriteFlag::bankedSlopeTransition)) n += 16;
        if (has_flag(spriteFlags, SpriteFlag::corkscrew)) n += 80;
        if (has_flag(spriteFlags, SpriteFlag::zeroGRoll)) n += 160;
        if (has_flag(spriteFlags, SpriteFlag::diveLoop)) n += 112;
        if (has_flag(vehicleFlags, VehicleFlag::restraintAnimation)) n += kRestraintFrames;
        return n;
    }

    int renderVehicleFrame(
        Context *context, const SpriteFlag spriteFlags, const int frame, Image *out) {
        if (frame > 0) {
            printMsg("Rendering restraint animation");
            renderRotation(context, kRestraintPerFrame, 0, 0, 0, out);
            return kRestraintPerFrame;
        }

        int base = 0;

        auto emit_if = [&](const SpriteFlag f, const char *msg, const std::span<const Rotation> rots) {
            if (has_flag(spriteFlags, f)) {
                printMsg(msg);
                base += renderGroup(context, rots, out + base);
            }
        };

        emit_if(SpriteFlag::flatSlope, "Rendering flat sprites", kFlatSlopeRot);
        emit_if(SpriteFlag::gentleSlope, "Rendering gentle sprites", kGentleSlopeRot);
        emit_if(SpriteFlag::steepSlope, "Rendering steep sprites", kSteepSlopeRot);
        emit_if(SpriteFlag::verticalSlope, "Rendering vertical sprites", kVerticalSlopeRot);
        emit_if(SpriteFlag::diagonalSlope, "Rendering diagonal sprites", kDiagonalSlopeRot);
        emit_if(SpriteFlag::banking, "Rendering banked sprites", kBankingRot);
        emit_if(SpriteFlag::inlineTwist, "Rendering inline twist sprites", kInlineTwistRot);
        emit_if(SpriteFlag::slopeBankTransition, "Rendering slope-bank transition sprites", kSlopeBankTransitionRot);
        emit_if(SpriteFlag::diagonalBankTransition, "Rendering diagonal slope-bank transition sprites",
                kDiagonalBankTransitionRot);
        emit_if(SpriteFlag::slopedBankTransition, "Rendering sloped bank transition sprites", kSlopedBankTransitionRot);
        emit_if(SpriteFlag::diagonalSlopedBankTransition,
                "Rendering diagonal sloped bank transition sprites", kDiagonalSlopedBankTransitionRot);
        emit_if(SpriteFlag::slopedBankedTurn, "Rendering sloped banked sprites", kSlopedBankedTurnRot);
        emit_if(SpriteFlag::bankedSlopeTransition, "Rendering banked slope transition sprites",
                kBankedSlopeTransitionRot);

        if (has_flag(spriteFlags, SpriteFlag::zeroGRoll)) {
            printMsg("Rendering zero G roll sprites");
            base += renderGroup(context, kZeroGRollBaseRot, out + base);
            const std::span<const Rotation> sb22 = has_flag(spriteFlags, SpriteFlag::diveLoop)
                                                       ? std::span<const Rotation>(kZeroGRollSteepBank22_8)
                                                       : std::span<const Rotation>(kZeroGRollSteepBank22_4);
            base += renderGroup(context, sb22, out + base);
        }
        if (has_flag(spriteFlags, SpriteFlag::diveLoop)) {
            printMsg("Rendering dive loop sprites");
            base += renderGroup(context, kDiveLoopRot, out + base);
        }
        if (has_flag(spriteFlags, SpriteFlag::corkscrew)) {
            printMsg("Rendering corkscrew sprites");
            const auto &rots = corkscrewRotations();
            base += renderGroup(context, rots, out + base);
        }

        return base;
    }
}