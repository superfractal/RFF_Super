//
// Modified by GPT-6 on 2026-09-14, 2026-09-16, 2026-09-22
//

#pragma once
#include "SurfacePresentation.hpp"
#include "SurfaceColorRegistry.hpp"
#include <cwchar>

namespace merutilm::rff2::workspace {
    struct SurfaceInspectorItems {
        static constexpr long basic = 1, detail = 2, category = 3, editor = 4, footer = 5, moreResets = 6,
                              removeEffect = 7, numbers = 100, colors = 1000, forms = 100000;
        static long id(const SurfaceParameter &parameter) {
            return numbers + long(&parameter - surfaceParameters().data());
        }
        static long id(const SurfaceColor &color) {
            return colors + long(&color - surfaceColors().data());
        }
        static const SurfaceParameter *parameter(long id) {
            if (id >= numbers && id < numbers + long(surfaceParameters().size())) {
                return &surfaceParameters()[id - numbers];
            }
            return nullptr;
        }
        static const SurfaceColor *color(long id) {
            if (id >= colors && id < colors + long(surfaceColors().size())) {
                return &surfaceColors()[id - colors];
            }
            return nullptr;
        }
        static std::wstring range(const SurfaceParameter &parameter) {
            if (parameter.numberKind == SurfaceParameter::NumberKind::ANGLE) {
                return L"Enter an angle: 0 <= value < 360.";
            }
            wchar_t hint[128];
            swprintf(hint, 128,
                     parameter.isInteger() ? L"Enter a whole number from %.6g to %.6g."
                                           : L"Enter a number from %.6g to %.6g.",
                     parameter.minimum, parameter.maximum);
            return hint;
        }
    };
}
