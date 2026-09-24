//
// Modified by GPT-6 on 2026-09-17, 2026-09-22
//

#pragma once
#include <algorithm>
#include <cwctype>
#include <string>
#include <vector>
#include <windows.h>

namespace merutilm::rff2 {
    struct MenuNode {
        int id = 0;
        UINT command = 0;
        std::wstring caption;
        std::wstring shortcut;
        wchar_t access = 0;
        std::vector<int> children;
        bool enabled = true, checked = false, checkable = false, separator = false, rightAligned = false;
    };

    struct MenuModel {
        std::vector<MenuNode> nodes{MenuNode{}};
        const MenuNode &at(int id) const {
            return nodes.at(id);
        }
        static std::wstring label(const std::wstring &caption) {
            std::wstring result;
            for (size_t i = 0; i < caption.size(); ++i) {
                if (caption[i] == L'&') {
                    if (i + 1 < caption.size() && caption[i + 1] == L'&') {
                        ++i;
                    } else {
                        continue;
                    }
                }
                result += caption[i];
            }
            return result;
        }
        static wchar_t mnemonic(const std::wstring &caption) {
            for (size_t i = 0; i + 1 < caption.size(); ++i) {
                if (caption[i] == L'&') {
                    if (caption[i + 1] == L'&') {
                        ++i;
                        continue;
                    }
                    return std::towupper(caption[i + 1]);
                }
            }
            const auto text = label(caption);
            return text.empty() ? 0 : std::towupper(text.front());
        }
    };

    struct MenuLayout {
        static RECT place(RECT anchor, int width, int height, RECT work, bool isSubmenu) {
            width = std::clamp(width, 1, int(std::max(1L, work.right - work.left)));
            height = std::clamp(height, 1, int(std::max(1L, work.bottom - work.top)));
            int left = isSubmenu ? anchor.right : anchor.left, top = isSubmenu ? anchor.top : anchor.bottom;
            if (left + width > work.right) {
                left = isSubmenu ? anchor.left - width : work.right - width;
            }
            if (top + height > work.bottom) {
                top = isSubmenu ? work.bottom - height : anchor.top - height;
            }
            left = std::clamp(left, int(work.left), int(work.right) - width);
            top = std::clamp(top, int(work.top), int(work.bottom) - height);
            return {left, top, left + width, top + height};
        }
    };
} // namespace merutilm::rff2
