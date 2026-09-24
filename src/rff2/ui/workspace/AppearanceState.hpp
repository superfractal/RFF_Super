//
// Modified by GPT-6 on 2026-09-14, 2026-09-23
//

#pragma once

#include "../../attr/ShaderAttribute.h"
#include <string>

namespace merutilm::rff2::workspace {
    struct AppearanceState {
        static ShaderAttribute defaults();
        static void keepMotion(const ShaderAttribute& from, ShaderAttribute& to);
        static std::string key(const ShaderAttribute& shader);
    };
}
