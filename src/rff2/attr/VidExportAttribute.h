//
// Created by Merutilm on 2025-05-04.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21.
// Modified by Opus 5 on 2026-08-12, 2026-08-14, 2026-08-19
//

#pragma once
#include "VidHdrTransfer.h"

namespace merutilm::rff2 {
    struct VidExportAttribute {
        float fps;
        uint32_t bitrate;
        // Bit-exact export: x264 in RGB, which needs the matroska container and ignores bitrate.
        bool lossless;
        uint32_t keyframeAA;
        // Temporal supersampling for color animation. Each output frame is rendered
        // colorAA times at evenly spread instants inside the frame's 1/fps slice and
        // averaged, removing Psychedelic / Color-Animation-Speed judder. 1 = off.
        uint32_t colorAA;
        // When true, video is built automatically right after keyframe generation finishes.
        // Default is false so keyframes are kept and the video is exported manually.
        bool autoCreateVideo;
        // Hold the main preview while a video export runs. The export draws on a window context of
        // its own, so nothing of the main window has to be produced at all.
        bool pauseMainPreview;
        // The same while keyframes are being generated. Generation runs *through* the main render
        // call - that is where its recompute / image requests are served - so only the drawing is
        // held there; the status bar (zoom ratio, period, elapsed time) keeps updating.
        bool pauseKeyframePreview;
        // Write generated keyframes as .rfmz rather than .rfm. The two hold the same table - the
        // packing is lossless - so this only decides how much room the folder takes.
        bool compressKeyframes;
        // Which curve the exported pixels carry. SDR keeps the 8-bit tone-mapped picture; PQ and HLG
        // send 10-bit BT.2020 and need HDR to be on, since only then does the chain hold light above white.
        VidHdrTransfer hdrTransfer = VidHdrTransfer::SDR;
        // The display brightness the HDR headroom lands on, in nits.
        float hdrPeakNits = 1000.0f;
    };
}
