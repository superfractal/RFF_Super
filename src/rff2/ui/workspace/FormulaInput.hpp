//
// Created by Opus 5.5 on 2026-11-15
//

#pragma once
#include "WorkspaceForm.hpp"
#include "../UiLanguage.hpp"
#include "../../formula/ExpressionParser.h"
#include <algorithm>
#include <array>
#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace merutilm::rff2::workspace {
    // The Custom Formula field's calculator keys, and the check that says which characters are wrong and why.
    struct FormulaInput {
        struct Check {
            std::wstring message; // empty when the formula parses
            size_t start = std::wstring::npos;
            size_t length = 0;
        };

        static Check check(std::wstring_view text) {
            if (text.find_first_not_of(L" \t\r\n") == std::wstring_view::npos) {
                return {UiLanguage::text(L"Enter a formula.")};
            }
            std::string narrow;
            narrow.reserve(text.size());
            for (size_t i = 0; i < text.size(); ++i) {
                const wchar_t character = text[i];
                if (character > 127) {
                    return {join(fill(L"Character %1: \"%2\" cannot be used in a formula.",
                                      {ordinal(i), std::wstring(1, character)}),
                                 symbolHint(character)),
                            i, 1};
                }
                narrow.push_back(char(character));
            }
            ExpressionParser parser;
            if (parser.parse(narrow)) {
                return {};
            }
            const auto &detail = parser.getErrorDetail();
            const auto at = ordinal(detail.position);
            const std::wstring token(detail.token.begin(), detail.token.end());
            const std::wstring other(detail.other.begin(), detail.other.end());
            std::wstring message;
            switch (detail.kind) {
            case ParseErrorKind::EMPTY:
                return {UiLanguage::text(L"Enter a formula.")};
            case ParseErrorKind::INVALID_NUMBER:
                message = fill(L"Character %1: \"%2\" is not a number.", {at, token});
                break;
            case ParseErrorKind::BAD_ASSIGNMENT:
                message = fill(L"Character %1: only \"z = ...\" can be assigned.", {at});
                break;
            case ParseErrorKind::UNKNOWN_NAME:
                message = join(fill(L"Character %1: \"%2\" is not a known name.", {at, token}), nameHint(detail.token));
                break;
            case ParseErrorKind::UNKNOWN_SYMBOL:
                message = join(fill(L"Character %1: \"%2\" cannot be used in a formula.", {at, token}),
                               token.empty() ? std::wstring() : symbolHint(token.front()));
                break;
            case ParseErrorKind::UNMATCHED_CLOSE:
                message = fill(L"Character %1: this \")\" has no \"(\" before it.", {at});
                break;
            case ParseErrorKind::UNCLOSED_OPEN:
                message = join(fill(L"Character %1: this \"(\" is never closed.", {at}),
                               UiLanguage::text(L"Add a \")\" to close it."));
                break;
            case ParseErrorKind::EMPTY_BRACKETS:
                message = fill(L"Character %1: the brackets are empty.", {at});
                break;
            case ParseErrorKind::MISSING_AFTER:
                message = join(fill(L"Character %1: \"%2\" needs a value after it.", {at, token}),
                               isFunctionName(detail.token) ? fill(L"Write it as %1(z).", {token}) : std::wstring());
                break;
            case ParseErrorKind::MISSING_BETWEEN:
                message = fill(L"Character %1: a value is missing between \"%2\" and \"%3\".", {at, token, other});
                break;
            case ParseErrorKind::MISSING_BEFORE:
                message = fill(L"Character %1: \"%2\" needs a value before it.", {at, token});
                break;
            default:
                return {UiLanguage::text(L"The formula does not work out to a single value.")};
            }
            if (detail.position >= text.size()) {
                return {std::move(message)};
            }
            return {std::move(message), detail.position, std::max<size_t>(1, std::min(detail.length, text.size() - detail.position))};
        }

        static std::shared_ptr<const FormKeypad> keypad() {
            static const auto shared = [] {
                using Action = FormKey::Action;
                auto pad = std::make_shared<FormKeypad>();
                const auto key = [](std::wstring label, std::wstring text = {}, std::wstring tip = {}, bool symbol = false,
                                    Action action = Action::INSERT, int span = 1) {
                    FormKey result{std::move(label), std::move(text), std::move(tip), action, symbol, span};
                    if (result.text.empty() && action == Action::INSERT) result.text = result.label;
                    return result;
                };
                pad->pad = {
                    key(L"z", L"z", L"The current value. It starts at 0."),
                    key(L"c", L"c", L"The position of the pixel."),
                    key(L"i", L"i", L"The imaginary unit."),
                    key(L"(", L"(", {}, true),
                    key(L")", L")", {}, true),
                    key(L"", {}, L"Delete before the cursor. A function name goes with its bracket.", false, Action::BACKSPACE),
                    key(L"7"), key(L"8"), key(L"9"),
                    key(L"÷", L"/", L"Divide. Types /.", true),
                    key(L"^", L"^", L"Power.", true),
                    key(L"Clear", {}, L"Clear the formula.", false, Action::CLEAR),
                    key(L"4"), key(L"5"), key(L"6"),
                    key(L"×", L"*", L"Multiply. Types *.", true),
                    key(L"x²", L"^2", L"Square. Types ^2.", true),
                    key(L"", {}, L"Move the cursor left.", false, Action::LEFT),
                    key(L"1"), key(L"2"), key(L"3"),
                    key(L"−", L"-", L"Subtract or negate. Types -.", true),
                    key(L"x³", L"^3", L"Cube. Types ^3.", true),
                    key(L"", {}, L"Move the cursor right.", false, Action::RIGHT),
                    key(L"0", L"0", {}, false, Action::INSERT, 2),
                    key(L"."),
                    key(L"+", L"+", {}, true),
                    key(L"π", L"pi", L"Pi. Types pi."),
                    key(L"e", L"e", L"Euler's number.")};
                const auto function = [](const wchar_t *name, const wchar_t *tip) {
                    return FormKey{name, name, tip, Action::WRAP};
                };
                pad->pages = {
                    {L"Parts",
                     {function(L"conj", L"Conjugate: a+bi becomes a-bi."),
                      function(L"abs", L"Absolute value |z|, a real number."),
                      function(L"norm", L"Squared absolute value |z|^2."),
                      function(L"arg", L"Argument (angle), a real number."),
                      function(L"real", L"Real part. re also works."),
                      function(L"imag", L"Imaginary part. im also works."),
                      function(L"rabs", L"Absolute value of the real part only."),
                      function(L"iabs", L"Absolute value of the imaginary part only."),
                      function(L"riabs", L"Absolute value of both parts, as in Burning Ship.")}},
                    {L"Exp and log",
                     {function(L"sqrt", L"Square root."),
                      function(L"exp", L"Exponential, e^z."),
                      function(L"log", L"Natural logarithm. ln also works.")}},
                    {L"Trigonometric",
                     {function(L"sin", L"Sine."), function(L"cos", L"Cosine."), function(L"tan", L"Tangent."),
                      function(L"asin", L"Inverse sine."), function(L"acos", L"Inverse cosine."),
                      function(L"atan", L"Inverse tangent."), function(L"sinh", L"Hyperbolic sine."),
                      function(L"cosh", L"Hyperbolic cosine."), function(L"tanh", L"Hyperbolic tangent.")}},
                    {L"Rounding",
                     {function(L"floor", L"Rounds both parts down."), function(L"ceil", L"Rounds both parts up."),
                      function(L"round", L"Rounds both parts to the nearest whole number."),
                      function(L"sign", L"The sign of each part: -1, 0 or 1.")}}};
                pad->examplesCaption = L"Examples";
                const auto example = [](const wchar_t *formula, const wchar_t *name) {
                    FormKey result{formula, formula, L"Replace the formula with this example.", Action::REPLACE};
                    result.name = name;
                    return result;
                };
                // Tricorn: Milnor (1992); Burning Ship: Michelitsch and Roessler (1992); see NOTICE.
                pad->examples = {example(L"z^2+c", L"Mandelbrot"), example(L"z^3+c", L"Third power"),
                                 example(L"conj(z)^2+c", L"Tricorn"),
                                 example(L"riabs(z)^2+conj(c)", L"Burning Ship")};
                return std::shared_ptr<const FormKeypad>(std::move(pad));
            }();
            return shared;
        }

      private:
        // The names ExpressionParser::isFunction and its constants accept.
        static constexpr std::array<std::string_view, 35> names{
            "conj", "abs", "norm", "arg", "real", "imag", "rabs", "iabs", "riabs", "sqrt", "exp", "log",
            "sin", "cos", "tan", "asin", "acos", "atan", "sinh", "cosh", "tanh", "floor", "ceil", "round",
            "sign", "ln", "re", "im", "z", "c", "i", "pi", "e", "tau", "phi"};
        static constexpr size_t functionCount = 28;

        static bool isFunctionName(std::string_view token) {
            return std::find(names.begin(), names.begin() + functionCount, token) != names.begin() + functionCount;
        }
        static std::wstring ordinal(size_t position) {
            return std::to_wstring(position == std::wstring::npos ? 0 : position + 1);
        }
        // Puts each value in place of %1, %2 ... in the translated template.
        static std::wstring fill(std::wstring_view english, std::initializer_list<std::wstring> values) {
            auto text = UiLanguage::text(english);
            size_t index = 1;
            for (const auto &value : values) {
                const std::wstring marker = L"%" + std::to_wstring(index++);
                for (size_t found = text.find(marker); found != std::wstring::npos; found = text.find(marker, found + value.size())) {
                    text.replace(found, marker.size(), value);
                }
            }
            return text;
        }
        static std::wstring join(std::wstring message, const std::wstring &hint) {
            if (hint.empty()) return message;
            return message + (UiLanguage::current() == Language::Japanese ? L"" : L" ") + hint;
        }
        static std::wstring symbolHint(wchar_t character) {
            switch (character) {
            case L'×': case L'✕': case L'＊': return UiLanguage::text(L"Use * to multiply.");
            case L'÷': case L'／': return UiLanguage::text(L"Use / to divide.");
            case L'²': return UiLanguage::text(L"Use ^2 for a square.");
            case L'³': return UiLanguage::text(L"Use ^3 for a cube.");
            case L'π': return UiLanguage::text(L"Use pi for π.");
            case L'−': case L'–': case L'ー': return UiLanguage::text(L"Use the - key for minus.");
            case L'|': return UiLanguage::text(L"Use abs( ) for an absolute value.");
            case L',': return UiLanguage::text(L"Functions take one value.");
            case L'[': case L']': case L'{': case L'}': return UiLanguage::text(L"Use ( ) for brackets.");
            default:
                return (character >= 0xff01 && character <= 0xff5e) || character == 0x3000
                           ? UiLanguage::text(L"Type formulas in half-width characters.")
                           : std::wstring();
            }
        }
        // Levenshtein (1966) edit distance by the Wagner-Fischer (1974) row recurrence; textbook method, see NOTICE.
        static size_t distance(std::string_view a, std::string_view b) {
            std::vector<size_t> row(b.size() + 1);
            for (size_t j = 0; j <= b.size(); ++j) row[j] = j;
            for (size_t i = 1; i <= a.size(); ++i) {
                size_t diagonal = row[0];
                row[0] = i;
                for (size_t j = 1; j <= b.size(); ++j) {
                    const size_t above = row[j];
                    row[j] = std::min({row[j] + 1, row[j - 1] + 1, diagonal + (a[i - 1] == b[j - 1] ? 0 : 1)});
                    diagonal = above;
                }
            }
            return row[b.size()];
        }
        static std::wstring nameHint(const std::string &word) {
            if (word.size() > 1 && word.find_first_not_of("zci") == std::string::npos) {
                std::wstring product;
                for (char letter : word) {
                    if (!product.empty()) product += L'*';
                    product += wchar_t(letter);
                }
                return fill(L"To multiply, write %1.", {product});
            }
            std::string_view best;
            size_t bestDistance = 3;
            for (const auto name : names) {
                if (const size_t d = distance(word, name); d < bestDistance) {
                    bestDistance = d;
                    best = name;
                }
            }
            return best.empty() ? std::wstring() : fill(L"Did you mean %1?", {std::wstring(best.begin(), best.end())});
        }
    };
}
