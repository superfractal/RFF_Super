//
// Created by Opus 5 on 2026-08-18.
// Modified by GPT-5 on 2026-08-18, 2026-08-23, 2026-08-31, 2026-09-01
// Modified by Opus 5 on 2026-08-25, 2026-08-26
//

#include "TimelineIO.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>

#include "../../vulkan_helper/core/logger.hpp"
#include "../ui/IOUtilities.h"
#include "../video/TimelineParams.hpp"

namespace merutilm::rff2 {

    namespace {
        // A corrupt or foreign file must not be able to ask for an allocation of any size it likes.
        constexpr uint32_t MAX_TRACKS = 4096;
        constexpr uint32_t MAX_KEYS = 65536;
        constexpr uint32_t MAX_HOLDS = 65536;

        bool hasMore(std::ifstream &in) {
            return in.rdbuf()->sgetc() != std::char_traits<char>::eof();
        }

        // A count past its ceiling is a file that is not what it claims to be, so it is reported the
        // way a truncated one is rather than silently loading as an empty timeline.
        bool checkCount(std::ifstream &in, const uint32_t count, const uint32_t limit) {
            if (in.fail()) {
                return false;
            }
            if (count > limit) {
                in.setstate(std::ios::failbit);
                return false;
            }
            return true;
        }

        // Depths order the keys, holds add up to the running time, and both are read straight off
        // the file. A NaN depth has no order at all, so sorting on it is undefined rather than
        // merely wrong, and an infinite hold makes the total length - and the frame count taken
        // from it - meaningless. Neither can be repaired into a sensible value, so a file carrying
        // one is refused the way a truncated one is.
        bool checkFinite(std::ifstream &in, const std::initializer_list<float> values) {
            if (in.fail()) {
                return false;
            }
            for (const float value: values) {
                if (!std::isfinite(value)) {
                    in.setstate(std::ios::failbit);
                    return false;
                }
            }
            return true;
        }
    }

    void TimelineIO::writeTimeline(std::ofstream &out, const VidTimelineAttribute &timeline) {
        IOUtilities::encodeAndWrite(out, timeline.enabled);
        IOUtilities::encodeAndWrite(out, static_cast<uint32_t>(timeline.tracks.size()));
        for (const auto &track: timeline.tracks) {
            IOUtilities::encodeAndWrite(out, track.targetId);
            IOUtilities::encodeAndWrite(out, track.enabled);
            // Reserved for the color tracks: which space their keys are blended in. Only OKLab is
            // written for now, and the field is here so adding the choice shifts no later field.
            IOUtilities::encodeAndWrite(out, static_cast<uint8_t>(0));
            IOUtilities::encodeAndWrite(out, static_cast<uint32_t>(track.keys.size()));
            for (const auto &key: track.keys) {
                IOUtilities::encodeAndWrite(out, key.depth);
                IOUtilities::encodeAndWrite(out, key.value);
                IOUtilities::encodeAndWrite(out, key.color.x);
                IOUtilities::encodeAndWrite(out, key.color.y);
                IOUtilities::encodeAndWrite(out, key.color.z);
                IOUtilities::encodeAndWrite(out, key.color.w);
                IOUtilities::encodeAndWrite(out, static_cast<int32_t>(key.out));
            }
        }
        IOUtilities::encodeAndWrite(out, static_cast<uint32_t>(timeline.holds.size()));
        for (const auto &[depth, seconds]: timeline.holds) {
            IOUtilities::encodeAndWrite(out, depth);
            IOUtilities::encodeAndWrite(out, seconds);
        }
        // Appended last: the keyframe count the editor's length readout assumes.
        IOUtilities::encodeAndWrite(out, timeline.estimateKeyframes);
    }

