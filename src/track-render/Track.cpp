/// Track.cpp
///
/// Track rendering pipeline. All curve sampling, banking math, sprite
/// emission, mesh transformation, and offset-table lookup is preserved
/// verbatim against the C-era source -- this file is the source of truth
/// for byte-equivalent track-sprite generation against the goldens captured
/// pre-modernization. Bisect established 5eff54f "Modernize iso-render" as
/// the first bad commit; its ripple touched this caller via inline vector
/// operators with different float-promotion semantics. Any "modernization"
/// of the math here (float-only literals like 0.5f * kTileSize instead of
/// 0.5 * kTileSize, std::clamp(v, 0.0f, 1.0f), std::numbers::pi_v<float>,
/// std::sqrt(float)) breaks byte-equivalence by 1 ULP on borderline curve
/// sample points and propagates outward through the rasterizer.

#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif

#include "Track.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <format>
#include <utility>

#include <jansson.h>

#include "Constants.hpp"
#include "Logging.hpp"
#include "Sprites.hpp"

namespace RCTGen {
    namespace {
        Vector3 startOffset;
        Vector3 endOffset;

        Vector3 changeCoordinates(Vector3 a) {
            Vector3 result = {a.z, a.y, a.x};
            return result;
        }

        struct TrackTransformArgs {
            CurveFn trackCurve;
            float scale;
            float offset;
            float zOffset;
            float length;
            TrackFlag flags;
            float trackLength;
        };

        TrackPoint getTrackPoint(CurveFn curve, TrackFlag flags, float zOffset, float length, float u) {
            TrackPoint trackPoint;
            if (u < 0) {
                trackPoint = curve(0);
                trackPoint.position = vector3_add(trackPoint.position, vector3_mult(trackPoint.tangent, u));
            } else if (u > length) {
                trackPoint = curve(length);
                trackPoint.position = vector3_add(trackPoint.position, vector3_mult(trackPoint.tangent, (u - length)));
            } else {
                trackPoint = curve(u);
            }

            if (has_flag(flags, TrackFlag::diagonal)) trackPoint.position.x += 0.5 * kTileSize;
            if (has_flag(flags, TrackFlag::diagonal2)) trackPoint.position.z += 0.5 * kTileSize;
            trackPoint.position.y += zOffset - 2 * kClearanceHeight;
            if (!has_flag(flags, TrackFlag::vertical)) trackPoint.position.z -= 0.5 * kTileSize;

            float v = u / length;
            if (v < 0) v = 0;
            else if (v > 1) v = 1;

            trackPoint.position = vector3_add(trackPoint.position,
                                              vector3_mult(startOffset, 2 * v * v * v - 3 * v * v + 1));
            trackPoint.position = vector3_add(trackPoint.position, vector3_mult(endOffset, -2 * v * v * v + 3 * v * v));
            return trackPoint;
        }

        TrackPoint onlyYaw(TrackPoint input) {
            TrackPoint output;
            output.position = input.position;
            output.normal = vector3(0, 1, 0);
            output.binormal = vector3_normalize(vector3_cross(output.normal, changeCoordinates(input.tangent)));
            output.tangent = vector3_normalize(vector3_cross(output.normal, output.binormal));
            return output;
        }

        Vertex trackTransform(Vector3 vertex, Vector3 normal, const bool flatShaded, void *data) {
            TrackTransformArgs args = *static_cast<TrackTransformArgs *>(data);

            vertex.z = args.scale * vertex.z + args.offset;

            TrackPoint trackPoint = getTrackPoint(args.trackCurve, args.flags, args.zOffset, args.length, vertex.z);

            Vertex out;
            out.vertex = changeCoordinates(vector3_add(trackPoint.position,
                                                       vector3_add(vector3_mult(trackPoint.normal, vertex.y),
                                                                   vector3_mult(trackPoint.binormal, vertex.x))));

            if (flatShaded) {
                TrackPoint central = getTrackPoint(args.trackCurve, args.flags, args.zOffset, args.length,
                                                   args.offset + (args.trackLength / 2));
                out.normal = changeCoordinates(vector3_add(vector3_mult(central.tangent, normal.z),
                                                           vector3_add(vector3_mult(central.normal, normal.y),
                                                                       vector3_mult(central.binormal, normal.x))));
            } else {
                out.normal = changeCoordinates(vector3_add(vector3_mult(trackPoint.tangent, normal.z),
                                                           vector3_add(vector3_mult(trackPoint.normal, normal.y),
                                                                       vector3_mult(trackPoint.binormal, normal.x))));
            }
            return out;
        }

        Vertex baseTransform(Vector3 vertex, Vector3 normal, const bool /*flatShaded*/, void *data) {
            TrackTransformArgs args = *static_cast<TrackTransformArgs *>(data);

            vertex.z = args.scale * vertex.z + args.offset;

            TrackPoint trackPoint = getTrackPoint(args.trackCurve, args.flags, args.zOffset, args.length, vertex.z);

            trackPoint.binormal = vector3_normalize(vector3_cross(vector3(0, 1, 0), trackPoint.tangent));
            trackPoint.normal = vector3_normalize(vector3_cross(trackPoint.tangent, trackPoint.binormal));

            Vertex out;
            out.vertex = changeCoordinates(vector3_add(trackPoint.position,
                                                       vector3_add(vector3_mult(vector3(0, 1, 0), vertex.y),
                                                                   vector3_mult(trackPoint.binormal, vertex.x))));
            out.normal = changeCoordinates(vector3_add(vector3_mult(trackPoint.tangent, normal.z),
                                                       vector3_add(vector3_mult(trackPoint.normal, normal.y),
                                                                   vector3_mult(trackPoint.binormal, normal.x))));
            return out;
        }

