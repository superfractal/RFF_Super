//
// Modified by GPT-5 on 2026-08-24
// Modified by GPT-6 on 2026-09-14, 2026-09-22
//

#pragma once

#include <cmath>
#include <bit>
#include <cstdint>
#include <limits>
#include <cwchar>
#include <cwctype>
#include <optional>
#include <string>

namespace merutilm::rff2 {
    class NumericExpression final {
        const wchar_t *cursor;
        bool valid = true;

        explicit NumericExpression(const std::wstring &text) : cursor(text.c_str()) {}
        static bool finite(double value) {
            static_assert(sizeof(double) == sizeof(uint64_t) && std::numeric_limits<double>::is_iec559);
            return (std::bit_cast<uint64_t>(value) & UINT64_C(0x7ff0000000000000)) !=
                   UINT64_C(0x7ff0000000000000);
        }

        void skipSpaces() {
            while (std::iswspace(*cursor)) {
                ++cursor;
            }
        }

        double primary() {
            skipSpaces();
            if (*cursor == L'(') {
                ++cursor;
                const double value = expression();
                skipSpaces();
                if (*cursor != L')') {
                    valid = false;
                    return 0.0;
                }
                ++cursor;
                return value;
            }

            wchar_t *end = nullptr;
            const double value = std::wcstod(cursor, &end);
            if (end == cursor || !finite(value)) {
                valid = false;
                return 0.0;
            }
            cursor = end;
            return value;
        }

        double unary() {
            skipSpaces();
            if (*cursor == L'+') {
                ++cursor;
                return unary();
            }
            if (*cursor == L'-') {
                ++cursor;
                return -unary();
            }
            return primary();
        }

        double term() {
            double value = unary();
            for (;;) {
                skipSpaces();
                const wchar_t operation = *cursor;
                if (operation != L'*' && operation != L'/') {
                    return value;
                }
                ++cursor;
                const double right = unary();
                if (operation == L'/' && right == 0.0) {
                    valid = false;
                    return 0.0;
                }
                value = operation == L'*' ? value * right : value / right;
                if (!finite(value)) {
                    valid = false;
                    return 0.0;
                }
            }
        }

        double expression() {
            double value = term();
            for (;;) {
                skipSpaces();
                const wchar_t operation = *cursor;
                if (operation != L'+' && operation != L'-') {
                    return value;
                }
                ++cursor;
                const double right = term();
                value = operation == L'+' ? value + right : value - right;
                if (!finite(value)) {
                    valid = false;
                    return 0.0;
                }
            }
        }

      public:
        [[nodiscard]] static std::optional<double> evaluate(const std::wstring &text) {
            NumericExpression parser(text);
            const double value = parser.expression();
            parser.skipSpaces();
            if (!parser.valid || *parser.cursor != L'\0' || !finite(value)) {
                return std::nullopt;
            }
            return value;
        }
    };
}
