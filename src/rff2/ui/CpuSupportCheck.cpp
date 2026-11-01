//
// Modified by Opus 5.5 on 2026-10-12, 2026-10-27
//

// Built for plain x86-64 (CMakeLists.txt), and calls nothing inline from the C++ library: it has to run on any processor.
#include <cwchar>
#include <windows.h>

#include "CpuFeatures.hpp"

namespace merutilm::rff2 {
    namespace {
        // The name of the set when this processor or Windows lacks it, nullptr when it is there.
        const wchar_t *missingName(const std::uint32_t bit) {
            switch (bit) {
                case CpuFeatures::SSE3: return __builtin_cpu_supports("sse3") ? nullptr : L"SSE3";
                case CpuFeatures::SSSE3: return __builtin_cpu_supports("ssse3") ? nullptr : L"SSSE3";
                case CpuFeatures::SSE4_1: return __builtin_cpu_supports("sse4.1") ? nullptr : L"SSE4.1";
                case CpuFeatures::SSE4_2: return __builtin_cpu_supports("sse4.2") ? nullptr : L"SSE4.2";
                case CpuFeatures::POPCNT: return __builtin_cpu_supports("popcnt") ? nullptr : L"POPCNT";
                case CpuFeatures::AVX: return __builtin_cpu_supports("avx") ? nullptr : L"AVX";
                case CpuFeatures::AVX2: return __builtin_cpu_supports("avx2") ? nullptr : L"AVX2";
                case CpuFeatures::FMA: return __builtin_cpu_supports("fma") ? nullptr : L"FMA";
                case CpuFeatures::F16C: return __builtin_cpu_supports("f16c") ? nullptr : L"F16C";
                case CpuFeatures::BMI: return __builtin_cpu_supports("bmi") ? nullptr : L"BMI1";
                case CpuFeatures::BMI2: return __builtin_cpu_supports("bmi2") ? nullptr : L"BMI2";
                case CpuFeatures::LZCNT: return __builtin_cpu_supports("lzcnt") ? nullptr : L"LZCNT";
                case CpuFeatures::MOVBE: return __builtin_cpu_supports("movbe") ? nullptr : L"MOVBE";
                case CpuFeatures::ADX: return __builtin_cpu_supports("adx") ? nullptr : L"ADX";
                case CpuFeatures::AVX512F: return __builtin_cpu_supports("avx512f") ? nullptr : L"AVX-512F";
                case CpuFeatures::AVX512CD: return __builtin_cpu_supports("avx512cd") ? nullptr : L"AVX-512CD";
                case CpuFeatures::AVX512BW: return __builtin_cpu_supports("avx512bw") ? nullptr : L"AVX-512BW";
                case CpuFeatures::AVX512DQ: return __builtin_cpu_supports("avx512dq") ? nullptr : L"AVX-512DQ";
                case CpuFeatures::AVX512VL: return __builtin_cpu_supports("avx512vl") ? nullptr : L"AVX-512VL";
                case CpuFeatures::AVX512VBMI: return __builtin_cpu_supports("avx512vbmi") ? nullptr : L"AVX-512VBMI";
                case CpuFeatures::AVX512VBMI2: return __builtin_cpu_supports("avx512vbmi2") ? nullptr : L"AVX-512VBMI2";
                case CpuFeatures::AVX512IFMA: return __builtin_cpu_supports("avx512ifma") ? nullptr : L"AVX-512IFMA";
                case CpuFeatures::AVX512VNNI: return __builtin_cpu_supports("avx512vnni") ? nullptr : L"AVX-512VNNI";
                case CpuFeatures::AVX512BITALG: return __builtin_cpu_supports("avx512bitalg") ? nullptr : L"AVX-512BITALG";
                case CpuFeatures::AVX512VPOPCNTDQ:
                    return __builtin_cpu_supports("avx512vpopcntdq") ? nullptr : L"AVX-512VPOPCNTDQ";
                case CpuFeatures::GFNI: return __builtin_cpu_supports("gfni") ? nullptr : L"GFNI";
                default: return nullptr;
            }
        }

        // Catches an instruction the check above does not cover, which would otherwise end the program without a word.
        LONG WINAPI unsupportedCpuFilter(EXCEPTION_POINTERS *info) {
            if (info && info->ExceptionRecord &&
                info->ExceptionRecord->ExceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION) {
                // Both languages: this can fire before the preferences that name the language are read.
                MessageBoxW(nullptr,
                    L"Your CPU does not support the instruction set (AVX / FMA, etc.) this program requires.\n\n"
                    L"This build is optimized for the build machine's CPU (-march=native).\n"
                    L"Run on a newer CPU, or rebuild without -march=native for better compatibility.\n\n"
                    L"このCPUは、このプログラムに必要な命令セット（AVX / FMAなど）に対応していません。\n\n"
                    L"このビルドは、ビルドしたコンピューターのCPU向けに最適化されています（-march=native）。\n"
                    L"新しいCPUで実行するか、互換性を高めるために -march=native を付けずにビルドし直してください。",
                    L"Unsupported CPU / 非対応のCPU",
                    MB_OK | MB_ICONERROR);
                return EXCEPTION_EXECUTE_HANDLER;
            }
            return EXCEPTION_CONTINUE_SEARCH;
        }

        // Priority 101 runs ahead of every other static initializer, some of which already use AVX.
        __attribute__((constructor(101), used)) void checkCpuSupport() {
            SetUnhandledExceptionFilter(unsupportedCpuFilter);
            __builtin_cpu_init();
            constexpr size_t CAPACITY = 1024;
            wchar_t missing[CAPACITY] = L"";
            for (std::uint32_t bit = 0; bit < CpuFeatures::COUNT; ++bit) {
                if ((CpuFeatures::programFeatures >> bit & 1u) == 0) {
                    continue;
                }
                if (const wchar_t *name = missingName(bit)) {
                    if (missing[0] != L'\0') {
                        wcsncat(missing, L", ", CAPACITY - wcslen(missing) - 1);
                    }
                    wcsncat(missing, name, CAPACITY - wcslen(missing) - 1);
                }
            }
            if (missing[0] == L'\0') {
                return;
            }
            // Both languages: this runs before the preferences that name the language are read.
            constexpr size_t TEXT_CAPACITY = CAPACITY * 3;
            wchar_t text[TEXT_CAPACITY] = L"";
            wcsncat(text, L"This copy of RFF_Super was built for a newer processor and cannot run on this one.\n\n"
                          L"Missing instruction sets: ", TEXT_CAPACITY - 1);
            wcsncat(text, missing, TEXT_CAPACITY - wcslen(text) - 1);
            wcsncat(text, L"\n\nIt was compiled for the build computer's processor (-march=native). Use a build made "
                          L"for general x86-64 processors, or build RFF_Super on this computer.\n\n"
                          L"このRFF_Superは新しいプロセッサー向けにビルドされているため、このコンピューターでは実行できません。\n\n"
                          L"不足している命令セット：",
                    TEXT_CAPACITY - wcslen(text) - 1);
            wcsncat(text, missing, TEXT_CAPACITY - wcslen(text) - 1);
            wcsncat(text, L"\n\nビルドしたコンピューターのプロセッサー向けにコンパイルされています（-march=native）。"
                          L"一般的なx86-64プロセッサー向けのビルドを使うか、このコンピューターでRFF_Superをビルドしてください。",
                    TEXT_CAPACITY - wcslen(text) - 1);
            MessageBoxW(nullptr, text, L"Unsupported CPU / 非対応のCPU", MB_OK | MB_ICONERROR);
            ExitProcess(1);
        }
    }
}
