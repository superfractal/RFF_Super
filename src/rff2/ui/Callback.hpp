//
// Created by Merutilm on 2025-05-19.
// Modified by AI; earlier exact modification date unavailable.
// Modified by Opus 5 on 2026-08-06, 2026-09-04
// Modified by GPT-5 on 2026-08-21, 2026-08-23
// Modified by GPT-6 on 2026-09-22, 2026-09-23, 2026-09-24
//

#pragma once
#include <codecvt>
#include <cmath>
#include <locale>
#include <windows.h>
#include <string>
#include <cwchar>
#include <cwctype>
#include <limits>
#include <stdexcept>
#include "../attr/NumericSettingLimits.hpp"

namespace merutilm::rff2 {
    namespace ValidCondition {
        constexpr auto POSITIVE_U_CHAR = [](const unsigned char &e) { return e > 0; };
        constexpr auto ALL_U_CHAR = [](const unsigned char &) { return true; };
        constexpr auto ALL_U_SHORT = [](const unsigned short &) { return true; };
        constexpr auto ALL_U_LONG = [](const unsigned long) { return true; };
        constexpr auto ALL_U_LONG_LONG = [](const unsigned long long) { return true; };
        constexpr auto FLOAT_ZERO_TO_ONE = [](const float &e) { return e >= 0 && e <= 1; };
        constexpr auto FLOAT_DEGREE = [](const float &e) { return e >= 0 && e < 360; };
        constexpr auto ALL_FLOAT = [](const float &e) { return std::isfinite(e); };
        constexpr auto POSITIVE_FLOAT = [](const float &e) { return e > 0; };
        constexpr auto POSITIVE_FLOAT_ZERO = [](const float &e) { return e >= 0; };

        // Bounds a slider's text field to that slider's own range. Without it a typed number
        // outside the range is stored while the thumb stays clamped at an end, so the thumb no
        // longer reflects the value and the next drag jumps it.
        inline auto floatInRange(const float min, const float max) {
            return [min, max](const float &e) { return std::isfinite(e) && e >= min && e <= max; };
        }
    }

    namespace Callback {
        constexpr auto NOTHING = [] {
            /*NO CALLBACKS*/
        };
    }

    namespace Parser {
        inline void requireComplete(const std::wstring &s, size_t parsed) {
            while (parsed < s.size() && std::iswspace(s[parsed])) {
                ++parsed;
            }
            if (parsed != s.size()) {
                throw std::invalid_argument("Unexpected characters after a number");
            }
        }
        template <class T> T unsignedInteger(const std::wstring &s) {
            const auto first = s.find_first_not_of(L" \t\r\n\f\v");
            if (first == std::wstring::npos || s[first] == L'-') {
                throw std::invalid_argument("Enter a nonnegative whole number");
            }
            size_t parsed = 0;
            const auto value = std::stoull(s, &parsed);
            requireComplete(s, parsed);
            if (value > std::numeric_limits<T>::max()) {
                throw std::out_of_range("Whole number is outside the supported range");
            }
            return static_cast<T>(value);
        }
        constexpr auto STRING = [](const std::wstring &s) {
            const int size = WideCharToMultiByte(CP_UTF8, 0, s.data(), -1, nullptr, 0, nullptr, nullptr);
            std::string str(size, 0);
            WideCharToMultiByte(CP_UTF8, 0, s.data(), -1, &str[0], size, nullptr, nullptr);
            str.pop_back();
            return str;
        };
        constexpr auto U_CHAR = [](const std::wstring &s) {
            return unsignedInteger<unsigned char>(s);
        };
        constexpr auto U_SHORT = [](const std::wstring &s) {
            return unsignedInteger<unsigned short>(s);
        };
        constexpr auto U_LONG = [](const std::wstring &s) {
            return unsignedInteger<unsigned long>(s);
        };
        constexpr auto U_LONG_LONG = [](const std::wstring &s) {
            return unsignedInteger<unsigned long long>(s);
        };
        constexpr auto FLOAT = [](const std::wstring &s) {
            const auto first = s.find_first_not_of(L" \t\r\n\f\v");
            const auto last = s.find_last_not_of(L" \t\r\n\f\v");
            if (first == std::wstring::npos) throw std::invalid_argument("Enter a number");
            for (size_t i = first; i <= last; ++i) {
                const wchar_t c = s[i];
                if ((c < L'0' || c > L'9') && c != L'+' && c != L'-' && c != L'.' && c != L'e' && c != L'E') {
                    throw std::invalid_argument("Enter a finite decimal number");
                }
            }
            size_t parsed = 0;
            const auto value = std::stof(s, &parsed);
            requireComplete(s, parsed);
            if (!NumericSettingLimits::Range{-std::numeric_limits<float>::max(),
                                             std::numeric_limits<float>::max()}(value)) {
                throw std::invalid_argument("Enter a finite number");
            }
            return value;
        };
    }

    namespace Unparser {
        constexpr auto STRING = [](const std::string &s) {
            const int size = MultiByteToWideChar(CP_UTF8, 0, s.data(), -1, nullptr, 0);
            std::wstring str(size, 0);
            MultiByteToWideChar(CP_UTF8, 0, s.data(), -1, &str[0], size);
            str.pop_back();
            return str;
        };
        constexpr auto U_CHAR = [](const unsigned char &s) { return std::to_wstring(s); };
        constexpr auto U_SHORT = [](const unsigned short &s) { return std::to_wstring(s); };
        constexpr auto U_LONG = [](const unsigned long &s) { return std::to_wstring(s); };
        constexpr auto U_LONG_LONG = [](const unsigned long long &s) { return std::to_wstring(s); };
        constexpr auto FLOAT = [](const float &s) { return std::to_wstring(s); };
        // Exponent notation for fields whose range spans many decades: %.2e above 1e3 keeps a
        // large value one short token (1.00e+06) where std::to_wstring writes the whole 30-digit
        // expansion plus six decimals, %.3g below it. Shared with the Cycle Length fields.
        constexpr auto FLOAT_SCIENTIFIC = [](const float &s) {
            wchar_t buf[32];
            if (s >= 1e3f || s <= -1e3f) {
                swprintf(buf, 32, L"%.2e", s);
            } else {
                swprintf(buf, 32, L"%.3g", s);
            }
            return std::wstring(buf);
        };
        // Formats a float with a fixed number of decimals (decimals match the arrow-key step).
        inline auto floatFixed(const int decimals) {
            return [decimals](const float &s) {
                const auto precision = static_cast<size_t>(decimals < 0 ? 6 : decimals);
                // Reserve sign, decimal point, terminator, and the extra slot required by the MinGW formatter.
                std::wstring text(precision + std::numeric_limits<float>::max_exponent10 + 5, L'\0');
                const int written = swprintf(text.data(), text.size(), L"%.*f", decimals,
                                             s == 0.0f ? 0.0f : s);
                if (written < 0 || static_cast<size_t>(written) >= text.size()) {
                    throw std::runtime_error("Failed to format a fixed-point value");
                }
                text.resize(static_cast<size_t>(written));
                return text;
            };
        }
        // Formats a float with up to `decimals` decimals, trimming trailing zeros and a bare decimal point.
        inline auto floatTrim(const int decimals) {
            return [formatter = floatFixed(decimals)](const float &s) {
                std::wstring str = formatter(s);
                if (str.find(L'.') != std::wstring::npos) {
                    str.erase(str.find_last_not_of(L'0') + 1);
                    if (!str.empty() && str.back() == L'.') str.pop_back();
                }
                return str;
            };
        }
    }
}
