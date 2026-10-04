//
// Modified by GPT-6 on 2026-09-27
//

#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <random>

namespace merutilm::rff2 {
    enum class LocalAiExplorationRoute { Free, Needle, Valley };

    class LocalAiRouteDeck {
        std::mt19937 random;
        std::array<LocalAiExplorationRoute, 3> routes;
        size_t cursor = 3;
        std::array<int, 3> lastSide{};
    public:
        explicit LocalAiRouteDeck(uint32_t seed = std::random_device{}()) : random(seed) {}
        struct Choice { LocalAiExplorationRoute route; int side; };
        Choice next() {
            if (cursor == routes.size()) {
                routes = {LocalAiExplorationRoute::Free, LocalAiExplorationRoute::Needle, LocalAiExplorationRoute::Valley};
                std::shuffle(routes.begin(), routes.end(), random);
                cursor = 0;
            }
            const auto route = routes[cursor++];
            auto &side = lastSide[static_cast<size_t>(route)];
            side = side ? -side : (std::uniform_int_distribution<int>(0, 1)(random) ? 1 : -1);
            return {route, side};
        }
    };
}