        constexpr int kDenom = 6;

        TrackModel getSupportIndex(int bank) {
            switch (bank) {
                case 0: return TrackModel::flat;
                case -1:
                case 1: return TrackModel::bankSixth;
                case -2:
                case 2: return TrackModel::bankThird;
                case -3:
                case 3: return TrackModel::bankHalf;
                case -4:
                case 4: return TrackModel::bankTwoThirds;
                case -5:
                case 5: return TrackModel::bankFiveSixths;
                case -6:
                case 6: return TrackModel::bank;
            }
            return TrackModel::flat;
        }

        TrackModel getSpecialIndex(TrackFlag flags) {
            using TF = TrackFlag;
            const TF masked = flags & TF::specialMask;
            switch (std::to_underlying(masked)) {
                case std::to_underlying(TF::specialSteepToVertical): return TrackModel::specialSteepToVertical;
                case std::to_underlying(TF::specialVerticalToSteep): return TrackModel::specialVerticalToSteep;
                case std::to_underlying(TF::specialVertical): return TrackModel::specialVertical;
                case std::to_underlying(TF::specialVerticalTwistLeft):
                case std::to_underlying(TF::specialVerticalTwistRight): return TrackModel::specialVerticalTwist;
                case std::to_underlying(TF::specialBarrelRollLeft):
                case std::to_underlying(TF::specialBarrelRollRight): return TrackModel::specialBarrelRoll;
                case std::to_underlying(TF::specialHalfLoop): return TrackModel::specialHalfLoop;
                case std::to_underlying(TF::specialQuarterLoop): return TrackModel::specialQuarterLoop;
                case std::to_underlying(TF::specialCorkscrewLeft):
                case std::to_underlying(TF::specialCorkscrewRight): return TrackModel::specialCorkscrew;
                case std::to_underlying(TF::specialZeroGRollLeft):
                case std::to_underlying(TF::specialZeroGRollRight): return TrackModel::specialZeroGRoll;
                case std::to_underlying(TF::specialLargeZeroGRollLeft):
                case std::to_underlying(TF::specialLargeZeroGRollRight): return TrackModel::specialLargeZeroGRoll;
                case std::to_underlying(TF::specialBrake): return TrackModel::specialBrake;
                case std::to_underlying(TF::specialBlockBrake): return TrackModel::specialBlockBrake;
                case std::to_underlying(TF::specialMagneticBrake): return TrackModel::specialMagneticBrake;
                case std::to_underlying(TF::specialBooster):
                case std::to_underlying(TF::specialLaunchedLift):
                case std::to_underlying(TF::specialVerticalBooster): return TrackModel::specialBooster;
            }
            assert(0);
            return TrackModel::flat;
        }

        bool modelLoaded(const TrackType *type, TrackModel m) {
            return (type->modelsLoaded & (1u << std::to_underlying(m))) != 0;
        }

