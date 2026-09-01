//
// Created by Merutilm on 2025-05-10.
// Modified by GPT-5 on 2026-08-23, 2026-08-27.
//

#pragma once

#include <algorithm>
#include <string>
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

        static int getRefreshInterval(const float logZoom) {
            return std::max(1, static_cast<int>(100000.0 / logZoom));
        };
    };
}
