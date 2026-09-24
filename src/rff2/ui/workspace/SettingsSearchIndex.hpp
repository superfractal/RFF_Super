//
// Modified by GPT-6 on 2026-09-14, 2026-09-15, 2026-09-22, 2026-09-23
//

#pragma once
#include "../UiLanguage.hpp"
#include <algorithm>
#include <cwctype>
#include <functional>
#include <exception>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace merutilm::rff2::workspace {
    struct SettingsSearchTarget {
        int module = 1;
        int section = 0;
        std::string field;
    };
    struct SettingsSearchResult {
        long identity = 0;
        SettingsSearchTarget target;
        std::wstring label;
        std::wstring path;
        std::wstring value;
        std::wstring state;
    };
    class SettingsSearchIndex {
        struct Entry {
            long identity;
            SettingsSearchTarget target;
            std::wstring label;
            std::wstring path;
            std::wstring normalizedLabel;
            std::wstring terms;
            std::function<std::wstring()> value;
            std::function<std::wstring()> state;
        };
        std::vector<Entry> entries;
        std::map<std::pair<int, std::string>, long> identities;

        static std::optional<int> matchScore(const Entry &entry, std::wstring_view query,
                                             const std::vector<std::wstring> &words) {
            int score = 0;
            if (entry.normalizedLabel == query) {
                score = 100;
            } else if (entry.normalizedLabel.starts_with(query)) {
                score = 50;
            }
            for (const auto &word : words) {
                if (entry.terms.find(word) == std::wstring::npos) {
                    return std::nullopt;
                }
                if (entry.normalizedLabel.find(word) != std::wstring::npos) {
                    score += 10;
                }
            }
            return score;
        }

      public:
        static std::wstring normalize(std::wstring_view text) {
            std::wstring result;
            for (wchar_t character : text) {
                WORD characterType = 0;
                GetStringTypeW(CT_CTYPE1, &character, 1, &characterType);
                if (characterType & (C1_ALPHA | C1_DIGIT)) {
                    result += wchar_t(std::towlower(character));
                } else if (!result.empty() && result.back() != L' ') {
                    result += L' ';
                }
            }
            if (!result.empty() && result.back() == L' ') {
                result.pop_back();
            }
            return result;
        }
        void eraseModule(int module) {
            std::erase_if(entries, [=](const Entry &entry) { return entry.target.module == module; });
        }
        void add(SettingsSearchTarget target, std::wstring label, std::wstring path, std::wstring aliases,
                 std::function<std::wstring()> value, std::function<std::wstring()> state) {
            const auto identityKey = std::make_pair(target.module, target.field);
            const auto identityEntry =
                identities.try_emplace(identityKey, long(identities.size()) + 100).first;
            std::erase_if(entries,
                          [&](const Entry &entry) { return entry.identity == identityEntry->second; });
            aliases += L" " + label + L" " + path;
            label = UiLanguage::text(label);
            path = UiLanguage::text(path);
            const auto normalizedLabel = normalize(label);
            const auto terms = normalize(label + L" " + path + L" " + aliases + L" " +
                                         std::wstring(target.field.begin(), target.field.end()));
            entries.push_back({identityEntry->second, std::move(target), std::move(label), std::move(path),
                               normalizedLabel, terms, std::move(value), std::move(state)});
        }
        std::vector<SettingsSearchResult> find(std::wstring_view query) const {
            const auto normalizedQuery = normalize(query);
            if (normalizedQuery.empty()) {
                return {};
            }
            std::vector<std::wstring> words;
            for (size_t wordStart = 0; wordStart < normalizedQuery.size();) {
                const auto wordEnd = normalizedQuery.find(L' ', wordStart);
                words.push_back(normalizedQuery.substr(
                    wordStart, wordEnd == std::wstring::npos ? wordEnd : wordEnd - wordStart));
                if (wordEnd == std::wstring::npos) {
                    break;
                }
                wordStart = wordEnd + 1;
            }
            std::vector<std::pair<int, const Entry *>> scoredMatches;
            for (const auto &entry : entries) {
                if (const auto score = matchScore(entry, normalizedQuery, words)) {
                    scoredMatches.emplace_back(*score, &entry);
                }
            }
            std::stable_sort(scoredMatches.begin(), scoredMatches.end(),
                             [](const auto &left, const auto &right) { return left.first > right.first; });
            std::vector<SettingsSearchResult> results;
            for (const auto &scoredMatch : scoredMatches) {
                const auto *entry = scoredMatch.second;
                SettingsSearchResult result{entry->identity, entry->target, entry->label, entry->path};
                try {
                    result.value = entry->value ? entry->value() : L"";
                    result.state = entry->state ? entry->state() : L"";
                } catch (const std::exception &) {
                    result.value = L"Unavailable";
                    result.state = L"Open to inspect";
                }
                results.push_back(std::move(result));
            }
            return results;
        }
    };
    inline std::wstring settingsSearchAliases(std::string_view id) {
        if (id == "render.fps") {
            return L"Display Framerate frame rate live preview speed performance";
        }
        if (id == "surface.styleDetailLight") {
            return L"fine detail too bright glow chaos rin";
        }
        if (id == "surface.styleDetailSuppress" || id == "surface.styleDetailThreshold") {
            return L"fine detail too bright glow chaos dense suppression rin";
        }
        if (id == "surface.paletteColorMix") {
            return L"PHONK color existing palette amount";
        }
        if (id == "surface.ukiyoColors") {
            return L"Ukiyo-e flat posterize quantization color count";
        }
        return {};
    }
} // namespace merutilm::rff2::workspace