        void renderTrackSection(Context *context, TrackSection *section, TrackType *trackType,
                                ViewFlag viewFlags, int trackMask, int renderedViews, Image *images) {
            int numMeshes = static_cast<int>(std::floor(0.5 + section->length / trackType->length));
            float scale = section->length / (numMeshes * trackType->length);

            // If alternating track meshes are used, prefer an even number of
            // meshes when distortion stays within tolerance.
            if (modelLoaded(trackType, TrackModel::trackAlt)) {
                int numMeshesEven = 2 * static_cast<int>(std::floor(0.5 + section->length / (2 * trackType->length)));
                if (has_flag(section->flags, TrackFlag::altPreferOdd))
                    numMeshesEven = 2 * static_cast<int>(std::floor(section->length / (2 * trackType->length))) + 1;
                float scaleEven = section->length / (numMeshesEven * trackType->length);
                if (scaleEven > 0.9 && scaleEven < 1.11111) {
                    numMeshes = numMeshesEven;
                    scale = scaleEven;
                }
            }

            float length = scale * trackType->length;
            float zOffset = ((trackType->zOffset / 8.0) * kClearanceHeight);

            Mesh *mesh = &trackType->mesh;
            Mesh *meshTie = &trackType->meshTie;

            context_begin_render(context);

            // Ghost models/track masks at start and end.
            TrackTransformArgs args;
            args.scale = scale;
            args.offset = -length;
            args.zOffset = zOffset;
            args.trackCurve = section->curve;
            args.flags = section->flags;
            args.length = section->length;
            args.trackLength = trackType->length;
            if (trackMask)
                context_add_model_transformed(context, &trackType->mask, trackTransform, &args, 0);
            else if (!has_flag(viewFlags, ViewFlag::extrudeBehind))
                context_add_model_transformed(context, mesh, trackTransform, &args, MESH_GHOST);
            args.offset = section->length;
            if (trackMask)
                context_add_model_transformed(context, &trackType->mask, trackTransform, &args, 0);
            else if (!has_flag(viewFlags, ViewFlag::extrudeInFront) && !has_flag(viewFlags, ViewFlag::maskEnd))
                context_add_model_transformed(context, mesh, trackTransform, &args, MESH_GHOST);

            // TODO VIEW_MASK_END not implemented for this case
            if (has_flag(trackType->flags, TrackTypeFlag::tieAtBoundary)) {
                int startAngle = 3;
                if (renderedViews & 1) startAngle = 0;
                else if (renderedViews & 2) startAngle = 1;
                else if (renderedViews & 4) startAngle = 2;
                if (any_of(section->flags, TrackFlag::diagonal | TrackFlag::diagonal2))
                    startAngle = (startAngle + 1) % 4;
                int endAngle = startAngle;
                if (has_flag(section->flags, TrackFlag::exit90DegLeft)) endAngle -= 1;
                else if (has_flag(section->flags, TrackFlag::exit90DegRight)) endAngle += 1;
                else if (has_flag(section->flags, TrackFlag::exit180Deg)) endAngle += 2;
                else if (has_flag(section->flags, TrackFlag::exit45DegLeft) && any_of(
                             section->flags, TrackFlag::diagonal | TrackFlag::diagonal2))
                    endAngle -= 1;
                else if (has_flag(section->flags, TrackFlag::exit45DegRight) && !any_of(
                             section->flags, TrackFlag::diagonal | TrackFlag::diagonal2))
                    endAngle += 1;
                if (endAngle < 0) endAngle += 4;
                if (endAngle > 3) endAngle -= 4;

                int startTie = startAngle <= 1;
                int endTie = endAngle > 1;

                double correctedLength = numMeshes * trackType->length;
                if (!startTie) correctedLength -= trackType->tieLength;
                if (endTie) correctedLength += trackType->tieLength;
                double correctedScale = section->length / correctedLength;

                double tieLength = correctedScale * trackType->tieLength;
                double interLength = correctedScale * (trackType->length - trackType->tieLength);

                double offset = 0;

                if (has_flag(viewFlags, ViewFlag::extrudeBehind)) {
                    numMeshes++;
                    offset -= (has_flag(viewFlags, ViewFlag::extrudeBehind) ? 1 : 0) * correctedScale * trackType->
                            length;
                }
                if (has_flag(viewFlags, ViewFlag::extrudeInFront)) numMeshes++;
                for (int i = 0; i < 2 * numMeshes + 1; i++) {
                    TrackTransformArgs ia;
                    ia.scale = correctedScale;
                    ia.offset = offset;
                    ia.zOffset = zOffset;
                    ia.trackCurve = section->curve;
                    ia.flags = section->flags;
                    ia.length = section->length;
                    ia.trackLength = trackType->length;
                    if (!(i & 1) && (i != 0 || startTie) && (i != 2 * numMeshes || endTie)) {
                        TrackPoint p = getTrackPoint(section->curve, section->flags, zOffset, ia.length,
                                                     ia.offset + trackType->tieLength / 2);
                        context_add_model(
                            context, &trackType->tieMesh,
                            transform(
                                matrix(p.binormal.z, p.normal.z, p.tangent.z,
                                       p.binormal.y, p.normal.y, p.tangent.y,
                                       p.binormal.x, p.normal.x, p.tangent.x),
                                changeCoordinates(p.position)),
                            trackMask);
                        context_add_model_transformed(context, meshTie, trackTransform, &ia, trackMask);
                        offset += tieLength;
                    } else if (i & 1) {
                        int useAlt = i & 2;
                        if (has_flag(section->flags, TrackFlag::altInvert)) useAlt = !useAlt;
                        if (has_flag(viewFlags, ViewFlag::extrudeBehind)) useAlt = !useAlt;
                        if (!modelLoaded(trackType, TrackModel::trackAlt)) useAlt = 0;
                        if (useAlt)
                            context_add_model_transformed(
                                context, &trackType->models[std::to_underlying(TrackModel::trackAlt)], trackTransform,
                                &ia, trackMask);
                        else
                            context_add_model_transformed(context, mesh, trackTransform, &ia, trackMask);
                        if (trackMask) {
                            if (startTie) ia.offset = offset - tieLength;
                            context_add_model_transformed(context, &trackType->mask, trackTransform, &ia, 0);
                        }
                        offset += interLength;
                    }
                }
            } else {
                if (has_flag(viewFlags, ViewFlag::extrudeBehind)) numMeshes++;
                if (has_flag(viewFlags, ViewFlag::extrudeInFront)) numMeshes++;
                if (has_flag(viewFlags, ViewFlag::maskEnd)) numMeshes++;
                for (int i = 0; i < numMeshes; i++) {
                    TrackTransformArgs ia;
                    ia.scale = scale;
                    ia.offset = (i - (has_flag(viewFlags, ViewFlag::extrudeBehind) ? 1 : 0)) * length;
                    ia.zOffset = zOffset;
                    ia.trackCurve = section->curve;
                    ia.flags = section->flags;
                    ia.length = section->length;
                    ia.trackLength = trackType->length;

                    bool altAvailable = modelLoaded(trackType, TrackModel::trackAlt);
                    int useAlt = altAvailable && (i & 1);
                    if (altAvailable && has_flag(section->flags, TrackFlag::altInvert)) useAlt = !useAlt;

                    if (trackMask)
                        context_add_model_transformed(context, &trackType->mask, trackTransform, &ia, 0);
                    int endingMask = trackMask || (has_flag(viewFlags, ViewFlag::maskEnd) && i + 1 == numMeshes);
                    if (useAlt)
                        context_add_model_transformed(
                            context, &trackType->models[std::to_underlying(TrackModel::trackAlt)], trackTransform, &ia,
                            endingMask);
                    else
                        context_add_model_transformed(context, mesh, trackTransform, &ia, endingMask);
                    if (modelLoaded(trackType, TrackModel::base)
                        && has_flag(trackType->flags, TrackTypeFlag::hasSupports)
                        && !has_flag(section->flags, TrackFlag::noSupports))
                        context_add_model_transformed(context, &trackType->models[std::to_underlying(TrackModel::base)],
                                                      baseTransform, &ia, trackMask);
                    if (has_flag(trackType->flags, TrackTypeFlag::separateTie)) {
                        TrackPoint p = getTrackPoint(section->curve, section->flags, zOffset, ia.length,
                                                     ia.offset + 0.5 * length);
                        context_add_model(context, &trackType->tieMesh,
                                          transform(matrix(p.binormal.z, p.normal.z, p.tangent.z,
                                                           p.binormal.y, p.normal.y, p.tangent.y,
                                                           p.binormal.x, p.normal.x, p.tangent.x),
                                                    changeCoordinates(p.position)),
                                          endingMask);
                    }
                }
            }

            if (any_of(section->flags, TrackFlag::specialMask)) {
                TrackModel idx = getSpecialIndex(section->flags);
                if (modelLoaded(trackType, idx)) {
                    Matrix3 mat = views[1];
                    TrackFlag s = section->flags & TrackFlag::specialMask;
                    if (s != TrackFlag::specialVerticalTwistRight
                        && s != TrackFlag::specialBarrelRollRight
                        && s != TrackFlag::specialCorkscrewRight
                        && s != TrackFlag::specialZeroGRollRight
                        && s != TrackFlag::specialLargeZeroGRollRight) {
                        mat.entries[6] *= -1;
                        mat.entries[7] *= -1;
                        mat.entries[8] *= -1;
                    }

                    if (s == TrackFlag::specialBrake || s == TrackFlag::specialMagneticBrake
                        || s == TrackFlag::specialBlockBrake || s == TrackFlag::specialBooster) {
                        float specialLength = trackType->brakeLength;
                        if (s == TrackFlag::specialBlockBrake) specialLength = kTileSize;
                        int numSpecial = static_cast<int>(std::floor(0.5 + section->length / specialLength));
                        float specialScale = section->length / (numSpecial * specialLength);
                        specialLength = specialScale * specialLength;
                        for (int i = 0; i < numSpecial; i++) {
                            TrackTransformArgs ia;
                            ia.scale = specialScale;
                            ia.offset = i * specialLength;
                            ia.zOffset = zOffset;
                            ia.trackCurve = section->curve;
                            ia.flags = section->flags;
                            ia.length = section->length;
                            ia.trackLength = trackType->length;
                            context_add_model_transformed(context, &trackType->models[std::to_underlying(idx)],
                                                          trackTransform, &ia, trackMask);
                        }
                    } else {
                        context_add_model(context, &trackType->models[std::to_underlying(idx)],
                                          transform(mat, vector3(
                                                        !has_flag(section->flags, TrackFlag::vertical)
                                                            ? -0.5 * kTileSize
                                                            : 0, zOffset - 2 * kClearanceHeight, 0)),
                                          trackMask);
                    }
                }
            }

            if (has_flag(trackType->flags, TrackTypeFlag::hasSupports)
                && !has_flag(section->flags, TrackFlag::noSupports)) {
                int numSupports = static_cast<int>(std::floor(0.5 + section->length / trackType->supportSpacing));
                float supportStep = section->length / numSupports;
                int entry = 0;
                int exit = 0;
                if (has_flag(section->flags, TrackFlag::entryBankLeft)) entry = kDenom;
                else if (has_flag(section->flags, TrackFlag::entryBankRight)) entry = -kDenom;

                if (has_flag(section->flags, TrackFlag::exitBankLeft)) exit = kDenom;
                else if (has_flag(section->flags, TrackFlag::exitBankRight)) exit = -kDenom;

                for (int i = 0; i < numSupports + 1; i++) {
                    int u = (i * kDenom) / numSupports;
                    int bankAngle = (entry * (kDenom - u) + (exit * u)) / kDenom;

                    TrackPoint trackPoint = getTrackPoint(section->curve, section->flags, zOffset, section->length,
                                                          i * supportStep);

                    TrackPoint supportPoint = onlyYaw(trackPoint);

                    Matrix3 rotation = matrix(supportPoint.binormal.x, supportPoint.normal.x, supportPoint.tangent.x,
                                              supportPoint.binormal.y, supportPoint.normal.y, supportPoint.tangent.y,
                                              supportPoint.binormal.z, supportPoint.normal.z, supportPoint.tangent.z);
                    if (bankAngle >= 0) rotation = matrix_mult(views[2], rotation);

                    Vector3 translation = changeCoordinates(supportPoint.position);
                    translation.y -= trackType->pivot / sqrt(
                                trackPoint.tangent.x * trackPoint.tangent.x + trackPoint.tangent.z * trackPoint.tangent.
                                z) -
                            trackType->pivot;

                    context_add_model(context, &trackType->models[std::to_underlying(getSupportIndex(bankAngle))],
                                      transform(rotation, translation), trackMask);
                }
            }

            context_finalize_render(context);

            for (int i = 0; i < 4; i++) {
                if (renderedViews & (1 << i)) {
                    if (trackMask) context_render_silhouette(context, rotate_y(0.5 * i * M_PI), images + i);
                    else context_render_view(context, rotate_y(0.5 * i * M_PI), images + i);
                }
            }
            context_end_render(context);
        }

