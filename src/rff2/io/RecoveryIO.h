//
// Created by Opus 5 on 2026-08-14.
// Modified by Opus 5 on 2026-08-15.
// Modified by GPT-5 on 2026-09-01
//

#pragma once
#include <filesystem>
#include <optional>

#include "../attr/Attribute.h"

namespace merutilm::rff2 {
    // How the run whose settings are being offered back ended. The offer is the same either way;
    // only what the panel says of it differs.
    enum class RecoveryReason : uint8_t {
        // It never shut down: its lock outlived it.
        CRASHED,
        // It was shut down by hand while it was still computing - a view too heavy to wait for is
        // closed rather than waited out, and that ending is the user's, not a fault.
        INTERRUPTED
    };

    // The settings of such a run, and what ended it.
    struct RecoveredSnapshot {
        std::filesystem::path path;
        RecoveryReason reason;
    };

    // Keeps what the running session is working on where the next start can find it, so a run that
    // ends without shutting down - or is given up on partway - is not lost. A live session holds a
    // lock file naming its process; a lock whose process is gone is what a crash leaves behind, and
    // the settings file beside it is what is offered back. The snapshot is an ordinary config file
    // (see ConfigIO), so it can also be opened by hand through Load Location / Settings.
    struct RecoveryIO {
        RecoveryIO() = delete;

        // The settings of the last run that did not reach a finished view, moved aside under one
        // fixed name so this session's own files can never land on it. Empty when the last run shut
        // down cleanly with nothing left running, or ended before it had anything to keep. A crash
        // outranks a run closed partway, and the offer is taken either way: what is left unoffered
        // would surface a start too late to make sense of. Takes every dead session's files with
        // it, so this is done once, at startup, before beginSession().
        static std::optional<RecoveredSnapshot> takeSnapshot();

        // Marks this process live. Until endSession(), its snapshot is a crash waiting to be offered.
        static void beginSession();

        // Shutting down: the lock goes, and so does the snapshot - unless the run is still computing
        // what it was asked for, which is the ending that keeps the snapshot for the next start to
        // offer back as INTERRUPTED.
        static void endSession(bool unfinished);

        // Writes the settings the session is working on. It is written whole to a temporary file and
        // moved over the snapshot, so a crash during the write cannot leave a half-written one.
        static void writeSnapshot(const Attribute &attr, uint16_t width, uint16_t height);

    };
}
