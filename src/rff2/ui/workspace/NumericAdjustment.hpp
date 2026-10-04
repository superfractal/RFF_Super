//
// Modified by GPT-6 on 2026-10-01
//

#pragma once
#include <algorithm>
#include <charconv>
#include <cmath>
#include <string>
#include <type_traits>

namespace merutilm::rff2::workspace {
    template<class T> auto numericAdjustment(T minimum, T maximum, double fixedStep = 0,
                                             bool fractional = false, bool decadeSteps = false, double zeroStep = 0) {
        return [=](const std::wstring &text, int direction, bool coarse) -> std::wstring {
            const auto first = text.find_first_not_of(L" \t\r\n");
            if (first == std::wstring::npos) return text;
            const auto token = text.substr(first, text.find_last_not_of(L" \t\r\n") - first + 1);
            if (token.find_first_not_of(L"0123456789+-.eE") != std::wstring::npos) return text;
            const std::string narrow(token.begin(), token.end());
            const char *begin = narrow.data() + (narrow.front() == '+');
            T current{};
            const auto parsed = std::from_chars(begin, narrow.data() + narrow.size(), current);
            if (parsed.ec != std::errc{} || parsed.ptr != narrow.data() + narrow.size() ||
                current < minimum || current > maximum) return text;
            T next;
            if constexpr (std::is_integral_v<T>) {
                next = current;
                for (int i = 0; i < (coarse ? 10 : 1); ++i) {
                    if (direction > 0 && next < maximum) ++next;
                    if (direction < 0 && next > minimum) --next;
                }
            } else {
                if (!std::isfinite(current)) return text;
                const double low = minimum, high = maximum;
                const double range = high - low;
                const bool logarithmic = zeroStep > 0 || (low > 0 && high / low >= 100);
                const bool whole = !fractional && fixedStep == 0 && !decadeSteps && range > 10 &&
                                   ((logarithmic && low >= 1) || range <= 10000);
                double value;
                if (decadeSteps) {
                    if (direction < 0 && current <= 1) {
                        if (minimum > 0) return text;
                        value = 0;
                    }
                    else if (direction > 0 && current < 1) value = 1;
                    else {
                        const double bounded = std::clamp(double(current), 1.0, 100000.0);
                        double exponent = std::floor(std::log10(bounded) + 1e-9);
                        if (direction < 0 && std::abs(bounded - std::pow(10.0, exponent)) <= bounded * 1e-9) --exponent;
                        const double step = std::pow(10.0, exponent) * (coarse ? 10 : 1);
                        value = std::clamp(std::round((bounded + direction * step) / step) * step, 1.0, 100000.0);
                    }
                } else if (fixedStep > 0) {
                    value = std::round((double(current) + direction * fixedStep * (coarse ? 10 : 1)) / fixedStep) * fixedStep;
                } else if (logarithmic) {
                    const double factor = coarse ? 10.0 : std::pow(10.0, 0.05);
                    value = direction > 0 ? (zeroStep > 0 && current <= 0 ? zeroStep : double(current) * factor) : double(current) / factor;
                } else {
                    const double step = range <= 10000 ? range / 100.0
                        : std::pow(10.0, std::floor(std::log10(std::max(1.0, std::abs(double(current)))))) / 100.0;
                    value = double(current) + direction * step * (coarse ? 10 : 1);
                }
                value = std::clamp(value, low, high);
                if (whole) {
                    value = std::round(value);
                    if (value == double(current)) value += direction;
                }
                next = static_cast<T>(std::clamp(value, low, high));
                if (!std::isfinite(next)) return text;
            }
            char buffer[96];
            const auto result = std::to_chars(buffer, buffer + sizeof(buffer), next);
            return result.ec == std::errc{} ? std::wstring(buffer, result.ptr) : text;
        };
    }
}
