//
// Created by Opus 5.5 on 2026-10-04
//

#pragma once
#include "PanelDrawing.hpp"
#include "WorkspaceForm.hpp"
#include "WorkspaceTheme.hpp"
#include <algorithm>
#include <string>
#include <vector>
#include <windows.h>

namespace merutilm::rff2::workspace {
    // Lays out, draws and hit-tests the choice editors a form shows in place of a drop-down list:
    // side-by-side segments, the 3x3 anchor grid, curve tiles and the Bold / Italic toggles.
    struct ChoicePicker {
        // One clickable area and the choice a click on it selects.
        struct Part {
            RECT bounds;
            int choice;
        };

        struct Look {
            const WorkspaceTheme &theme;
            HFONT font;
            float scale;

            int px(int value) const {
                return int(value * scale + .5f);
            }
        };

        static bool handles(const FormField &field) {
            using Editor = FormField::Editor;
            return !field.choices.empty() &&
                   (field.editor == Editor::SEGMENTS || field.editor == Editor::ANCHOR_GRID ||
                    field.editor == Editor::CURVE_TILES || field.editor == Editor::STYLE_TOGGLES);
        }

        // The height the editor needs at this width, in pixels.
        static int height(const FormField &field, int width, HDC dc, const Look &look) {
            using Editor = FormField::Editor;
            switch (field.editor) {
            case Editor::ANCHOR_GRID:
                return 3 * look.px(cellHeight) + 2 * look.px(cellGap);
            case Editor::CURVE_TILES: {
                const int rows = (int(field.choices.size()) + tileColumns - 1) / tileColumns;
                return rows * look.px(tileHeight) + (rows - 1) * look.px(tileRowGap);
            }
            case Editor::STYLE_TOGGLES:
                return look.px(toggleSize);
            default: {
                // Labels wrap inside their segment, so the row grows to the longest of them.
                const int count = int(field.choices.size());
                const int labelWidth = std::max(1, (width + look.px(gap)) / count - look.px(gap) - look.px(8));
                int labelHeight = 0;
                for (const auto &choice : field.choices) {
                    labelHeight = std::max(labelHeight, wrappedHeight(dc, look.font, choice.label, labelWidth));
                }
                const int curve = field.curve ? look.px(curveHeight) : 0;
                return std::max(look.px(segmentHeight), curve + labelHeight + look.px(8));
            }
            }
        }

        static std::vector<Part> parts(const FormField &field, const RECT &client, int current, const Look &look) {
            using Editor = FormField::Editor;
            const int count = int(field.choices.size());
            const int width = int(client.right - client.left);
            const int spacing = look.px(gap);
            std::vector<Part> result;
            switch (field.editor) {
            case Editor::ANCHOR_GRID:
                for (int i = 0; i < std::min(count, 9); ++i) {
                    const int left = client.left + (i % 3) * (look.px(cellWidth) + look.px(cellGap));
                    const int top = client.top + (i / 3) * (look.px(cellHeight) + look.px(cellGap));
                    result.push_back({{left, top, left + look.px(cellWidth), top + look.px(cellHeight)}, i});
                }
                break;
            case Editor::CURVE_TILES: {
                const int columns = std::min(count, tileColumns);
                for (int i = 0; i < count; ++i) {
                    const int column = i % columns, row = i / columns;
                    const int left = client.left + (width + spacing) * column / columns;
                    const int right = client.left + (width + spacing) * (column + 1) / columns - spacing;
                    const int top = client.top + row * (look.px(tileHeight) + look.px(tileRowGap));
                    result.push_back({{left, top, right, top + look.px(tileHeight)}, i});
                }
                break;
            }
            case Editor::STYLE_TOGGLES: {
                // Choice index bit 1 is Bold and bit 2 is Italic, so each toggle flips one bit.
                const int style = std::max(current, 0);
                const int size = look.px(toggleSize);
                result.push_back({{client.left, client.top, client.left + size, client.top + size}, style ^ 1});
                const int italicLeft = client.left + size + spacing;
                result.push_back({{italicLeft, client.top, italicLeft + size, client.top + size}, style ^ 2});
                break;
            }
            default:
                for (int i = 0; i < count; ++i) {
                    const int left = client.left + (width + spacing) * i / count;
                    const int right = client.left + (width + spacing) * (i + 1) / count - spacing;
                    result.push_back({{left, client.top, right, client.bottom}, i});
                }
                break;
            }
            return result;
        }

