//
// Created by Merutilm on 2025-05-10.
// Modified by GPT-5 on 2026-08-23, 2026-08-27
// Modified by Opus 5 on 2026-09-03
// Modified by GPT-6 on 2026-09-21, 2026-09-22, 2026-09-23
//

#pragma once

#include <algorithm>
#include <cwctype>
#include <string>
#include <string_view>
#include <ctime>
#include <filesystem>
#include <system_error>

#include "../constants/Constants.hpp"

namespace merutilm::rff2 {
    struct Utilities {
        Utilities() = delete;

        template<class Clock, class Duration>
        static std::wstring elapsed_time(const std::chrono::time_point<Clock, Duration> start) {
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start);
            const auto hms = std::chrono::hh_mm_ss(elapsed);
            return std::format(L"T : {:02d}:{:02d}:{:02d}:{:03d}", hms.hours().count(),
                                         hms.minutes().count(), hms.seconds().count(), hms.subseconds().count());
        }

        static float getCurrentTime() {
            return std::chrono::duration<float>(std::chrono::steady_clock::now() - Constants::Fractal::INIT_TIME)
                .count();
        }


        static std::filesystem::path getDefaultPath() {
            constexpr size_t MAX_MODULE_PATH_CHARS = 32768;
            std::wstring buffer(MAX_PATH, L'\0');
            for (;;) {
                const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
                if (length == 0) {
                    const DWORD error = GetLastError();
                    throw std::system_error(static_cast<int>(error == ERROR_SUCCESS ? ERROR_GEN_FAILURE : error),
                                            std::system_category(), "GetModuleFileNameW");
                }
                if (length < buffer.size()) {
                    return std::filesystem::path(std::wstring(buffer.data(), length)).parent_path().parent_path();
                }
                if (buffer.size() >= MAX_MODULE_PATH_CHARS) {
                    throw std::system_error(ERROR_INSUFFICIENT_BUFFER, std::system_category(),
                                            "GetModuleFileNameW");
                }
                buffer.resize(std::min(buffer.size() * 2, MAX_MODULE_PATH_CHARS), L'\0');
            }
        }

        static bool endsWith(const std::wstring &str, const std::wstring &suffix) {
            return str.size() >= suffix.size() && std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
        }

        static std::vector<std::wstring> split(const std::wstring &input, const wchar_t delimiter) {
            std::vector<std::wstring> split;
            std::wstringstream ss(input);
            std::wstring val;

            while (getline(ss, val, delimiter)) {
                split.push_back(val);
            }

            return split;
        }

        // Orders names the way a file manager does: a run of digits counts as its value, so 0009
        // comes before 0010 and map2 before map10, whatever padding the names carry.
        static bool naturalLess(const std::wstring &left, const std::wstring &right) {
            size_t leftIndex = 0;
            size_t rightIndex = 0;
            while (leftIndex < left.size() && rightIndex < right.size()) {
                if (std::iswdigit(left[leftIndex]) && std::iswdigit(right[rightIndex])) {
                    size_t leftEnd = leftIndex;
                    size_t rightEnd = rightIndex;
                    while (leftEnd < left.size() && std::iswdigit(left[leftEnd])) {
                        ++leftEnd;
                    }
                    while (rightEnd < right.size() && std::iswdigit(right[rightEnd])) {
                        ++rightEnd;
                    }
                    // Leading zeros carry no value, so they are dropped before the digits are compared.
                    std::wstring_view leftDigits(left.data() + leftIndex, leftEnd - leftIndex);
                    std::wstring_view rightDigits(right.data() + rightIndex, rightEnd - rightIndex);
                    leftDigits.remove_prefix(std::min(leftDigits.find_first_not_of(L'0'), leftDigits.size() - 1));
                    rightDigits.remove_prefix(std::min(rightDigits.find_first_not_of(L'0'), rightDigits.size() - 1));
                    if (leftDigits.size() != rightDigits.size()) {
                        return leftDigits.size() < rightDigits.size();
                    }
                    if (leftDigits != rightDigits) {
                        return leftDigits < rightDigits;
                    }
                    leftIndex = leftEnd;
                    rightIndex = rightEnd;
                    continue;
                }
                const wchar_t leftCharacter = std::towlower(left[leftIndex]);
                if (const wchar_t rightCharacter = std::towlower(right[rightIndex]); leftCharacter != rightCharacter) {
                    return leftCharacter < rightCharacter;
                }
                ++leftIndex;
                ++rightIndex;
            }
            return left.size() - leftIndex < right.size() - rightIndex;
        }

        // The lower-cased extension of a path, with its dot, for comparing against a fixed list.
        static std::wstring lowerExtension(const std::filesystem::path &path) {
            std::wstring ext = path.extension().wstring();
            std::ranges::transform(ext, ext.begin(), [](const wchar_t c) { return std::towlower(c); });
            return ext;
        }

        static int getRefreshInterval(const float logZoom) {
            return std::max(1, static_cast<int>(100000.0 / logZoom));
        };
    };
}