        bool isInMask(int x, int y, const Mask *mask) {
            for (std::uint32_t i = 0; i < mask->numRects; i++) {
                if (x >= mask->rects[i].x_lower && x < mask->rects[i].x_upper
                    && y >= mask->rects[i].y_lower && y < mask->rects[i].y_upper)
                    return true;
            }
            return false;
        }

        bool compareVec(Vector3 a, Vector3 b, int rot) {
            return vector3_norm(vector3_sub(a, vector3_normalize(matrix_vector(views[rot], b)))) < 0.15;
        }

        int offsetTableIndexWithRot(TrackPoint track, int rot) {
            int banked = std::fabs(std::fabs(std::asin(
                                       std::sqrt(track.normal.x * track.normal.x + track.normal.z * track.normal.z))) -
                                   0.25 * M_PI) < 0.1;
            int right = (banked && track.binormal.y < 0) ? 0x10 : 0;
            if (compareVec(track.tangent, vector3(0, 0, kTileSize), rot)) {
                if (track.normal.y < -0.9) return std::to_underlying(Offset::inverted);
                else if (banked) return right | std::to_underlying(Offset::bank);
                else return std::to_underlying(Offset::flat);
            } else if (compareVec(track.tangent, vector3(0, 2 * kClearanceHeight, kTileSize), rot)) {
                if (banked) return right | std::to_underlying(Offset::gentleBank);
                else return std::to_underlying(Offset::gentle);
            } else if (compareVec(track.tangent, vector3(0, 8 * kClearanceHeight, kTileSize), rot))
                return std::to_underlying(Offset::steep);
            else if (compareVec(track.tangent, vector3(-kTileSize, 0, kTileSize), rot)) {
                if (banked) return right | std::to_underlying(Offset::diagonalBank);
                return std::to_underlying(Offset::diagonal);
            } else if (compareVec(track.tangent, vector3(-kTileSize, 2 * kClearanceHeight, kTileSize), rot) && !
                       banked) {
                return std::to_underlying(Offset::diagonalGentle);
            } else if (compareVec(track.tangent, vector3(-kTileSize, 8 * kClearanceHeight, kTileSize), rot))
                return std::to_underlying(Offset::diagonalSteep);
            return 0xFF;
        }

