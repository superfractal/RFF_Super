//
// Created by Opus 5.5 on 2026-10-08.
//

#pragma once

#include <algorithm>
#include <cmath>

#include "../attr/VidHdrTransfer.h"

// SMPTE ST 2084 and ARIB STD-B67 transfer functions, acknowledged in NOTICE; moved here from VideoWindow.cpp unchanged.
namespace merutilm::rff2::HdrTransfer {
    // A stored code value in [0, 1] to linear light: nits for PQ, scene light with 1.0 as the headroom for HLG.
    inline float decode(const float code, const VidHdrTransfer transfer) {
        const float v = std::clamp(code, 0.0f, 1.0f);
        if (transfer == VidHdrTransfer::PQ) {
            constexpr float m1 = 0.1593017578125f;
            constexpr float m2 = 78.84375f;
            constexpr float c1 = 0.8359375f;
            constexpr float c2 = 18.8515625f;
            constexpr float c3 = 18.6875f;
            const float p = std::pow(v, 1.0f / m2);
            const float y = std::max(p - c1, 0.0f) / std::max(c2 - c3 * p, 1e-6f);
            return std::pow(y, 1.0f / m1) * 10000.0f;
        }
        if (transfer == VidHdrTransfer::HLG) {
            constexpr float a = 0.17883277f;
            constexpr float b = 0.28466892f;
            constexpr float c = 0.55991073f;
            return v <= 0.5f ? v * v / 3.0f : (std::exp((v - c) / a) + b) / 12.0f;
        }
        return v;
    }

    // The inverse of decode; HLG above 1.0 comes out past 1.0, so callers clamp the code they store.
    inline float encode(const float linear, const VidHdrTransfer transfer) {
        const float v = std::max(linear, 0.0f);
        if (transfer == VidHdrTransfer::PQ) {
            constexpr float m1 = 0.1593017578125f;
            constexpr float m2 = 78.84375f;
            constexpr float c1 = 0.8359375f;
            constexpr float c2 = 18.8515625f;
            constexpr float c3 = 18.6875f;
            const float y = std::pow(std::clamp(v / 10000.0f, 0.0f, 1.0f), m1);
            return std::pow((c1 + c2 * y) / (1.0f + c3 * y), m2);
        }
        if (transfer == VidHdrTransfer::HLG) {
            constexpr float a = 0.17883277f;
            constexpr float b = 0.28466892f;
            constexpr float c = 0.55991073f;
            return v <= 1.0f / 12.0f ? std::sqrt(3.0f * v)
                                     : a * std::log(std::max(12.0f * v - b, 1e-6f)) + c;
        }
        return v;
    }
}
