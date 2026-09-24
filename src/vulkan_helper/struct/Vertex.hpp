//
// Created by Merutilm on 2025-07-15.
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include "../core/vkh_base.hpp"

namespace merutilm::vkh {
    struct Vertex {
        glm::vec3 position = {};
        glm::vec3 color = {};
        glm::vec2 texcoord = {};

        static Vertex generate(const glm::vec3 &position, const glm::vec3 &color, const glm::vec2 &texcoord) {
            const glm::vec3 flippedPosition{position.x, -position.y, position.z};
            return Vertex{flippedPosition, color, texcoord};
        }
    };
}
