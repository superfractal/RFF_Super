//
// Created by Merutilm on 2025-05-19.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21, 2026-08-23.
// Modified by Opus 5 on 2026-08-06
//
#pragma once
#include <codecvt>
#include <cmath>
#include <locale>
#include <windows.h>
#include <string>
#include <cwchar>

namespace merutilm::rff2 {
    namespace ValidCondition {
        constexpr auto POSITIVE_CHAR = [](const char &e) { return e > 0; };
        constexpr auto NEGATIVE_CHAR = [](const char &e) { return e < 0; };
        constexpr auto POSITIVE_CHAR_ZERO = [](const char &e) { return e >= 0; };
        constexpr auto NEGATIVE_CHAR_ZERO = [](const char &e) { return e <= 0; };
        constexpr auto POSITIVE_U_CHAR = [](const unsigned char &e) { return e > 0; };
        constexpr auto ALL_U_CHAR = [](const unsigned char &) { return true; };
        constexpr auto POSITIVE_SHORT = [](const short &e) { return e > 0; };
        constexpr auto NEGATIVE_SHORT = [](const short &e) { return e < 0; };
        constexpr auto POSITIVE_SHORT_ZERO = [](const short &e) { return e >= 0; };
        constexpr auto NEGATIVE_SHORT_ZERO = [](const short &e) { return e <= 0; };
        constexpr auto POSITIVE_U_SHORT = [](const unsigned short &e) { return e > 0; };
        constexpr auto ALL_U_SHORT = [](const unsigned short &) { return true; };
        constexpr auto POSITIVE_INT = [](const int &e) { return e > 0; };
        constexpr auto NEGATIVE_INT = [](const int &e) { return e < 0; };
        constexpr auto POSITIVE_INT_ZERO = [](const int &e) { return e >= 0; };
        constexpr auto NEGATIVE_INT_ZERO = [](const int &e) { return e <= 0; };
        constexpr auto POSITIVE_LONG = [](const long &e) { return e > 0; };
        constexpr auto NEGATIVE_LONG = [](const long &e) { return e < 0; };
        constexpr auto POSITIVE_LONG_ZERO = [](const long &e) { return e >= 0; };
        constexpr auto NEGATIVE_LONG_ZERO = [](const long &e) { return e <= 0; };
        constexpr auto POSITIVE_LONG_LONG = [](const long long &e) { return e > 0; };
        constexpr auto NEGATIVE_LONG_LONG = [](const long long &e) { return e < 0; };
        constexpr auto POSITIVE_LONG_LONG_ZERO = [](const long long &e) { return e >= 0; };
        constexpr auto NEGATIVE_LONG_LONG_ZERO = [](const long long &e) { return e <= 0; };
        constexpr auto POSITIVE_U_LONG = [](const unsigned long &e) { return e > 0; };
        constexpr auto ALL_U_LONG = [](const unsigned long) { return true; };
        constexpr auto POSITIVE_U_LONG_LONG = [](const unsigned long long &e) { return e > 0; };
        constexpr auto ALL_U_LONG_LONG = [](const unsigned long long) { return true; };
        constexpr auto FLOAT_ZERO_TO_ONE = [](const float &e) { return e >= 0 && e <= 1; };
        constexpr auto FLOAT_DEGREE = [](const float &e) { return e >= 0 && e < 360; };
        constexpr auto ALL_FLOAT = [](const float &e) { return std::isfinite(e); };
        constexpr auto POSITIVE_FLOAT = [](const float &e) { return e > 0; };
        constexpr auto NEGATIVE_FLOAT = [](const float &e) { return e < 0; };
        constexpr auto POSITIVE_FLOAT_ZERO = [](const float &e) { return e >= 0; };
        constexpr auto NEGATIVE_FLOAT_ZERO = [](const float &e) { return e <= 0; };
        constexpr auto DOUBLE_ZERO_TO_ONE = [](const double &e) { return e >= 0 && e <= 1; };
        constexpr auto DOUBLE_DEGREE = [](const double &e) { return e >= 0 && e < 360; };
        constexpr auto ALL_DOUBLE = [](const double &) { return true; };
        constexpr auto POSITIVE_DOUBLE = [](const double &e) { return e > 0; };
        constexpr auto NEGATIVE_DOUBLE = [](const double &e) { return e < 0; };
        constexpr auto POSITIVE_DOUBLE_ZERO = [](const double &e) { return e >= 0; };
        constexpr auto NEGATIVE_DOUBLE_ZERO = [](const double &e) { return e <= 0; };
        constexpr auto POSITIVE_LONG_DOUBLE = [](const long double &e) { return e > 0; };
        constexpr auto NEGATIVE_LONG_DOUBLE = [](const long double &e) { return e < 0; };
        constexpr auto POSITIVE_LONG_DOUBLE_ZERO = [](const long double &e) { return e >= 0; };
        constexpr auto NEGATIVE_LONG_DOUBLE_ZERO = [](const long double &e) { return e <= 0; };

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
        constexpr auto STRING = [](const std::wstring &s) {
            const int size = WideCharToMultiByte(CP_UTF8, 0, s.data(), -1, nullptr, 0, nullptr, nullptr);
            std::string str(size, 0);
            WideCharToMultiByte(CP_UTF8, 0, s.data(), -1, &str[0], size, nullptr, nullptr);
            str.pop_back();
            return str;
        };
        constexpr auto WSTRING = [](const std::wstring &s) { return s; };
        constexpr auto CHAR = [](const std::wstring &s) { return static_cast<char>(std::stoi(s) & 0xFF); };
        constexpr auto U_CHAR = [](const std::wstring &s) { return static_cast<unsigned char>(std::stoul(s) & 0xFF); };
        constexpr auto SHORT = [](const std::wstring &s) { return static_cast<short>(std::stoi(s) & 0xFFFF); };
        constexpr auto U_SHORT = [](const std::wstring &s) {
            return static_cast<unsigned short>(std::stoul(s) & 0xFFFF);
        };
        constexpr auto INT = [](const std::wstring &s) { return std::stoi(s); };
        constexpr auto LONG = [](const std::wstring &s) { return std::stol(s); };
        constexpr auto LONG_LONG = [](const std::wstring &s) { return std::stoll(s); };
        constexpr auto U_LONG = [](const std::wstring &s) { return std::stoul(s); };
        constexpr auto U_LONG_LONG = [](const std::wstring &s) { return std::stoull(s); };
        constexpr auto FLOAT = [](const std::wstring &s) { return std::stof(s); };
        constexpr auto DOUBLE = [](const std::wstring &s) { return std::stod(s); };
        constexpr auto LONG_DOUBLE = [](const std::wstring &s) { return std::stold(s); };
    }

