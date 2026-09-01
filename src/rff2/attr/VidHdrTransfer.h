//
// Created by Opus 5 on 2026-08-19.
//

#pragma once

namespace merutilm::rff2 {
    // What the exported video's pixels mean: a tone-mapped SDR picture, or scene light carried in one of the HDR curves.
    enum class VidHdrTransfer {
        SDR = 0,
        PQ = 1,
        HLG = 2
    };
}
