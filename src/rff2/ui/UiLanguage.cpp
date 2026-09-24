//
// Modified by GPT-6 on 2026-09-15, 2026-09-19, 2026-09-22
//

#include "UiLanguage.hpp"
#include <atomic>
#include <unordered_map>
#include <windows.h>

namespace merutilm::rff2 {
    namespace {
        std::atomic<Language> active{Language::English}, selected{Language::English};
        const std::unordered_map<std::wstring_view, const wchar_t *> &japanese() {
            static const std::unordered_map<std::wstring_view, const wchar_t *> entries{
#include "UiTranslationsJa.inc"
            };
            return entries;
        }
    } // namespace
    Language UiLanguage::current() {
        return active.load(std::memory_order_relaxed);
    }
    Language UiLanguage::preferred() {
        return selected.load(std::memory_order_relaxed);
    }
    void UiLanguage::choose(Language language) {
        selected.store(language == Language::Japanese ? language : Language::English,
                       std::memory_order_relaxed);
    }
    void UiLanguage::load(Language language) {
        choose(language);
        active.store(preferred(), std::memory_order_relaxed);
    }
    const wchar_t *UiLanguage::label(const wchar_t *english) {
        if (current() == Language::Japanese) {
            if (auto found = japanese().find(english); found != japanese().end()) {
                return found->second;
            }
        }
        return english;
    }
    std::wstring UiLanguage::text(std::wstring_view english) {
        if (current() == Language::Japanese) {
            if (auto found = japanese().find(english); found != japanese().end()) {
                return found->second;
            }
            if (const auto first = english.find_first_not_of(L' ');
                first != std::wstring_view::npos && first > 0) {
                return std::wstring(english.substr(0, first)) + text(english.substr(first));
            }
            if (english.ends_with(L" *")) {
                return text(english.substr(0, english.size() - 2)) + L" *";
            }
            for (const auto prefix :
                 {L"Enter a whole number from ", L"Enter a finite number from ", L"Enter a number from "}) {
                if (english.starts_with(prefix) && english.ends_with(L".")) {
                    const auto range = english.substr(wcslen(prefix), english.size() - wcslen(prefix) - 1);
                    if (const auto split = range.find(L" to "); split != std::wstring_view::npos) {
                        return std::wstring(range.substr(0, split)) + L"～" +
                               std::wstring(range.substr(split + 4)) +
                               (english.starts_with(L"Enter a whole") ? L"の整数を入力してください。"
                                                                      : L"の有限の数値を入力してください。");
                    }
                }
            }
            for (const auto prefix : {L"Add Parameter: ",
                                      L"Add: ",
                                      L"Select: ",
                                      L"Detail · ",
                                      L"Reset effect: ",
                                      L"Reset ",
                                      L"Remove ",
                                      L"Favorite ",
                                      L"Choose ",
                                      L"Check this value. ",
                                      L"Cannot apply. ",
                                      L"Discard pending changes in ",
                                      L"Timeline paused. ",
                                      L"Timeline playing. ",
                                      L"Original color array. ",
                                      L"Generated recipe. ",
                                      L"Before edit: ",
                                      L"Search settings. ",
                                      L"Rendering video: ",
                                      L"Video saved: ",
                                      L"Processing... ",
                                      L"Replace this file?\n\n"}) {
                if (english.starts_with(prefix)) {
                    return std::wstring(label(prefix)) + text(english.substr(wcslen(prefix)));
                }
            }
            for (const auto suffix :
                 {L" changed", L" controls", L" matching control", L" matching controls",
                  L" results across all settings", L" result across all settings", L" non-default value",
                  L" non-default values", L" controls changed from reset defaults.", L" keyframes",
                  L" loaded", L" setting needs attention. Your changes have not been applied.",
                  L" settings need attention. Your changes have not been applied."}) {
                const auto length = wcslen(suffix);
                if (english.ends_with(suffix) && english.size() > length) {
                    const auto number = english.substr(0, english.size() - length);
                    if (number.find_first_not_of(L"0123456789") == std::wstring_view::npos) {
                        return std::wstring(number) + label(suffix);
                    }
                }
            }
            for (const auto middle : {L" pending change in ", L" pending changes across "}) {
                const auto split = english.find(middle);
                if (split != std::wstring_view::npos &&
                    english.substr(0, split).find_first_not_of(L"0123456789") == std::wstring_view::npos) {
                    auto title = english.substr(split + wcslen(middle));
                    if (title.ends_with(L".")) {
                        title.remove_suffix(1);
                    }
                    return text(title) + L"に未適用の変更が" + std::wstring(english.substr(0, split)) +
                           L"件あります。";
                }
            }
            if (const auto split = english.find(L" to "); split != std::wstring_view::npos) {
                const auto low = english.substr(0, split), high = english.substr(split + 4);
                const auto numeric = [](std::wstring_view value) {
                    return !value.empty() &&
                           value.find_first_not_of(L"0123456789+-.eE") == std::wstring_view::npos;
                };
                if (numeric(low) && numeric(high)) {
                    return std::wstring(low) + L"～" + std::wstring(high);
                }
                if (japanese().contains(low) && numeric(high)) {
                    return text(low) + L" → " + std::wstring(high);
                }
            }
            if (const auto split = english.find(L'\t'); split != std::wstring_view::npos) {
                return text(english.substr(0, split)) + std::wstring(english.substr(split));
            }
            if (const auto split = english.find(L" / "); split != std::wstring_view::npos) {
                return text(english.substr(0, split)) + L" / " + text(english.substr(split + 3));
            }
        }
        return std::wstring(english);
    }
    std::wstring UiLanguage::utf8(std::string_view english) {
        if (english.empty()) {
            return {};
        }
        UINT encoding = CP_UTF8;
        int size = MultiByteToWideChar(encoding, MB_ERR_INVALID_CHARS, english.data(), int(english.size()),
                                       nullptr, 0);
        if (!size) {
            encoding = CP_ACP;
            size = MultiByteToWideChar(encoding, 0, english.data(), int(english.size()), nullptr, 0);
        }
        std::wstring wide(size, L'\0');
        MultiByteToWideChar(encoding, 0, english.data(), int(english.size()), wide.data(), size);
        return text(wide);
    }
    const wchar_t *UiLanguage::fontFace() {
        return current() == Language::Japanese ? L"Yu Gothic UI" : L"Segoe UI";
    }
    int UiLanguage::drawText(HDC dc, const wchar_t *source, int count, RECT *bounds, UINT format) {
        if (!source) {
            return 0;
        }
        if (current() == Language::English) {
            return DrawTextW(dc, source, count, bounds, format);
        }
        auto translated = text(std::wstring_view(source, count < 0 ? wcslen(source) : size_t(count)));
        return DrawTextW(dc, translated.data(), int(translated.size()), bounds, format);
    }
    void UiLanguage::caption(HWND window) {
        std::wstring original(GetWindowTextLengthW(window) + 1, L'\0');
        GetWindowTextW(window, original.data(), int(original.size()));
        original.resize(wcslen(original.c_str()));
        const auto translated = text(original);
        if (translated != original) {
            SetWindowTextW(window, translated.c_str());
        }
    }
} // namespace merutilm::rff2
