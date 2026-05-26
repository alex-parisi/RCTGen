/// Mask.cpp

#include "Mask.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <format>
#include <utility>

#include <jansson.h>

#include "image.h"
#include "Json.hpp"
#include "Logging.hpp"

namespace RCTGen {
    namespace {
        constexpr std::size_t kMaxRects = 500000;
        constexpr std::size_t kMaxMasks = 2040;

        struct MaskList {
            rect_t *rects;
            Mask *masks;
            ViewSet *views;
        };

        char maskIndex(const image_t *mask, int x, int y, int s) {
            if (s == 0)
                return mask->pixels[x + y * mask->width] & 0x7;
            return (mask->pixels[x + y * mask->width] & 0x38) >> 3;
        }

        void maskListAddRects(MaskList *list, int *total, const rect_t *rects, int count) {
            assert(static_cast<std::size_t>(*total) < kMaxRects);
            std::memcpy(list->rects + *total, rects, count * sizeof(rect_t));
            *total += count;
        }

        void maskListAddMask(MaskList *list, int *total, const Mask *mask) {
            assert(static_cast<std::size_t>(*total) < kMaxMasks);
            list->masks[*total] = *mask;
            (*total)++;
        }

        // Push a rect into the list, applying mirror transform and merging with
        // an existing rect that shares the same X-band and stacks vertically.
        void addRect(image_t *mask, int mirror, rect_t rect, rect_t *rects, int *numRects) {
            if (mirror) {
                int temp = rect.x_upper;
                if (rect.x_lower == 0) rect.x_upper = INT32_MAX;
                else rect.x_upper = -(rect.x_lower + mask->x_offset);
                if (temp == mask->width) rect.x_lower = INT32_MIN;
                else rect.x_lower = -(temp + mask->x_offset);
            } else {
                if (rect.x_lower == 0) rect.x_lower = INT32_MIN;
                else rect.x_lower += mask->x_offset;
                if (rect.x_upper == mask->width) rect.x_upper = INT32_MAX;
                else rect.x_upper += mask->x_offset;
            }
            if (rect.y_lower == 0) rect.y_lower = INT32_MIN;
            else rect.y_lower += mask->y_offset;
            if (rect.y_upper == mask->height) rect.y_upper = INT32_MAX;
            else rect.y_upper += mask->y_offset;

            for (int i = 0; i < *numRects; i++) {
                if (rects[i].x_lower == rect.x_lower
                    && rects[i].x_upper == rect.x_upper
                    && rects[i].y_upper == rect.y_lower) {
                    rects[i].y_upper = rect.y_upper;
                    return;
                }
            }
            rects[*numRects] = rect;
            (*numRects)++;
        }

        void processSlice(image_t *mask, int mirror, int y, char sprite,
                          rect_t *rects, int *numRects, int s) {
            int startX = -1;
            for (int x = 0; x < mask->width; x++) {
                if (maskIndex(mask, x, y, s) == sprite && startX == -1) startX = x;

                if (maskIndex(mask, x, y, s) != sprite && startX >= 0) {
                    rect_t rect = {startX, y, x, y + 1};
                    addRect(mask, mirror, rect, rects, numRects);
                    startX = -1;
                }

                if (x == mask->width - 1 && startX >= 0) {
                    rect_t rect = {startX, y, x + 1, y + 1};
                    addRect(mask, mirror, rect, rects, numRects);
                }
            }
        }

        struct ProcessOpts {
            int mirror;
            int splitEnds;
            std::array<int, 8> split;
            std::array<int, 8> transfer;
            std::array<int, 8> xOffset;
            std::array<int, 8> yOffset;
            int numSprites;
        };

