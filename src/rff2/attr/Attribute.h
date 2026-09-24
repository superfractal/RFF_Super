//
// Modified by GPT-6 on 2026-09-11, 2026-09-23, 2026-09-25
//

#pragma once

#include "FractalAttribute.h"
#include "RenderAttribute.h"
#include "ShaderAttribute.h"
#include "VideoAttribute.h"

namespace merutilm::rff2 {
    struct Attribute final {
        FractalAttribute fractal;
        RenderAttribute render;
        ShaderAttribute shader;
        VideoAttribute video;
    };
}