    void TimelineIO::readTimeline(std::ifstream &in, VidTimelineAttribute &out) {
        VidTimelineAttribute t = {};
        IOUtilities::readAndDecode(in, &t.enabled);
        uint32_t trackCount = 0;
        IOUtilities::readAndDecode(in, &trackCount);
        if (!checkCount(in, trackCount, MAX_TRACKS)) {
            return;
        }
        std::unordered_set<uint16_t> targetIds;
        for (uint32_t i = 0; i < trackCount; ++i) {
            VidTimelineTrack track = {};
            IOUtilities::readAndDecode(in, &track.targetId);
            if (in.fail() || !targetIds.insert(track.targetId).second) {
                in.setstate(std::ios::failbit);
                return;
            }
            IOUtilities::readAndDecode(in, &track.enabled);
            uint8_t colorSpace = 0;
            IOUtilities::readAndDecode(in, &colorSpace);
            uint32_t keyCount = 0;
            IOUtilities::readAndDecode(in, &keyCount);
            if (!checkCount(in, keyCount, MAX_KEYS)) {
                return;
            }
            track.keys.reserve(keyCount);
            for (uint32_t k = 0; k < keyCount; ++k) {
                VidTimelineKey key = {};
                IOUtilities::readAndDecode(in, &key.depth);
                IOUtilities::readAndDecode(in, &key.value);
                IOUtilities::readAndDecode(in, &key.color.x);
                IOUtilities::readAndDecode(in, &key.color.y);
                IOUtilities::readAndDecode(in, &key.color.z);
                IOUtilities::readAndDecode(in, &key.color.w);
                int32_t interpolation = 0;
                IOUtilities::readAndDecode(in, &interpolation);
                // A file may name a curve this build does not have; hold the value instead.
                key.out = interpolation >= 0 && interpolation <= static_cast<int32_t>(VidKeyInterpolation::CUBIC)
                              ? static_cast<VidKeyInterpolation>(interpolation)
                              : VidKeyInterpolation::STEP;
                if (!checkFinite(in, {key.depth, key.value, key.color.x, key.color.y, key.color.z, key.color.w})) {
                    return;
                }
                track.keys.push_back(key);
            }
            // Playback order is the order the evaluator walks, so it is restored here rather than
            // trusted: a file written by hand, or by an editor that sorted another way, still runs.
            std::ranges::stable_sort(track.keys, [](const VidTimelineKey &a, const VidTimelineKey &b) {
                return a.depth > b.depth;
            });
            // A key outside the parameter's own range is held to it here rather than left to the
            // shader, so the number the editor shows and the file keeps is the one that is drawn.
            if (const TimelineParamDesc *param = TimelineParams::find(track.targetId); param != nullptr) {
                for (auto &key: track.keys) {
                    if (param->kind == TimelineParamKind::COLOR) {
                        key.color = glm::clamp(key.color, glm::vec4(0.0f), glm::vec4(1.0f));
                    } else if (std::isfinite(key.value)) {
                        key.value = std::clamp(key.value, param->minValue, param->maxValue);
                    }
                }
            }
            t.tracks.push_back(std::move(track));
        }
        uint32_t holdCount = 0;
        IOUtilities::readAndDecode(in, &holdCount);
        if (!checkCount(in, holdCount, MAX_HOLDS)) {
            return;
        }
        t.holds.reserve(holdCount);
        for (uint32_t i = 0; i < holdCount; ++i) {
            VidTimelineHold hold = {};
            IOUtilities::readAndDecode(in, &hold.depth);
            IOUtilities::readAndDecode(in, &hold.seconds);
            if (!checkFinite(in, {hold.depth, hold.seconds})) {
                return;
            }
            t.holds.push_back(hold);
        }
        if (hasMore(in)) {
            IOUtilities::readAndDecode(in, &t.estimateKeyframes);
            if (!checkFinite(in, {t.estimateKeyframes})) {
                return;
            }
        }
        if (in.fail()) {
            return;
        }
        out = std::move(t);
    }

    void TimelineIO::writeConfigBlock(std::ofstream &out, const VidTimelineAttribute &timeline) {
        IOUtilities::encodeAndWrite(out, CONFIG_BLOCK_MAGIC);
        writeTimeline(out, timeline);
    }

    void TimelineIO::readConfigBlock(std::ifstream &in, VidTimelineAttribute &out) {
        // A read that already failed is a truncated file, and the caller has to keep hearing about
        // it; only what this block itself runs short of is cleared below.
        if (in.fail()) {
            return;
        }
        uint32_t magic = 0;
        IOUtilities::readAndDecode(in, &magic);
        const bool recognized = !in.fail() && magic == CONFIG_BLOCK_MAGIC;
        if (recognized) {
            readTimeline(in, out);
        }
        // Fewer bytes here than the block asked for means it is simply not in this file - a marker
        // that is only part of a word, or an older build's leftover field. Nothing is read past this
        // point, so the shortfall is not corruption and must not be reported as it.
        if (in.fail() && !recognized) {
            in.clear();
        }
    }

    bool TimelineIO::save(const std::filesystem::path &path, const VidTimelineAttribute &timeline) {
        const std::filesystem::path temporary = IOUtilities::temporaryFilePath(path);
        std::ofstream out(temporary, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            vkh::logger::w_log(L"ERROR : Cannot save timeline");
            return false;
        }
        IOUtilities::encodeAndWrite(out, MAGIC);
        IOUtilities::encodeAndWrite(out, VERSION);
        writeTimeline(out, timeline);
        out.close();
        if (out.fail() || !IOUtilities::commitTemporaryFile(temporary, path)) {
            IOUtilities::discardTemporaryFile(temporary);
            vkh::logger::w_log(L"ERROR : Cannot save timeline");
            return false;
        }
        return true;
    }

    bool TimelineIO::load(const std::filesystem::path &path, VidTimelineAttribute &out) {
        if (!std::filesystem::exists(path)) {
            return false;
        }
        std::ifstream in(path, std::ios::in | std::ios::binary);
        if (!in.is_open()) {
            return false;
        }
        uint32_t magic = 0;
        uint32_t version = 0;
        IOUtilities::readAndDecode(in, &magic);
        IOUtilities::readAndDecode(in, &version);
        if (magic != MAGIC || version > VERSION) {
            vkh::logger::w_log(L"ERROR : Not a valid timeline file");
            return false;
        }
        // Decoded into a temporary, then committed only on success, so a corrupt file never leaves
        // the live timeline half-overwritten.
        VidTimelineAttribute t = {};
        readTimeline(in, t);
        if (in.fail()) {
            vkh::logger::w_log(L"ERROR : Timeline file is corrupted");
            return false;
        }
        out = std::move(t);
        return true;
    }
}
