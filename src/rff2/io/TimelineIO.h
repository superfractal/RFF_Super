//
// Created by Opus 5 on 2026-08-18.
// Modified by GPT-5 on 2026-08-18
//

#pragma once
#include <filesystem>
#include <fstream>

#include "../attr/VidTimelineAttribute.h"

namespace merutilm::rff2 {
    // Saves/loads a video timeline - the speed curve, its holds, and (later) the shader tracks - as
    // a file of its own, so one sense of pacing can be carried from one keyframe folder to another.
    //
    // The tracks are variable-length, which the flat append-only stream ConfigIO is cannot carry
    // (docs/config-file-format.md), so this is counted and versioned rather than positional.
    struct TimelineIO {
        TimelineIO() = delete;

        static constexpr uint32_t MAGIC = 0x54564652; // "RFVT"
        static constexpr uint32_t VERSION = 1;

        // Marks the timeline block inside a settings file. Every field appended to that file before
        // this one is bare, so a config from a build that wrote one more of them would hand its
        // bytes to the block below; the marker is what tells the two apart.
        static constexpr uint32_t CONFIG_BLOCK_MAGIC = 0x424C5456; // "VTLB"

        static bool save(const std::filesystem::path &path, const VidTimelineAttribute &timeline);

        static bool load(const std::filesystem::path &path, VidTimelineAttribute &out);

        // Stream-level halves, reused by the full-config serializer.
        static void writeTimeline(std::ofstream &out, const VidTimelineAttribute &timeline);

        static void readTimeline(std::ifstream &in, VidTimelineAttribute &out);

        // The same behind CONFIG_BLOCK_MAGIC. The read leaves the timeline untouched when the
        // marker is not the one written, which is also where it stops: past an unknown field
        // nothing further can be located.
        static void writeConfigBlock(std::ofstream &out, const VidTimelineAttribute &timeline);

        static void readConfigBlock(std::ifstream &in, VidTimelineAttribute &out);
    };
}
