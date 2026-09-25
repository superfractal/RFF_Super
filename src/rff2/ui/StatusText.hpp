// Modified by GPT-6 on 2026-09-26
#pragma once

#include <cstdint>
#include <string>

namespace merutilm::rff2::StatusText {
    inline std::wstring grouped(const uint64_t value) {
        auto text = std::to_wstring(value);
        for (size_t position = text.size(); position > 3;) {
            position -= 3;
            text.insert(position, 1, L',');
        }
        return text;
    }

}