        [[nodiscard]] JsonResult<void> processMask(
            image_t *mask, const ProcessOpts &opts,
            View *view, MaskList *list, int *numMasksTotal, int *numRectsTotal) {
            int numSprites = opts.numSprites;
            rect_t *rects = static_cast<rect_t *>(
                std::malloc(sizeof(rect_t) * (mask->width * mask->height + 1) / 2));
            rect_t *secondaryRects = static_cast<rect_t *>(
                std::malloc(sizeof(rect_t) * (mask->width * mask->height + 1) / 2));

            // Find origin point and sprite count.
            int offsetFound = 0;
            int nontrivial = 0;
            for (int x = 0; x < mask->width; x++) {
                for (int y = 0; y < mask->height; y++) {
                    if (mask->pixels[x + y * mask->width] & 0x40) {
                        if (offsetFound) {
                            std::free(rects);
                            std::free(secondaryRects);
                            return std::unexpected(std::string(
                                "Multiple origin points found in mask"));
                        }
                        mask->x_offset = -x;
                        mask->y_offset = -y;
                        offsetFound = 1;
                    }
                    if (maskIndex(mask, x, y, 0) > numSprites) numSprites = maskIndex(mask, x, y, 0);
                    if (maskIndex(mask, x, y, 1) > numSprites) numSprites = maskIndex(mask, x, y, 1);
                    if (maskIndex(mask, x, y, 0) != 1 || maskIndex(mask, x, y, 1) != 1) nontrivial = 1;
                }
            }
            if (!offsetFound) {
                std::free(rects);
                std::free(secondaryRects);
                return std::unexpected(std::string("No origin point found in mask"));
            }

            if (!nontrivial && !opts.split[0] && numSprites == 1) {
                view->masks = nullptr;
                view->numSprites = numSprites;
                view->flags = ViewFlag::none;
                std::free(rects);
                std::free(secondaryRects);
                return {};
            }

            int offset = *numRectsTotal;
            int maskStart = *numMasksTotal;
            for (int sprite = 0; sprite < numSprites; sprite++) {
                // If split_ends set then last mask uses same rects as first.
                if (opts.splitEnds && sprite == numSprites - 1) {
                    if (opts.split[numSprites - 1]) {
                        std::free(rects);
                        std::free(secondaryRects);
                        return std::unexpected(std::string(
                            "Cannot use split and split_ends simultaneously"));
                    }
                    Mask m;
                    m.xOffset = opts.xOffset[sprite];
                    m.yOffset = opts.yOffset[sprite];
                    m.rects = list->masks[maskStart].rects;
                    m.numRects = list->masks[maskStart].numRects;
                    m.op = MaskOp::difference;
                    maskListAddMask(list, numMasksTotal, &m);
                    break;
                }

                int numRects = 0;
                int secondaryNumRects = 0;
                for (int y = 0; y < mask->height; y++) {
                    processSlice(mask, opts.mirror, y, sprite + 1, rects, &numRects, 0);
                    processSlice(mask, opts.mirror, y, sprite + 1, secondaryRects, &secondaryNumRects, 1);
                }

                int secondaryIdentical = 1;
                for (int i = 0; i < numRects; i++) {
                    if (rects[i].x_lower != secondaryRects[i].x_lower
                        || rects[i].x_upper != secondaryRects[i].x_upper
                        || rects[i].y_lower != secondaryRects[i].y_lower
                        || rects[i].y_upper != secondaryRects[i].y_upper) {
                        secondaryIdentical = 0;
                        break;
                    }
                }
                maskListAddRects(list, numRectsTotal, rects, numRects);

                Mask m;
                m.xOffset = opts.xOffset[sprite];
                m.yOffset = opts.yOffset[sprite];
                m.rects = list->rects + offset;
                m.numRects = numRects;

                if (!opts.transfer[sprite]) {
                    if (opts.splitEnds && sprite == 0) {
                        if (opts.split[0]) {
                            std::free(rects);
                            std::free(secondaryRects);
                            return std::unexpected(std::string(
                                "Cannot use split and split_ends simultaneously"));
                        }
                        m.op = MaskOp::intersect;
                        maskListAddMask(list, numMasksTotal, &m);
                    } else if (sprite > 0 && opts.transfer[sprite - 1]) {
                        if (opts.split[sprite]) {
                            std::free(rects);
                            std::free(secondaryRects);
                            return std::unexpected(std::string(
                                "Cannot use transfer and split simultaneously"));
                        }
                        m.op = MaskOp::difference;
                        maskListAddMask(list, numMasksTotal, &m);
                    } else if (opts.split[sprite]) {
                        m.op = MaskOp::intersect;
                        maskListAddMask(list, numMasksTotal, &m);

                        m.op = MaskOp::difference;
                        if (!secondaryIdentical) {
                            maskListAddRects(list, numRectsTotal, secondaryRects, secondaryNumRects);
                            m.numRects = secondaryNumRects;
                            m.rects = list->rects + offset + numRects;
                            offset += secondaryNumRects;
                        }
                        maskListAddMask(list, numMasksTotal, &m);
                    } else {
                        m.op = MaskOp::none;
                        maskListAddMask(list, numMasksTotal, &m);
                    }
                } else {
                    if (opts.split[sprite]) {
                        std::free(rects);
                        std::free(secondaryRects);
                        return std::unexpected(std::string(
                            "Cannot use transfer and split simultaneously"));
                    }
                    if (sprite + 1 < numSprites && opts.transfer[sprite + 1]) {
                        std::free(rects);
                        std::free(secondaryRects);
                        return std::unexpected(std::string(
                            "Cannot use transfer on consecutive sprites"));
                    }
                    m.op = MaskOp::transferNext;
                    maskListAddMask(list, numMasksTotal, &m);
                }
                offset += numRects;
            }

            view->masks = list->masks + maskStart;
            view->numSprites = *numMasksTotal - maskStart;
            view->flags = ViewFlag::none;
            for (int i = 0; i < view->numSprites; i++) {
                if (view->masks[i].op != MaskOp::none) {
                    view->flags |= ViewFlag::needsTrackMask;
                    break;
                }
            }

            std::free(rects);
            std::free(secondaryRects);
            return {};
        }

