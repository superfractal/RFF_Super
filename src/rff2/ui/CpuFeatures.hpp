//
// Modified by Opus 5.5 on 2026-10-12
//

#pragma once
#include <cstdint>

namespace merutilm::rff2::CpuFeatures {
    // The instruction sets the compiler may emit by itself under -march; crypto and the rarely emitted sets are left out.
    enum Bit : std::uint32_t {
        SSE3, SSSE3, SSE4_1, SSE4_2, POPCNT, AVX, AVX2, FMA, F16C, BMI, BMI2, LZCNT, MOVBE, ADX,
        AVX512F, AVX512CD, AVX512BW, AVX512DQ, AVX512VL, AVX512VBMI, AVX512VBMI2, AVX512IFMA,
        AVX512VNNI, AVX512BITALG, AVX512VPOPCNTDQ, GFNI, COUNT
    };

    // The sets the including translation unit was compiled for, read from the compiler's predefined macros.
    constexpr std::uint64_t compiledHere() {
        std::uint64_t bits = 0;
        const auto set = [&bits](const Bit bit) { bits |= std::uint64_t{1} << bit; };
#ifdef __SSE3__
        set(SSE3);
#endif
#ifdef __SSSE3__
        set(SSSE3);
#endif
#ifdef __SSE4_1__
        set(SSE4_1);
#endif
#ifdef __SSE4_2__
        set(SSE4_2);
#endif
#ifdef __POPCNT__
        set(POPCNT);
#endif
#ifdef __AVX__
        set(AVX);
#endif
#ifdef __AVX2__
        set(AVX2);
#endif
#ifdef __FMA__
        set(FMA);
#endif
#ifdef __F16C__
        set(F16C);
#endif
#ifdef __BMI__
        set(BMI);
#endif
#ifdef __BMI2__
        set(BMI2);
#endif
#ifdef __LZCNT__
        set(LZCNT);
#endif
#ifdef __MOVBE__
        set(MOVBE);
#endif
#ifdef __ADX__
        set(ADX);
#endif
#ifdef __AVX512F__
        set(AVX512F);
#endif
#ifdef __AVX512CD__
        set(AVX512CD);
#endif
#ifdef __AVX512BW__
        set(AVX512BW);
#endif
#ifdef __AVX512DQ__
        set(AVX512DQ);
#endif
#ifdef __AVX512VL__
        set(AVX512VL);
#endif
#ifdef __AVX512VBMI__
        set(AVX512VBMI);
#endif
#ifdef __AVX512VBMI2__
        set(AVX512VBMI2);
#endif
#ifdef __AVX512IFMA__
        set(AVX512IFMA);
#endif
#ifdef __AVX512VNNI__
        set(AVX512VNNI);
#endif
#ifdef __AVX512BITALG__
        set(AVX512BITALG);
#endif
#ifdef __AVX512VPOPCNTDQ__
        set(AVX512VPOPCNTDQ);
#endif
#ifdef __GFNI__
        set(GFNI);
#endif
        return bits;
    }

    // Defined in Main.cpp, which is built with the program's own -march, so it names what the program needs.
    extern const std::uint64_t programFeatures;
}
