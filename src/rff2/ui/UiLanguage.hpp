//
// Modified by GPT-6 on 2026-09-15, 2026-09-23
//

#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <windows.h>

namespace merutilm::rff2 {
    enum class Language : uint8_t {
        English = 0,
        Japanese = 1,
    };

    struct UiLanguage {
        static Language current();
        static Language preferred();
        static void load(Language language);
        static void choose(Language language);
        static std::wstring text(std::wstring_view english);
        static const wchar_t *label(const wchar_t *english);
        static std::wstring utf8(std::string_view english);
        static const wchar_t *fontFace();
        static int drawText(HDC dc, const wchar_t *text, int count, RECT *bounds, UINT format);
        static void caption(HWND window);
    };
}
