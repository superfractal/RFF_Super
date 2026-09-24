//
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include "../attr/VidTimelineAttribute.h"
#include "../attr/ShaderAttribute.h"
#include <nlohmann/json.hpp>
#include <string_view>

namespace merutilm::rff2 {
    struct TimelineJsonIO {
        static constexpr size_t maximumBytes = 16 * 1024 * 1024;
        static nlohmann::json document(const VidTimelineAttribute &timeline);
        static VidTimelineAttribute parse(std::string_view text);
        static std::string systemPrompt(const ShaderAttribute &shader);
    };
}
