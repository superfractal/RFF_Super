//
// Created by Opus 5 on 2026-08-29.
// Modified by Fable 5.1 on 2026-09-02
//

#pragma once

namespace merutilm::rff2 {
    // The coordinate the gloss bands are laid along. Values are the shader's mode numbers.
    // Every one of them belongs to the relief, so none of them moves when the palette does.
    enum class ShdSlopeGlossSource {
        SHADING = 0,
        RELIEF = 1,
        ASPECT = 2,
        SHADING_FINE = 3
    };
}