        // Read optional boolean property, defaulting to false when missing.
        [[nodiscard]] JsonResult<bool> readOptionalBool(
            const Json *parent, const char *key) {
            const Json *v = json_object_get(parent, key);
            if (v == nullptr) return false;
            if (!json_is_boolean(v)) {
                return std::unexpected(std::format(
                    "Property \"{}\" is not a boolean", key));
            }
            return json_boolean_value(v) != 0;
        }

        [[nodiscard]] JsonResult<int> readBoolArrayInto(
            const Json *parent, const char *key,
            std::array<int, 8> &out) {
            const Json *arr = json_object_get(parent, key);
            if (arr == nullptr) return 0;
            if (!json_is_array(arr)) {
                return std::unexpected(std::format(
                    "Property \"{}\" is not an array", key));
            }
            const auto n = std::min<std::size_t>(json_array_size(arr), 8);
            for (std::size_t k = 0; k < n; k++) {
                const Json *item = json_array_get(arr, k);
                if (!json_is_boolean(item)) {
                    return std::unexpected(std::format(
                        "Property \"{}\" contains non-boolean value", key));
                }
                out[k] = json_boolean_value(item);
            }
            return static_cast<int>(json_array_size(arr));
        }

        [[nodiscard]] JsonResult<int> readOffsetsInto(
            const Json *parent,
            std::array<int, 8> &xOff, std::array<int, 8> &yOff) {
            const Json *arr = json_object_get(parent, "offset");
            if (arr == nullptr) return 0;
            if (!json_is_array(arr)) {
                return std::unexpected(std::string("\"offset\" is not an array"));
            }
            const auto n = std::min<std::size_t>(json_array_size(arr), 8);
            for (std::size_t k = 0; k < n; k++) {
                const Json *item = json_array_get(arr, k);
                if (!json_is_array(item) || json_array_size(item) != 2) {
                    return std::unexpected(std::string(
                        "\"offset\" contains an element which is not a pair of integers"));
                }
                const Json *x = json_array_get(item, 0);
                const Json *y = json_array_get(item, 1);
                if (!json_is_integer(x) || !json_is_integer(y)) {
                    return std::unexpected(std::string(
                        "\"offset\" contains an element which is not a pair of integers"));
                }
                xOff[k] = static_cast<int>(json_integer_value(x));
                yOff[k] = static_cast<int>(json_integer_value(y));
            }
            return static_cast<int>(json_array_size(arr));
        }
    } // namespace

