//
// Modified by Opus 5.5 on 2026-10-10, 2026-10-25
//

#pragma once

#include <algorithm>
#include <cstdint>
#include <cwchar>
#include <filesystem>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "../constants/Constants.hpp"
#include "../ui/IOUtilities.h"

namespace merutilm::rff2 {
    // A contiguous run of numbered keyframe files; position 1 of the run is file number `first`.
    struct KeyframeRange {
        uint32_t first = 0;
        uint32_t count = 0;

        [[nodiscard]] uint32_t last() const { return count ? first + count - 1 : 0; }
        [[nodiscard]] uint32_t fileNumber(const uint32_t position) const { return first + position - 1; }
    };

    namespace keyframe_range {
        // File numbers usable as keyframes: an .rfm/.rfmz map, or an .rfsm header with its .png beside it.
        inline std::set<uint32_t> numbers(const std::filesystem::path &directory, const bool staticImages) {
            std::set<uint32_t> found;
            std::error_code error;
            std::filesystem::directory_iterator entry(directory, error);
            const std::filesystem::directory_iterator end;
            for (; !error && entry != end; entry.increment(error)) {
                std::error_code typeError;
                if (!entry->is_regular_file(typeError)) continue;
                const std::wstring stem = entry->path().stem().wstring();
                const std::wstring extension = entry->path().extension().wstring();
                if (stem.empty() || stem.size() > 9 || extension.size() < 2) continue;
                bool digits = true;
                uint32_t number = 0;
                for (const wchar_t c : stem) {
                    if (c < L'0' || c > L'9') { digits = false; break; }
                    number = number * 10 + uint32_t(c - L'0');
                }
                // Only the exact names the keyframe writer produces, so 035.rfm is not mistaken for 0035.rfm.
                if (!digits || number == 0 || IOUtilities::fileNameFormat(number, extension.substr(1)) !=
                                                  entry->path().filename().wstring()) continue;
                const wchar_t *ext = extension.c_str() + 1;
                if (staticImages) {
                    if (_wcsicmp(ext, Constants::Extension::STATIC_MAP) == 0 &&
                        std::filesystem::exists(directory / IOUtilities::fileNameFormat(number, Constants::Extension::IMAGE))) {
                        found.insert(number);
                    }
                } else if (_wcsicmp(ext, Constants::Extension::DYNAMIC_MAP) == 0 ||
                           _wcsicmp(ext, Constants::Extension::COMPRESSED_MAP) == 0) {
                    found.insert(number);
                }
            }
            return found;
        }

        // From the requested first file (0 = lowest present) up to the requested last (0 = end of the run), stopping at a gap.
        inline KeyframeRange resolve(const std::set<uint32_t> &numbers, const uint32_t requestedFirst,
                                     const uint32_t requestedLast) {
            const auto start = numbers.lower_bound(std::max<uint32_t>(requestedFirst, 1));
            if (start == numbers.end()) return {};
            KeyframeRange range{*start, 0};
            for (auto it = start; it != numbers.end() && *it == range.first + range.count; ++it) {
                if (requestedLast != 0 && *it > requestedLast) break;
                ++range.count;
            }
            return range;
        }

        inline KeyframeRange resolve(const std::filesystem::path &directory, const bool staticImages,
                                     const uint32_t requestedFirst, const uint32_t requestedLast) {
            return resolve(numbers(directory, staticImages), requestedFirst, requestedLast);
        }
    }
}