        int offsetTableIndex(TrackPoint track) {
            int index = offsetTableIndexWithRot(track, 0);
            if (index != 0xFF) return index;
            index = offsetTableIndexWithRot(track, 1);
            if (index != 0xFF) return 0x60 | index;
            index = offsetTableIndexWithRot(track, 3);
            if (index != 0xFF) return 0x20 | index;
            return 0xFF;
        }

        Vector3 getOffset(int table, int viewAngle, std::span<const float, 88> offsetTable) {
            int index = table & 0xF;
            int endAngle = table >> 5;
            int right = (table & 0x10) >> 4;
            int rotatedViewAngle = (viewAngle + endAngle + 2 * right) % 4;

            Vector3 offset = vector3(0, 0, 0);
            if (table == 0xFF) return offset;

            offset.x = 0;
            offset.z = offsetTable[8 * index + 2 * rotatedViewAngle] * kTileSize / 32.0;
            offset.y = offsetTable[8 * index + 2 * rotatedViewAngle + 1] * kClearanceHeight / 8.0;

            if (right) offset.z *= -1;

            if (index >= 6 && index <= 8) {
                offset.z *= M_SQRT1_2;
                offset.x = offset.z;
            }

            if (endAngle != 0) offset = matrix_vector(rotate_y(-0.5 * M_PI * endAngle), offset);
            return offset;
        }

        void setOffset(int viewAngle, TrackSection *section, std::span<const float, 88> offsetTable) {
            int startTable = offsetTableIndex(section->curve(0));
            int endTable = offsetTableIndex(section->curve(section->length));
            startOffset = getOffset(startTable, viewAngle, offsetTable);
            endOffset = getOffset(endTable, viewAngle, offsetTable);
        }

        int mod(int k, int n) {
            return ((k % n) + n) % n;
        }

        void applyLift(Image *image, Image *pattern) {
            for (int x = 0; x < image->width; x++) {
                for (int y = 0; y < image->height; y++) {
                    std::uint8_t pixel = image->pixels[y * image->width + x];
                    if (pixel >= 1 && pixel <= 3) {
                        int xi = mod(x + image->x_offset - pattern->x_offset, pattern->width);
                        int yi = mod(y + image->y_offset - pattern->y_offset, pattern->height);
                        image->pixels[y * image->width + x] = pattern->pixels[xi + yi * pattern->width];
                    }
                }
            }
        }

