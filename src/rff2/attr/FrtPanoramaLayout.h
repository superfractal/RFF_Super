//
// Created by Opus 5 on 2026-08-31.
//

#pragma once

namespace merutilm::rff2 {
    // Where the plane is put relative to the viewer under a 360 projection.
    enum class FrtPanoramaLayout {
        // The plane lies under the viewer, in true perspective, with sky above the horizon.
        GROUND = 0,
        // The whole plane is wrapped onto the whole sphere, conformally, so the far field fills the upper half turned inside out.
        SPHERE = 1
    };
}
