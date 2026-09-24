//
// Modified by GPT-6 on 2026-09-17, 2026-09-18, 2026-09-22, 2026-09-23
//

#pragma once
#include <array>
#include <cstddef>
#include <ostream>
#include <sstream>
#include <string>

namespace merutilm::rff2::workspace {
    struct AppearancePanelLayout {
        struct Slot {
            bool open = false;
            int module = 1;
            int side = 0;
            bool operator==(const Slot &) const = default;
        };
        bool mainLeft = false;
        bool navigationRight = false;
        bool navigationOuter = true;
        int layerHeight = 300;
        int sideWidth = 340;
        int bottomHeight = 320;
        std::array<Slot, 4> slots{};
        bool operator==(const AppearancePanelLayout &) const = default;

        void read(const std::string &line) {
            std::istringstream input(line);
            std::string key;
            std::getline(input, key, '=');
            int parsedValue;

            if (key == "appearance-main-left") {
                if (input >> parsedValue && (parsedValue == 0 || parsedValue == 1)) {
                    mainLeft = parsedValue;
                }
                return;
            }
            if (key == "navigation-right") {
                if (input >> parsedValue && (parsedValue == 0 || parsedValue == 1)) {
                    navigationRight = parsedValue;
                }
                return;
            }
            if (key == "navigation-outer") {
                if (input >> parsedValue && (parsedValue == 0 || parsedValue == 1)) {
                    navigationOuter = parsedValue;
                }
                return;
            }
            if (key == "layer-height") {
                if (input >> parsedValue && parsedValue >= 200 && parsedValue <= 800) {
                    layerHeight = parsedValue;
                }
                return;
            }
            if (key == "appearance-side-width") {
                if (input >> parsedValue && parsedValue >= 300 && parsedValue <= 640) {
                    sideWidth = parsedValue;
                }
                return;
            }
            if (key == "appearance-bottom-height") {
                if (input >> parsedValue && parsedValue >= 240 && parsedValue <= 800) {
                    bottomHeight = parsedValue;
                }
                return;
            }
            if (key == "appearance-panel") {
                int module;
                int side;
                if (input >> parsedValue >> module >> side && parsedValue >= 0 &&
                    parsedValue < static_cast<int>(slots.size()) &&
                    (module == 1 || (module >= 10 && module <= 16)) &&
                    side >= 0 && side <= 2) {
                    slots[parsedValue] = {true, module, side};
                }
            }
        }

        void write(std::ostream &output) const {
            output << "navigation-right=" << navigationRight << '\n';
            output << "navigation-outer=" << navigationOuter << '\n';
            output << "appearance-main-left=" << mainLeft << '\n'
                   << "layer-height=" << layerHeight << '\n'
                   << "appearance-side-width=" << sideWidth << '\n'
                   << "appearance-bottom-height=" << bottomHeight << '\n';
            for (std::size_t slotIndex = 0; slotIndex < slots.size(); ++slotIndex) {
                const Slot &slot = slots[slotIndex];
                if (slot.open) {
                    output << "appearance-panel=" << slotIndex << ' ' << slot.module << ' '
                           << slot.side << '\n';
                }
            }
        }
    };
}