    namespace Unparser {
        constexpr auto STRING = [](const std::string &s) {
            const int size = MultiByteToWideChar(CP_UTF8, 0, s.data(), -1, nullptr, 0);
            std::wstring str(size, 0);
            MultiByteToWideChar(CP_UTF8, 0, s.data(), -1, &str[0], size);
            str.pop_back();
            return str;
        };
        constexpr auto WSTRING = [](const std::wstring &s) { return s; };
        constexpr auto CHAR = [](const char &s) { return std::to_wstring(s); };
        constexpr auto U_CHAR = [](const unsigned char &s) { return std::to_wstring(s); };
        constexpr auto SHORT = [](const short &s) { return std::to_wstring(s); };
        constexpr auto U_SHORT = [](const unsigned short &s) { return std::to_wstring(s); };
        constexpr auto INT = [](const int &s) { return std::to_wstring(s); };
        constexpr auto LONG = [](const long &s) { return std::to_wstring(s); };
        constexpr auto LONG_LONG = [](const long long &s) { return std::to_wstring(s); };
        constexpr auto U_LONG = [](const unsigned long &s) { return std::to_wstring(s); };
        constexpr auto U_LONG_LONG = [](const unsigned long long &s) { return std::to_wstring(s); };
        constexpr auto FLOAT = [](const float &s) { return std::to_wstring(s); };
        // Formats a float with a fixed number of decimals (decimals match the arrow-key step).
        inline auto floatFixed(const int decimals) {
            return [decimals](const float &s) {
                wchar_t buf[32];
                swprintf(buf, 32, L"%.*f", decimals, s == 0.0f ? 0.0f : s);
                return std::wstring(buf);
            };
        }
        // Formats a float with up to `decimals` decimals, trimming trailing zeros and a bare decimal point.
        inline auto floatTrim(const int decimals) {
            return [decimals](const float &s) {
                wchar_t buf[32];
                swprintf(buf, 32, L"%.*f", decimals, s == 0.0f ? 0.0f : s);
                std::wstring str(buf);
                if (str.find(L'.') != std::wstring::npos) {
                    str.erase(str.find_last_not_of(L'0') + 1);
                    if (!str.empty() && str.back() == L'.') str.pop_back();
                }
                return str;
            };
        }
        constexpr auto DOUBLE = [](const double &s) { return std::to_wstring(s); };
        constexpr auto LONG_DOUBLE = [](const long double &s) { return std::to_wstring(s); };
    }
}