        void writeTrackSection(Context *context, TrackSectionId id, TrackType *trackType,
                               std::span<const float, 88> offsetTable,
                               const std::filesystem::path &baseDir,
                               const std::filesystem::path &outputDir,
                               json_t *sprites) {
            const int sectionIdx = std::to_underlying(id);
            TrackSection *section = &kTrackSections[sectionIdx];
            View *views_ = trackType->masks_[sectionIdx].data();

            int zOffsetInt = static_cast<int>(trackType->zOffset + 0.499999);
            Image fullSprites[4];
            Image trackMasks[4];
            for (int i = 0; i < 4; i++) {
                if (views_[i].numSprites == 0) continue;
                if (has_flag(trackType->flags, TrackTypeFlag::specialOffsets)) {
                    setOffset(i, section, offsetTable);
                }
                if (has_flag(views_[i].flags, ViewFlag::needsTrackMask))
                    renderTrackSection(context, section, trackType, views_[i].flags, 1, 1 << i, trackMasks);
                renderTrackSection(context, section, trackType, views_[i].flags, 0, 1 << i, fullSprites);
            }

            for (int i = 0; i < 4; i++) {
                int angle = i;

                if (has_flag(trackType->flags, TrackTypeFlag::hasLift)) {
                    if (views_[i].numSprites == 0) angle = i % 2;
                }

                if (views_[angle].numSprites == 0) continue;

                if (has_flag(trackType->flags, TrackTypeFlag::hasLift) && section->chainPattern != nullptr) {
                    applyLift(fullSprites + angle, section->chainPattern + i);
                }

                View *view = views_ + angle;

                for (int sprite = 0; sprite < view->numSprites; sprite++) {
                    std::string relative;
                    if (view->numSprites == 1)
                        relative = std::format("{}{}{}_{}.png", outputDir.string(), section->name, trackType->suffix,
                                               i + 1);
                    else
                        relative = std::format("{}{}{}_{}_{}.png", outputDir.string(), section->name, trackType->suffix,
                                               i + 1, sprite + 1);
                    std::string final = std::format("{}{}", baseDir.string(), relative);
                    printMsg("{}", final);

                    Image partSprite;
                    image_copy(fullSprites + angle, &partSprite);
                    if (view->masks != nullptr) {
                        for (int x = 0; x < fullSprites[angle].width; x++) {
                            for (int y = 0; y < fullSprites[angle].height; y++) {
                                int yOffsetExtra = has_flag(section->flags, TrackFlag::offsetSpriteMask)
                                                       ? (zOffsetInt - 8)
                                                       : 0;
                                bool inMask = isInMask(x + fullSprites[angle].x_offset,
                                                       y + fullSprites[angle].y_offset + yOffsetExtra,
                                                       view->masks + sprite);

                                if (view->masks[sprite].op != MaskOp::none) {
                                    int mx = (x + fullSprites[angle].x_offset) - trackMasks[angle].x_offset;
                                    int my = (y + fullSprites[angle].y_offset) - trackMasks[angle].y_offset;
                                    bool inTrackMask =
                                            mx >= 0 && my >= 0 && mx < trackMasks[angle].width && my < trackMasks[angle]
                                            .height
                                            && trackMasks[angle].pixels[mx + my * trackMasks[angle].width] != 0;

                                    switch (view->masks[sprite].op) {
                                        case MaskOp::difference:
                                            inMask = inMask && !inTrackMask;
                                            break;
                                        case MaskOp::intersect:
                                            inMask = inMask && inTrackMask;
                                            break;
                                        case MaskOp::transferNext:
                                            // Preserving the original's bitwise & of bool operands.
                                            if (((sprite < view->numSprites - 1) & inTrackMask)
                                                && isInMask(x + fullSprites[angle].x_offset,
                                                            y + fullSprites[angle].y_offset + yOffsetExtra,
                                                            view->masks + sprite + 1))
                                                inMask = true;
                                            break;
                                        case MaskOp::none:
                                            break;
                                    }
                                }

                                if (inMask)
                                    partSprite.pixels[x + partSprite.width * y] = fullSprites[angle].pixels[
                                        x + fullSprites[angle].width * y];
                                else
                                    partSprite.pixels[x + partSprite.width * y] = 0;
                            }
                        }
                        partSprite.x_offset += view->masks[sprite].xOffset;
                        partSprite.y_offset += view->masks[sprite].yOffset;
                    }

                    std::FILE *file = std::fopen(final.c_str(), "wb");
                    if (file == nullptr) {
                        printMsg("Error: could not open {} for writing", final);
                        std::exit(1);
                    }
                    image_crop(&partSprite);
                    image_write_png(&partSprite, nullptr, file);
                    std::fclose(file);

                    json_t *entry = json_object();
                    json_object_set(entry, "path", json_string(relative.c_str()));
                    json_object_set(entry, "x", json_integer(partSprite.x_offset));
                    json_object_set(entry, "y", json_integer(partSprite.y_offset));
                    json_object_set(entry, "palette", json_string("keep"));
                    json_array_append(sprites, entry);
                    image_destroy(&partSprite);
                }
            }
        }

        // For each enabled group, emit the matching list of track sections.
        // The order here matches the original code one-for-one and is part of
        // the byte-equivalent contract: regression goldens are keyed on the
        // PNG filenames, so adding/removing/reordering sections changes the
        // output even though byte contents would not.
        void writeGroup(Context *context, TrackType *trackType,
                        std::span<const float, 88> offsetTable,
                        const std::filesystem::path &baseDir,
                        const std::filesystem::path &outputDir,
                        json_t *sprites,
                        std::initializer_list<TrackSectionId> ids) {
            for (auto id: ids)
                writeTrackSection(context, id, trackType, offsetTable, baseDir, outputDir, sprites);
        }
    } // namespace

