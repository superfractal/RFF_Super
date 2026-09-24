//
// Created by Opus 5 on 2026-08-18.
// Modified by GPT-5 on 2026-08-18, 2026-08-23, 2026-08-31, 2026-09-01
// Modified by Opus 5 on 2026-08-25, 2026-08-26
// Modified by GPT-6 on 2026-09-14, 2026-09-15, 2026-09-18, 2026-09-19, 2026-09-20, 2026-09-21, 2026-09-23, 2026-09-24
//

#include "TimelineIO.h"
#include "AudioTimelineIO.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_set>

#include "../../vulkan_helper/core/logger.hpp"
#include "../ui/IOUtilities.h"
#include "../video/TimelineParams.hpp"
#include "../attr/NumericSettingLimits.hpp"

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
            for (const float value : values) {
                if (!std::isfinite(value)) {
                    in.setstate(std::ios::failbit);
                    return false;
                }
            }
            return true;
        }
    } // namespace

    void TimelineIO::writeTimeline(std::ostream &out, const VidTimelineAttribute &timeline,
                                   const bool includeRotation) {
        IOUtilities::encodeAndWrite(out, timeline.enabled);
        IOUtilities::encodeAndWrite(out, static_cast<uint32_t>(timeline.tracks.size()));
        for (const auto &track : timeline.tracks) {
            IOUtilities::encodeAndWrite(out, track.targetId);
            IOUtilities::encodeAndWrite(out, track.enabled);
            IOUtilities::encodeAndWrite(out, static_cast<uint32_t>(track.keys.size()));
            for (const auto &key : track.keys) {
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
        for (const auto &[depth, seconds] : timeline.holds) {
            IOUtilities::encodeAndWrite(out, depth);
            IOUtilities::encodeAndWrite(out, seconds);
        }
        // Appended last: the keyframe count the editor's length readout assumes.
        IOUtilities::encodeAndWrite(out, timeline.estimateKeyframes);
        if (includeRotation) {
            writeRotation(out, timeline);
            writeOverlay(out, timeline.zoomOverlay);
        }
    }

    void TimelineIO::readTimeline(std::ifstream &in, VidTimelineAttribute &out, const bool includeRotation, const bool legacyLayout) {
        VidTimelineAttribute loadedTimeline = {};
        loadedTimeline.zoomOverlay.visible = out.zoomOverlay.visible;
        IOUtilities::readAndDecode(in, &loadedTimeline.enabled);
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
            if (legacyLayout) {
                uint8_t discarded;
                IOUtilities::readAndDecode(in, &discarded);
            }
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
                key.out =
                    interpolation >= 0 && interpolation <= static_cast<int32_t>(VidKeyInterpolation::CUBIC)
                        ? static_cast<VidKeyInterpolation>(interpolation)
                        : VidKeyInterpolation::STEP;
                if (!checkFinite(
                        in, {key.depth, key.value, key.color.x, key.color.y, key.color.z, key.color.w})) {
                    return;
                }
                track.keys.push_back(key);
            }
            // Playback order is the order the evaluator walks, so it is restored here rather than
            // trusted: a file written by hand, or by an editor that sorted another way, still runs.
            std::ranges::stable_sort(track.keys, [](const VidTimelineKey &a, const VidTimelineKey &b) {
                return a.depth > b.depth;
            });
            if (const auto *param = TimelineParams::find(track.targetId);
                param && param->kind == TimelineParamKind::FLOAT) {
                for (const auto &key : track.keys) {
                    if (!NumericSettingLimits::Range{param->minValue, param->maxValue}(key.value)) {
                        vkh::logger::w_log(L"ERROR : Timeline value requires adjustment: {}", param->name);
                        in.setstate(std::ios::failbit);
                        return;
                    }
                }
            }
            // A key outside the parameter's own range is held to it here rather than left to the
            // shader, so the number the editor shows and the file keeps is the one that is drawn.
            if (const TimelineParamDesc *param = TimelineParams::find(track.targetId); param != nullptr) {
                for (auto &key : track.keys) {
                    if (param->kind == TimelineParamKind::COLOR) {
                        key.color = glm::clamp(key.color, glm::vec4(0.0f), glm::vec4(1.0f));
                    } else if (std::isfinite(key.value)) {
                        key.value = std::clamp(key.value, param->minValue, param->maxValue);
                    }
                }
            }
            if (track.targetId < 1202 || track.targetId > 1221) {
                loadedTimeline.tracks.push_back(std::move(track));
            }
        }
        uint32_t holdCount = 0;
        IOUtilities::readAndDecode(in, &holdCount);
        if (!checkCount(in, holdCount, MAX_HOLDS)) {
            return;
        }
        loadedTimeline.holds.reserve(holdCount);
        for (uint32_t i = 0; i < holdCount; ++i) {
            VidTimelineHold hold = {};
            IOUtilities::readAndDecode(in, &hold.depth);
            IOUtilities::readAndDecode(in, &hold.seconds);
            if (!checkFinite(in, {hold.depth, hold.seconds})) {
                return;
            }
            loadedTimeline.holds.push_back(hold);
        }
        if (hasMore(in)) {
            IOUtilities::readAndDecode(in, &loadedTimeline.estimateKeyframes);
            if (!checkFinite(in, {loadedTimeline.estimateKeyframes})) {
                return;
            }
        }
        if (in.fail()) {
            return;
        }
        if (includeRotation) {
            readRotation(in, loadedTimeline);
            readOverlay(in, loadedTimeline.zoomOverlay);
        }
        if (in.fail()) {
            return;
        }
        out = std::move(loadedTimeline);
    }

    void TimelineIO::writeOverlay(std::ostream &out, const VidZoomOverlayAttribute &overlaySettings) {
        const auto writeValue = [&](auto v) { IOUtilities::encodeAndWrite(out, v); };
        const auto color = [&](glm::vec4 c) {
            for (int i = 0; i < 4; ++i) {
                writeValue(c[i]);
            }
        };
        writeValue(uint32_t{0x31564f5a});
        writeValue(uint32_t(overlaySettings.visible));
        writeValue(uint32_t(overlaySettings.custom));
        writeValue(overlaySettings.anchor);
        writeValue(overlaySettings.x);
        writeValue(overlaySettings.y);
        writeValue(overlaySettings.size);
        writeValue(uint32_t(overlaySettings.family.size()));
        out.write(overlaySettings.family.data(), static_cast<std::streamsize>(overlaySettings.family.size()));
        writeValue(overlaySettings.style);
        color(overlaySettings.color);
        writeValue(uint32_t(overlaySettings.outline));
        writeValue(overlaySettings.outlineWidth);
        color(overlaySettings.outlineColor);
        writeValue(uint32_t(overlaySettings.shadow));
        writeValue(overlaySettings.shadowX);
        writeValue(overlaySettings.shadowY);
        color(overlaySettings.shadowColor);
    }

    bool TimelineIO::readOverlay(std::ifstream &in, VidZoomOverlayAttribute &overlay) {
        if (in.fail() || !hasMore(in)) {
            return false;
        }
        uint32_t marker = 0;
        IOUtilities::readAndDecode(in, &marker);
        if (in.fail() || marker != 0x31564f5a) {
            in.setstate(std::ios::failbit);
            return false;
        }
        auto overlaySettings = overlay;
        const auto readValue = [&](auto &v) {
            if (!in.fail() && hasMore(in)) {
                IOUtilities::readAndDecode(in, &v);
            }
        };
        const auto readFlag = [&](bool &v) {
            uint32_t n = v;
            readValue(n);
            if (n > 1) {
                in.setstate(std::ios::failbit);
            } else {
                v = n != 0;
            }
        };
        const auto color = [&](glm::vec4 &v) {
            for (int i = 0; i < 4; ++i) {
                readValue(v[i]);
            }
        };
        readFlag(overlaySettings.visible);
        readFlag(overlaySettings.custom);
        readValue(overlaySettings.anchor);
        readValue(overlaySettings.x);
        readValue(overlaySettings.y);
        readValue(overlaySettings.size);
        if (!in.fail() && hasMore(in)) {
            uint32_t length = 0;
            IOUtilities::readAndDecode(in, &length);
            if (in.fail() || length == 0 || length > 256) {
                in.setstate(std::ios::failbit);
                return false;
            }
            overlaySettings.family.resize(length);
            in.read(overlaySettings.family.data(), length);
            if (in.fail() || overlaySettings.family.find('\0') != std::string::npos ||
                MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, overlaySettings.family.data(), int(length),
                                    nullptr, 0) == 0) {
                in.setstate(std::ios::failbit);
                return false;
            }
        }
        readValue(overlaySettings.style);
        color(overlaySettings.color);
        readFlag(overlaySettings.outline);
        readValue(overlaySettings.outlineWidth);
        color(overlaySettings.outlineColor);
        readFlag(overlaySettings.shadow);
        readValue(overlaySettings.shadowX);
        readValue(overlaySettings.shadowY);
        color(overlaySettings.shadowColor);
        const auto range = [](float v, float lo, float hi) { return std::isfinite(v) && v >= lo && v <= hi; };
        const auto validColor = [&](glm::vec4 v) {
            for (int i = 0; i < 4; ++i) {
                if (!range(v[i], 0, 1)) {
                    return false;
                }
            }
            return true;
        };
        if (in.fail() || overlaySettings.anchor > 8 || overlaySettings.style > 3 ||
            !range(overlaySettings.x, 0, 1) || !range(overlaySettings.y, 0, 1) ||
            !range(overlaySettings.size, .005f, .2f) || !range(overlaySettings.outlineWidth, 0, .25f) ||
            !range(overlaySettings.shadowX, -1, 1) || !range(overlaySettings.shadowY, -1, 1) ||
            !validColor(overlaySettings.color) || !validColor(overlaySettings.outlineColor) ||
            !validColor(overlaySettings.shadowColor)) {
            in.setstate(std::ios::failbit);
            return false;
        }
        overlay = std::move(overlaySettings);
        return true;
    }

    void TimelineIO::writeOverlayPrecision(std::ostream &out, const VidZoomOverlayAttribute &overlay) {
        IOUtilities::encodeAndWrite(out, uint32_t{0x3150445a});
        IOUtilities::encodeAndWrite(out, overlay.decimalPlaces);
    }

    void TimelineIO::readOverlayPrecision(std::ifstream &in, VidZoomOverlayAttribute &overlay) {
        overlay.decimalPlaces = 6;
        if (in.fail() || !hasMore(in)) {
            return;
        }
        uint32_t marker = 0;
        IOUtilities::readAndDecode(in, &marker);
        if (in.fail() || marker != 0x3150445a) {
            in.setstate(std::ios::failbit);
            return;
        }
        IOUtilities::readAndDecode(in, &overlay.decimalPlaces);
        if (overlay.decimalPlaces > 9) {
            in.setstate(std::ios::failbit);
        }
    }

    void TimelineIO::writeRotation(std::ostream &out, const VidTimelineAttribute &timeline) {
        IOUtilities::encodeAndWrite(out, uint32_t{0x31544F52});
        IOUtilities::encodeAndWrite(out, static_cast<uint32_t>(timeline.rotationMode));
        IOUtilities::encodeAndWrite(out, timeline.rotationPeriod);
        IOUtilities::encodeAndWrite(out, static_cast<uint32_t>(timeline.rotationDirection));
        IOUtilities::encodeAndWrite(out, timeline.rotationStartAngle);
    }

    void TimelineIO::readRotation(std::ifstream &in, VidTimelineAttribute &out) {
        if (in.fail() || !hasMore(in)) {
            return;
        }
        const auto position = in.tellg();
        uint32_t magic = 0;
        IOUtilities::readAndDecode(in, &magic);
        if (in.fail()) {
            return;
        }
        if (magic != 0x31544F52) {
            in.seekg(position);
            return;
        }
        uint32_t mode = static_cast<uint32_t>(out.rotationMode),
                 direction = static_cast<uint32_t>(out.rotationDirection);
        if (hasMore(in)) {
            IOUtilities::readAndDecode(in, &mode);
        }
        if (hasMore(in)) {
            IOUtilities::readAndDecode(in, &out.rotationPeriod);
        }
        if (hasMore(in)) {
            IOUtilities::readAndDecode(in, &direction);
        }
        if (hasMore(in)) {
            IOUtilities::readAndDecode(in, &out.rotationStartAngle);
        }
        if (!checkFinite(in, {out.rotationPeriod, out.rotationStartAngle}) || mode > 1 || direction > 1 ||
            out.rotationPeriod < 0.01f || out.rotationPeriod > 86400.0f ||
            std::abs(out.rotationStartAngle) > 360000.0f) {
            in.setstate(std::ios::failbit);
            return;
        }
        out.rotationMode = static_cast<VidRotationMode>(mode);
        out.rotationDirection = static_cast<VidRotationDirection>(direction);
    }

    void TimelineIO::writeConfigBlock(std::ostream &out, const VidTimelineAttribute &timeline) {
        IOUtilities::encodeAndWrite(out, CONFIG_BLOCK_MAGIC);
        writeTimeline(out, timeline, false);
    }

    void TimelineIO::readConfigBlock(std::ifstream &in, VidTimelineAttribute &out, const bool legacyLayout) {
        // A read that already failed is a truncated file, and the caller has to keep hearing about
        // it; only what this block itself runs short of is cleared below.
        if (in.fail()) {
            return;
        }
        uint32_t magic = 0;
        IOUtilities::readAndDecode(in, &magic);
        const bool recognized = !in.fail() && magic == CONFIG_BLOCK_MAGIC;
        if (recognized) {
            readTimeline(in, out, false, legacyLayout);
        }
        // Fewer bytes here than the block asked for means it is simply not in this file - a marker
        // that is only part of a word, or an older build's leftover field. Nothing is read past this
        // point, so the shortfall is not corruption and must not be reported as it.
        if (in.fail() && !recognized) {
            in.clear();
        }
    }

    bool TimelineIO::save(const std::filesystem::path &path, const VidTimelineAttribute &timeline) {
        auto audio = timeline.audio;
        try {
            AudioTimelineIO::relativePaths(audio, path);
        } catch (const std::filesystem::filesystem_error &) {
            return false;
        }
        if (!audio.valid()) {
            return false;
        }
        const std::filesystem::path temporary = IOUtilities::temporaryFilePath(path);
        std::ofstream out(temporary, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            vkh::logger::w_log(L"ERROR : Cannot save timeline");
            return false;
        }
        IOUtilities::encodeAndWrite(out, MAGIC);
        IOUtilities::encodeAndWrite(out, VERSION);
        writeTimeline(out, timeline);
        AudioTimelineIO::write(out, audio);
        writeOverlayPrecision(out, timeline.zoomOverlay);
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
        if (magic != MAGIC || version < 1 || version > VERSION) {
            vkh::logger::w_log(L"ERROR : Not a valid timeline file");
            return false;
        }
        // Decoded into a temporary, then committed only on success, so a corrupt file never leaves
        // the live timeline half-overwritten.
        VidTimelineAttribute loadedTimeline = {};
        loadedTimeline.zoomOverlay.visible = out.zoomOverlay.visible;
        readTimeline(in, loadedTimeline, true, version < 3);
        if (version >= 2 && !AudioTimelineIO::read(in, loadedTimeline.audio, true)) {
            return false;
        }
        readOverlayPrecision(in, loadedTimeline.zoomOverlay);
        try {
            AudioTimelineIO::resolvePaths(loadedTimeline.audio, path);
        } catch (const std::filesystem::filesystem_error &) {
            return false;
        }
        if (in.fail()) {
            vkh::logger::w_log(L"ERROR : Timeline file is corrupted");
            return false;
        }
        out = std::move(loadedTimeline);
        return true;
    }
} // namespace merutilm::rff2