    std::expected<void, std::string> loadMasks(
        const std::filesystem::path &filename,
        std::array<ViewSet, kNumTrackSections> &views) {
        auto root = loadFile(filename);
        if (!root) return std::unexpected(root.error());

        Json *json = root->get();
        if (!json_is_object(json)) {
            return std::unexpected(std::string("Top level must be object"));
        }

        int numRects = 0;
        int numMasks = 0;
        MaskList list{};
        list.rects = static_cast<rect_t *>(std::calloc(kMaxRects, sizeof(rect_t)));
        list.masks = static_cast<Mask *>(std::calloc(kMaxMasks, sizeof(Mask)));
        list.views = views.data();
        std::memset(list.views, 0, kNumTrackSections * sizeof(ViewSet));

        for (int i = 0; i < kNumTrackSections; i++) {
            printMsg("Loading {}", kTrackSections[i].name);

            Json *section = json_object_get(json, kTrackSections[i].name);
            if (section == nullptr) continue;
            if (!json_is_array(section)) {
                return std::unexpected(std::format(
                    "Property {} does not exist or is not an array", kTrackSections[i].name));
            }

            const std::size_t numAngles = json_array_size(section);
            for (std::size_t j = 0; j < numAngles; j++) {
                Json *item = json_array_get(section, j);
                if (!json_is_object(item)) {
                    return std::unexpected(std::string("Array element is not an object"));
                }

                Json *maskJson = json_object_get(item, "mask");
                if (!maskJson || !json_is_string(maskJson)) {
                    return std::unexpected(std::string(
                        "\"mask\" not found or is not a string"));
                }
                const char *maskPath = json_string_value(maskJson);
                std::FILE *file = std::fopen(maskPath, "rb");
                if (file == nullptr) {
                    return std::unexpected(std::format(
                        "Could not open {}", maskPath));
                }

                image_t image;
                if (image_read_png(&image, file) != 0) {
                    std::fclose(file);
                    return std::unexpected(std::format(
                        "Failed loading {}", maskPath));
                }
                std::fclose(file);

                auto extrudeBehind = readOptionalBool(item, "extrude_behind");
                if (!extrudeBehind) return std::unexpected(extrudeBehind.error());
                auto extrudeInFront = readOptionalBool(item, "extrude_in_front");
                if (!extrudeInFront) return std::unexpected(extrudeInFront.error());
                auto maskEnd = readOptionalBool(item, "mask_end");
                if (!maskEnd) return std::unexpected(maskEnd.error());
                auto mirror = readOptionalBool(item, "mirror");
                if (!mirror) return std::unexpected(mirror.error());
                auto splitEnds = readOptionalBool(item, "split_ends");
                if (!splitEnds) return std::unexpected(splitEnds.error());

                ProcessOpts opts{};
                opts.mirror = *mirror;
                opts.splitEnds = *splitEnds;

                auto numFromSplit = readBoolArrayInto(item, "split", opts.split);
                if (!numFromSplit) return std::unexpected(numFromSplit.error());
                opts.numSprites = *numFromSplit;

                auto numFromTransfer = readBoolArrayInto(item, "transfer", opts.transfer);
                if (!numFromTransfer) return std::unexpected(numFromTransfer.error());
                if (*numFromTransfer > opts.numSprites) opts.numSprites = *numFromTransfer;

                auto numFromOffset = readOffsetsInto(item, opts.xOffset, opts.yOffset);
                if (!numFromOffset) return std::unexpected(numFromOffset.error());
                if (*numFromOffset > opts.numSprites) opts.numSprites = *numFromOffset;

                if (auto r = processMask(&image, opts, &list.views[i][j],
                                         &list, &numMasks, &numRects); !r) {
                    return std::unexpected(r.error());
                }
                if (*extrudeBehind) list.views[i][j].flags |= ViewFlag::extrudeBehind;
                if (*extrudeInFront) list.views[i][j].flags |= ViewFlag::extrudeInFront;
                if (*maskEnd) list.views[i][j].flags |= ViewFlag::maskEnd;
            }
        }

        // NOTE: rects and masks arrays are leaked intentionally — the rendered
        // ViewSet keeps pointers into them for the rest of the program's
        // lifetime. Process teardown reclaims everything.
        return {};
    }
}