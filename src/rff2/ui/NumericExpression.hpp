// Modified by GPT-5 on 2026-08-24

#pragma once

#include <cmath>
#include <cwchar>
#include <cwctype>
#include <optional>
#include <string>

namespace merutilm::rff2 {
    class NumericExpression final {
        const wchar_t *cursor;
        bool valid = true;

        explicit NumericExpression(const std::wstring &text) : cursor(text.c_str()) {}

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
            if (end == cursor) {
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
                value = operation == L'*' ? value * right : value / right;
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
            }
        }

    public:
        [[nodiscard]] static std::optional<double> evaluate(const std::wstring &text) {
            NumericExpression parser(text);
            const double value = parser.expression();
            parser.skipSpaces();
            if (!parser.valid || *parser.cursor != L'\0' || !std::isfinite(value)) {
                return std::nullopt;
            }
            return value;
        }
    };
}