    int writeTrackType(Context *context, TrackType *trackType, json_t *sprites,
                       std::span<const float, 88> offsetTable,
                       const std::filesystem::path &baseDir,
                       const std::filesystem::path &outputDir) {
        using TG = TrackGroup;
        using ID = TrackSectionId;
        const TG groups = trackType->groups;

        auto emit = [&](std::initializer_list<ID> ids) {
            writeGroup(context, trackType, offsetTable, baseDir, outputDir, sprites, ids);
        };

        if (has_flag(groups, TG::flat)) emit({ID::flat});
        if (has_flag(groups, TG::brakes)) emit({ID::brake});
        if (has_flag(groups, TG::blockBrakes)) emit({ID::block_brake});
        if (has_flag(groups, TG::slopedBrakes)) emit({ID::brake_gentle});
        if (has_flag(groups, TG::magneticBrakes)) emit({ID::magnetic_brake});

        if (has_flag(groups, TG::boosters)) emit({ID::booster});
        if (has_flag(groups, TG::launchedLifts)) emit({ID::launched_lift});
        if (has_flag(groups, TG::verticalBoosters)) emit({ID::vertical_booster});

        if (has_flag(groups, TG::gentleSlopes)) emit({ID::flat_to_gentle, ID::gentle_to_flat, ID::gentle});
        if (has_flag(groups, TG::magneticBrakes)) emit({ID::magnetic_brake_gentle});

        if (has_flag(groups, TG::steepSlopes)) emit({ID::gentle_to_steep, ID::steep_to_gentle, ID::steep});
        if (has_flag(groups, TG::verticalSlopes)) emit({ID::steep_to_vertical, ID::vertical_to_steep, ID::vertical});

        if (has_flag(groups, TG::turns))
            emit({
                ID::small_turn_left, ID::medium_turn_left,
                ID::large_turn_left_to_diag, ID::large_turn_right_to_diag
            });

        if (has_flag(groups, TG::diagonals)) emit({ID::flat_diag});
        if (has_flag(groups, TG::diagonalBrakes)) {
            if (has_flag(groups, TG::brakes)) emit({ID::brake_diag});
            if (has_flag(groups, TG::blockBrakes)) emit({ID::block_brake_diag});
            if (has_flag(groups, TG::magneticBrakes)) emit({ID::magnetic_brake_diag});
        }
        if (has_flag(groups, TG::diagonals) && has_flag(groups, TG::gentleSlopes))
            emit({ID::flat_to_gentle_diag, ID::gentle_to_flat_diag, ID::gentle_diag});
        if (has_flag(groups, TG::diagonalBrakes)) {
            if (has_flag(groups, TG::slopedBrakes)) emit({ID::brake_gentle_diag});
            if (has_flag(groups, TG::magneticBrakes)) emit({ID::magnetic_brake_gentle_diag});
        }
        if (has_flag(groups, TG::diagonals) && has_flag(groups, TG::steepSlopes))
            emit({ID::gentle_to_steep_diag, ID::steep_to_gentle_diag, ID::steep_diag});

        if (has_flag(groups, TG::bankedTurns)) {
            emit({
                ID::flat_to_left_bank, ID::flat_to_right_bank,
                ID::left_bank_to_gentle, ID::right_bank_to_gentle,
                ID::gentle_to_left_bank, ID::gentle_to_right_bank,
                ID::left_bank
            });
            if (has_flag(groups, TG::diagonals))
                emit({
                    ID::flat_to_left_bank_diag, ID::flat_to_right_bank_diag,
                    ID::left_bank_to_gentle_diag, ID::right_bank_to_gentle_diag,
                    ID::gentle_to_left_bank_diag, ID::gentle_to_right_bank_diag,
                    ID::left_bank_diag
                });
            emit({
                ID::small_turn_left_bank, ID::medium_turn_left_bank,
                ID::large_turn_left_to_diag_bank, ID::large_turn_right_to_diag_bank
            });
        }

        if (has_flag(groups, TG::slopedTurns) && has_flag(groups, TG::gentleSlopes))
            emit({
                ID::small_turn_left_gentle, ID::small_turn_right_gentle,
                ID::medium_turn_left_gentle, ID::medium_turn_right_gentle
            });
        if (has_flag(groups, TG::steepSlopedTurns) && has_flag(groups, TG::steepSlopes))
            emit({ID::very_small_turn_left_steep, ID::very_small_turn_right_steep});
        if (has_flag(groups, TG::slopedTurns) && has_flag(groups, TG::verticalSlopes))
            emit({ID::vertical_twist_left, ID::vertical_twist_right});

        if (has_flag(groups, TG::bankedSlopedTurns))
            emit({
                ID::gentle_to_gentle_left_bank, ID::gentle_to_gentle_right_bank,
                ID::gentle_left_bank_to_gentle, ID::gentle_right_bank_to_gentle,
                ID::left_bank_to_gentle_left_bank, ID::right_bank_to_gentle_right_bank,
                ID::gentle_left_bank_to_left_bank, ID::gentle_right_bank_to_right_bank,
                ID::gentle_left_bank, ID::gentle_right_bank,
                ID::flat_to_gentle_left_bank, ID::flat_to_gentle_right_bank,
                ID::gentle_left_bank_to_flat, ID::gentle_right_bank_to_flat,
                ID::small_turn_left_bank_gentle, ID::small_turn_right_bank_gentle,
                ID::medium_turn_left_bank_gentle, ID::medium_turn_right_bank_gentle
            });

        if (has_flag(groups, TG::sBends)) emit({ID::s_bend_left, ID::s_bend_right});
        if (has_flag(groups, TG::bankedSBends)) emit({ID::s_bend_left_bank, ID::s_bend_right_bank});

        if (has_flag(groups, TG::helices))
            emit({
                ID::small_helix_left, ID::small_helix_right,
                ID::medium_helix_left, ID::medium_helix_right
            });

        if (has_flag(groups, TG::steepBankTransitions))
            emit({
                ID::gentle_left_bank_to_steep, ID::gentle_right_bank_to_steep,
                ID::steep_to_gentle_left_bank, ID::steep_to_gentle_right_bank,
                ID::gentle_left_bank_to_steep_diag, ID::gentle_right_bank_to_steep_diag,
                ID::steep_to_gentle_left_bank_diag, ID::steep_to_gentle_right_bank_diag
            });

        if (has_flag(groups, TG::largeSteepSlopedTurns)) {
            printMsg("Here");
            emit({
                ID::small_turn_left_steep, ID::small_turn_right_steep,
                ID::large_turn_left_to_diag_steep, ID::large_turn_right_to_diag_steep,
                ID::large_turn_left_to_orthogonal_steep, ID::large_turn_right_to_orthogonal_steep
            });
        }

        if (has_flag(groups, TG::inlineTwists)) emit({ID::inline_twist_left, ID::inline_twist_right});
        if (has_flag(groups, TG::bankedInlineTwists)) emit({ID::inline_twist_left_bank, ID::inline_twist_right_bank});
        if (has_flag(groups, TG::barrelRolls)) emit({ID::barrel_roll_left, ID::barrel_roll_right});
        if (has_flag(groups, TG::bankedBarrelRolls)) emit({ID::barrel_roll_left_bank, ID::barrel_roll_right_bank});
        if (has_flag(groups, TG::halfLoops)) emit({ID::half_loop});
        if (has_flag(groups, TG::verticalLoops)) emit({ID::vertical_loop_left, ID::vertical_loop_right});
        if (has_flag(groups, TG::largeSlopeTransitions))
            emit({ID::flat_to_steep, ID::steep_to_flat, ID::flat_to_steep_diag, ID::steep_to_flat_diag});
        if (has_flag(groups, TG::quarterLoops)) emit({ID::quarter_loop});
        if (has_flag(groups, TG::corkscrews)) emit({ID::corkscrew_left, ID::corkscrew_right});
        if (has_flag(groups, TG::largeCorkscrews)) emit({ID::large_corkscrew_left, ID::large_corkscrew_right});
        if (has_flag(groups, TG::turnBankTransitions))
            emit({ID::small_turn_left_bank_to_gentle, ID::small_turn_right_bank_to_gentle});

        if (has_flag(groups, TG::mediumHalfLoops))
            emit({ID::medium_half_loop_left, ID::medium_half_loop_right});
        if (has_flag(groups, TG::largeHalfLoops))
            emit({ID::large_half_loop_left, ID::large_half_loop_right});
        if (has_flag(groups, TG::zeroGRolls))
            emit({
                ID::zero_g_roll_left, ID::zero_g_roll_right,
                ID::large_zero_g_roll_left, ID::large_zero_g_roll_right
            });
        if (has_flag(groups, TG::bankedZeroGRolls))
            emit({ID::zero_g_roll_left_bank, ID::zero_g_roll_right_bank});
        if (has_flag(groups, TG::diveLoops))
            emit({ID::dive_loop_45_left, ID::dive_loop_45_right});

        if (has_flag(groups, TG::smallSlopeTransitions))
            emit({
                ID::small_flat_to_steep, ID::small_steep_to_flat,
                ID::small_flat_to_steep_diag, ID::small_steep_to_flat_diag
            });

        if (has_flag(groups, TG::largeSlopedTurns))
            emit({
                ID::large_turn_left_to_diag_gentle, ID::large_turn_right_to_diag_gentle,
                ID::large_turn_left_to_orthogonal_gentle, ID::large_turn_right_to_orthogonal_gentle
            });

        if (has_flag(groups, TG::largeBankedSlopedTurns))
            emit({
                ID::gentle_to_gentle_left_bank_diag, ID::gentle_to_gentle_right_bank_diag,
                ID::gentle_left_bank_to_gentle_diag, ID::gentle_right_bank_to_gentle_diag,
                ID::left_bank_to_gentle_left_bank_diag, ID::right_bank_to_gentle_right_bank_diag,
                ID::gentle_left_bank_to_left_bank_diag, ID::gentle_right_bank_to_right_bank_diag,
                ID::gentle_left_bank_diag, ID::gentle_right_bank_diag,
                ID::flat_to_gentle_left_bank_diag, ID::flat_to_gentle_right_bank_diag,
                ID::gentle_left_bank_to_flat_diag, ID::gentle_right_bank_to_flat_diag,
                ID::large_turn_left_bank_to_diag_gentle, ID::large_turn_right_bank_to_diag_gentle,
                ID::large_turn_left_bank_to_orthogonal_gentle, ID::large_turn_right_bank_to_orthogonal_gentle
            });

        return 0;
    }
}