        static int hit(const std::vector<Part> &parts, POINT point) {
            for (size_t i = 0; i < parts.size(); ++i) {
                if (PtInRect(&parts[i].bounds, point)) {
                    return int(i);
                }
            }
            return -1;
        }

        // The choice an arrow key moves to; the grid and the tiles also move by rows.
        static int step(const FormField &field, int current, WPARAM key) {
            using Editor = FormField::Editor;
            const int count = int(field.choices.size());
            const int rowLength = field.editor == Editor::ANCHOR_GRID   ? 3
                                  : field.editor == Editor::CURVE_TILES ? tileColumns
                                                                        : 1;
            int next = std::max(current, 0);
            if (key == VK_LEFT) {
                --next;
            } else if (key == VK_RIGHT) {
                ++next;
            } else if (key == VK_UP) {
                next -= rowLength;
            } else if (key == VK_DOWN) {
                next += rowLength;
            } else if (key == VK_HOME) {
                next = 0;
            } else if (key == VK_END) {
                next = count - 1;
            } else {
                return current;
            }
            return std::clamp(next, 0, count - 1);
        }

        static void paint(HDC dc, const FormField &field, const RECT &client, int current, int hoveredPart,
                          bool focused, bool enabled, bool dimmed, const Look &look) {
            using Editor = FormField::Editor;
            const auto &theme = look.theme;
            PanelDrawing::fill(dc, client, theme.background);
            const auto areas = parts(field, client, current, look);
            for (size_t i = 0; i < areas.size(); ++i) {
                const Part &part = areas[i];
                const bool selected = field.editor == Editor::STYLE_TOGGLES
                                          ? (std::max(current, 0) & (i == 0 ? 1 : 2)) != 0
                                          : part.choice == current;
                const bool strong = field.editor == Editor::ANCHOR_GRID || field.editor == Editor::STYLE_TOGGLES;
                COLORREF face = theme.field, border = theme.track, ink = theme.foreground;
                if (selected && enabled) {
                    face = strong ? theme.accent : theme.selected;
                    border = theme.accent;
                    ink = strong ? theme.background : theme.selectedForeground;
                } else if (int(i) == hoveredPart && enabled) {
                    border = theme.accent;
                }
                if (!enabled || (dimmed && !selected)) {
                    ink = theme.secondary;
                }
                PanelDrawing::rounded(dc, part.bounds, face, border, look.px(6));
                paintPartContent(dc, field, part, i, ink, look);
            }
            // The grid and the toggles name the current choice beside them.
            if ((field.editor == Editor::ANCHOR_GRID || field.editor == Editor::STYLE_TOGGLES) && !areas.empty() &&
                current >= 0 && current < int(field.choices.size())) {
                const RECT caption{areas.back().bounds.right + look.px(12), client.top, client.right,
                                   field.editor == Editor::ANCHOR_GRID ? client.bottom : areas.back().bounds.bottom};
                PanelDrawing::text(dc, field.choices[size_t(current)].label, caption, theme.secondary, look.font);
            }
            if (focused) {
                paintFocus(dc, field, areas, current, theme.accent);
            }
        }

      private:
        static constexpr int gap = 4;
        static constexpr int segmentHeight = 28;
        static constexpr int curveHeight = 22;
        static constexpr int cellWidth = 34;
        static constexpr int cellHeight = 20;
        static constexpr int cellGap = 3;
        // Three per row leaves each tile wide enough for its whole name, such as Square root.
        static constexpr int tileColumns = 3;
        static constexpr int tileHeight = 48;
        // Matches the space between a setting's label and its control.
        static constexpr int tileRowGap = 7;
        static constexpr int toggleSize = 28;

