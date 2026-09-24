//
// Modified by GPT-6 on 2026-09-13, 2026-09-14, 2026-09-15, 2026-09-22
//

#pragma once
#include "../UiLanguage.hpp"
#include <algorithm>
#include <windows.h>
#include <string_view>

namespace merutilm::rff2::workspace {
    struct PanelDrawing {
        static void fill(HDC dc, RECT rect, COLORREF color) {
            const auto brush=CreateSolidBrush(color);
            FillRect(dc,&rect,brush);
            DeleteObject(brush);
        }

        static void border(HDC dc, RECT rect, COLORREF color, int thickness) {
            if(rect.right<=rect.left||rect.bottom<=rect.top||thickness<=0)return;
            const int edge=std::min<LONG>(thickness,std::min(rect.right-rect.left,rect.bottom-rect.top));
            fill(dc,{rect.left,rect.top,rect.right,rect.top+edge},color);
            fill(dc,{rect.left,rect.bottom-edge,rect.right,rect.bottom},color);
            fill(dc,{rect.left,rect.top,rect.left+edge,rect.bottom},color);
            fill(dc,{rect.right-edge,rect.top,rect.right,rect.bottom},color);
        }

        static void rounded(HDC dc, RECT rect, COLORREF face, COLORREF border, int radius) {
            const auto brush=CreateSolidBrush(face);
            const auto pen=CreatePen(PS_SOLID,1,border);
            const auto previousBrush=SelectObject(dc,brush);
            const auto previousPen=SelectObject(dc,pen);
            RoundRect(dc,rect.left,rect.top,rect.right,rect.bottom,radius,radius);
            SelectObject(dc,previousPen);
            SelectObject(dc,previousBrush);
            DeleteObject(pen);
            DeleteObject(brush);
        }

        static void text(HDC dc, std::wstring_view value, RECT rect, COLORREF color, HFONT font,
                         UINT alignment = DT_LEFT) {
            const int saved = SaveDC(dc);
            if (saved == 0) {
                return;
            }
            SelectObject(dc, font);
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, color);
            try {
                UiLanguage::drawText(dc, value.data(), static_cast<int>(value.size()), &rect,
                                     alignment | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
            } catch (...) {
                RestoreDC(dc, saved);
                throw;
            }
            RestoreDC(dc, saved);
        }
    };
}
