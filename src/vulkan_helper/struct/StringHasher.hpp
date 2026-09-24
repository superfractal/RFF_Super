//
// Created by Merutilm on 2025-07-09.
// Modified by GPT-6 on 2026-09-23
//

#pragma once

#include <cstddef>
#include <functional>
#include <string>

#include "../core/vkh_base.hpp"

namespace merutilm::vkh {
    struct StringHasher {
        using is_transparent = void;

        size_t operator()(const std::string &key) const {
            return std::hash<std::string>{}(key);
        }
    };
}