        static void paintPartContent(HDC dc, const FormField &field, const Part &part, size_t index, COLORREF ink,
                                     const Look &look) {
            using Editor = FormField::Editor;
            const RECT &box = part.bounds;
            switch (field.editor) {
            case Editor::ANCHOR_GRID:
                break;
            case Editor::STYLE_TOGGLES: {
                const HFONT styled = styledFont(look.font, index == 0, index == 1);
                PanelDrawing::text(dc, index == 0 ? L"B" : L"I", box, ink, styled ? styled : look.font, DT_CENTER);
                if (styled) {
                    DeleteObject(styled);
                }
                break;
            }
            case Editor::CURVE_TILES: {
                const RECT graph{box.left + look.px(6), box.top + look.px(5), box.right - look.px(6),
                                 box.top + look.px(25)};
                drawCurve(dc, field, size_t(part.choice), graph, ink, look.px(2));
                // The label gets the lower 22 pixels, tall enough for descenders such as the g in Log.
                const RECT label{box.left + look.px(2), graph.bottom, box.right - look.px(2), box.bottom - look.px(1)};
                PanelDrawing::text(dc, field.choices[size_t(part.choice)].label, label, ink, look.font, DT_CENTER);
                break;
            }
            default: {
                RECT label{box.left + look.px(4), box.top + look.px(4), box.right - look.px(4), box.bottom - look.px(4)};
                if (field.curve) {
                    const RECT graph{box.left + look.px(10), box.top + look.px(5), box.right - look.px(10),
                                     box.top + look.px(curveHeight)};
                    drawCurve(dc, field, size_t(part.choice), graph, ink, look.px(2));
                    label.top = graph.bottom;
                }
                drawWrapped(dc, look.font, field.choices[size_t(part.choice)].label, label, ink);
                break;
            }
            }
        }

        static void paintFocus(HDC dc, const FormField &field, const std::vector<Part> &areas, int current,
                               COLORREF color) {
            if (areas.empty()) {
                return;
            }
            const bool toggles = field.editor == FormField::Editor::STYLE_TOGGLES;
            const auto found = std::find_if(areas.begin(), areas.end(),
                                            [current](const Part &part) { return part.choice == current; });
            RECT ring = toggles || found == areas.end() ? areas.front().bounds : found->bounds;
            InflateRect(&ring, 2, 2);
            PanelDrawing::border(dc, ring, color, 1);
        }

        // The curve the field gives for this choice, x and y both from 0 to 1, drawn across the area.
        static void drawCurve(HDC dc, const FormField &field, size_t choice, const RECT &area, COLORREF color,
                              int thickness) {
            constexpr int samples = 32;
            const int width = std::max(1L, area.right - area.left - 1);
            const int height = std::max(1L, area.bottom - area.top - 1);
            POINT points[samples + 1];
            for (int i = 0; i <= samples; ++i) {
                const double x = double(i) / samples;
                const double y = std::clamp(field.curve(choice, x), 0.0, 1.0);
                points[i] = {area.left + LONG(x * width + .5), area.bottom - 1 - LONG(y * height + .5)};
            }
            const HPEN pen = CreatePen(PS_SOLID, std::max(1, thickness), color);
            const HGDIOBJ previous = SelectObject(dc, pen);
            Polyline(dc, points, samples + 1);
            SelectObject(dc, previous);
            DeleteObject(pen);
        }

        static int wrappedHeight(HDC dc, HFONT font, const std::wstring &text, int width) {
            const HGDIOBJ previous = SelectObject(dc, font);
            RECT measured{0, 0, std::max(1, width), 0};
            UiLanguage::drawText(dc, text.c_str(), int(text.size()), &measured,
                                 DT_CALCRECT | DT_WORDBREAK | DT_CENTER | DT_NOPREFIX);
            SelectObject(dc, previous);
            return int(measured.bottom);
        }

        // Centred both ways, wrapping onto a second line when the label is wider than its segment.
        static void drawWrapped(HDC dc, HFONT font, const std::wstring &text, const RECT &area, COLORREF color) {
            const int textHeight = wrappedHeight(dc, font, text, int(area.right - area.left));
            RECT line = area;
            line.top = std::max<LONG>(area.top, (area.top + area.bottom - textHeight) / 2);
            line.bottom = std::min<LONG>(area.bottom, line.top + textHeight);
            const int saved = SaveDC(dc);
            if (saved == 0) {
                return;
            }
            SelectObject(dc, font);
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, color);
            UiLanguage::drawText(dc, text.c_str(), int(text.size()), &line,
                                 DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_END_ELLIPSIS);
            RestoreDC(dc, saved);
        }

        static HFONT styledFont(HFONT base, bool bold, bool italic) {
            LOGFONTW description{};
            if (!GetObjectW(base, sizeof(description), &description)) {
                return nullptr;
            }
            if (bold) {
                description.lfWeight = FW_BOLD;
            }
            description.lfItalic = italic ? TRUE : FALSE;
            return CreateFontIndirectW(&description);
        }
    };
} // namespace merutilm::rff2::workspace
