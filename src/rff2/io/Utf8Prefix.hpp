//
// Modified by GPT-6 on 2026-09-29
//

#pragma once
#include <algorithm>
#include <string>
#include <string_view>

namespace merutilm::rff2 {
    inline std::string utf8Prefix(const std::string_view text, const size_t maximumBytes) {
        size_t length = std::min(text.size(), maximumBytes);
        while (length > 0 && length < text.size() &&
               (static_cast<unsigned char>(text[length]) & 0xc0) == 0x80) {
            --length;
        }
        return std::string(text.substr(0, length));
    }
}
