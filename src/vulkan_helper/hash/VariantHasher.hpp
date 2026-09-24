//
// Created by Merutilm on 2025-09-01.
// Modified by GPT-6 on 2026-09-23
//

#pragma once

#include <cstddef>
#include <functional>
#include <variant>

namespace merutilm::vkh {
    struct VariantHasher {
        using is_transparent = void;

        template<typename... Alternatives>
        size_t operator()(const std::variant<Alternatives...> &value) const {
            auto visitor = []<typename T>(const T &alternative) {
                return std::hash<T>{}(alternative);
            };
            return std::visit(visitor, value);
        }
    };
}
