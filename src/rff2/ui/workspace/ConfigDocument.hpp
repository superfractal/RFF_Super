//
// Modified by GPT-6 on 2026-09-14, 2026-09-23
//

#pragma once

#include "../../io/ConfigIO.h"

#include <cstdint>
#include <sstream>
#include <string>
#include <utility>

namespace merutilm::rff2::workspace {
    class ConfigDocument {
        std::string savedSnapshot;
        bool hasSavedSnapshot = false;

        static std::string encode(const Attribute& attribute, uint16_t width, uint16_t height) {
            std::ostringstream output(std::ios::out | std::ios::binary);
            output.exceptions(std::ios::badbit | std::ios::failbit);
            ConfigIO::write(output, attribute, width, height);
            return std::move(output).str();
        }

    public:
        void accept(const Attribute& attribute, uint16_t width, uint16_t height) {
            auto serialized = encode(attribute, width, height);
            savedSnapshot = std::move(serialized);
            hasSavedSnapshot = true;
        }

        bool isDirty(const Attribute& attribute, uint16_t width, uint16_t height) const {
            return hasSavedSnapshot && encode(attribute, width, height) != savedSnapshot;
        }
    };
}
