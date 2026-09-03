//
// Created by Merutilm on 2025-05-10.
// Modified by GPT-5 on 2026-08-23, 2026-08-27.
// Modified by Opus 5 on 2026-09-03
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
            return static_cast<float>(std::chrono::system_clock::now().time_since_epoch().count() - Constants::Fractal::INIT_TIME)
                   / 1e9;
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

        static std::wstring joinString(const std::wstring &delimiter, const std::vector<std::wstring> &arr) {
            std::wostringstream v;
            for (int i = 0; i < arr.size(); ++i) {
                if (i > 0) {
                    v << delimiter;
                }
                v << arr[i];
            }
            return v.str();
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
        static bool naturalLess(const std::wstring &a, const std::wstring &b) {
            size_t i = 0;
            size_t j = 0;
            while (i < a.size() && j < b.size()) {
                if (std::iswdigit(a[i]) && std::iswdigit(b[j])) {
                    size_t ea = i;
                    size_t eb = j;
                    while (ea < a.size() && std::iswdigit(a[ea])) ++ea;
                    while (eb < b.size() && std::iswdigit(b[eb])) ++eb;
                    // Leading zeros carry no value, so they are dropped before the digits are compared.
                    std::wstring_view na(a.data() + i, ea - i);
                    std::wstring_view nb(b.data() + j, eb - j);
                    na.remove_prefix(std::min(na.find_first_not_of(L'0'), na.size() - 1));
                    nb.remove_prefix(std::min(nb.find_first_not_of(L'0'), nb.size() - 1));
                    if (na.size() != nb.size()) {
                        return na.size() < nb.size();
                    }
                    if (na != nb) {
                        return na < nb;
                    }
                    i = ea;
                    j = eb;
                    continue;
                }
                const wchar_t ca = std::towlower(a[i]);
                if (const wchar_t cb = std::towlower(b[j]); ca != cb) {
                    return ca < cb;
                }
                ++i;
                ++j;
            }
            return a.size() - i < b.size() - j;
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
