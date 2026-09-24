//
// Created by Merutilm on 2025-05-08.
// Modified by GPT-6 on 2026-09-23
//

#pragma once

namespace merutilm::rff2 {
    struct VidAnimationAttribute {
        // Legacy export settings are stored in this order in the configuration stream.
        float overZoom;
        bool showText;
        // Keyframes per second when no timeline speed track overrides the constant speed.
        float mps;
    };
}
