//
// Modified by GPT-6 on 2026-09-13, 2026-09-18, 2026-09-22
//

#pragma once

namespace merutilm::rff2::workspace {
    struct InspectorLayout {
        static constexpr int width=340;
        static constexpr int inset=20;
        static constexpr int rightInset=32;
        static constexpr int right=width-rightInset;
        static constexpr int trackWidth=right-inset;
        static constexpr int fieldWidth=74;
        static constexpr int fieldHeight=26;
    };
}
