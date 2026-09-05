// Modified by GPT-5 on 2026-08-18, 2026-08-23, 2026-08-24, 2026-08-26, 2026-08-27, 2026-08-31
// Modified by Opus 5 on 2026-08-19, 2026-08-20, 2026-08-21, 2026-08-22, 2026-08-23, 2026-08-25, 2026-08-26, 2026-08-31, 2026-09-01, 2026-09-03

#include "TimelineWindow.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <cwchar>
#include <format>
#include <limits>
#include <ranges>
#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>
#include <commctrl.h>
#include <windowsx.h>

#include "../attr/VidTimelineTarget.h"
#include "../constants/Constants.hpp"
#include "../io/PreferencesIO.h"
#include "../io/TimelineIO.h"
#include "Callback.hpp"
#include "CallbackShader.hpp"
#include "RenderScene.hpp"
#include "SettingsMenu.hpp"
#include "IOUtilities.h"
#include "NumericExpression.hpp"
#include "SettingsWindow.hpp"
#include "CallbackVideo.hpp"
#include "VideoRenderScene.hpp"
#include "VideoWindow.hpp"
#include "../video/VideoFrameSource.hpp"
#include "../video/TimelineParams.hpp"
#include "opencv2/imgproc.hpp"

namespace merutilm::rff2 {
    namespace {
        constexpr auto TIMELINE_WINDOW_CLASS = L"RFF2TLW";
        std::atomic<int> openTimelineWindows{0};
        constexpr UINT WM_TIMELINE_PREVIEW_READY = WM_APP + 0x251;
        constexpr UINT WM_TIMELINE_EXPORT_FINISHED = WM_APP + 0x252;
        constexpr UINT_PTR PLAYBACK_TIMER = 1;
        constexpr UINT_PTR PREVIEW_STATUS_TIMER = 2;
        constexpr UINT_PTR EDGE_SCROLL_TIMER = 3;
        // A preview that returns sooner than this is never announced: the line would only blink.
        constexpr UINT PREVIEW_STATUS_DELAY = 250;
        // 30 preview steps a second is as fine as the scrubbed preview can follow.
        constexpr UINT PLAYBACK_INTERVAL = 33;
        // How often a drag held at an edge of the axis pulls the view or the stack along.
        constexpr UINT EDGE_SCROLL_INTERVAL = 16;
        // The share of the shown span the view travels each step, at the far side of the edge zone.
        constexpr float SCRUB_EDGE_RATE = 0.02f;
        // Selected UI colors use Tailwind CSS v3's MIT-licensed palette; see NOTICE.
        struct TimelineTheme {
            COLORREF background;
            COLORREF panel;
            COLORREF panelRaised;
            COLORREF previewBackground;
            COLORREF border;
            COLORREF grid;
            COLORREF text;
            COLORREF mutedText;
            COLORREF accent;
            COLORREF accentHover;
            COLORREF accentPressed;
            COLORREF accentSoft;
            COLORREF accentBorder;
            COLORREF focusRing;
            COLORREF accentText;
            COLORREF activeText;
            COLORREF buttonHoverBorder;
            COLORREF hold;
            COLORREF selected;
            COLORREF buttonHover;
            COLORREF toggleOff;
            COLORREF disabledTrack;
            COLORREF linkedTrack;
            COLORREF distanceTick;
        };

        constexpr TimelineTheme DARK_THEME = {
            .background = RGB(25, 28, 33), .panel = RGB(29, 32, 38), .panelRaised = RGB(35, 38, 45),
            .previewBackground = RGB(15, 17, 20), .border = RGB(48, 52, 59), .grid = RGB(41, 45, 52),
            .text = RGB(233, 235, 238), .mutedText = RGB(152, 157, 164), .accent = RGB(45, 99, 200),
            .accentHover = RGB(45, 99, 200), .accentPressed = RGB(90, 135, 214),
            .accentSoft = RGB(34, 71, 138), .accentBorder = RGB(122, 160, 224),
            .focusRing = RGB(160, 190, 236), .accentText = RGB(160, 190, 236),
            .activeText = RGB(233, 240, 251), .buttonHoverBorder = RGB(90, 95, 103),
            .hold = RGB(245, 158, 11), .selected = RGB(249, 115, 22), .buttonHover = RGB(45, 49, 56),
            .toggleOff = RGB(58, 62, 70), .disabledTrack = RGB(85, 89, 96),
            .linkedTrack = RGB(226, 228, 232), .distanceTick = RGB(74, 222, 128),
        };

        constexpr TimelineTheme LIGHT_THEME = {
            .background = RGB(247, 249, 252), .panel = RGB(255, 255, 255), .panelRaised = RGB(248, 250, 252),
            .previewBackground = RGB(237, 241, 245), .border = RGB(214, 222, 232), .grid = RGB(229, 234, 240),
            .text = RGB(17, 24, 39), .mutedText = RGB(75, 85, 99), .accent = RGB(37, 99, 235),
            .accentHover = RGB(29, 78, 216), .accentPressed = RGB(30, 64, 175),
            .accentSoft = RGB(239, 246, 255), .accentBorder = RGB(191, 219, 254),
            .focusRing = RGB(147, 197, 253), .accentText = RGB(29, 78, 216),
            .activeText = RGB(255, 255, 255), .buttonHoverBorder = RGB(184, 195, 209),
            .hold = RGB(217, 119, 6), .selected = RGB(234, 88, 12), .buttonHover = RGB(243, 246, 250),
            .toggleOff = RGB(214, 222, 232), .disabledTrack = RGB(100, 116, 139),
            .linkedTrack = RGB(51, 65, 85), .distanceTick = RGB(21, 128, 61),
        };

        const TimelineTheme &timelineTheme(const bool lightMode) {
            return lightMode ? LIGHT_THEME : DARK_THEME;
        }
        constexpr float MIN_VIEW_SPAN = 1.0f;
        // Past this many parameters changing between one report and the next, what happened is a
        // preset being loaded rather than a row being moved, and none of it is recorded.
        constexpr size_t MAX_RECORDED_AT_ONCE = 8;
        // Steps of the timeline Undo walks back through; the oldest is dropped past this.
        constexpr size_t MAX_UNDO_STEPS = 64;
        // A change arriving within this many milliseconds of the last one belongs to the same step.
        constexpr ULONGLONG UNDO_COALESCE_MS = 500;

        // The Shader menu's own panels, as the Timeline Editor offers them: the same code builds
        // them here, so what opens is that panel and not a set of rows standing for it. groupPrefix
        // is what the parameter table calls the settings the panel covers - one panel edits all
        // four Texture layers, which the table holds apart as Texture 1 to Texture 4.
        struct ShaderPanel {
            const wchar_t *name;
            const std::function<void(SettingsMenu &, RenderScene &)> *callback;
            const wchar_t *groupPrefix;
        };

        const std::array SHADER_PANELS = {
            ShaderPanel{L"Palette", &CallbackShader::PALETTE, L"Palette"},
            ShaderPanel{L"Stripe", &CallbackShader::STRIPE, L"Stripe"},
            ShaderPanel{L"Slope", &CallbackShader::SLOPE, L"Slope"},
            ShaderPanel{L"Color", &CallbackShader::COLOR, L"Color"},
            ShaderPanel{L"Fog", &CallbackShader::FOG, L"Fog"},
            ShaderPanel{L"Bloom", &CallbackShader::BLOOM, L"Bloom"},
            ShaderPanel{L"Texture", &CallbackShader::TEXTURE, L"Texture"},
            ShaderPanel{L"Pattern", &CallbackShader::PATTERN, L"Pattern"},
            ShaderPanel{L"Warp", &CallbackShader::WARP, L"Warp"},
        };
        // A panel worth opening over a PNG source is one holding a parameter that still moves there.
        bool panelMovesOverStaticImage(const ShaderPanel &panel) {
            const size_t prefix = std::wcslen(panel.groupPrefix);
            return std::ranges::any_of(TimelineParams::all(), [&](const TimelineParamDesc &param) {
                return std::wcsncmp(param.group, panel.groupPrefix, prefix) == 0 &&
                       TimelineParams::movesOverStaticImage(param.id);
            });
        }

        // A parameter whose own range is wider than this is plotted against its keys, not its range.
        constexpr float WIDE_VALUE_RANGE = 1000.0f;
        constexpr uint16_t SPEED_TARGET = vidTimelineTargetId(VidTimelineTarget::SPEED);
        constexpr uint16_t COLOR_ANIMATION_TARGET = vidTimelineTargetId(VidTimelineTarget::PALETTE_ANIMATION_SPEED);
        constexpr uint16_t FOG_OPACITY_TARGET = vidTimelineTargetId(VidTimelineTarget::FOG_OPACITY);
        constexpr uint16_t CYCLE_R_TARGET = vidTimelineTargetId(VidTimelineTarget::PALETTE_INTERVAL_R);
        constexpr uint16_t CYCLE_G_TARGET = vidTimelineTargetId(VidTimelineTarget::PALETTE_INTERVAL_G);
        constexpr uint16_t CYCLE_B_TARGET = vidTimelineTargetId(VidTimelineTarget::PALETTE_INTERVAL_B);
        constexpr wchar_t FORMULA_FIELD_HINT[] =
            L"Click: enter a value or formula (+ \u2212 \u00d7 \u00f7 parentheses) \u00b7 Drag: scrub";

        int sc(const int value) {
            return Constants::Win32::settingsScaled(value);
        }

        void fillRect(const HDC hdc, const RECT &rect, const COLORREF color) {
            const HBRUSH brush = CreateSolidBrush(color);
            FillRect(hdc, &rect, brush);
            DeleteObject(brush);
        }

        void frameRect(const HDC hdc, const RECT &rect, const COLORREF color) {
            const HBRUSH brush = CreateSolidBrush(color);
            FrameRect(hdc, &rect, brush);
            DeleteObject(brush);
        }

        bool contains(const RECT &rect, const POINT point) {
            return PtInRect(&rect, point) != FALSE;
        }

        void drawText(const HDC hdc, const std::wstring &text, RECT rect, const COLORREF color, const UINT format,
                      const HFONT font) {
            const HGDIOBJ previous = SelectObject(hdc, font);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, color);
            DrawTextW(hdc, text.c_str(), -1, &rect, format);
            SelectObject(hdc, previous);
        }

        std::wstring durationText(const float value) {
            const float seconds = std::max(0.0f, value);
            const int minutes = static_cast<int>(seconds / 60.0f);
            return std::format(L"{:02}:{:04.1f}", minutes, seconds - static_cast<float>(minutes) * 60.0f);
        }

        // Dragging a handle snaps to whole keyframes.
        float snapDepth(const float depth) {
            return std::round(depth);
        }

        int depthX(const float depth, const float startDepth, const float endDepth, const RECT &axis) {
            const float span = std::max(startDepth - endDepth, 1e-6f);
            const float ratio = std::clamp((startDepth - depth) / span, -64.0f, 64.0f);
            return axis.left + static_cast<int>(ratio * static_cast<float>(axis.right - axis.left));
        }

        bool visibleX(const int x, const RECT &axis, const int slack) {
            return x >= axis.left - slack && x <= axis.right + slack;
        }

        // Rounds the label step to 1, 2 or 5 times a power of ten, so the labels land on round depths.
        float depthTickStep(const float span, const int slots) {
            const float raw = std::max(span, 1e-4f) / static_cast<float>(std::max(slots, 1));
            const float magnitude = std::pow(10.0f, std::floor(std::log10(raw)));
            const float normalized = raw / magnitude;
            const float nice = normalized <= 1.0f ? 1.0f : normalized <= 2.0f ? 2.0f :
                               normalized <= 5.0f ? 5.0f : 10.0f;
            return nice * magnitude;
        }

        bool colorCycleTarget(const uint16_t targetId) {
            return targetId == CYCLE_R_TARGET || targetId == CYCLE_G_TARGET || targetId == CYCLE_B_TARGET;
        }

        std::wstring trackName(const uint16_t targetId) {
            if (targetId == SPEED_TARGET) {
                return L"Speed";
            }
            if (targetId == COLOR_ANIMATION_TARGET) {
                return L"Color Animation Speed";
            }
            if (targetId == CYCLE_R_TARGET) {
                return L"Cycle Length R";
            }
            if (targetId == CYCLE_G_TARGET) {
                return L"Cycle Length G";
            }
            if (targetId == CYCLE_B_TARGET) {
                return L"Cycle Length B";
            }
            if (const TimelineParamDesc *param = TimelineParams::find(targetId); param != nullptr) {
                return std::wstring(param->group) + L" / " + param->name;
            }
            return std::format(L"Track 0x{:04X}", targetId);
        }

        // The R row stands for all three channels while they are linked.
        std::wstring rowName(const uint16_t targetId, const bool linkedRgb) {
            if (linkedRgb && targetId == CYCLE_R_TARGET) {
                return L"Cycle Length RGB";
            }
            return trackName(targetId);
        }

        std::wstring interpolationName(const VidKeyInterpolation interpolation) {
            switch (interpolation) {
                case VidKeyInterpolation::LINEAR:
                    return L"Linear";
                case VidKeyInterpolation::SMOOTH:
                    return L"Smooth";
                case VidKeyInterpolation::CUBIC:
                    return L"Cubic";
                case VidKeyInterpolation::STEP:
                default:
                    return L"Step";
            }
        }

        bool editableTarget(const uint16_t targetId) {
            if (targetId == SPEED_TARGET) {
                return true;
            }
            const TimelineParamDesc *param = TimelineParams::find(targetId);
            // A color has no single value to plot on a row or to drag a key up and down, so a color
            // track is shown as the keys it carries and edited nowhere.
            return param != nullptr && param->kind != TimelineParamKind::COLOR;
        }

        // How far an arrow key moves a typed value, read off the range the parameter is edited in.
        double keyValueStep(const uint16_t targetId) {
            if (colorCycleTarget(targetId)) {
                return 1.0;
            }
            const TimelineParamDesc *param = TimelineParams::find(targetId);
            if (param == nullptr) {
                return 0.1;
            }
            if (param->kind == TimelineParamKind::BOOL || param->kind == TimelineParamKind::ENUM) {
                return 1.0;
            }
            return param->maxValue - param->minValue <= 2.0f ? 0.01 : 0.1;
        }

        // A switch or a mode means nothing between its steps, so its keys hold until the next one.
        VidKeyInterpolation defaultInterpolation(const uint16_t targetId) {
            const TimelineParamDesc *param = TimelineParams::find(targetId);
            return param != nullptr && (param->kind == TimelineParamKind::BOOL ||
                                        param->kind == TimelineParamKind::ENUM)
                       ? VidKeyInterpolation::STEP
                       : VidKeyInterpolation::SMOOTH;
        }

        float evaluateDisplayedTrack(const VidTimelineTrack &track, const uint16_t targetId, const float depth,
                                     const float fallback) {
            if (const TimelineParamDesc *param = TimelineParams::find(targetId); param != nullptr) {
                return TimelineSchedule::evaluateTrack(track, depth, fallback, param->minValue, param->maxValue);
            }
            return TimelineSchedule::evaluateTrack(track, depth, fallback);
        }

        // A track nobody has keyed yet: the two ends a track opens with, both on one value. A file
        // may hold a ramp of two keys, which differ, and is a curve like any other.
        bool flatTrack(const VidTimelineTrack &track) {
            return track.keys.size() <= 2 &&
                   std::ranges::all_of(track.keys, [&track](const VidTimelineKey &key) {
                       return key.value == track.keys.front().value;
                   });
        }

        bool flatColorTrack(const VidTimelineTrack &track) {
            return track.keys.size() <= 2 &&
                   std::ranges::all_of(track.keys, [&track](const VidTimelineKey &key) {
                       return key.color == track.keys.front().color;
                   });
        }

        bool sameKeys(const std::vector<VidTimelineKey> &a, const std::vector<VidTimelineKey> &b) {
            return std::ranges::equal(a, b, [](const VidTimelineKey &x, const VidTimelineKey &y) {
                return x.depth == y.depth && x.value == y.value && x.out == y.out;
            });
        }

        COLORREF trackColor(const uint16_t targetId, const bool active, const bool lightMode) {
            if (!active) {
                return timelineTheme(lightMode).disabledTrack;
            }
            if (targetId == COLOR_ANIMATION_TARGET) {
                return lightMode ? RGB(126, 34, 206) : RGB(168, 85, 247);
            }
            if (targetId == FOG_OPACITY_TARGET) {
                return lightMode ? RGB(15, 118, 110) : RGB(20, 184, 166);
            }
            if (targetId == CYCLE_R_TARGET) {
                return lightMode ? RGB(220, 38, 38) : RGB(248, 113, 113);
            }
            if (targetId == CYCLE_G_TARGET) {
                return lightMode ? RGB(21, 128, 61) : RGB(74, 222, 128);
            }
            if (targetId == CYCLE_B_TARGET) {
                return lightMode ? RGB(37, 99, 235) : RGB(96, 165, 250);
            }
            const TimelineParamDesc *param = TimelineParams::find(targetId);
            if (param == nullptr) {
                return timelineTheme(lightMode).accent;
            }
            // Every row of one settings group is drawn in one color, so a stack of them is read by group.
            switch (param->dirty) {
                case TimelineDirtyMask::PALETTE:
                    return lightMode ? RGB(190, 24, 93) : RGB(244, 114, 182);
                case TimelineDirtyMask::STRIPE:
                    return lightMode ? RGB(161, 98, 7) : RGB(250, 204, 21);
                case TimelineDirtyMask::SLOPE:
                    return lightMode ? RGB(79, 70, 229) : RGB(129, 140, 248);
                case TimelineDirtyMask::COLOR:
                    return lightMode ? RGB(14, 116, 144) : RGB(34, 211, 238);
                case TimelineDirtyMask::FOG:
                    return lightMode ? RGB(15, 118, 110) : RGB(20, 184, 166);
                case TimelineDirtyMask::BLOOM:
                    return lightMode ? RGB(194, 65, 12) : RGB(251, 146, 60);
                case TimelineDirtyMask::TEXTURE:
                    return lightMode ? RGB(77, 124, 15) : RGB(163, 230, 53);
                case TimelineDirtyMask::PATTERN:
                    return lightMode ? RGB(162, 28, 175) : RGB(217, 70, 239);
                case TimelineDirtyMask::WARP:
                    return lightMode ? RGB(3, 105, 161) : RGB(56, 189, 248);
                default:
                    return timelineTheme(lightMode).accent;
            }
        }

        int valueY(const float value, const float minValue, const float maxValue, const RECT &row) {
            const float span = std::max(maxValue - minValue, 1e-6f);
            const float ratio = std::clamp((value - minValue) / span, 0.0f, 1.0f);
            return row.bottom - sc(8) - static_cast<int>((row.bottom - row.top - sc(16)) * ratio);
        }

        void drawButton(const HDC hdc, const RECT &rect, const std::wstring &label, const bool hovered,
                        const bool active, const HFONT font, const TimelineTheme &theme) {
            const COLORREF fill = active ? hovered ? theme.accentHover : theme.accent
                                         : hovered ? theme.buttonHover : theme.panelRaised;
            const COLORREF border = active ? theme.accentPressed
                                           : hovered ? theme.buttonHoverBorder : theme.border;
            const HBRUSH brush = CreateSolidBrush(fill);
            const HPEN pen = CreatePen(PS_SOLID, 1, border);
            const HGDIOBJ oldBrush = SelectObject(hdc, brush);
            const HGDIOBJ oldPen = SelectObject(hdc, pen);
            RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, sc(8), sc(8));
            SelectObject(hdc, oldPen);
            SelectObject(hdc, oldBrush);
            DeleteObject(pen);
            DeleteObject(brush);
            drawText(hdc, label, rect, active ? theme.activeText : theme.text,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE, font);
        }

        HBITMAP createPreviewBitmap(const HWND window, const cv::Mat &image, SIZE &size) {
            if (image.empty()) {
                return nullptr;
            }
            cv::Mat bgra;
            if (image.channels() == 3) {
                cv::cvtColor(image, bgra, cv::COLOR_BGR2BGRA);
            } else if (image.channels() == 4) {
                bgra = image;
            } else {
                return nullptr;
            }
            if (bgra.depth() != CV_8U) {
                bgra.convertTo(bgra, CV_8U);
            }

            BITMAPINFO info = {};
            info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            info.bmiHeader.biWidth = bgra.cols;
            info.bmiHeader.biHeight = -bgra.rows;
            info.bmiHeader.biPlanes = 1;
            info.bmiHeader.biBitCount = 32;
            info.bmiHeader.biCompression = BI_RGB;
            void *pixels = nullptr;
            const HDC hdc = GetDC(window);
            const HBITMAP bitmap = CreateDIBSection(hdc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
            ReleaseDC(window, hdc);
            if (bitmap == nullptr || pixels == nullptr) {
                if (bitmap != nullptr) {
                    DeleteObject(bitmap);
                }
                return nullptr;
            }
            const size_t rowBytes = static_cast<size_t>(bgra.cols) * 4;
            for (int y = 0; y < bgra.rows; ++y) {
                std::memcpy(static_cast<unsigned char *>(pixels) + static_cast<size_t>(y) * rowBytes,
                            bgra.ptr(y), rowBytes);
            }
            size = {bgra.cols, bgra.rows};
            return bitmap;
        }

        int textWidth(const HDC hdc, const std::wstring &text, const HFONT font) {
            const HGDIOBJ previous = SelectObject(hdc, font);
            SIZE size = {};
            GetTextExtentPoint32W(hdc, text.c_str(), static_cast<int>(text.size()), &size);
            SelectObject(hdc, previous);
            return size.cx;
        }

        void fillRoundRect(const HDC hdc, const RECT &rect, const COLORREF fill, const COLORREF border,
                           const int radius) {
            const HBRUSH brush = CreateSolidBrush(fill);
            const HPEN pen = CreatePen(PS_SOLID, 1, border);
            const HGDIOBJ oldBrush = SelectObject(hdc, brush);
            const HGDIOBJ oldPen = SelectObject(hdc, pen);
            RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
            SelectObject(hdc, oldPen);
            SelectObject(hdc, oldBrush);
            DeleteObject(pen);
            DeleteObject(brush);
        }

        // A field framed with its caption riding the top border, and the field itself is what gets clicked.
        RECT drawCaptionBox(const HDC hdc, const RECT &box, const std::wstring &caption, const COLORREF background,
                            const HFONT captionFont, const bool hovered, const bool active,
                            const TimelineTheme &theme) {
            fillRoundRect(hdc, box, hovered ? theme.buttonHover : theme.panelRaised,
                          active ? theme.accent : theme.border, sc(10));
            const int captionWidth = textWidth(hdc, caption, captionFont) + sc(10);
            const RECT plate = {box.left + sc(12), box.top - sc(11), box.left + sc(12) + captionWidth,
                                box.top + sc(11)};
            fillRect(hdc, plate, background);
            drawText(hdc, caption, plate, theme.mutedText, DT_CENTER | DT_VCENTER | DT_SINGLELINE, captionFont);
            return {box.left + sc(12), box.top + sc(10), box.right - sc(12), box.bottom - sc(4)};
        }

        void drawToggle(const HDC hdc, const RECT &rect, const bool on, const TimelineTheme &theme) {
            const int radius = static_cast<int>(rect.bottom - rect.top) / 2;
            fillRoundRect(hdc, rect, on ? theme.accent : theme.toggleOff,
                          on ? theme.accentBorder : theme.border, radius * 2);
            const int knob = std::max(radius - sc(4), sc(3));
            const int centerX = on ? static_cast<int>(rect.right) - radius : static_cast<int>(rect.left) + radius;
            const int centerY = static_cast<int>(rect.top + rect.bottom) / 2;
            const HBRUSH brush = CreateSolidBrush(RGB(245, 246, 248));
            const HPEN pen = CreatePen(PS_SOLID, 1, RGB(226, 228, 232));
            const HGDIOBJ oldBrush = SelectObject(hdc, brush);
            const HGDIOBJ oldPen = SelectObject(hdc, pen);
            Ellipse(hdc, centerX - knob, centerY - knob, centerX + knob, centerY + knob);
            SelectObject(hdc, oldPen);
            SelectObject(hdc, oldBrush);
            DeleteObject(pen);
            DeleteObject(brush);
        }

        enum class TransportGlyph { PLAY, PAUSE, STOP, LOOP };

        // The transport glyphs are drawn rather than typed, so no symbol font has to be present.
        void drawTransportButton(const HDC hdc, const RECT &rect, const TransportGlyph glyph, const bool hovered,
                                 const bool active, const TimelineTheme &theme) {
            fillRoundRect(hdc, rect, active ? theme.accent : hovered ? theme.buttonHover : theme.panelRaised,
                          active ? theme.accentBorder : theme.border, sc(8));
            const COLORREF ink = active ? theme.activeText : theme.text;
            const int cx = static_cast<int>(rect.left + rect.right) / 2;
            const int cy = static_cast<int>(rect.top + rect.bottom) / 2;
            const int size = std::max(static_cast<int>(std::min(rect.right - rect.left, rect.bottom - rect.top)) / 4,
                                      sc(4));
            const HBRUSH brush = CreateSolidBrush(ink);
            const HPEN pen = CreatePen(PS_SOLID, sc(2), ink);
            const HGDIOBJ oldBrush = SelectObject(hdc, brush);
            const HGDIOBJ oldPen = SelectObject(hdc, pen);
            switch (glyph) {
                case TransportGlyph::PLAY: {
                    const POINT points[3] = {{cx - size + sc(2), cy - size}, {cx + size, cy},
                                             {cx - size + sc(2), cy + size}};
                    Polygon(hdc, points, 3);
                    break;
                }
                case TransportGlyph::PAUSE:
                    Rectangle(hdc, cx - size, cy - size, cx - sc(2), cy + size);
                    Rectangle(hdc, cx + sc(2), cy - size, cx + size, cy + size);
                    break;
                case TransportGlyph::STOP:
                    Rectangle(hdc, cx - size, cy - size, cx + size, cy + size);
                    break;
                case TransportGlyph::LOOP: {
                    // The familiar repeat mark: a closed pill with one arrow head per straight run.
                    const int halfWidth = size + sc(3);
                    const int halfHeight = std::max(size - sc(3), sc(3));
                    const HGDIOBJ hollow = SelectObject(hdc, GetStockObject(NULL_BRUSH));
                    RoundRect(hdc, cx - halfWidth, cy - halfHeight, cx + halfWidth, cy + halfHeight,
                              halfHeight * 2, halfHeight * 2);
                    SelectObject(hdc, hollow);
                    const int head = std::max(halfHeight - sc(1), sc(3));
                    const POINT forward[3] = {{cx - head, cy - halfHeight - head},
                                              {cx - head, cy - halfHeight + head},
                                              {cx + head, cy - halfHeight}};
                    const POINT backward[3] = {{cx + head, cy + halfHeight - head},
                                               {cx + head, cy + halfHeight + head},
                                               {cx - head, cy + halfHeight}};
                    Polygon(hdc, forward, 3);
                    Polygon(hdc, backward, 3);
                    break;
                }
            }
            SelectObject(hdc, oldPen);
            SelectObject(hdc, oldBrush);
            DeleteObject(pen);
            DeleteObject(brush);
        }

        void drawDot(const HDC hdc, const int cx, const int cy, const int radius, const COLORREF color) {
            const HBRUSH brush = CreateSolidBrush(color);
            const HPEN pen = CreatePen(PS_SOLID, 1, color);
            const HGDIOBJ oldBrush = SelectObject(hdc, brush);
            const HGDIOBJ oldPen = SelectObject(hdc, pen);
            Ellipse(hdc, cx - radius, cy - radius, cx + radius, cy + radius);
            SelectObject(hdc, oldPen);
            SelectObject(hdc, oldBrush);
            DeleteObject(pen);
            DeleteObject(brush);
        }

        // One drawn mark per row, so a track is recognizable before its name is read.
        void drawTrackIcon(const HDC hdc, const RECT &rect, const uint16_t targetId, const COLORREF color) {
            const int cx = static_cast<int>(rect.left + rect.right) / 2;
            const int cy = static_cast<int>(rect.top + rect.bottom) / 2;
            const int radius = std::max(
                static_cast<int>(std::min(rect.right - rect.left, rect.bottom - rect.top)) / 2 - sc(2), sc(4));
            const HPEN pen = CreatePen(PS_SOLID, sc(2), color);
            const HGDIOBJ oldPen = SelectObject(hdc, pen);
            const HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
            if (targetId == SPEED_TARGET) {
                Ellipse(hdc, cx - radius, cy - radius, cx + radius, cy + radius);
                MoveToEx(hdc, cx, cy + sc(1), nullptr);
                LineTo(hdc, cx + radius / 2, cy - radius / 2);
            } else if (colorCycleTarget(targetId)) {
                // A complete centered ring keeps the Cycle Length mark aligned with every other track icon.
                Ellipse(hdc, cx - radius, cy - radius, cx + radius, cy + radius);
            } else if (targetId == COLOR_ANIMATION_TARGET) {
                Ellipse(hdc, cx - radius, cy - radius, cx + radius, cy + radius);
                drawDot(hdc, cx - radius / 2, cy - radius / 3, sc(2), color);
                drawDot(hdc, cx + radius / 2, cy - radius / 3, sc(2), color);
                drawDot(hdc, cx, cy + radius / 2, sc(2), color);
            } else if (targetId == FOG_OPACITY_TARGET) {
                Arc(hdc, cx - radius, cy - radius / 2, cx, cy + radius, cx - radius, cy + radius, cx, cy);
                Arc(hdc, cx - radius / 2, cy - radius, cx + radius / 2, cy + radius / 2, cx - radius / 2, cy,
                    cx + radius / 2, cy);
                MoveToEx(hdc, cx - radius, cy + radius / 2, nullptr);
                LineTo(hdc, cx + radius, cy + radius / 2);
            } else {
                Rectangle(hdc, cx - radius, cy - radius / 2, cx + radius, cy + radius / 2);
            }
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(pen);
        }

        void drawMagnifier(const HDC hdc, const RECT &rect, const COLORREF color) {
            const int radius = std::max(
                static_cast<int>(std::min(rect.right - rect.left, rect.bottom - rect.top)) / 3, sc(4));
            const int cx = static_cast<int>(rect.left + rect.right) / 2 - sc(2);
            const int cy = static_cast<int>(rect.top + rect.bottom) / 2 - sc(2);
            const HPEN pen = CreatePen(PS_SOLID, sc(2), color);
            const HGDIOBJ oldPen = SelectObject(hdc, pen);
            const HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Ellipse(hdc, cx - radius, cy - radius, cx + radius, cy + radius);
            MoveToEx(hdc, cx + radius - sc(1), cy + radius - sc(1), nullptr);
            LineTo(hdc, cx + radius + sc(5), cy + radius + sc(5));
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(pen);
        }

        // The bracket mark of a fit-to-window control.
        void registerTimelineWindowClass() {
            static const bool registered = [] {
                WNDCLASSEXW wc = {};
                wc.cbSize = sizeof(wc);
                wc.style = CS_DBLCLKS;
                wc.hInstance = GetModuleHandleW(nullptr);
                wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
                wc.hIcon = static_cast<HICON>(LoadImageW(wc.hInstance, MAKEINTRESOURCEW(1), IMAGE_ICON, 32, 32,
                                                        LR_DEFAULTCOLOR));
                wc.lpfnWndProc = TimelineWindow::windowProc;
                wc.lpszClassName = TIMELINE_WINDOW_CLASS;
                return RegisterClassExW(&wc) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
            }();
            (void) registered;
        }
    }

    TimelineWindow::TimelineWindow(SettingsMenu &menu, RenderScene &scene) :
        engine(scene.engine),
        sourceAttribute(&scene.getAttribute()),
        attribute(scene.getAttribute()),
        settingsMenu(&menu),
        renderScene(&scene),
        schedule(TimelineSchedule::create(scene.getAttribute().video.timeline,
                                          scene.getAttribute().video.timeline.estimateKeyframes,
                                          -scene.getAttribute().video.animation.overZoom,
                                           scene.getAttribute().video.animation.mps)) {
        lightMode = timelineLightMode();
        titleFont = CreateFontW(sc(30), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                DEFAULT_PITCH | FF_SWISS, Constants::Win32::uiFontFace());
        bodyFont = CreateFontW(sc(26), 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                               DEFAULT_PITCH | FF_SWISS, Constants::Win32::uiFontFace());
        smallFont = CreateFontW(sc(22), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                DEFAULT_PITCH | FF_SWISS, Constants::Win32::uiFontFace());
        captionFont = CreateFontW(sc(20), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                  DEFAULT_PITCH | FF_SWISS, Constants::Win32::uiFontFace());
        valueFont = CreateFontW(sc(25), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                DEFAULT_PITCH | FF_SWISS, Constants::Win32::uiFontFace());
        fieldEditBrush = CreateSolidBrush(timelineTheme(lightMode).panelRaised);
        ensureEditableTracks();
        undoBaseline = attribute.video.timeline;
        previewDepth = schedule.getStartDepth();
        resetView();
    }

    VidTimelineTrack *TimelineWindow::track(const uint16_t targetId) {
        for (auto &track: attribute.video.timeline.tracks) {
            if (track.targetId == targetId) {
                return &track;
            }
        }
        return nullptr;
    }

    const VidTimelineTrack *TimelineWindow::track(const uint16_t targetId) const {
        return TimelineSchedule::findTrack(attribute.video.timeline, targetId);
    }

    float TimelineWindow::baseValue(const uint16_t targetId) const {
        if (targetId == SPEED_TARGET) {
            return std::max(attribute.video.animation.mps, VidTimelineAttribute::MIN_SPEED);
        }
        const TimelineParamDesc *param = TimelineParams::find(targetId);
        return param != nullptr && param->getValue != nullptr ? param->getValue(attribute.shader) : 0.0f;
    }

    VidTimelineTrack &TimelineWindow::ensureScalarTrack(const uint16_t targetId) {
        if (VidTimelineTrack *existing = track(targetId); existing != nullptr) {
            return *existing;
        }
        const float value = baseValue(targetId);
        const VidKeyInterpolation out = defaultInterpolation(targetId);
        // A new track starts flat on the value the settings already hold, so adding one changes nothing
        // until one of its keys is moved.
        VidTimelineTrack track = {
            .targetId = targetId,
            .enabled = true,
            .keys = {
                {.depth = attribute.video.timeline.estimateKeyframes, .value = value,
                 .color = glm::vec4(1.0f), .out = out},
                {.depth = -attribute.video.animation.overZoom, .value = value,
                 .color = glm::vec4(1.0f), .out = out}
            }
        };
        if (targetId == SPEED_TARGET) {
            attribute.video.timeline.tracks.insert(attribute.video.timeline.tracks.begin(), std::move(track));
            return attribute.video.timeline.tracks.front();
        }
        attribute.video.timeline.tracks.push_back(std::move(track));
        return attribute.video.timeline.tracks.back();
    }

    void TimelineWindow::ensureEditableTracks() {
        // A timeline that carries no track at all opens on the two a pacing is most often built from;
        // every other parameter is added from the track menu, and a removed one stays removed.
        const bool fresh = attribute.video.timeline.tracks.empty();
        (void) ensureScalarTrack(SPEED_TARGET);
        // The color animation is the palette's, which a PNG source never runs, so a timeline opened
        // on one starts with the pacing alone rather than a track that could not move a picture.
        if (fresh && !attribute.video.data.isStatic) {
            (void) ensureScalarTrack(COLOR_ANIMATION_TARGET);
        }
    }

    float TimelineWindow::playheadValue(const uint16_t targetId) const {
        const float base = baseValue(targetId);
        const VidTimelineTrack *current = track(targetId);
        return current == nullptr ? base : evaluateDisplayedTrack(*current, targetId, previewDepth, base);
    }

    void TimelineWindow::setParameterValue(const uint16_t targetId, const float value) {
        const TimelineParamDesc *param = TimelineParams::find(targetId);
        if (param == nullptr || !editableTarget(targetId)) {
            MessageBeep(MB_ICONWARNING);
            return;
        }
        const bool created = track(targetId) == nullptr;
        VidTimelineTrack &current = ensureScalarTrack(targetId);
        current.enabled = true;
        const float depth = std::clamp(snapDepth(previewDepth), schedule.getEndDepth(), schedule.getStartDepth());
        const float clamped = std::clamp(value, param->minValue, param->maxValue);
        if (created) {
            // The row it just gained sits at the foot of the stack, so the stack is scrolled to it.
            // How far that is is known once the rows are laid out, and it is held to their range there.
            trackScrollOffset = std::numeric_limits<int>::max() / 2;
        }
        // A track that carries one value and nothing more is not a curve yet, and setting the
        // parameter sets it throughout, the way the Shader settings themselves do - a value that
        // took hold around the playhead alone and nowhere else is not what setting one reads as.
        // Putting a key on the row is what turns the parameter into a curve, and from then on the
        // panel writes to the key at the playhead.
        if (flatTrack(current)) {
            for (auto &key: current.keys) {
                key.value = clamped;
            }
            selectedTrackTarget = targetId;
            selectedTrackKey = -1;
            hoveredTrackKey = {};
            (void) syncLinkedColorCycle(targetId);
            commitTimeline();
            return;
        }
        int at = -1;
        for (int i = 0; i < static_cast<int>(current.keys.size()); ++i) {
            if (std::abs(current.keys[i].depth - depth) < 1.0f) {
                at = i;
                break;
            }
        }
        if (at >= 0) {
            current.keys[at].value = clamped;
        } else {
            if (current.keys.size() >= VidTimelineAttribute::MAX_KEYS_PER_TRACK) {
                MessageBeep(MB_ICONWARNING);
                return;
            }
            current.keys.push_back({.depth = depth, .value = clamped, .color = glm::vec4(1.0f),
                                    .out = defaultInterpolation(targetId)});
            std::ranges::stable_sort(current.keys, [](const VidTimelineKey &a, const VidTimelineKey &b) {
                return a.depth > b.depth;
            });
            at = 0;
            float nearest = std::abs(current.keys.front().depth - depth);
            for (int i = 1; i < static_cast<int>(current.keys.size()); ++i) {
                if (const float distance = std::abs(current.keys[i].depth - depth); distance < nearest) {
                    at = i;
                    nearest = distance;
                }
            }
        }
        selectedTrackTarget = targetId;
        selectedTrackKey = at;
        hoveredTrackKey = {};
        (void) syncLinkedColorCycle(targetId);
        commitTimeline();
    }

    void TimelineWindow::setParameterColor(const uint16_t targetId, const glm::vec4 &color) {
        const TimelineParamDesc *param = TimelineParams::find(targetId);
        if (param == nullptr || param->kind != TimelineParamKind::COLOR) {
            return;
        }
        const bool created = track(targetId) == nullptr;
        VidTimelineTrack &current = ensureScalarTrack(targetId);
        current.enabled = true;
        const float depth = std::clamp(snapDepth(previewDepth), schedule.getEndDepth(), schedule.getStartDepth());
        if (created) {
            trackScrollOffset = std::numeric_limits<int>::max() / 2;
        }
        // A color track holding one color throughout is not a curve yet, as a number's is not.
        if (created || flatColorTrack(current)) {
            for (auto &key: current.keys) {
                key.color = color;
            }
            hoveredTrackKey = {};
            commitTimeline();
            return;
        }
        for (auto &key: current.keys) {
            if (std::abs(key.depth - depth) < 1.0f) {
                key.color = color;
                commitTimeline();
                return;
            }
        }
        if (current.keys.size() >= VidTimelineAttribute::MAX_KEYS_PER_TRACK) {
            MessageBeep(MB_ICONWARNING);
            return;
        }
        current.keys.push_back({.depth = depth, .value = 0.0f, .color = color,
                                .out = VidKeyInterpolation::SMOOTH});
        std::ranges::stable_sort(current.keys, [](const VidTimelineKey &a, const VidTimelineKey &b) {
            return a.depth > b.depth;
        });
        selectedTrackTarget = targetId;
        selectedTrackKey = -1;
        hoveredTrackKey = {};
        commitTimeline();
    }

    void TimelineWindow::removeTrack(const uint16_t targetId) {
        // Speed is what the schedule is integrated from, so the row it is edited on always stands.
        if (targetId == SPEED_TARGET || track(targetId) == nullptr) {
            MessageBeep(MB_ICONWARNING);
            return;
        }
        std::vector<uint16_t> removed = {targetId};
        if (linkColorCycle && colorCycleTarget(targetId)) {
            removed = {CYCLE_R_TARGET, CYCLE_G_TARGET, CYCLE_B_TARGET};
        }
        const auto isRemoved = [&removed](const uint16_t id) {
            return std::ranges::find(removed, id) != removed.end();
        };
        std::erase_if(attribute.video.timeline.tracks, [&isRemoved](const VidTimelineTrack &current) {
            return isRemoved(current.targetId);
        });
        std::erase_if(selectedTrackTargets, isRemoved);
        if (isRemoved(selectedTrackTarget)) {
            selectedTrackTarget = SPEED_TARGET;
            selectedTrackKey = -1;
        }
        if (isRemoved(editedTrackTarget)) {
            keyEditor.reset();
            editedTrackKey = -1;
        }
        hoveredTrackKey = {};
        commitTimeline();
    }

    bool TimelineWindow::syncLinkedColorCycle(const uint16_t sourceTarget) {
        if (!linkColorCycle || !colorCycleTarget(sourceTarget)) {
            return false;
        }
        const VidTimelineTrack *source = track(sourceTarget);
        if (source == nullptr) {
            return false;
        }
        // Copied out first: making a channel that has no track yet can move the one being read.
        const std::vector<VidTimelineKey> keys = source->keys;
        const bool enabled = source->enabled;
        bool changed = false;
        for (const uint16_t targetId: {CYCLE_R_TARGET, CYCLE_G_TARGET, CYCLE_B_TARGET}) {
            if (targetId == sourceTarget) {
                continue;
            }
            VidTimelineTrack &mirrored = ensureScalarTrack(targetId);
            if (mirrored.enabled == enabled && sameKeys(mirrored.keys, keys)) {
                continue;
            }
            mirrored.keys = keys;
            mirrored.enabled = enabled;
            changed = true;
        }
        return changed;
    }

    void TimelineWindow::toggleTheme() {
        lightMode = !lightMode;
        timelineLightModeFlag().store(lightMode, std::memory_order_relaxed);
        // Kept straight away, so the next start opens the editor in the colors it was left in.
        PreferencesIO::save();
        applyPanelTheme();
        hoverTheme = false;
        if (fieldEditBrush != nullptr) {
            DeleteObject(fieldEditBrush);
        }
        fieldEditBrush = CreateSolidBrush(timelineTheme(lightMode).panelRaised);
        if (fieldEdit != nullptr) {
            InvalidateRect(fieldEdit, nullptr, TRUE);
        }
        if (window != nullptr) {
            InvalidateRect(window, nullptr, FALSE);
        }
    }

    void TimelineWindow::adoptPanel(SettingsWindow &panel) {
        std::erase_if(themedPanels, [](const HWND open) { return !IsWindow(open); });
        themedPanels.push_back(panel.getWindow());
        panel.setDarkOverride(!lightMode);
    }

    void TimelineWindow::applyPanelTheme() const {
        for (const HWND panel: themedPanels) {
            if (SettingsWindow *opened = SettingsWindow::of(panel); opened != nullptr) {
                opened->setDarkOverride(!lightMode);
            }
        }
    }

    void TimelineWindow::toggleFullscreen() {
        if (window == nullptr) {
            return;
        }
        if (fullscreen) {
            // Cleared first so the restored placement is held to the work area again.
            fullscreen = false;
            hoverFullscreen = false;
            SetWindowLongPtrW(window, GWL_STYLE, windowedStyle);
            SetWindowPlacement(window, &windowedPlacement);
            SetWindowPos(window, nullptr, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            InvalidateRect(window, nullptr, FALSE);
            return;
        }
        MONITORINFO monitor = {};
        monitor.cbSize = sizeof(MONITORINFO);
        windowedPlacement = {};
        windowedPlacement.length = sizeof(WINDOWPLACEMENT);
        // The windowed size and the frame style both have to survive the switch, so they are kept before it happens.
        if (!GetWindowPlacement(window, &windowedPlacement) ||
            !GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitor)) {
            return;
        }
        windowedStyle = GetWindowLongPtrW(window, GWL_STYLE);
        SetWindowLongPtrW(window, GWL_STYLE, (windowedStyle & ~WS_OVERLAPPEDWINDOW) | WS_POPUP);
        fullscreen = true;
        hoverFullscreen = false;
        SetWindowPos(window, HWND_TOP, monitor.rcMonitor.left, monitor.rcMonitor.top,
                     monitor.rcMonitor.right - monitor.rcMonitor.left,
                     monitor.rcMonitor.bottom - monitor.rcMonitor.top,
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        InvalidateRect(window, nullptr, FALSE);
    }

    std::pair<float, float> TimelineWindow::valueRange(const uint16_t targetId) const {
        const VidTimelineTrack *current = track(targetId);
        const float fallback = baseValue(targetId);
        if (targetId == SPEED_TARGET) {
            float peak = std::max(fallback, VidTimelineAttribute::MIN_SPEED);
            if (current != nullptr) {
                for (const auto &key: current->keys) {
                    peak = std::max(peak, key.value);
                }
            }
            return {0.0f, std::max({peak * 1.2f, fallback * 2.0f, VidTimelineAttribute::MIN_SPEED * 10.0f})};
        }
        const TimelineParamDesc *param = TimelineParams::find(targetId);
        if (param == nullptr) {
            return {0.0f, 1.0f};
        }
        if (param->maxValue - param->minValue <= WIDE_VALUE_RANGE) {
            return {param->minValue, param->maxValue};
        }
        // A range that spans a billion would draw every key onto one flat line, so a row that wide is
        // scaled to the keys it carries instead.
        if (param->minValue >= 0.0f) {
            float peak = std::max(fallback, 1.0f);
            if (current != nullptr) {
                for (const auto &key: current->keys) {
                    peak = std::max(peak, key.value);
                }
            }
            return {0.0f, std::max(peak * 1.2f, fallback * 2.0f)};
        }
        float magnitude = std::max(1.0f, std::abs(fallback));
        if (current != nullptr) {
            for (const auto &key: current->keys) {
                magnitude = std::max(magnitude, std::abs(key.value));
            }
        }
        magnitude *= 1.2f;
        return {-magnitude, magnitude};
    }

    const TimelineWindow::TrackLayout *TimelineWindow::layout(const uint16_t targetId) const {
        for (const auto &item: trackLayouts) {
            if (item.targetId == targetId) {
                return &item;
            }
        }
        return nullptr;
    }

    float TimelineWindow::displayDistance(const float depth) const {
        return schedule.getStartDepth() - depth;
    }

    float TimelineWindow::depthFromDistance(const float distance) const {
        return schedule.getStartDepth() - distance;
    }

    float TimelineWindow::viewSpan() const {
        return std::max(viewStartDepth - viewEndDepth, 1e-6f);
    }

    float TimelineWindow::viewDepthAt(const int x) const {
        const float width = static_cast<float>(std::max(timelineAxis.right - timelineAxis.left, 1L));
        const float ratio = std::clamp(static_cast<float>(x - timelineAxis.left) / width, 0.0f, 1.0f);
        return viewStartDepth + (viewEndDepth - viewStartDepth) * ratio;
    }

    void TimelineWindow::resetView() {
        viewStartDepth = schedule.getStartDepth();
        viewEndDepth = schedule.getEndDepth();
    }

    void TimelineWindow::clampView() {
        const float fullStart = schedule.getStartDepth();
        const float fullEnd = schedule.getEndDepth();
        const float fullSpan = std::max(fullStart - fullEnd, 1e-6f);
        const float minSpan = std::min(fullSpan, MIN_VIEW_SPAN);
        float span = viewStartDepth - viewEndDepth;
        if (!std::isfinite(span) || span <= 0.0f) {
            span = fullSpan;
        }
        span = std::clamp(span, minSpan, fullSpan);
        float start = viewStartDepth;
        if (!std::isfinite(start)) {
            start = fullStart;
        }
        viewStartDepth = std::clamp(start, fullEnd + span, fullStart);
        viewEndDepth = viewStartDepth - span;
    }

    void TimelineWindow::zoomView(const float pivotDepth, const float scale) {
        const float fullSpan = std::max(schedule.getStartDepth() - schedule.getEndDepth(), 1e-6f);
        const float minSpan = std::min(fullSpan, MIN_VIEW_SPAN);
        const float current = viewSpan();
        const float span = std::clamp(current / std::max(scale, 1e-3f), minSpan, fullSpan);
        const float ratio = std::clamp((viewStartDepth - pivotDepth) / current, 0.0f, 1.0f);
        viewStartDepth = pivotDepth + span * ratio;
        viewEndDepth = viewStartDepth - span;
        clampView();
    }

    void TimelineWindow::panView(const float depthDelta) {
        viewStartDepth += depthDelta;
        viewEndDepth += depthDelta;
        clampView();
    }

    void TimelineWindow::updateScrollThumb(const POINT point) {
        const int trackWidth = static_cast<int>(scrollTrack.right - scrollTrack.left);
        const int thumbWidth = static_cast<int>(scrollThumb.right - scrollThumb.left);
        const int travel = std::max(trackWidth - thumbWidth, 1);
        const float ratio = std::clamp(
            static_cast<float>(point.x - scrollGrabOffset - scrollTrack.left) / static_cast<float>(travel),
            0.0f, 1.0f);
        const float fullSpan = std::max(schedule.getStartDepth() - schedule.getEndDepth(), 1e-6f);
        const float span = viewSpan();
        viewStartDepth = schedule.getStartDepth() - (fullSpan - span) * ratio;
        viewEndDepth = viewStartDepth - span;
        clampView();
        if (window != nullptr) {
            InvalidateRect(window, nullptr, FALSE);
        }
    }

    void TimelineWindow::pageScrollView(const POINT point) {
        panView(point.x < scrollThumb.left ? viewSpan() : -viewSpan());
        if (window != nullptr) {
            InvalidateRect(window, nullptr, FALSE);
        }
    }

    void TimelineWindow::scrollTracks(const int delta) {
        const int previous = trackScrollOffset;
        trackScrollOffset = std::clamp(trackScrollOffset + delta, 0, trackScrollRange);
        if (trackScrollOffset != previous && window != nullptr) {
            InvalidateRect(window, nullptr, FALSE);
        }
    }

    void TimelineWindow::updateTrackScrollThumb(const POINT point) {
        const int barHeight = static_cast<int>(trackScrollTrack.bottom - trackScrollTrack.top);
        const int thumbHeight = static_cast<int>(trackScrollThumb.bottom - trackScrollThumb.top);
        const int travel = std::max(barHeight - thumbHeight, 1);
        const float ratio = std::clamp(
            static_cast<float>(point.y - trackScrollGrabOffset - trackScrollTrack.top) / static_cast<float>(travel),
            0.0f, 1.0f);
        trackScrollOffset = static_cast<int>(std::lround(ratio * static_cast<float>(trackScrollRange)));
        if (window != nullptr) {
            InvalidateRect(window, nullptr, FALSE);
        }
    }

    void TimelineWindow::pageScrollTracks(const POINT point) {
        const int page = std::max(static_cast<int>(trackScrollThumb.bottom - trackScrollThumb.top), sc(38));
        scrollTracks(point.y < trackScrollThumb.top ? -page : page);
    }

    void TimelineWindow::retargetTrackDepths(const float previousStartDepth) {
        const float startDepth = attribute.video.timeline.estimateKeyframes;
        const float endDepth = -attribute.video.animation.overZoom;
        if (std::abs(startDepth - previousStartDepth) < 1e-3f) {
            return;
        }
        // The axis runs from the keyframe count down to the over-zoom, so a key above the new start
        // has no place left on it: it is drawn nowhere and grabbed nowhere, and a track whose opening
        // key is one of them reads as a track that lost its first diamond. The key that stood at the
        // old start follows the new one, and anything else beyond the ends is brought onto the axis.
        for (auto &track: attribute.video.timeline.tracks) {
            for (auto &key: track.keys) {
                key.depth = key.depth >= previousStartDepth - 1e-3f
                                ? startDepth
                                : std::clamp(key.depth, endDepth, startDepth);
            }
            std::ranges::stable_sort(track.keys, [](const VidTimelineKey &a, const VidTimelineKey &b) {
                return a.depth > b.depth;
            });
            // Keys the move brought onto one depth are one key, so only the first of them is kept.
            const auto duplicates = std::ranges::unique(track.keys,
                                                        [](const VidTimelineKey &a, const VidTimelineKey &b) {
                                                            return std::abs(a.depth - b.depth) < 1e-3f;
                                                        });
            track.keys.erase(duplicates.begin(), duplicates.end());
        }
        selectedTrackKey = -1;
        hoveredTrackKey = {};
    }

    void TimelineWindow::rebuildSchedule() {
        const bool wasFullView = viewSpan() >= schedule.getStartDepth() - schedule.getEndDepth() - 1e-3f;
        schedule = TimelineSchedule::create(attribute.video.timeline, attribute.video.timeline.estimateKeyframes,
                                            -attribute.video.animation.overZoom, attribute.video.animation.mps);
        previewDepth = std::clamp(previewDepth, schedule.getEndDepth(), schedule.getStartDepth());
        if (wasFullView) {
            resetView();
        } else {
            clampView();
        }
        syncPlaybackClock();
    }

    void TimelineWindow::commitTimeline() {
        attribute.video.timeline.enabled = true;
        recordUndoStep();
        rebuildSchedule();
        if (sourceAttribute != nullptr) {
            sourceAttribute->video.timeline = attribute.video.timeline;
        }
        if (!draggingTrackKey) {
            requestFramePreview();
        }
        if (window != nullptr) {
            InvalidateRect(window, nullptr, FALSE);
        }
    }

    void TimelineWindow::recordUndoStep() {
        if (restoringUndoStep) {
            // The step just put back is where the next change is measured from, and the gesture
            // clock is cleared so that change opens a step of its own however quickly it follows.
            undoBaseline = attribute.video.timeline;
            lastUndoStep = 0;
            return;
        }
        const ULONGLONG now = GetTickCount64();
        // A key being dragged, or a value scrubbed in a settings panel, reports its change on every
        // mouse move. One step for the whole gesture is what Undo is asked to take back, so a change
        // arriving on the heels of the last one extends that step rather than opening another.
        if (const bool extend = !undoSteps.empty() &&
                                (draggingTrackKey || now - lastUndoStep < UNDO_COALESCE_MS); !extend) {
            undoSteps.push_back(undoBaseline);
            if (undoSteps.size() > MAX_UNDO_STEPS) {
                undoSteps.erase(undoSteps.begin());
            }
            // The steps taken back are what a new change branches away from, and are gone with it.
            redoSteps.clear();
        }
        lastUndoStep = now;
        undoBaseline = attribute.video.timeline;
    }

    void TimelineWindow::undoTimeline() {
        if (undoSteps.empty()) {
            MessageBeep(MB_ICONWARNING);
            return;
        }
        redoSteps.push_back(attribute.video.timeline);
        VidTimelineAttribute restored = std::move(undoSteps.back());
        undoSteps.pop_back();
        applyRestoredTimeline(std::move(restored));
    }

    void TimelineWindow::redoTimeline() {
        if (redoSteps.empty()) {
            MessageBeep(MB_ICONWARNING);
            return;
        }
        undoSteps.push_back(attribute.video.timeline);
        VidTimelineAttribute restored = std::move(redoSteps.back());
        redoSteps.pop_back();
        applyRestoredTimeline(std::move(restored));
    }

    void TimelineWindow::applyRestoredTimeline(VidTimelineAttribute &&restored) {
        attribute.video.timeline = std::move(restored);
        // A key the step being put back had added is not there to stay picked, edited or hovered.
        keyEditor.reset();
        editedTrackKey = -1;
        selectedTrackKey = -1;
        selectedTrackTargets.clear();
        hoveredTrackKey = {};
        restoringUndoStep = true;
        commitTimeline();
        restoringUndoStep = false;
    }

    TimelineWindow::KeyHit TimelineWindow::hitTrackKey(const POINT point) const {
        const int radius = sc(9);
        KeyHit best = {};
        int bestDistance = radius * radius + 1;
        for (const auto &item: trackLayouts) {
            const VidTimelineTrack *current = track(item.targetId);
            if (current == nullptr || !item.editable) {
                continue;
            }
            for (int i = 0; i < static_cast<int>(current->keys.size()); ++i) {
                const auto &key = current->keys[i];
                const int x = depthX(key.depth, viewStartDepth, viewEndDepth, timelineAxis);
                if (!visibleX(x, timelineAxis, radius)) {
                    continue;
                }
                const int y = valueY(key.value, item.minValue, item.maxValue, item.row);
                if (y < timelineAxis.top || y > timelineAxis.bottom) {
                    continue;
                }
                const int dx = point.x - x;
                const int dy = point.y - y;
                const int distance = dx * dx + dy * dy;
                if (distance <= radius * radius && distance < bestDistance) {
                    best = {.targetId = item.targetId, .keyIndex = i};
                    bestDistance = distance;
                }
            }
        }
        return best;
    }

    uint16_t TimelineWindow::hitTrackRow(const POINT point, const bool editableOnly) const {
        if (point.y < timelineAxis.top || point.y > timelineAxis.bottom) {
            return UINT16_MAX;
        }
        for (const auto &item: trackLayouts) {
            if ((item.editable || !editableOnly) && contains(item.row, point)) {
                return item.targetId;
            }
        }
        return UINT16_MAX;
    }

    uint16_t TimelineWindow::hitTrackLabel(const POINT point) const {
        if (point.y < timelineAxis.top || point.y > timelineAxis.bottom) {
            return UINT16_MAX;
        }
        for (const auto &item: trackLayouts) {
            if (contains(item.label, point)) {
                return item.targetId;
            }
        }
        return UINT16_MAX;
    }

    int TimelineWindow::trackRowDropTarget(const POINT point) const {
        int index = -1;
        int firstVisible = -1;
        for (const auto &item: trackLayouts) {
            if (item.order < 0) {
                continue;
            }
            if (firstVisible < 0) {
                firstVisible = item.order;
            }
            // A row the pointer stands past the middle of is a row the carried one lands below.
            if (point.y >= static_cast<int>(item.row.top + item.row.bottom) / 2) {
                index = item.order + 1;
            }
        }
        if (index < 0) {
            index = std::max(firstVisible, 0);
        }
        return std::clamp(index, 0, static_cast<int>(reorderRowTargets.size()));
    }

    bool TimelineWindow::trackRowSelected(const uint16_t targetId) const {
        if (targetId == selectedTrackTarget) {
            return true;
        }
        // A list that has lost the row picked last was picked before the selection moved on.
        return std::ranges::find(selectedTrackTargets, selectedTrackTarget) != selectedTrackTargets.end() &&
               std::ranges::find(selectedTrackTargets, targetId) != selectedTrackTargets.end();
    }

    void TimelineWindow::selectTrackRow(const uint16_t targetId, const bool extend, const bool range) {
        selectedTrackKey = -1;
        if (std::ranges::find(selectedTrackTargets, selectedTrackTarget) == selectedTrackTargets.end()) {
            selectedTrackTargets.assign(1, selectedTrackTarget);
        }
        const auto anchor = std::ranges::find(reorderRowTargets, selectedTrackTarget);
        const auto reached = std::ranges::find(reorderRowTargets, targetId);
        if (range && anchor != reorderRowTargets.end() && reached != reorderRowTargets.end()) {
            // Shift takes the whole run between the row picked last and this one, that one included.
            const auto first = std::min(anchor, reached);
            const auto last = std::max(anchor, reached);
            selectedTrackTargets.assign(first, last + 1);
            return;
        }
        if (extend) {
            const auto picked = std::ranges::find(selectedTrackTargets, targetId);
            if (picked != selectedTrackTargets.end()) {
                // Ctrl on a row already picked drops it, unless it is the only one left picked.
                if (selectedTrackTargets.size() > 1) {
                    selectedTrackTargets.erase(picked);
                    if (targetId == selectedTrackTarget) {
                        selectedTrackTarget = selectedTrackTargets.front();
                    }
                }
                return;
            }
            selectedTrackTargets.push_back(targetId);
            selectedTrackTarget = targetId;
            return;
        }
        selectedTrackTargets.assign(1, targetId);
        selectedTrackTarget = targetId;
    }

    void TimelineWindow::moveTrackRows(const std::vector<uint16_t> &targets, int dropIndex) {
        std::vector<uint16_t> order = reorderRowTargets;
        const auto carried = [&targets](const uint16_t id) {
            return std::ranges::find(targets, id) != targets.end();
        };
        // The rows travel as one block, stacked the way the stack already holds them.
        std::vector<uint16_t> block;
        for (const uint16_t id: order) {
            if (carried(id)) {
                block.push_back(id);
            }
        }
        if (block.empty()) {
            return;
        }
        dropIndex = std::clamp(dropIndex, 0, static_cast<int>(order.size()));
        // Every carried row above the drop point takes that point up with it as it leaves the stack.
        int landing = dropIndex;
        for (int i = 0; i < dropIndex; ++i) {
            if (carried(order[i])) {
                --landing;
            }
        }
        std::erase_if(order, carried);
        order.insert(order.begin() + landing, block.begin(), block.end());
        // A block put back where it already stood leaves the stack as it was, and Undo nothing to do.
        if (order == reorderRowTargets) {
            return;
        }
        auto &tracks = attribute.video.timeline.tracks;
        std::vector<VidTimelineTrack> stacked;
        stacked.reserve(tracks.size());
        std::vector<bool> taken(tracks.size(), false);
        auto take = [&](const uint16_t id) {
            for (size_t i = 0; i < tracks.size(); ++i) {
                if (!taken[i] && tracks[i].targetId == id) {
                    taken[i] = true;
                    stacked.push_back(std::move(tracks[i]));
                    return;
                }
            }
        };
        for (const uint16_t id: order) {
            take(id);
            // The channels the R row stands for while they are linked follow it to its new place.
            if (id == CYCLE_R_TARGET && linkColorCycle) {
                take(CYCLE_G_TARGET);
                take(CYCLE_B_TARGET);
            }
        }
        // A row a static source hides is on no stack to be moved, and follows the ones that are.
        for (size_t i = 0; i < tracks.size(); ++i) {
            if (!taken[i]) {
                stacked.push_back(std::move(tracks[i]));
            }
        }
        tracks = std::move(stacked);
        // Only the order the rows are stacked in changes, so the schedule they drive is untouched.
        recordUndoStep();
        if (sourceAttribute != nullptr) {
            sourceAttribute->video.timeline = attribute.video.timeline;
        }
        if (window != nullptr) {
            InvalidateRect(window, nullptr, FALSE);
        }
    }

    void TimelineWindow::updateTrackKey(const POINT point) {
        VidTimelineTrack *current = track(selectedTrackTarget);
        if (current == nullptr || selectedTrackKey < 0 || selectedTrackKey >= static_cast<int>(current->keys.size())) {
            return;
        }
        current->enabled = true;
        const float startDepth = schedule.getStartDepth();
        const float endDepth = schedule.getEndDepth();
        float depth = snapDepth(viewDepthAt(point.x));
        constexpr float gap = 1.0f;
        if (selectedTrackKey > 0) {
            depth = std::min(depth, current->keys[selectedTrackKey - 1].depth - gap);
        }
        if (selectedTrackKey + 1 < static_cast<int>(current->keys.size())) {
            depth = std::max(depth, current->keys[selectedTrackKey + 1].depth + gap);
        }
        const TrackLayout *item = layout(selectedTrackTarget);
        if (item == nullptr) {
            return;
        }
        const float plotHeight = static_cast<float>(std::max(item->row.bottom - item->row.top - sc(16), 1L));
        const float valueRatio = std::clamp(
            static_cast<float>(item->row.bottom - sc(8) - point.y) / plotHeight, 0.0f, 1.0f);
        float value = dragValueMin + (dragValueMax - dragValueMin) * valueRatio;
        if (selectedTrackTarget == SPEED_TARGET) {
            value = std::max(value, VidTimelineAttribute::MIN_SPEED);
        } else if (const TimelineParamDesc *param = TimelineParams::find(selectedTrackTarget); param != nullptr) {
            value = std::clamp(value, param->minValue, param->maxValue);
        }
        current->keys[selectedTrackKey].depth = std::clamp(depth, endDepth, startDepth);
        current->keys[selectedTrackKey].value = value;
        previewDepth = current->keys[selectedTrackKey].depth;
        (void) syncLinkedColorCycle(selectedTrackTarget);
        commitTimeline();
    }

    void TimelineWindow::addTrackKey(const uint16_t targetId, const POINT point) {
        const TrackLayout *item = layout(targetId);
        if (item == nullptr || !contains(item->row, point)) {
            return;
        }
        VidTimelineTrack &current = ensureScalarTrack(targetId);
        current.enabled = true;
        if (current.keys.size() >= VidTimelineAttribute::MAX_KEYS_PER_TRACK) {
            MessageBeep(MB_ICONWARNING);
            return;
        }
        const float startDepth = schedule.getStartDepth();
        const float endDepth = schedule.getEndDepth();
        const float depth = std::clamp(snapDepth(viewDepthAt(point.x)), endDepth, startDepth);
        for (int i = 0; i < static_cast<int>(current.keys.size()); ++i) {
            if (std::abs(current.keys[i].depth - depth) < 1.0f) {
                selectedTrackTarget = targetId;
                selectedTrackKey = i;
                previewDepth = current.keys[i].depth;
                MessageBeep(MB_ICONWARNING);
                return;
            }
        }
        float value = current.keys.empty()
                          ? baseValue(targetId)
                          : TimelineSchedule::evaluateTrack(current, depth, baseValue(targetId));
        if (targetId == SPEED_TARGET) {
            value = std::max(value, VidTimelineAttribute::MIN_SPEED);
        }
        current.keys.push_back({.depth = depth, .value = value, .color = glm::vec4(1.0f),
                                .out = defaultInterpolation(targetId)});
        std::ranges::stable_sort(current.keys, [](const VidTimelineKey &a, const VidTimelineKey &b) {
            return a.depth > b.depth;
        });
        selectedTrackTarget = targetId;
        selectedTrackKey = 0;
        previewDepth = depth;
        float nearest = std::abs(current.keys.front().depth - depth);
        for (int i = 1; i < static_cast<int>(current.keys.size()); ++i) {
            const float distance = std::abs(current.keys[i].depth - depth);
            if (distance < nearest) {
                selectedTrackKey = i;
                nearest = distance;
            }
        }
        (void) syncLinkedColorCycle(targetId);
        commitTimeline();
    }

    void TimelineWindow::deleteTrackKey() {
        VidTimelineTrack *current = track(selectedTrackTarget);
        if (current == nullptr || selectedTrackKey < 0 || selectedTrackKey >= static_cast<int>(current->keys.size())) {
            return;
        }
        if (current->keys.size() <= 1) {
            MessageBeep(MB_ICONWARNING);
            return;
        }
        current->enabled = true;
        current->keys.erase(current->keys.begin() + selectedTrackKey);
        selectedTrackKey = std::min(selectedTrackKey, static_cast<int>(current->keys.size()) - 1);
        (void) syncLinkedColorCycle(selectedTrackTarget);
        commitTimeline();
    }

    void TimelineWindow::setTrackInterpolation(const VidKeyInterpolation interpolation) {
        VidTimelineTrack *current = track(selectedTrackTarget);
        if (current == nullptr || selectedTrackKey < 0 || selectedTrackKey >= static_cast<int>(current->keys.size())) {
            return;
        }
        current->enabled = true;
        current->keys[selectedTrackKey].out = interpolation;
        (void) syncLinkedColorCycle(selectedTrackTarget);
        commitTimeline();
    }

    void TimelineWindow::openTrackKeyEditor() {
        VidTimelineTrack *current = track(selectedTrackTarget);
        if (current == nullptr || selectedTrackKey < 0 || selectedTrackKey >= static_cast<int>(current->keys.size())) {
            return;
        }
        keyEditor.reset();
        editedTrackTarget = selectedTrackTarget;
        editedTrackKey = selectedTrackKey;
        editedDistance = displayDistance(current->keys[editedTrackKey].depth);
        editedValue = current->keys[editedTrackKey].value;
        editedInterpolation = current->keys[editedTrackKey].out;

        auto editor = std::make_unique<SettingsWindow>(rowName(editedTrackTarget, linkColorCycle) + L" Key", 430);
        editor->registerSectionHeader(L"Exact Values", false);
        editor->registerTextInput<float>(
            L"Distance", &editedDistance, Unparser::floatTrim(4), Parser::FLOAT,
            [this](const float &value) {
                if (!std::isfinite(value) || value < 0.0f ||
                    value > schedule.getStartDepth() - schedule.getEndDepth()) {
                    return false;
                }
                const VidTimelineTrack *current = track(editedTrackTarget);
                if (current == nullptr) {
                    return false;
                }
                const float depth = depthFromDistance(value);
                for (int i = 0; i < static_cast<int>(current->keys.size()); ++i) {
                    if (i != editedTrackKey && std::abs(current->keys[i].depth - depth) < 1.0f) {
                        return false;
                    }
                }
                return true;
            },
            [this] { commitTrackKeyEditor(); }, L"Set Key Distance",
            std::format(L"How far the video has run at this key, counted from 0 at its first frame. "
                        L"Keyframe {:.1f} minus this distance is the keyframe file it lands on. "
                        L"Press Enter to apply the typed value.", schedule.getStartDepth()), 0.1);
        const bool speed = editedTrackTarget == SPEED_TARGET;
        editor->registerTextInput<float>(
            speed ? L"Speed" : L"Value", &editedValue, Unparser::floatTrim(5), Parser::FLOAT,
            [this](const float &value) {
                if (!std::isfinite(value)) {
                    return false;
                }
                if (editedTrackTarget == SPEED_TARGET) {
                    return value >= VidTimelineAttribute::MIN_SPEED;
                }
                const TimelineParamDesc *param = TimelineParams::find(editedTrackTarget);
                return param != nullptr && value >= param->minValue && value <= param->maxValue;
            },
            [this] { commitTrackKeyEditor(); }, speed ? L"Set Key Speed" : L"Set Key Value",
            speed ? L"The exact speed in keyframes per second. Press Enter to apply the typed value."
                  : L"The exact shader value at this keyframe depth. Press Enter to apply it.",
            keyValueStep(editedTrackTarget));
        editor->registerSelectionInput<VidKeyInterpolation>(
            L"Interpolation", &editedInterpolation, [this] { commitTrackKeyEditor(); }, L"Set Interpolation",
            L"How this key reaches the next key on the same track.");
        editor->registerStaticText(L"Enter applies a number immediately. Arrow keys change it by one step.");
        editor->setWindowCloseFunction([] {});
        keyEditor = std::move(editor);
        adoptPanel(*keyEditor);
    }

    void TimelineWindow::commitTrackKeyEditor() {
        VidTimelineTrack *current = track(editedTrackTarget);
        if (current == nullptr || editedTrackKey < 0 || editedTrackKey >= static_cast<int>(current->keys.size())) {
            return;
        }
        VidTimelineKey edited = current->keys[editedTrackKey];
        edited.depth = std::clamp(depthFromDistance(editedDistance), schedule.getEndDepth(),
                                  schedule.getStartDepth());
        for (int i = 0; i < static_cast<int>(current->keys.size()); ++i) {
            if (i != editedTrackKey && std::abs(current->keys[i].depth - edited.depth) < 1.0f) {
                MessageBeep(MB_ICONWARNING);
                return;
            }
        }
        if (editedTrackTarget == SPEED_TARGET) {
            edited.value = std::max(editedValue, VidTimelineAttribute::MIN_SPEED);
        } else if (const TimelineParamDesc *param = TimelineParams::find(editedTrackTarget); param != nullptr) {
            edited.value = std::clamp(editedValue, param->minValue, param->maxValue);
        }
        edited.out = editedInterpolation;
        current->keys[editedTrackKey] = edited;
        std::ranges::stable_sort(current->keys, [](const VidTimelineKey &a, const VidTimelineKey &b) {
            return a.depth > b.depth;
        });
        editedTrackKey = 0;
        previewDepth = edited.depth;
        float nearest = std::abs(current->keys.front().depth - edited.depth);
        for (int i = 1; i < static_cast<int>(current->keys.size()); ++i) {
            const float distance = std::abs(current->keys[i].depth - edited.depth);
            if (distance < nearest) {
                editedTrackKey = i;
                nearest = distance;
            }
        }
        selectedTrackTarget = editedTrackTarget;
        selectedTrackKey = editedTrackKey;
        current->enabled = true;
        (void) syncLinkedColorCycle(editedTrackTarget);
        commitTimeline();
    }

    void TimelineWindow::loadTimeline() {
        const auto path = IOUtilities::ioFileDialog(L"Open Video Timeline", Constants::Extension::DESC_TIMELINE,
                                                    IOUtilities::OPEN_FILE, Constants::Extension::TIMELINE);
        if (path == nullptr) {
            return;
        }
        VidTimelineAttribute loaded = {};
        if (!TimelineIO::load(*path, loaded)) {
            MessageBoxW(window, L"The selected .rfvt file could not be loaded.", L"Timeline Editor",
                        MB_OK | MB_ICONERROR);
            return;
        }
        attribute.video.timeline = std::move(loaded);
        // The keys of the file were written against the keyframe count it carries, so they are moved
        // onto this folder's axis when its own count takes over.
        const float fileStartDepth = attribute.video.timeline.estimateKeyframes;
        if (frameSource != nullptr) {
            attribute.video.timeline.estimateKeyframes = static_cast<float>(frameSource->getFrameCount());
            retargetTrackDepths(fileStartDepth);
        }
        ensureEditableTracks();
        // Three channels a file holds apart are not linked ones, so the link follows what it carries.
        const VidTimelineTrack *cycleR = track(CYCLE_R_TARGET);
        const VidTimelineTrack *cycleG = track(CYCLE_G_TARGET);
        const VidTimelineTrack *cycleB = track(CYCLE_B_TARGET);
        linkColorCycle = cycleR != nullptr && cycleG != nullptr && cycleB != nullptr &&
                         sameKeys(cycleR->keys, cycleG->keys) && sameKeys(cycleR->keys, cycleB->keys);
        selectedTrackTarget = SPEED_TARGET;
        selectedTrackKey = -1;
        hoveredTrackKey = {};
        rebuildSchedule();
        if (sourceAttribute != nullptr) {
            sourceAttribute->video.timeline = attribute.video.timeline;
        }
        requestFramePreview();
        InvalidateRect(window, nullptr, FALSE);
    }

    void TimelineWindow::saveTimeline() const {
        const auto path = IOUtilities::ioFileDialog(L"Save Video Timeline", Constants::Extension::DESC_TIMELINE,
                                                    IOUtilities::SAVE_FILE, Constants::Extension::TIMELINE);
        if (path != nullptr && !TimelineIO::save(*path, attribute.video.timeline)) {
            MessageBoxW(window, L"The .rfvt file could not be saved.", L"Timeline Editor", MB_OK | MB_ICONERROR);
        }
    }

    void TimelineWindow::openExportSettings() {
        if (settingsMenu == nullptr || renderScene == nullptr) {
            return;
        }
        std::erase_if(settingsMenu->activeSettingsWindows,
                      [](const SettingsMenu::ActiveSettingsWindow &active) {
                          return !IsWindow(active.window->getWindow());
                      });
        const size_t before = settingsMenu->activeSettingsWindows.size();
        CallbackVideo::EXPORT_SETTINGS(*settingsMenu, *renderScene);
        if (settingsMenu->activeSettingsWindows.size() <= before) {
            return;
        }
        SettingsWindow &window = *settingsMenu->activeSettingsWindows.back().window;
        if (sourceAttribute != nullptr) {
            // The editor exports from keyframes that already exist, so the rows steering keyframe generation are greyed rather than left offering an edit this export never reads.
            VidExportAttribute &exportation = sourceAttribute->video.exportation;
            const std::unordered_set<const void *> kept = {
                &exportation.fps, &exportation.bitrate, &exportation.lossless,
                &exportation.keyframeAA, &exportation.colorAA, &exportation.pauseMainPreview,
                &exportation.hdrTransfer, &exportation.hdrPeakNits,
            };
            window.disableRowsInObjectExcept(&exportation, sizeof(VidExportAttribute), kept);
        }
        adoptPanel(window);
    }

    void TimelineWindow::openExportMenu() {
        if (exporting || window == nullptr) {
            return;
        }
        constexpr UINT CMD_EXPORT_VIDEO = 1;
        constexpr UINT CMD_EXPORT_SETTINGS = 2;
        const HMENU menu = CreatePopupMenu();
        AppendMenuW(menu, MF_STRING, CMD_EXPORT_VIDEO, L"Export Video");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, CMD_EXPORT_SETTINGS, L"Export Settings");
        MENUITEMINFOW item = {sizeof(item)};
        item.fMask = MIIM_STATE;
        item.fState = MFS_DEFAULT;
        SetMenuItemInfoW(menu, CMD_EXPORT_VIDEO, FALSE, &item);
        POINT at = {exportButton.left, exportButton.bottom + sc(2)};
        ClientToScreen(window, &at);
        const int chosen = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN,
                                          at.x, at.y, 0, window, nullptr);
        DestroyMenu(menu);
        if (chosen == CMD_EXPORT_VIDEO) {
            exportVideo();
        } else if (chosen == CMD_EXPORT_SETTINGS) {
            openExportSettings();
        }
    }

    void TimelineWindow::exportVideo() {
        if (exporting || renderScene == nullptr || sourceAttribute == nullptr) {
            return;
        }

        if (frameSource == nullptr) {
            const auto directory = IOUtilities::ioDirectoryDialog(L"Select Sample Keyframe folder");
            if (directory == nullptr) {
                return;
            }
            std::wstring error;
            std::unique_ptr<VideoFrameSource> opened = VideoFrameSource::open(
                *directory, attribute.video.data.isStatic, error);
            if (opened == nullptr) {
                MessageBoxW(window, error.c_str(), L"Timeline Export", MB_OK | MB_ICONERROR);
                return;
            }
            frameSource = std::move(opened);
            attribute.video.data.isStatic = frameSource->isStatic();
            const float previousStartDepth = attribute.video.timeline.estimateKeyframes;
            attribute.video.timeline.estimateKeyframes = static_cast<float>(frameSource->getFrameCount());
            retargetTrackDepths(previousStartDepth);
            rebuildSchedule();
            sourceAttribute->video.data.isStatic = attribute.video.data.isStatic;
            sourceAttribute->video.timeline = attribute.video.timeline;
        }

        const bool lossless = sourceAttribute->video.exportation.lossless;
        const auto save = IOUtilities::ioFileDialog(L"Save Video Location", Constants::Extension::DESC_VIDEO,
                                                     IOUtilities::SAVE_FILE,
                                                     lossless ? Constants::Extension::VIDEO_LOSSLESS
                                                              : Constants::Extension::VIDEO);
        if (save == nullptr) {
            return;
        }

        const std::filesystem::path directory = frameSource->getDirectory();
        const Attribute exportAttribute = *sourceAttribute;
        const HWND notifyWindow = window;
        RenderScene *const scene = renderScene;
        stopFramePreviewWorker();
        destroyFramePreview();
        exporting = true;
        hoverExport = false;
        setPlaying(false);
        InvalidateRect(window, nullptr, FALSE);

        scene->getBackgroundThreads().createThread(
            [engine = &engine, scene, exportAttribute, directory, save = *save, notifyWindow,
             owner = this](const BackgroundThread &) {
                struct ExportActivity final {
                    RenderScene *scene;
                    explicit ExportActivity(RenderScene *scene) : scene(scene) { scene->setVideoExportActive(true); }
                    ~ExportActivity() { scene->setVideoExportActive(false); }
                } activity(scene);
                VideoWindow::createVideo(*engine, exportAttribute, directory, save);
                PostMessageW(notifyWindow, WM_TIMELINE_EXPORT_FINISHED,
                             reinterpret_cast<WPARAM>(owner), 0);
            });
    }

    void TimelineWindow::loadKeyframeDirectory() {
        const auto directory = IOUtilities::ioDirectoryDialog(L"Open Video Keyframes");
        if (directory == nullptr) {
            return;
        }
        std::wstring error;
        std::unique_ptr<VideoFrameSource> opened = VideoFrameSource::open(
            *directory, attribute.video.data.isStatic, error);
        if (opened == nullptr) {
            MessageBoxW(window, error.c_str(), L"Timeline Preview", MB_OK | MB_ICONERROR);
            return;
        }
        if (engine.isValidWindowContext(Constants::VulkanWindow::VIDEO_WINDOW_ATTACHMENT_INDEX) &&
            !previewWorker.joinable() && !previewContextAttached.load()) {
            MessageBoxW(window, L"The video renderer is already being used by another preview or export.",
                        L"Timeline Preview", MB_OK | MB_ICONWARNING);
            return;
        }

        stopFramePreviewWorker();
        destroyFramePreview();
        frameSource = std::move(opened);
        keyframeLogZooms.clear();
        attribute.video.data.isStatic = frameSource->isStatic();
        const float previousStartDepth = attribute.video.timeline.estimateKeyframes;
        attribute.video.timeline.estimateKeyframes = static_cast<float>(frameSource->getFrameCount());
        retargetTrackDepths(previousStartDepth);
        previewDepth = attribute.video.timeline.estimateKeyframes;
        rebuildSchedule();
        if (sourceAttribute != nullptr) {
            sourceAttribute->video.data.isStatic = attribute.video.data.isStatic;
            // The keys moved with the new upper Depth, so the whole timeline goes back, not the count alone.
            sourceAttribute->video.timeline = attribute.video.timeline;
        }
        (void) initializeFramePreview();
    }

    bool TimelineWindow::initializeFramePreview() {
        if (frameSource == nullptr || window == nullptr) {
            return false;
        }
        previewRenderWindow = CreateWindowExW(0, Constants::Win32::CLASS_VIDEO_RENDER_WINDOW, nullptr,
                                              WS_CHILD, 0, 0, sc(64), sc(64), window, nullptr,
                                              GetModuleHandleW(nullptr), nullptr);
        if (previewRenderWindow == nullptr) {
            MessageBoxW(window, L"Could not create the keyframe renderer.", L"Timeline Preview",
                        MB_OK | MB_ICONERROR);
            return false;
        }
        {
            std::scoped_lock lock(previewBitmapMutex);
            previewMessage = L"Building the keyframe renderer... (the first time can take minutes)";
        }
        {
            std::scoped_lock lock(previewRequestMutex);
            previewRequestGeneration = 0;
        }
        previewWorkerFailed.store(false);
        InvalidateRect(window, nullptr, FALSE);
        UpdateWindow(window);
        // Built here, on the thread that owns the window and the engine's contexts, rather than on the
        // worker: attaching a context from another thread moves the list the main window reads every frame.
        const HCURSOR previousCursor = SetCursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32514)));
        const bool prepared = createFramePreview(attribute);
        SetCursor(previousCursor);
        if (!prepared) {
            InvalidateRect(window, nullptr, FALSE);
            return false;
        }
        // Kept now rather than at shutdown: the compile above is the expensive one, and a session that
        // ends any other way would otherwise pay for it again on the next start.
        engine.getCore().getLogicalDevice().savePipelineCache();
        startFramePreviewWorker();
        requestFramePreview();
        return true;
    }

    void TimelineWindow::destroyFramePreview() {
        previewScene.reset();
        if (previewContextAttached.exchange(false)) {
            engine.detachWindowContext(Constants::VulkanWindow::VIDEO_WINDOW_ATTACHMENT_INDEX);
        }
        if (previewRenderWindow != nullptr) {
            DestroyWindow(previewRenderWindow);
            previewRenderWindow = nullptr;
        }
    }

    bool TimelineWindow::createFramePreview(const Attribute &initialAttribute) {
        if (frameSource == nullptr || window == nullptr || previewRenderWindow == nullptr) {
            return false;
        }
        try {
            const auto context = engine.attachWindowContext(
                previewRenderWindow, Constants::VulkanWindow::VIDEO_WINDOW_ATTACHMENT_INDEX);
            previewContextAttached.store(true);
            previewScene = std::make_unique<VideoRenderScene>(
                engine, *context, VkExtent2D{frameSource->getWidth(), frameSource->getHeight()}, initialAttribute);
        } catch (const std::exception &e) {
            previewWorkerFailed.store(true);
            const std::string text = e.what();
            {
                std::scoped_lock lock(previewBitmapMutex);
                previewMessage = std::format(L"Keyframe renderer failed: {}",
                                             std::wstring(text.begin(), text.end()));
            }
            PostMessageW(window, WM_TIMELINE_PREVIEW_READY, 0, 0);
            return false;
        }
        return true;
    }

    void TimelineWindow::startFramePreviewWorker() {
        previewWorker = std::jthread([this](const std::stop_token stopToken) {
            uint64_t processedGeneration = 0;
            while (!stopToken.stop_requested()) {
                float depth = 0.0f;
                float sec = 0.0f;
                VidTimelineAttribute timeline = {};
                ShaderAttribute shader = {};
                {
                    std::unique_lock lock(previewRequestMutex);
                    previewRequestCondition.wait(lock, [this, &stopToken, &processedGeneration] {
                        return stopToken.stop_requested() || previewRequestGeneration != processedGeneration;
                    });
                    if (stopToken.stop_requested()) {
                        break;
                    }
                    processedGeneration = previewRequestGeneration;
                    depth = requestedPreviewDepth;
                    sec = requestedPreviewSec;
                    timeline = requestedPreviewTimeline;
                    shader = requestedPreviewShader;
                }
                try {
                    (void) renderFramePreview(depth, sec, timeline, shader);
                } catch (const std::exception &e) {
                    // What went wrong is carried into the editor, where it can be read and reported.
                    const std::string text = e.what();
                    {
                        std::scoped_lock lock(previewBitmapMutex);
                        previewMessage = std::format(L"Keyframe preview failed: {}",
                                                     std::wstring(text.begin(), text.end()));
                    }
                    previewWorkerFailed.store(true);
                    PostMessageW(window, WM_TIMELINE_PREVIEW_READY, 0, 0);
                } catch (...) {
                    {
                        std::scoped_lock lock(previewBitmapMutex);
                        previewMessage = L"The keyframe preview could not be rendered";
                    }
                    previewWorkerFailed.store(true);
                    PostMessageW(window, WM_TIMELINE_PREVIEW_READY, 0, 0);
                }
            }
        });
    }

    void TimelineWindow::stopFramePreviewWorker() {
        // Whatever was outstanding ends with the worker, so the rendering line must not outlive it.
        previewPending = false;
        previewBusy = false;
        if (window != nullptr) {
            KillTimer(window, PREVIEW_STATUS_TIMER);
        }
        if (!previewWorker.joinable()) {
            return;
        }
        previewWorker.request_stop();
        previewRequestCondition.notify_all();
        previewWorker.join();
    }

    void TimelineWindow::requestFramePreview() {
        // Read here rather than held from when the editor opened: the Shader menu stays usable while
        // it is, and a fog or color changed there belongs in the next preview. Read before the
        // return below as well, since the track rows are drawn against it whether a keyframe folder
        // has been chosen or not.
        if (sourceAttribute != nullptr) {
            attribute.shader = sourceAttribute->shader;
        }
        if (frameSource == nullptr || !previewWorker.joinable() || previewWorkerFailed.load()) {
            return;
        }
        {
            std::scoped_lock lock(previewRequestMutex);
            requestedPreviewDepth = previewDepth;
            requestedPreviewSec = schedule.timeAt(previewDepth);
            requestedPreviewTimeline = attribute.video.timeline;
            requestedPreviewShader = attribute.shader;
            ++previewRequestGeneration;
        }
        // The rendering line waits out the delay rather than replacing the status at once, and a run
        // of requests keeps the deadline it started with so a steady stream of them still reaches it.
        previewBusyDepth = previewDepth;
        if (!previewPending) {
            previewPending = true;
            SetTimer(window, PREVIEW_STATUS_TIMER, PREVIEW_STATUS_DELAY, nullptr);
        }
        previewRequestCondition.notify_one();
        InvalidateRect(window, nullptr, FALSE);
    }

    bool TimelineWindow::renderFramePreview(const float depth, const float sec,
                                             const VidTimelineAttribute &timeline,
                                             const ShaderAttribute &shader) {
        if (frameSource == nullptr || previewScene == nullptr) {
            return false;
        }
        std::wstring error;
        if (!frameSource->load(depth, error)) {
            {
                std::scoped_lock lock(previewBitmapMutex);
                previewMessage = error;
            }
            PostMessageW(window, WM_TIMELINE_PREVIEW_READY, 0, 0);
            return false;
        }

        const float sampledDepth = frameSource->getSampledDepth();
        previewScene->updateBase(shader, timeline);
        previewScene->setStatic(frameSource->isStatic());
        previewScene->setCurrentFrame(sampledDepth);
        previewScene->setTime(sec);
        if (frameSource->isStatic()) {
            auto &normal = frameSource->getNormalStatic();
            auto &zoomed = frameSource->getZoomedStatic();
            previewScene->setMap(&normal, &zoomed);
            previewScene->applyCurrentStaticImage(frameSource->getNormalImage(), frameSource->getZoomedImage());
        } else {
            auto &normal = frameSource->getNormalDynamic();
            auto &zoomed = frameSource->getZoomedDynamic();
            previewScene->setMap(&normal, &zoomed);
            previewScene->applyCurrentDynamicMap(normal, zoomed, sampledDepth);
            const uint64_t normalMax = normal.getMaxIteration();
            const uint64_t zoomedMax = zoomed.getMaxIteration();
            previewScene->setMaxIterationDynamic(static_cast<double>(std::max(normalMax, zoomedMax)),
                                                 static_cast<double>(normalMax), static_cast<double>(zoomedMax));
            previewScene->applyTimelineShader(depth, sec);
        }
        previewScene->renderOffscreenOnce();
        previewScene->queueImage();

        std::unique_ptr<VideoBufferCache> buffer;
        {
            std::scoped_lock lock(previewScene->getBufferCachedMutex());
            auto &queued = previewScene->getQueuedBuffers();
            if (!queued.empty()) {
                buffer = std::move(queued.front());
                queued.pop();
                previewScene->getBufferCachedCondition().notify_all();
            }
        }
        if (buffer == nullptr) {
            {
                std::scoped_lock lock(previewBitmapMutex);
                previewMessage = L"The rendered keyframe could not be copied to the editor";
            }
            PostMessageW(window, WM_TIMELINE_PREVIEW_READY, 0, 0);
            return false;
        }
        SIZE size = {};
        const HBITMAP bitmap = createPreviewBitmap(window, buffer->image, size);
        if (bitmap == nullptr) {
            {
                std::scoped_lock lock(previewBitmapMutex);
                previewMessage = L"The rendered keyframe has an unsupported image format";
            }
            PostMessageW(window, WM_TIMELINE_PREVIEW_READY, 0, 0);
            return false;
        }
        {
            std::scoped_lock lock(previewBitmapMutex);
            if (previewBitmap != nullptr) {
                DeleteObject(previewBitmap);
            }
            previewBitmap = bitmap;
            previewSize = size;
            previewMessage = std::format(L"{} keyframes  |  {}  |  {}",
                                         frameSource->getFrameCount(),
                                         frameSource->isStatic() ? L"PNG" : L"RFM/RFMZ",
                                         frameSource->getDirectory().filename().wstring());
        }
        PostMessageW(window, WM_TIMELINE_PREVIEW_READY, 0, 0);
        return true;
    }

    // The zoom keyframe `id` was rendered at, as its own RFM/RFMZ or RFSM file records it.
    float TimelineWindow::keyframeLogZoom(const uint32_t id) {
        const float increment = std::log10(std::max(attribute.video.data.defaultZoomIncrement, 1.000001f));
        // Without a keyframe to read, the zoom being explored is the only anchor the editor has.
        const float estimated = attribute.fractal.logZoom - static_cast<float>(static_cast<int64_t>(id) - 1) * increment;
        if (frameSource == nullptr || id == 0 || id > frameSource->getFrameCount()) {
            return estimated;
        }
        const uint32_t count = frameSource->getFrameCount();
        if (keyframeLogZooms.size() != static_cast<size_t>(count)) {
            keyframeLogZooms.assign(count, std::nullopt);
        }
        // Not a NaN kept in the vector: -ffast-math is on, and a test for one there is folded away.
        std::optional<float> &cached = keyframeLogZooms[id - 1];
        if (cached.has_value()) {
            return *cached;
        }
        if (frameSource->isStatic()) {
            const RFFStaticMapBinary header = RFFStaticMapBinary::readByID(frameSource->getDirectory(), id);
            cached = header.hasData() ? header.getLogZoom() : estimated;
        } else if (float logZoom = 0;
                   RFFDynamicMapBinary::readLogZoomByID(frameSource->getDirectory(), id, logZoom)) {
            cached = logZoom;
        } else {
            cached = estimated;
        }
        return *cached;
    }

    // The same interpolation the exporter uses, so this number is the zoom the frame is rendered at.
    float TimelineWindow::zoomExponentAt(const float depth) {
        const float increment = std::log10(std::max(attribute.video.data.defaultZoomIncrement, 1.000001f));
        float sampled = std::max(depth, 0.0f);
        if (sampled < 1.0f) {
            // Nothing is stored below the first keyframe, so its zoom is carried one increment deeper.
            const float first = keyframeLogZoom(1);
            return std::lerp(first, first + increment, 1.0f - sampled);
        }
        auto zoomedID = static_cast<uint32_t>(sampled);
        // The last keyframe is where the timeline ends, so the pair below it carries that end.
        if (frameSource != nullptr && zoomedID >= frameSource->getFrameCount()) {
            zoomedID = frameSource->getFrameCount() - 1;
            sampled = static_cast<float>(frameSource->getFrameCount());
        }
        return std::lerp(keyframeLogZoom(zoomedID + 1), keyframeLogZoom(zoomedID),
                         static_cast<float>(zoomedID + 1) - sampled);
    }

    void TimelineWindow::updateFieldDrag(const POINT point) {
        if (fieldDrag == FieldDrag::NONE || timelineAxis.right <= timelineAxis.left) {
            return;
        }
        // A readout moves the playhead at the rate the axis under it would, so both read the same drag.
        const float axisWidth = static_cast<float>(timelineAxis.right - timelineAxis.left);
        const float travel = static_cast<float>(point.x - fieldDragOriginX);
        if (!fieldDragMoved && std::abs(travel) < static_cast<float>(sc(4))) {
            return;
        }
        fieldDragMoved = true;
        float depth = fieldDragDepth;
        if (fieldDrag == FieldDrag::DEPTH) {
            depth -= travel * viewSpan() / axisWidth;
        } else {
            const float shownSeconds = schedule.timeAt(viewEndDepth) - schedule.timeAt(viewStartDepth);
            depth = schedule.depthAt(schedule.timeAt(fieldDragDepth) + travel * shownSeconds / axisWidth);
        }
        previewDepth = std::clamp(snapDepth(depth), schedule.getEndDepth(), schedule.getStartDepth());
        syncPlaybackClock();
        InvalidateRect(window, nullptr, FALSE);
    }

    void TimelineWindow::beginFieldEdit(const FieldEdit field) {
        if (field == FieldEdit::NONE) {
            return;
        }
        closeFieldEdit();

        const RECT box = field == FieldEdit::DISTANCE ? distanceField : keyframeField;
        if (box.right <= box.left || box.bottom <= box.top) {
            return;
        }
        const std::wstring value = field == FieldEdit::DISTANCE
                                       ? Unparser::floatTrim(4)(displayDistance(previewDepth))
                                       : Unparser::floatTrim(4)(previewDepth);
        const RECT editRect = {box.left + sc(8), box.top + sc(9), box.right - sc(8), box.bottom - sc(4)};
        activeFieldEdit = field;
        fieldEdit = CreateWindowExW(0, WC_EDITW, value.c_str(),
                                    WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_CENTER,
                                    editRect.left, editRect.top, editRect.right - editRect.left,
                                    editRect.bottom - editRect.top, window, nullptr, GetModuleHandleW(nullptr), nullptr);
        if (fieldEdit == nullptr) {
            activeFieldEdit = FieldEdit::NONE;
            return;
        }
        SendMessageW(fieldEdit, WM_SETFONT, reinterpret_cast<WPARAM>(valueFont), TRUE);
        SendMessageW(fieldEdit, EM_SETLIMITTEXT, 128, 0);
        SendMessageW(fieldEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(sc(4), sc(4)));
        SetWindowSubclass(fieldEdit, fieldEditProc, 1, reinterpret_cast<DWORD_PTR>(this));
        SetFocus(fieldEdit);
        SendMessageW(fieldEdit, EM_SETSEL, 0, -1);
        InvalidateRect(window, nullptr, FALSE);
    }

    bool TimelineWindow::commitFieldEdit() {
        if (fieldEdit == nullptr || activeFieldEdit == FieldEdit::NONE) {
            return true;
        }
        const int length = GetWindowTextLengthW(fieldEdit);
        std::wstring text(static_cast<size_t>(length) + 1, L'\0');
        GetWindowTextW(fieldEdit, text.data(), length + 1);
        text.resize(length);
        const std::optional<double> value = NumericExpression::evaluate(text);
        if (!value.has_value() || std::abs(*value) > std::numeric_limits<float>::max()) {
            MessageBeep(MB_ICONWARNING);
            SendMessageW(fieldEdit, EM_SETSEL, 0, -1);
            return false;
        }

        const double depth = activeFieldEdit == FieldEdit::DISTANCE ? depthFromDistance(static_cast<float>(*value))
                                                                    : *value;
        previewDepth = std::clamp(static_cast<float>(depth), schedule.getEndDepth(), schedule.getStartDepth());
        syncPlaybackClock();
        closeFieldEdit();
        requestFramePreview();
        InvalidateRect(window, nullptr, FALSE);
        return true;
    }

    void TimelineWindow::closeFieldEdit() {
        if (fieldEdit == nullptr) {
            activeFieldEdit = FieldEdit::NONE;
            return;
        }
        const HWND edit = fieldEdit;
        fieldEdit = nullptr;
        activeFieldEdit = FieldEdit::NONE;
        RemoveWindowSubclass(edit, fieldEditProc, 1);
        DestroyWindow(edit);
        InvalidateRect(window, nullptr, FALSE);
    }

    bool TimelineWindow::scrubEdgeScroll(const POINT point) {
        const int width = static_cast<int>(timelineAxis.right - timelineAxis.left);
        const int margin = std::min(sc(28), width / 4);
        if (margin <= 0) {
            return false;
        }
        const int past = point.x < timelineAxis.left + margin
                             ? point.x - static_cast<int>(timelineAxis.left) - margin
                             : point.x > timelineAxis.right - margin
                                   ? point.x - static_cast<int>(timelineAxis.right) + margin
                                   : 0;
        if (past == 0) {
            return false;
        }
        // The further the pointer is held past the edge the faster the view follows it, up to a cap.
        const float rate = std::clamp(static_cast<float>(past) / static_cast<float>(margin), -3.0f, 3.0f);
        const float previousStart = viewStartDepth;
        panView(-viewSpan() * SCRUB_EDGE_RATE * rate);
        return viewStartDepth != previousStart;
    }

    bool TimelineWindow::rowEdgeScroll(const POINT point) {
        const int margin = std::min(sc(20), static_cast<int>(timelineAxis.bottom - timelineAxis.top) / 4);
        if (margin <= 0) {
            return false;
        }
        const int delta = point.y < timelineAxis.top + margin
                              ? -sc(6)
                              : point.y > timelineAxis.bottom - margin ? sc(6) : 0;
        if (delta == 0) {
            return false;
        }
        const int previous = trackScrollOffset;
        scrollTracks(delta);
        return trackScrollOffset != previous;
    }

    void TimelineWindow::updateScrubDepth(const POINT point) {
        if (timelineAxis.right <= timelineAxis.left) {
            return;
        }
        // A depth read at an edge of the view rounds to a keyframe just outside what the view
        // shows, where the playhead is drawn nowhere, so it snaps to the ones the view holds.
        const float lowest = std::ceil(viewEndDepth);
        const float highest = std::floor(viewStartDepth);
        const float snapped = snapDepth(viewDepthAt(point.x));
        const float held = lowest <= highest
                               ? std::clamp(snapped, lowest, highest)
                               : std::clamp(snapped, viewEndDepth, viewStartDepth);
        previewDepth = std::clamp(held, schedule.getEndDepth(), schedule.getStartDepth());
        syncPlaybackClock();
        InvalidateRect(window, nullptr, FALSE);
    }

    void TimelineWindow::syncPlaybackClock() {
        playSeconds = schedule.timeAt(previewDepth);
        if (playing) {
            playTick = GetTickCount64();
        }
    }

    TimelineWindow::~TimelineWindow() {
        // The Shader panels this editor opened stand on rows held to what a track can carry, and
        // they write into a timeline that is going away, so they close with it. A panel opened from
        // the Shader menu is untouched and keeps every row it has.
        for (const HWND panel: recordingPanels) {
            if (IsWindow(panel)) {
                DestroyWindow(panel);
            }
        }
        stopFramePreviewWorker();
        destroyFramePreview();
        if (mainPreviewPauseClaimed) {
            --openTimelineWindows;
        }
        if (previewBitmap != nullptr) {
            DeleteObject(previewBitmap);
        }
        if (titleFont != nullptr) {
            DeleteObject(titleFont);
        }
        if (bodyFont != nullptr) {
            DeleteObject(bodyFont);
        }
        if (smallFont != nullptr) {
            DeleteObject(smallFont);
        }
        if (captionFont != nullptr) {
            DeleteObject(captionFont);
        }
        if (valueFont != nullptr) {
            DeleteObject(valueFont);
        }
        if (fieldEditBrush != nullptr) {
            DeleteObject(fieldEditBrush);
        }
    }

    void TimelineWindow::open(SettingsMenu &menu, RenderScene &scene, HWND owner) {
        registerTimelineWindowClass();
        if (owner != nullptr) {
            owner = GetAncestor(owner, GA_ROOT);
        }
        auto *timeline = new TimelineWindow(menu, scene);
        if (!timeline->create(owner)) {
            delete timeline;
        }
    }

    bool TimelineWindow::isOpen() {
        return openTimelineWindows.load(std::memory_order_relaxed) > 0;
    }

    bool TimelineWindow::create(const HWND owner) {
        const int designWidth = sc(1180);
        const int designHeight = sc(780);
        RECT frame = {0, 0, designWidth, designHeight};
        AdjustWindowRectEx(&frame, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW);

        RECT work = {};
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
        const int width = std::min(frame.right - frame.left, work.right - work.left);
        const int height = std::min(frame.bottom - frame.top, work.bottom - work.top);
        const int x = work.left + (work.right - work.left - width) / 2;
        const int y = work.top + (work.bottom - work.top - height) / 2;

        // A tool window's caption draws no minimize or maximize box and carries no taskbar button to come back to.
        window = CreateWindowExW(WS_EX_APPWINDOW, TIMELINE_WINDOW_CLASS, L"RFF_Super - Timeline Editor",
                                 WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, x, y, width, height, owner, nullptr,
                                 GetModuleHandleW(nullptr), this);
        if (window == nullptr) {
            return false;
        }
        fieldTooltip = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr,
                                       WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
                                       CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                       window, nullptr, GetModuleHandleW(nullptr), nullptr);
        if (fieldTooltip != nullptr) {
            SetWindowPos(fieldTooltip, HWND_TOPMOST, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            SendMessageW(fieldTooltip, TTM_SETMAXTIPWIDTH, 0, sc(520));
            SendMessageW(fieldTooltip, TTM_SETDELAYTIME, TTDT_INITIAL, 350);
            for (UINT_PTR id = 1; id <= 2; ++id) {
                TOOLINFOW tool = {};
                tool.cbSize = sizeof(tool);
                tool.uFlags = TTF_SUBCLASS;
                tool.hwnd = window;
                tool.uId = id;
                tool.lpszText = const_cast<wchar_t *>(FORMULA_FIELD_HINT);
                SendMessageW(fieldTooltip, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&tool));
            }
        }
        ++openTimelineWindows;
        mainPreviewPauseClaimed = true;
        // Every shader re-render a settings panel asks for is reported here from now on, which is
        // what lets an edit made in one of those panels be recorded as a key.
        if (renderScene != nullptr) {
            recordBaseline = sourceAttribute->shader;
            renderScene->getRequests().shaderEditListener.store(window, std::memory_order_release);
        }
        ShowWindow(window, SW_SHOW);
        UpdateWindow(window);
        SetForegroundWindow(window);
        return true;
    }

    void TimelineWindow::setPlaying(const bool play) {
        if (play == playing) {
            return;
        }
        playing = play;
        if (!playing) {
            KillTimer(window, PLAYBACK_TIMER);
            return;
        }
        // Starting at the end replays from the top rather than sitting still on the last frame.
        playSeconds = schedule.timeAt(previewDepth);
        if (playSeconds >= schedule.getTotalSeconds() - 1e-3f) {
            playSeconds = 0.0f;
            previewDepth = schedule.getStartDepth();
        }
        playTick = GetTickCount64();
        SetTimer(window, PLAYBACK_TIMER, PLAYBACK_INTERVAL, nullptr);
    }

    void TimelineWindow::stopPlayback() {
        setPlaying(false);
        playSeconds = 0.0f;
        previewDepth = schedule.getStartDepth();
        requestFramePreview();
    }

    void TimelineWindow::advancePlayback() {
        const ULONGLONG now = GetTickCount64();
        const float elapsed = static_cast<float>(now - playTick) / 1000.0f;
        playTick = now;
        const float total = std::max(schedule.getTotalSeconds(), 1e-3f);
        playSeconds += elapsed;
        if (playSeconds >= total) {
            if (loopPlayback) {
                playSeconds = std::fmod(playSeconds, total);
            } else {
                playSeconds = total;
                setPlaying(false);
            }
        }
        previewDepth = schedule.depthAt(playSeconds);
        requestFramePreview();
        InvalidateRect(window, nullptr, FALSE);
    }

    void TimelineWindow::setViewZoom(const float factor) {
        const float fullSpan = std::max(schedule.getStartDepth() - schedule.getEndDepth(), 1e-6f);
        const float minSpan = std::min(fullSpan, MIN_VIEW_SPAN);
        const float span = std::clamp(fullSpan / std::max(factor, 0.01f), minSpan, fullSpan);
        // Zooming keeps the playhead where it is whenever it is on screen to begin with.
        const float pivot = previewDepth <= viewStartDepth && previewDepth >= viewEndDepth
                                ? previewDepth
                                : (viewStartDepth + viewEndDepth) * 0.5f;
        viewStartDepth = pivot + span * 0.5f;
        viewEndDepth = viewStartDepth - span;
        clampView();
    }

    void TimelineWindow::openZoomMenu() {
        const HMENU menu = CreatePopupMenu();
        if (menu == nullptr) {
            return;
        }
        static constexpr int PERCENTS[] = {100, 200, 400, 800, 1600, 3200, 6400};
        constexpr int count = static_cast<int>(std::size(PERCENTS));
        for (int i = 0; i < count; ++i) {
            const std::wstring item = PERCENTS[i] == 100
                                          ? std::format(L"{}%  (fit)", PERCENTS[i])
                                          : std::format(L"{}%", PERCENTS[i]);
            AppendMenuW(menu, MF_STRING, static_cast<UINT_PTR>(i + 1), item.c_str());
        }
        POINT at = {zoomPresetButton.left, zoomPresetButton.bottom};
        ClientToScreen(window, &at);
        const int chosen = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN, at.x, at.y, 0,
                                          window, nullptr);
        DestroyMenu(menu);
        if (chosen >= 1 && chosen <= count) {
            setViewZoom(static_cast<float>(PERCENTS[chosen - 1]) / 100.0f);
            InvalidateRect(window, nullptr, FALSE);
        }
    }

    void TimelineWindow::openTrackMenu(const POINT point) {
        // What the click landed on decides what the menu offers: a key, a row of one parameter, or
        // neither, which still adds a parameter.
        const KeyHit hit = hitTrackKey(point);
        uint16_t rowTarget = hitTrackLabel(point);
        if (rowTarget == UINT16_MAX) {
            rowTarget = hitTrackRow(point, false);
        }
        if (hit.valid()) {
            rowTarget = hit.targetId;
            selectedTrackTarget = hit.targetId;
            selectedTrackKey = hit.keyIndex;
        } else if (rowTarget != UINT16_MAX) {
            selectedTrackTarget = rowTarget;
            selectedTrackKey = -1;
        }
        InvalidateRect(window, nullptr, FALSE);

        const HMENU menu = CreatePopupMenu();
        if (menu == nullptr) {
            return;
        }
        constexpr int CMD_ADD_KEY = 1;
        constexpr int CMD_DELETE_KEY = 2;
        constexpr int CMD_REMOVE_TRACK = 3;
        constexpr int CMD_INTERPOLATION = 10;
        constexpr int CMD_PARAMETER = 100;
        constexpr int INTERPOLATION_COUNT = 4;

        const VidTimelineTrack *current = rowTarget == UINT16_MAX ? nullptr : track(rowTarget);
        if (hit.valid() && current != nullptr) {
            const HMENU interpolations = CreatePopupMenu();
            for (int i = 0; i < INTERPOLATION_COUNT; ++i) {
                const auto mode = static_cast<VidKeyInterpolation>(i);
                AppendMenuW(interpolations, MF_STRING | (current->keys[hit.keyIndex].out == mode ? MF_CHECKED : 0),
                            CMD_INTERPOLATION + i, interpolationName(mode).c_str());
            }
            AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(interpolations), L"To Next Key");
            AppendMenuW(menu, MF_STRING, CMD_DELETE_KEY, L"Delete Key");
            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        } else if (rowTarget != UINT16_MAX && editableTarget(rowTarget) && layout(rowTarget) != nullptr &&
                   contains(layout(rowTarget)->row, point)) {
            AppendMenuW(menu, MF_STRING, CMD_ADD_KEY, L"Add Key Here");
            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        }

        // The menu names the Shader menu's panels, not the two hundred parameters under them, and
        // opening one opens that panel itself.
        const HMENU parameters = CreatePopupMenu();
        for (size_t i = 0; i < SHADER_PANELS.size(); ++i) {
            // A panel nothing of which reaches a PNG is not offered at all: the command still counts
            // from the panel's own index, so what is left opens the panel it names.
            if (attribute.video.data.isStatic && !panelMovesOverStaticImage(SHADER_PANELS[i])) {
                continue;
            }
            AppendMenuW(parameters, MF_STRING, static_cast<UINT_PTR>(CMD_PARAMETER) + i,
                        SHADER_PANELS[i].name);
        }
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(parameters), L"Parameters");
        const bool removable = current != nullptr && rowTarget != SPEED_TARGET;
        const std::wstring removeItem = rowTarget == UINT16_MAX
                                            ? std::wstring(L"Remove Parameter")
                                            : L"Remove " + rowName(rowTarget, linkColorCycle);
        AppendMenuW(menu, MF_STRING | (removable ? 0 : MF_GRAYED), CMD_REMOVE_TRACK, removeItem.c_str());

        POINT at = point;
        ClientToScreen(window, &at);
        const int chosen = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN, at.x, at.y, 0,
                                          window, nullptr);
        DestroyMenu(menu);
        if (chosen >= CMD_PARAMETER) {
            openShaderPanel(static_cast<size_t>(chosen - CMD_PARAMETER));
            return;
        }
        if (chosen >= CMD_INTERPOLATION && chosen < CMD_INTERPOLATION + INTERPOLATION_COUNT) {
            setTrackInterpolation(static_cast<VidKeyInterpolation>(chosen - CMD_INTERPOLATION));
            return;
        }
        switch (chosen) {
            case CMD_ADD_KEY:
                addTrackKey(rowTarget, point);
                break;
            case CMD_DELETE_KEY:
                deleteTrackKey();
                break;
            case CMD_REMOVE_TRACK:
                removeTrack(rowTarget);
                break;
            default:
                break;
        }
    }

    void TimelineWindow::openShaderPanel(const size_t index) {
        if (settingsMenu == nullptr || renderScene == nullptr || index >= SHADER_PANELS.size()) {
            return;
        }
        const ShaderPanel &panel = SHADER_PANELS[index];
        // A panel that was closed leaves its entry standing until the next one is opened, and the
        // Shader menu's own code drops those as it adds its window. Counting across the call would
        // then read a replaced entry as no new panel at all and leave this one unarmed, so the
        // closed ones go first and the count below is the panel itself.
        std::erase_if(settingsMenu->activeSettingsWindows,
                      [](const SettingsMenu::ActiveSettingsWindow &active) {
                          return !IsWindow(active.window->getWindow());
                      });
        // The Shader menu's own panel, built by the Shader menu's own code against the very
        // attribute it edits there: what opens here is that panel and not a copy of it.
        const size_t before = settingsMenu->activeSettingsWindows.size();
        (*panel.callback)(*settingsMenu, *renderScene);
        if (settingsMenu->activeSettingsWindows.size() <= before) {
            return;
        }
        SettingsWindow &window = *settingsMenu->activeSettingsWindows.back().window;
        // Every parameter the timeline can carry, by where it sits in the attribute the rows are
        // bound to. A row on any other part of the shader is one no track can drive.
        std::unordered_set<const void *> driven;
        for (const auto &param: TimelineParams::all()) {
            // Over a PNG source a row the picture cannot answer is left out of the kept set with the
            // ones no track drives, so the panel greys it out rather than offering an edit that does nothing.
            if (param.address != nullptr &&
                (!attribute.video.data.isStatic || TimelineParams::movesOverStaticImage(param.id))) {
                driven.insert(param.address(sourceAttribute->shader));
            }
        }
        window.disableRowsInObjectExcept(&sourceAttribute->shader, sizeof(ShaderAttribute), driven);
        recordBaseline = sourceAttribute->shader;
        std::erase_if(recordingPanels, [](const HWND open) { return !IsWindow(open); });
        recordingPanels.push_back(window.getWindow());
        adoptPanel(window);
    }

    bool TimelineWindow::recording() const {
        return std::ranges::any_of(recordingPanels, [](const HWND open) { return IsWindow(open) != FALSE; });
    }

    void TimelineWindow::recordShaderEdits() {
        if (sourceAttribute == nullptr) {
            return;
        }
        const ShaderAttribute &now = sourceAttribute->shader;
        // The editor draws its rows against its own copy, so it follows the settings either way.
        attribute.shader = now;
        if (!recording()) {
            // Nothing of this editor's is open on the shader, so an edit made elsewhere is only
            // read by the next preview, as it always was.
            recordBaseline = now;
            return;
        }
        std::vector<const TimelineParamDesc *> changed;
        for (const auto &param: TimelineParams::all()) {
            const bool differs = param.kind == TimelineParamKind::COLOR
                                     ? param.getColor(now) != param.getColor(recordBaseline)
                                     : param.getValue(now) != param.getValue(recordBaseline);
            if (differs) {
                changed.push_back(&param);
            }
        }
        recordBaseline = now;
        // A whole group of settings replaced at once is a preset being loaded, not a row being
        // moved, and putting a key on every parameter it touched is not what loading one asks for.
        if (changed.empty() || changed.size() > MAX_RECORDED_AT_ONCE) {
            if (!changed.empty() && window != nullptr) {
                InvalidateRect(window, nullptr, FALSE);
            }
            return;
        }
        for (const TimelineParamDesc *param: changed) {
            if (param->kind == TimelineParamKind::COLOR) {
                setParameterColor(param->id, param->getColor(now));
            } else {
                setParameterValue(param->id, param->getValue(now));
            }
        }
    }

    void TimelineWindow::paint(const HDC target, const RECT &client) {
        const TimelineTheme &theme = timelineTheme(lightMode);
        const int width = client.right - client.left;
        const int height = client.bottom - client.top;
        const HDC canvas = CreateCompatibleDC(target);
        const HBITMAP bitmap = CreateCompatibleBitmap(target, std::max(width, 1), std::max(height, 1));
        const HGDIOBJ previousBitmap = SelectObject(canvas, bitmap);
        fillRect(canvas, client, theme.background);

        // One spacing value for the window edges, the header inset and the gaps between the three
        // panels, so nothing sits closer to its neighbour than anything else does.
        const int margin = sc(12);
        const int headerHeight = sc(88);
        const int transportHeight = sc(72);
        const int labelWidth = sc(360);
        const int rulerRowHeight = sc(34);
        const int axisTopInset = rulerRowHeight * 2 + sc(8);
        const int footerHeight = sc(46);
        const int zoomRowHeight = sc(38);
        const int scrollHeight = sc(12);
        const int minRowHeight = sc(52);
        const int minAxisHeight = sc(96);
        // The panel below has to keep its ruler, its rows, the zoom bar and the footer, so the preview gives way first.
        const int timelineMinHeight = axisTopInset + minAxisHeight + zoomRowHeight + footerHeight;
        // The preview and the editor take the same height, with the transport at its own between them.
        const int contentTop = headerHeight + margin;
        const int available = std::max(height - contentTop - transportHeight - margin * 3, sc(160));
        int timelineHeight = available - available / 2;
        if (timelineHeight < timelineMinHeight) {
            timelineHeight = std::min(available, timelineMinHeight);
        }
        const int previewHeight = available - timelineHeight;

        RECT header = {0, 0, width, headerHeight};
        fillRect(canvas, header, theme.panel);
        const int boxTop = sc(30);
        const int boxBottom = headerHeight - sc(16);
        const int buttonTop = boxTop + sc(2);
        const int buttonBottom = boxBottom - sc(2);
        const int headerInset = margin + sc(20);
        exportButton = {width - headerInset - sc(112), buttonTop, width - headerInset, buttonBottom};
        saveButton = {exportButton.left - sc(10) - sc(112), buttonTop, exportButton.left - sc(10), buttonBottom};
        loadButton = {saveButton.left - sc(10) - sc(96), buttonTop, saveButton.left - sc(10), buttonBottom};
        fullscreenButton = {loadButton.left - sc(20) - sc(104), buttonTop, loadButton.left - sc(20), buttonBottom};
        framesButton = {fullscreenButton.left - sc(20) - sc(146), boxTop, fullscreenButton.left - sc(20), boxBottom};
        themeButton = {framesButton.left - sc(20) - sc(126), boxTop, framesButton.left - sc(20), boxBottom};

        RECT title = {headerInset, buttonTop, themeButton.left - sc(12), buttonBottom};
        drawText(canvas, L"Timeline Editor", title, theme.text,
                 DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS,
                 titleFont);
        {
            const RECT inner = drawCaptionBox(canvas, themeButton, L"Theme", theme.panel, captionFont, hoverTheme,
                                              lightMode, theme);
            const int mid = static_cast<int>(inner.top + inner.bottom) / 2;
            const RECT toggle = {inner.right - sc(46), mid - sc(11), inner.right, mid + sc(11)};
            drawToggle(canvas, toggle, lightMode, theme);
            drawText(canvas, lightMode ? L"Light" : L"Dark",
                     {inner.left, inner.top, toggle.left - sc(8), inner.bottom},
                     lightMode ? theme.accentText : theme.text, DT_LEFT | DT_VCENTER | DT_SINGLELINE, bodyFont);
        }
        {
            const bool loaded = frameSource != nullptr;
            const RECT inner = drawCaptionBox(canvas, framesButton, L"Keyframes", theme.panel, captionFont,
                                              hoverFrames, loaded, theme);
            drawText(canvas, loaded ? L"Loaded" : L"Select folder", inner,
                     loaded ? theme.accentText : theme.text,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, bodyFont);
        }
        drawButton(canvas, fullscreenButton, fullscreen ? L"WINDOW" : L"FULL", hoverFullscreen, fullscreen,
                   smallFont, theme);
        drawButton(canvas, loadButton, L"LOAD", hoverLoad, false, smallFont, theme);
        drawButton(canvas, saveButton, L"SAVE", hoverSave, false, smallFont, theme);
        drawButton(canvas, exportButton, exporting ? L"EXPORTING" : L"EXPORT", hoverExport, true,
                   smallFont, theme);

        RECT previewPanel = {margin, contentTop, width - margin, contentTop + previewHeight};
        fillRect(canvas, previewPanel, theme.previewBackground);
        frameRect(canvas, previewPanel, theme.border);
        RECT preview = previewPanel;
        preview.left += sc(10);
        preview.right -= sc(10);
        preview.top += sc(10);
        preview.bottom -= sc(10);
        {
            std::scoped_lock lock(previewBitmapMutex);
            if (previewBitmap != nullptr && previewSize.cx > 0 && previewSize.cy > 0) {
                const double sourceRatio = static_cast<double>(previewSize.cx) / previewSize.cy;
                const double targetRatio = static_cast<double>(preview.right - preview.left) /
                                           std::max(preview.bottom - preview.top, 1L);
                RECT image = preview;
                if (targetRatio > sourceRatio) {
                    const int imageWidth = static_cast<int>((preview.bottom - preview.top) * sourceRatio);
                    image.left += (preview.right - preview.left - imageWidth) / 2;
                    image.right = image.left + imageWidth;
                } else {
                    const int imageHeight = static_cast<int>((preview.right - preview.left) / sourceRatio);
                    image.top += (preview.bottom - preview.top - imageHeight) / 2;
                    image.bottom = image.top + imageHeight;
                }
                const HDC source = CreateCompatibleDC(canvas);
                const HGDIOBJ previous = SelectObject(source, previewBitmap);
                SetStretchBltMode(canvas, HALFTONE);
                StretchBlt(canvas, image.left, image.top, image.right - image.left, image.bottom - image.top,
                           source, 0, 0, previewSize.cx, previewSize.cy, SRCCOPY);
                SelectObject(source, previous);
                DeleteDC(source);
            } else {
                // An empty panel named for a render read as one already loaded, so with no keyframe
                // folder behind it the panel says that instead of naming what it would hold.
                drawText(canvas, frameSource == nullptr ? L"NO KEYFRAMES LOADED" : L"CURRENT RENDER PREVIEW",
                         preview, theme.mutedText, DT_CENTER | DT_VCENTER | DT_SINGLELINE, bodyFont);
            }
        }

        RECT transport = {margin, previewPanel.bottom + margin, width - margin,
                          previewPanel.bottom + margin + transportHeight};
        fillRect(canvas, transport, theme.panel);
        frameRect(canvas, transport, theme.border);
        const int transportMid = static_cast<int>(transport.top + transport.bottom) / 2;
        const int glyphSize = sc(28);
        const int glyphGap = sc(6);
        const int transportPad = sc(14);
        playButton = {transport.left + transportPad, transportMid - glyphSize / 2,
                      transport.left + transportPad + glyphSize, transportMid + glyphSize / 2};
        pauseButton = {playButton.right + glyphGap, playButton.top, playButton.right + glyphGap + glyphSize,
                       playButton.bottom};
        stopButton = {pauseButton.right + glyphGap, playButton.top, pauseButton.right + glyphGap + glyphSize,
                      playButton.bottom};
        loopButton = {stopButton.right + transportPad, playButton.top, stopButton.right + transportPad + glyphSize,
                      playButton.bottom};
        drawTransportButton(canvas, playButton, TransportGlyph::PLAY, hoverPlay, playing, theme);
        drawTransportButton(canvas, pauseButton, TransportGlyph::PAUSE, hoverPause, false, theme);
        drawTransportButton(canvas, stopButton, TransportGlyph::STOP, hoverStop, false, theme);
        drawTransportButton(canvas, loopButton, TransportGlyph::LOOP, hoverLoop, loopPlayback, theme);
        const int separatorX = static_cast<int>(loopButton.right) + transportPad;
        {
            const HPEN separator = CreatePen(PS_SOLID, 1, theme.border);
            const HGDIOBJ oldSeparator = SelectObject(canvas, separator);
            MoveToEx(canvas, separatorX, transport.top + sc(10), nullptr);
            LineTo(canvas, separatorX, transport.bottom - sc(10));
            SelectObject(canvas, oldSeparator);
            DeleteObject(separator);
        }

        const float startDepth = schedule.getStartDepth();
        const float zoomExponent = zoomExponentAt(previewDepth);
        const std::wstring fields[4][2] = {
            {L"Distance", std::format(L"{:.1f}", displayDistance(previewDepth))},
            {L"Keyframe", std::format(L"{:.1f}", previewDepth)},
            // Read off the keyframe files; with none open the depth axis is a placeholder and the
            // zoom extrapolated along it says nothing, so the readout stands empty until one is.
            {L"Zoom", frameSource == nullptr ? std::wstring(L"\u2014") : std::format(L"1E{:.1f}", zoomExponent)},
            {L"Time", std::format(L"{} / {}", durationText(schedule.timeAt(previewDepth)),
                                  durationText(schedule.getTotalSeconds()))},
        };
        RECT *const fieldRects[4] = {&distanceField, &keyframeField, &zoomField, &timeField};
        // The readouts share one width and one gap, so the row is spaced as evenly as it reads.
        const int fieldGap = sc(10);
        const int fieldsLeft = separatorX + transportPad;
        const int fieldWidth = std::clamp(
            (static_cast<int>(transport.right) - sc(240) - transportPad - fieldsLeft - fieldGap * 3) / 4,
            sc(104), sc(190));
        int fieldX = fieldsLeft;
        RECT activeEditRect = {};
        for (int i = 0; i < 4; ++i) {
            const RECT box = {fieldX, transportMid - sc(22), fieldX + fieldWidth, transportMid + sc(22)};
            *fieldRects[i] = {};
            if (box.right > transport.right - transportPad) {
                break;
            }
            *fieldRects[i] = box;
            // The boxes that carry the playhead light up while one of them is being dragged.
            const bool edited = activeFieldEdit == FieldEdit::DISTANCE ? i == 0
                                : activeFieldEdit == FieldEdit::KEYFRAME && i == 1;
            const bool dragged = fieldDrag == FieldDrag::DEPTH ? i == 0 || i == 1
                                     : fieldDrag == FieldDrag::TIME && i == 3;
            const bool hovered = hoveredFieldEdit == FieldEdit::DISTANCE ? i == 0
                                   : hoveredFieldEdit == FieldEdit::KEYFRAME && i == 1;
            const RECT inner = drawCaptionBox(canvas, box, fields[i][0], theme.panel, captionFont, hovered,
                                              dragged || edited, theme);
            if (edited) {
                activeEditRect = {box.left + sc(8), box.top + sc(9), box.right - sc(8), box.bottom - sc(4)};
            } else {
                drawText(canvas, fields[i][1], inner, theme.text, DT_CENTER | DT_VCENTER | DT_SINGLELINE, valueFont);
            }
            fieldX = box.right + fieldGap;
        }
        std::wstring status;
        if (activeFieldEdit != FieldEdit::NONE) {
            status = L"Formula: +  -  *  /  ( )    Enter: apply    Esc: cancel";
        } else if (hoveredFieldEdit != FieldEdit::NONE) {
            status = FORMULA_FIELD_HINT;
        } else if (previewBusy) {
            status = std::format(L"Rendering d {:.2f}...", previewBusyDepth);
        } else {
            std::scoped_lock lock(previewBitmapMutex);
            status = previewMessage;
        }
        drawText(canvas, status, {fieldX, transport.top, transport.right - transportPad, transport.bottom},
                 theme.mutedText, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, smallFont);

        RECT timeline = {margin, transport.bottom + margin, width - margin,
                         transport.bottom + margin + timelineHeight};
        fillRect(canvas, timeline, theme.panel);
        frameRect(canvas, timeline, theme.border);
        timelinePanel = timeline;
        // One left column for the whole panel: the track icons, the ruler captions and the magnifier
        // all start at the same inset, and every word that follows an icon starts at the same one too.
        const int rowIconLeft = timeline.left + sc(18);
        const int rowIconWidth = sc(24);
        const int rowTextLeft = rowIconLeft + rowIconWidth + sc(12);
        // The column at the right end carries the track scrollbar and nothing else, and the axis
        // stops short of it. The bar sets the width, so the corner where the two bars meet is the
        // two bars and the gap between them rather than a control wedged in with them.
        const int rowRightPad = sc(18);
        const int rightColumnWidth = sc(12);
        const int rightColumnLeft = timeline.right - rowRightPad - rightColumnWidth;
        // The rows are the tracks the timeline holds, stacked in the order the file keeps them, so
        // a row carried over another one keeps the place it was dropped in.
        std::vector<const VidTimelineTrack *> displayedTracks;
        for (const auto &track: attribute.video.timeline.tracks) {
            // The R row carries all three channels while they are linked, so G and B are not shown.
            if (linkColorCycle && (track.targetId == CYCLE_G_TARGET || track.targetId == CYCLE_B_TARGET)) {
                continue;
            }
            // A row a PNG source cannot move is not stacked here either; the track stays in the file
            // and comes back with the row the moment an RFM/RFMZ source is opened.
            if (attribute.video.data.isStatic && !TimelineParams::movesOverStaticImage(track.targetId)) {
                continue;
            }
            displayedTracks.push_back(&track);
        }
        const int trackCount = std::max<int>(1, static_cast<int>(displayedTracks.size()));
        const int guideTop = timeline.bottom - footerHeight;
        const int zoomRowTop = guideTop - zoomRowHeight;
        const int axisTop = timeline.top + axisTopInset;
        const int axisBottom = zoomRowTop - sc(6);
        const int viewHeight = std::max(axisBottom - axisTop, 1);
        // The rows share the view four ways at the most: past that a row is too short to read the
        // value or to drag a key on, so the stack scrolls at a quarter of the view rather than
        // thinning every row further towards nothing.
        constexpr int maxVisibleTracks = 4;
        const int trackRowHeight = std::max(minRowHeight, viewHeight / maxVisibleTracks);
        const bool tracksScroll = trackCount * trackRowHeight > viewHeight;
        // The axis stops short of the right column whether the rows scroll or not, so the bar that
        // column holds keeps its place and widening the window never drops it.
        timelineAxis = {timeline.left + labelWidth, axisTop, rightColumnLeft - sc(16), axisBottom};
        RECT &axis = timelineAxis;
        trackLayouts.clear();
        trackScrollTrack = {};
        trackScrollThumb = {};
        if (axis.right <= axis.left || axis.bottom <= axis.top) {
            BitBlt(target, 0, 0, width, height, canvas, 0, 0, SRCCOPY);
            SelectObject(canvas, previousBitmap);
            DeleteObject(bitmap);
            DeleteDC(canvas);
            return;
        }

        const float fullEndDepth = schedule.getEndDepth();
        clampView();
        const float viewStart = viewStartDepth;
        const float viewEnd = viewEndDepth;
        // The ruler carries the distance above the time it is reached at, both read off the same tick.
        const int distanceRowTop = timeline.top + sc(4);
        const int timeRowTop = distanceRowTop + rulerRowHeight;
        rulerStrip = {axis.left, distanceRowTop, axis.right, axisTop};
        drawText(canvas, L"Distance (d)", {rowIconLeft, distanceRowTop, axis.left - sc(12),
                                           distanceRowTop + rulerRowHeight}, theme.mutedText,
                 DT_LEFT | DT_VCENTER | DT_SINGLELINE, smallFont);
        drawText(canvas, L"Time", {rowIconLeft, timeRowTop, axis.left - sc(12),
                                   timeRowTop + rulerRowHeight}, theme.mutedText,
                 DT_LEFT | DT_VCENTER | DT_SINGLELINE, smallFont);
        const int tickSlots = std::clamp(static_cast<int>((axis.right - axis.left) / sc(132)), 2, 16);
        const float tickStep = depthTickStep(viewStart - viewEnd, tickSlots);
        const int tickDecimals = tickStep < 0.095f ? 2 : tickStep < 0.95f ? 1 : 0;
        const float viewStartDistance = displayDistance(viewStart);
        const float viewEndDistance = displayDistance(viewEnd);
        const float firstTick = std::ceil(viewStartDistance / tickStep) * tickStep;
        const int tickCount = std::min(static_cast<int>((viewEndDistance - firstTick) / tickStep) + 1, 64);
        for (int i = 0; i < tickCount; ++i) {
            const float distance = firstTick + tickStep * static_cast<float>(i);
            const float depth = depthFromDistance(distance);
            const int x = depthX(depth, viewStart, viewEnd, axis);
            // The minor marks sit between this tick and the next, on the distance row alone.
            const HPEN minorPen = CreatePen(PS_SOLID, 1, theme.grid);
            const HGDIOBJ oldMinor = SelectObject(canvas, minorPen);
            for (int minor = 1; minor < 5; ++minor) {
                const int minorX = depthX(depthFromDistance(distance + tickStep * static_cast<float>(minor) / 5.0f),
                                          viewStart, viewEnd, axis);
                if (!visibleX(minorX, axis, 0)) {
                    continue;
                }
                MoveToEx(canvas, minorX, timeRowTop - sc(7), nullptr);
                LineTo(canvas, minorX, timeRowTop - sc(1));
            }
            SelectObject(canvas, oldMinor);
            DeleteObject(minorPen);
            if (!visibleX(x, axis, 0)) {
                continue;
            }
            const HPEN pen = CreatePen(PS_SOLID, 1, theme.grid);
            const HGDIOBJ oldPen = SelectObject(canvas, pen);
            MoveToEx(canvas, x, timeRowTop - sc(11), nullptr);
            LineTo(canvas, x, axis.bottom);
            SelectObject(canvas, oldPen);
            DeleteObject(pen);
            drawText(canvas, std::format(L"d {:.{}f}", distance, tickDecimals),
                     {x - sc(62), distanceRowTop, x + sc(62), timeRowTop - sc(4)}, theme.distanceTick,
                     DT_CENTER | DT_VCENTER | DT_SINGLELINE, smallFont);
            drawText(canvas, durationText(schedule.timeAt(depth)),
                     {x - sc(58), timeRowTop, x + sc(58), timeRowTop + rulerRowHeight}, theme.mutedText,
                     DT_CENTER | DT_VCENTER | DT_SINGLELINE, smallFont);
        }

        // Every row is carried by its name cell to any place in the stack, the Speed row included.
        reorderRowTargets.clear();
        for (const VidTimelineTrack *item: displayedTracks) {
            reorderRowTargets.push_back(item->targetId);
        }
        // Every track reads the same, so every row gets the same height.
        const int rowHeight = tracksScroll ? trackRowHeight : viewHeight / trackCount;
        const int contentHeight = trackCount * rowHeight;
        trackScrollRange = std::max(contentHeight - viewHeight, 0);
        trackScrollOffset = std::clamp(trackScrollOffset, 0, trackScrollRange);
        // Rows are drawn at their own height wherever the scroll puts them, and cut at the view.
        const int trackClipRight = static_cast<int>(axis.right) + sc(10);
        const HRGN trackClip = CreateRectRgn(timeline.left + sc(2), axis.top, trackClipRight, axis.bottom);
        SelectClipRgn(canvas, trackClip);
        for (int row = 0; row < trackCount; ++row) {
            const int top = axis.top - trackScrollOffset + row * rowHeight;
            // Every row is the same height, the last one included, so the stack reads evenly.
            const int bottom = top + rowHeight;
            if (bottom <= axis.top) {
                continue;
            }
            if (top >= axis.bottom) {
                break;
            }
            const VidTimelineTrack *track = row < static_cast<int>(displayedTracks.size())
                                                ? displayedTracks[row]
                                                : nullptr;
            const uint16_t targetId = track != nullptr ? track->targetId :
                vidTimelineTargetId(VidTimelineTarget::SPEED);
            const bool trackActive = track == nullptr || (track->enabled && attribute.video.timeline.enabled);
            const COLORREF rowColor = linkColorCycle && targetId == CYCLE_R_TARGET && trackActive
                                          ? theme.linkedTrack
                                          : trackColor(targetId, trackActive, lightMode);
            const RECT labelCell = {timeline.left + sc(8), top + sc(4), axis.left - sc(20), bottom - sc(4)};
            // The row standing in for an empty stack is on no stack to be carried anywhere.
            const int rowOrder = track != nullptr ? row : -1;
            const bool carriedRow = draggingTrackRow && trackRowDragMoved &&
                                   std::ranges::find(carriedRows, targetId) != carriedRows.end();
            if (trackRowSelected(targetId)) {
                fillRoundRect(canvas, labelCell, theme.panelRaised, theme.accent, sc(8));
            }
            if (carriedRow) {
                fillRoundRect(canvas, labelCell, theme.accentSoft, theme.accentBorder, sc(8));
            }
            const int labelMid = static_cast<int>(labelCell.top + labelCell.bottom) / 2;
            const RECT iconBox = {rowIconLeft, labelMid - sc(12), rowIconLeft + rowIconWidth,
                                  labelMid + sc(12)};
            drawTrackIcon(canvas, iconBox, targetId, rowColor);
            const float shownValue = track != nullptr
                                         ? evaluateDisplayedTrack(*track, targetId, previewDepth,
                                                                  baseValue(targetId))
                                         : baseValue(targetId);
            const std::wstring valueLabel = std::format(L"{:.3f}", shownValue);
            const int valueWidth = textWidth(canvas, valueLabel, smallFont) + sc(6);
            const int valueLeft = labelCell.right - valueWidth - sc(10);
            drawText(canvas, rowName(targetId, linkColorCycle),
                     {rowTextLeft, labelCell.top - sc(1), valueLeft - sc(12),
                       labelCell.bottom - sc(1)}, trackActive ? theme.text : theme.mutedText,
                     DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, bodyFont);
            drawText(canvas, valueLabel,
                     {valueLeft, labelCell.top, labelCell.right - sc(10),
                       labelCell.bottom}, trackActive ? theme.mutedText : theme.disabledTrack,
                     DT_RIGHT | DT_VCENTER | DT_SINGLELINE, smallFont);

            const HPEN divider = CreatePen(PS_SOLID, 1, theme.grid);
            const HGDIOBJ oldPen = SelectObject(canvas, divider);
            MoveToEx(canvas, timeline.left + sc(8), bottom, nullptr);
            LineTo(canvas, timeline.right - sc(8), bottom);
            SelectObject(canvas, oldPen);
            DeleteObject(divider);

            if (draggingTrackRow && trackRowDragMoved && rowOrder >= 0) {
                const int lastRow = static_cast<int>(reorderRowTargets.size());
                // The line stands where the carried row lands: over this row, or under the last one.
                const int dropY = trackRowDropIndex == rowOrder
                                      ? top
                                      : trackRowDropIndex == lastRow && rowOrder == lastRow - 1 ? bottom : -1;
                if (dropY >= 0) {
                    const HPEN dropPen = CreatePen(PS_SOLID, sc(2), theme.selected);
                    const HGDIOBJ oldDrop = SelectObject(canvas, dropPen);
                    MoveToEx(canvas, timeline.left + sc(8), dropY, nullptr);
                    LineTo(canvas, axis.right, dropY);
                    SelectObject(canvas, oldDrop);
                    DeleteObject(dropPen);
                }
            }

            const bool editable = editableTarget(targetId);
            float minValue = 0.0f;
            float maxValue = 1.0f;
            if (editable) {
                std::tie(minValue, maxValue) = valueRange(targetId);
            }
            trackLayouts.push_back({.targetId = targetId, .row = {axis.left, top, axis.right, bottom},
                                    .label = labelCell, .editable = editable, .minValue = minValue,
                                    .maxValue = maxValue, .order = rowOrder});
            if (editable && track != nullptr) {
                const RECT rowRect = {axis.left, top, axis.right, bottom};
                if (minValue < 0.0f && maxValue > 0.0f) {
                    const int zeroY = valueY(0.0f, minValue, maxValue, rowRect);
                    const HPEN zeroPen = CreatePen(PS_DOT, 1, theme.grid);
                    const HGDIOBJ oldZero = SelectObject(canvas, zeroPen);
                    MoveToEx(canvas, axis.left, zeroY, nullptr);
                    LineTo(canvas, axis.right, zeroY);
                    SelectObject(canvas, oldZero);
                    DeleteObject(zeroPen);
                }
                const int samples = std::max(2, static_cast<int>(axis.right - axis.left));
                const HPEN curve = CreatePen(PS_SOLID, sc(2), rowColor);
                const HGDIOBJ oldCurve = SelectObject(canvas, curve);
                for (int i = 0; i < samples; ++i) {
                    const float depth = viewStart + (viewEnd - viewStart) * static_cast<float>(i) /
                                        static_cast<float>(samples - 1);
                    const float value = evaluateDisplayedTrack(*track, targetId, depth, baseValue(targetId));
                    const int x = axis.left + i;
                    const int y = valueY(value, minValue, maxValue, rowRect);
                    if (i == 0) {
                        MoveToEx(canvas, x, y, nullptr);
                    } else {
                        LineTo(canvas, x, y);
                    }
                }
                SelectObject(canvas, oldCurve);
                DeleteObject(curve);
            }

            if (track != nullptr) {
                for (int keyIndex = 0; keyIndex < static_cast<int>(track->keys.size()); ++keyIndex) {
                    const auto &key = track->keys[keyIndex];
                    const int x = depthX(key.depth, viewStart, viewEnd, axis);
                    if (!visibleX(x, axis, sc(8))) {
                        continue;
                    }
                    const int y = editable ? valueY(key.value, minValue, maxValue,
                                                    {axis.left, top, axis.right, bottom}) : (top + bottom) / 2;
                    const bool selected = targetId == selectedTrackTarget && keyIndex == selectedTrackKey;
                    const bool hovered = targetId == hoveredTrackKey.targetId &&
                                         keyIndex == hoveredTrackKey.keyIndex;
                    const int radius = selected ? sc(7) : hovered ? sc(6) : sc(5);
                    const HBRUSH keyBrush = CreateSolidBrush(selected ? theme.selected : rowColor);
                    const HPEN keyPen = CreatePen(PS_SOLID, selected ? sc(2) : 1,
                                                  selected ? RGB(255, 237, 213) : theme.focusRing);
                    const HGDIOBJ oldBrush = SelectObject(canvas, keyBrush);
                    const HGDIOBJ oldKeyPen = SelectObject(canvas, keyPen);
                    // A keyframe is drawn as the diamond every editor draws it as.
                    const POINT diamond[4] = {{x, y - radius}, {x + radius, y}, {x, y + radius}, {x - radius, y}};
                    Polygon(canvas, diamond, 4);
                    SelectObject(canvas, oldKeyPen);
                    SelectObject(canvas, oldBrush);
                    DeleteObject(keyPen);
                    DeleteObject(keyBrush);
                }
            }
        }
        SelectClipRgn(canvas, nullptr);
        DeleteObject(trackClip);

        // The right column is the bar's own width, so it stands clear of the axis without a gutter.
        trackScrollTrack = {rightColumnLeft, axis.top, rightColumnLeft + rightColumnWidth, axis.bottom};
        fillRect(canvas, trackScrollTrack, theme.panelRaised);
        frameRect(canvas, trackScrollTrack, theme.border);
        {
            const int barHeight = static_cast<int>(trackScrollTrack.bottom - trackScrollTrack.top);
            const int thumbHeight = std::clamp(
                static_cast<int>(static_cast<float>(barHeight) * static_cast<float>(viewHeight) /
                                 static_cast<float>(std::max(contentHeight, 1))),
                std::min(sc(28), barHeight), barHeight);
            const float scrolled = trackScrollRange > 0
                                       ? static_cast<float>(trackScrollOffset) /
                                         static_cast<float>(trackScrollRange)
                                       : 0.0f;
            const int thumbTop = trackScrollTrack.top +
                                 static_cast<int>(static_cast<float>(barHeight - thumbHeight) * scrolled);
            trackScrollThumb = {trackScrollTrack.left, thumbTop, trackScrollTrack.right, thumbTop + thumbHeight};
            // With every row on screen the bar stays, filled and quiet, rather than leaving the column empty.
            const bool thumbHeld = trackScrollRange > 0 && (draggingTrackScrollThumb || hoverTrackScrollThumb);
            fillRect(canvas, trackScrollThumb,
                     trackScrollRange == 0 ? theme.buttonHover : thumbHeld ? theme.accent : theme.accentSoft);
            frameRect(canvas, trackScrollThumb, thumbHeld ? theme.accentBorder : theme.border);
        }

        const float fullSpan = std::max(startDepth - fullEndDepth, 1e-6f);
        const float shownSpan = std::min(viewStart - viewEnd, fullSpan);
        const int zoomMid = (axisBottom + guideTop) / 2;
        drawMagnifier(canvas, {rowIconLeft, zoomMid - sc(12), rowIconLeft + rowIconWidth, zoomMid + sc(12)},
                      theme.mutedText);
        drawText(canvas, L"Zoom", {rowTextLeft, axisBottom, rowTextLeft + sc(62), guideTop}, theme.text,
                 DT_LEFT | DT_VCENTER | DT_SINGLELINE, bodyFont);
        // Two pixels between the word and the box read as one crowded control rather than two.
        zoomPresetButton = {rowTextLeft + sc(66), zoomMid - sc(13), rowTextLeft + sc(148), zoomMid + sc(13)};
        drawButton(canvas, zoomPresetButton, L"", hoverZoomPreset, false, smallFont, theme);
        // Centred, the label moved every time the percentage gained or lost a digit, which read as
        // the arrow twitching while the view was zoomed. The number holds its own left edge and
        // grows to the right, and the arrow keeps the place it is drawn in whatever the number is.
        drawText(canvas, std::format(L"{:.0f}%", 100.0f * fullSpan / std::max(shownSpan, 1e-6f)),
                 {zoomPresetButton.left + sc(10), zoomPresetButton.top, zoomPresetButton.right - sc(22),
                  zoomPresetButton.bottom}, theme.text, DT_LEFT | DT_VCENTER | DT_SINGLELINE, smallFont);
        drawText(canvas, L"\x25BE", {zoomPresetButton.left, zoomPresetButton.top,
                                     zoomPresetButton.right - sc(9), zoomPresetButton.bottom},
                 theme.text, DT_RIGHT | DT_VCENTER | DT_SINGLELINE, smallFont);
        scrollTrack = {axis.left, zoomMid - scrollHeight / 2, axis.right, zoomMid + scrollHeight / 2};
        fillRect(canvas, scrollTrack, theme.panelRaised);
        frameRect(canvas, scrollTrack, theme.border);
        const int scrollWidth = static_cast<int>(scrollTrack.right - scrollTrack.left);
        const int thumbWidth = std::clamp(static_cast<int>(scrollWidth * (shownSpan / fullSpan)),
                                          std::min(sc(28), scrollWidth), scrollWidth);
        const float scrolled = fullSpan > shownSpan
                                   ? std::clamp((startDepth - viewStart) / (fullSpan - shownSpan), 0.0f, 1.0f)
                                   : 0.0f;
        const int thumbLeft = scrollTrack.left + static_cast<int>((scrollWidth - thumbWidth) * scrolled);
        scrollThumb = {thumbLeft, scrollTrack.top, thumbLeft + thumbWidth, scrollTrack.bottom};
        const bool thumbActive = draggingScrollThumb || hoverScrollThumb;
        fillRect(canvas, scrollThumb, thumbActive ? theme.accent : theme.accentSoft);
        frameRect(canvas, scrollThumb, thumbActive ? theme.accentBorder : theme.border);

        const HPEN guideDivider = CreatePen(PS_SOLID, 1, theme.grid);
        const HGDIOBJ oldGuideDivider = SelectObject(canvas, guideDivider);
        MoveToEx(canvas, timeline.left + sc(8), guideTop, nullptr);
        LineTo(canvas, timeline.right - sc(8), guideTop);
        SelectObject(canvas, oldGuideDivider);
        DeleteObject(guideDivider);
        RECT guidance = {rowIconLeft, guideTop + sc(7), timeline.right - rowRightPad,
                         timeline.bottom - sc(7)};
        const std::wstring guideTitle = L"Controls:";
        drawText(canvas, guideTitle, guidance, theme.accent, DT_LEFT | DT_VCENTER | DT_SINGLELINE, smallFont);
        guidance.left += textWidth(canvas, guideTitle, smallFont) + sc(10);
        const VidTimelineTrack *selectedTrack = track(selectedTrackTarget);
        if (selectedTrack != nullptr && selectedTrackKey >= 0 &&
            selectedTrackKey < static_cast<int>(selectedTrack->keys.size())) {
            const auto &key = selectedTrack->keys[selectedTrackKey];
            const std::wstring unit = selectedTrackTarget == SPEED_TARGET ? L" kf/s" : L"";
            drawText(canvas, std::format(L"{}  Key {}    Distance d {:.3f} (kf {:.1f})    {:.4f}{}    {}    |    Drag an empty area: scrub preview",
                                         rowName(selectedTrackTarget, linkColorCycle), selectedTrackKey + 1,
                                         displayDistance(key.depth), key.depth, key.value, unit,
                                         interpolationName(key.out)),
                     guidance, theme.text, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, smallFont);
        } else {
            drawText(canvas, L"Wheel: zoom    Shift+Wheel: scroll    Ctrl+Wheel: tracks    0: fit    Right-click: parameter panels and keys    Double-click a track: add key    Delete: remove key",
                     guidance, theme.mutedText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, smallFont);
        }

        const int playheadX = depthX(previewDepth, viewStart, viewEnd, axis);
        if (visibleX(playheadX, axis, 0)) {
            const HPEN playhead = CreatePen(PS_SOLID, sc(2), theme.accent);
            const HGDIOBJ oldPlayhead = SelectObject(canvas, playhead);
            MoveToEx(canvas, playheadX, timeRowTop, nullptr);
            LineTo(canvas, playheadX, axis.bottom);
            SelectObject(canvas, oldPlayhead);
            DeleteObject(playhead);
            // The head of the playhead, so where the preview stands is visible at a glance.
            const HBRUSH headBrush = CreateSolidBrush(theme.accent);
            const HPEN headPen = CreatePen(PS_SOLID, 1, theme.accentText);
            const HGDIOBJ oldHeadBrush = SelectObject(canvas, headBrush);
            const HGDIOBJ oldHeadPen = SelectObject(canvas, headPen);
            const POINT head[3] = {{playheadX - sc(7), timeRowTop - sc(9)}, {playheadX + sc(7), timeRowTop - sc(9)},
                                   {playheadX, timeRowTop + sc(1)}};
            Polygon(canvas, head, 3);
            SelectObject(canvas, oldHeadPen);
            SelectObject(canvas, oldHeadBrush);
            DeleteObject(headPen);
            DeleteObject(headBrush);
        }

        for (const auto &hold: attribute.video.timeline.holds) {
            const int x = depthX(hold.depth, viewStart, viewEnd, axis);
            if (!visibleX(x, axis, 0)) {
                continue;
            }
            const HPEN holdPen = CreatePen(PS_DOT, 1, theme.hold);
            const HGDIOBJ oldHold = SelectObject(canvas, holdPen);
            MoveToEx(canvas, x, axis.top, nullptr);
            LineTo(canvas, x, axis.bottom);
            SelectObject(canvas, oldHold);
            DeleteObject(holdPen);
        }

        BitBlt(target, 0, 0, width, height, canvas, 0, 0, SRCCOPY);
        SelectObject(canvas, previousBitmap);
        DeleteObject(bitmap);
        DeleteDC(canvas);
        if (fieldEdit != nullptr && activeEditRect.right > activeEditRect.left) {
            SetWindowPos(fieldEdit, nullptr, activeEditRect.left, activeEditRect.top,
                         activeEditRect.right - activeEditRect.left, activeEditRect.bottom - activeEditRect.top,
                         SWP_NOACTIVATE | SWP_NOZORDER);
        }
        if (fieldTooltip != nullptr) {
            const RECT tooltipRects[2] = {distanceField, keyframeField};
            for (UINT_PTR id = 1; id <= 2; ++id) {
                TOOLINFOW tool = {};
                tool.cbSize = sizeof(tool);
                tool.hwnd = window;
                tool.uId = id;
                tool.rect = tooltipRects[id - 1];
                SendMessageW(fieldTooltip, TTM_NEWTOOLRECTW, 0, reinterpret_cast<LPARAM>(&tool));
            }
        }
    }

    LRESULT TimelineWindow::fieldEditProc(const HWND hwnd, const UINT message, const WPARAM wParam,
                                          const LPARAM lParam, [[maybe_unused]] const UINT_PTR subclassId,
                                          const DWORD_PTR referenceData) {
        auto *self = reinterpret_cast<TimelineWindow *>(referenceData);
        if (self == nullptr) {
            return DefSubclassProc(hwnd, message, wParam, lParam);
        }
        if (message == WM_GETDLGCODE) {
            return DLGC_WANTALLKEYS;
        }
        if (message == WM_KEYDOWN) {
            if (wParam == VK_RETURN) {
                if (self->commitFieldEdit()) {
                    SetFocus(self->window);
                }
                return 0;
            }
            if (wParam == VK_ESCAPE) {
                self->closeFieldEdit();
                SetFocus(self->window);
                return 0;
            }
        }
        if (message == WM_CHAR && (wParam == VK_RETURN || wParam == L'\n' || wParam == VK_ESCAPE)) {
            return 0;
        }
        if (message == WM_KILLFOCUS && self->fieldEdit == hwnd) {
            if (!self->commitFieldEdit()) {
                self->closeFieldEdit();
            }
            return 0;
        }
        return DefSubclassProc(hwnd, message, wParam, lParam);
    }

    LRESULT TimelineWindow::windowProc(const HWND hwnd, const UINT message, const WPARAM wParam,
                                       const LPARAM lParam) {
        // A throw cannot cross back over the Win32 callback boundary, so what a click ran into is
        // answered here and the editor stays open rather than the program ending on it.
        try {
            return handleMessage(hwnd, message, wParam, lParam);
        } catch (const std::exception &e) {
            MessageBoxA(hwnd, e.what(), "Timeline Editor", MB_OK | MB_ICONERROR);
        } catch (...) {
            MessageBoxW(hwnd, L"The Timeline Editor ran into an unexpected error.", L"Timeline Editor",
                        MB_OK | MB_ICONERROR);
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    LRESULT TimelineWindow::handleMessage(const HWND hwnd, const UINT message, const WPARAM wParam,
                                          const LPARAM lParam) {
        TimelineWindow *self = reinterpret_cast<TimelineWindow *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            const auto *create = reinterpret_cast<CREATESTRUCTW *>(lParam);
            self = static_cast<TimelineWindow *>(create->lpCreateParams);
            self->window = hwnd;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (self == nullptr) {
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }

        switch (message) {
            case WM_GETMINMAXINFO: {
                if (self->fullscreen) {
                    break;
                }
                auto *info = reinterpret_cast<MINMAXINFO *>(lParam);
                // The prefilled maximize box follows the primary monitor, which runs the panel bars off the screen on any other one.
                MONITORINFO monitor = {};
                monitor.cbSize = sizeof(MONITORINFO);
                if (GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor)) {
                    info->ptMaxPosition.x = monitor.rcWork.left - monitor.rcMonitor.left;
                    info->ptMaxPosition.y = monitor.rcWork.top - monitor.rcMonitor.top;
                    info->ptMaxSize.x = monitor.rcWork.right - monitor.rcWork.left;
                    info->ptMaxSize.y = monitor.rcWork.bottom - monitor.rcWork.top;
                    info->ptMaxTrackSize.x = std::max(info->ptMaxTrackSize.x, info->ptMaxSize.x);
                    info->ptMaxTrackSize.y = std::max(info->ptMaxTrackSize.y, info->ptMaxSize.y);
                }
                // A screen smaller than the design minimum keeps the window inside it rather than hanging it off the edge.
                info->ptMinTrackSize.x = std::min<LONG>(sc(760), info->ptMaxSize.x);
                info->ptMinTrackSize.y = std::min<LONG>(sc(560), info->ptMaxSize.y);
                return 0;
            }
            case WM_ERASEBKGND:
                return 1;
            case WM_SIZE:
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            case WM_CTLCOLOREDIT:
                if (reinterpret_cast<HWND>(lParam) == self->fieldEdit) {
                    const TimelineTheme &theme = timelineTheme(self->lightMode);
                    const HDC editDc = reinterpret_cast<HDC>(wParam);
                    SetTextColor(editDc, theme.text);
                    SetBkColor(editDc, theme.panelRaised);
                    return reinterpret_cast<LRESULT>(self->fieldEditBrush);
                }
                break;
            case WM_TIMELINE_PREVIEW_READY:
                self->previewPending = false;
                self->previewBusy = false;
                KillTimer(hwnd, PREVIEW_STATUS_TIMER);
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            case WM_TIMELINE_EXPORT_FINISHED:
                if (reinterpret_cast<TimelineWindow *>(wParam) != self) {
                    return 0;
                }
                self->exporting = false;
                (void) self->initializeFramePreview();
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            case RenderSceneRequests::WM_SHADER_EDITED:
                self->recordShaderEdits();
                return 0;
            case WM_SETCURSOR: {
                if (LOWORD(lParam) == HTCLIENT) {
                    POINT point = {};
                    GetCursorPos(&point);
                    ScreenToClient(hwnd, &point);
                    const bool interactive = contains(self->framesButton, point) ||
                                              contains(self->loadButton, point) || contains(self->saveButton, point) ||
                                              contains(self->exportButton, point) ||
                                              contains(self->themeButton, point) ||
                                              contains(self->fullscreenButton, point) ||
                                             self->hitTrackKey(point).valid() ||
                                             self->hitTrackLabel(point) != UINT16_MAX;
                    // The readouts that carry the playhead are dragged sideways, and say so.
                    const bool slider = contains(self->distanceField, point) ||
                                        contains(self->keyframeField, point) || contains(self->timeField, point);
                    const int cursor = slider ? 32644 : interactive ? 32649
                                           : contains(self->timelineAxis, point) ? 32515 : 32512;
                    SetCursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(cursor)));
                    return TRUE;
                }
                break;
            }
            case WM_MOUSEMOVE: {
                const POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                TRACKMOUSEEVENT tracking = {sizeof(tracking), TME_LEAVE, hwnd, 0};
                TrackMouseEvent(&tracking);
                if (self->draggingTrackRow) {
                    // A press that has not left the row yet is a click on the name, not a carry.
                    if (!self->trackRowDragMoved && std::abs(point.y - self->trackRowDragOriginY) < sc(4)) {
                        return 0;
                    }
                    self->trackRowDragMoved = true;
                    self->trackRowDropIndex = self->trackRowDropTarget(point);
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
                if (self->draggingTrackKey) {
                    self->updateTrackKey(point);
                    return 0;
                }
                if (self->scrubbingTimeline) {
                    self->updateScrubDepth(point);
                    return 0;
                }
                if (self->fieldDrag != FieldDrag::NONE) {
                    self->updateFieldDrag(point);
                    return 0;
                }
                if (self->draggingScrollThumb) {
                    self->updateScrollThumb(point);
                    return 0;
                }
                if (self->draggingTrackScrollThumb) {
                    self->updateTrackScrollThumb(point);
                    return 0;
                }
                if (self->draggingRuler) {
                    self->panView(self->rulerGrabDepth - self->viewDepthAt(point.x));
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
                const KeyHit hoveredKey = self->hitTrackKey(point);
                const bool frames = contains(self->framesButton, point);
                const bool load = contains(self->loadButton, point);
                const bool save = contains(self->saveButton, point);
                const bool exportVideo = contains(self->exportButton, point);
                const bool thumb = contains(self->scrollThumb, point);
                const bool theme = contains(self->themeButton, point);
                const bool trackThumb = contains(self->trackScrollThumb, point);
                const bool full = contains(self->fullscreenButton, point);
                const bool play = contains(self->playButton, point);
                const bool pause = contains(self->pauseButton, point);
                const bool stop = contains(self->stopButton, point);
                const bool loop = contains(self->loopButton, point);
                const bool zoomPreset = contains(self->zoomPresetButton, point);
                const FieldEdit formulaField = contains(self->distanceField, point) ? FieldEdit::DISTANCE
                                               : contains(self->keyframeField, point) ? FieldEdit::KEYFRAME
                                                                                     : FieldEdit::NONE;
                if (hoveredKey.targetId != self->hoveredTrackKey.targetId ||
                    hoveredKey.keyIndex != self->hoveredTrackKey.keyIndex ||
                    frames != self->hoverFrames || load != self->hoverLoad || save != self->hoverSave ||
                    exportVideo != self->hoverExport ||
                    thumb != self->hoverScrollThumb || theme != self->hoverTheme ||
                    trackThumb != self->hoverTrackScrollThumb || full != self->hoverFullscreen ||
                    play != self->hoverPlay || pause != self->hoverPause || stop != self->hoverStop ||
                    loop != self->hoverLoop || zoomPreset != self->hoverZoomPreset ||
                    formulaField != self->hoveredFieldEdit) {
                    self->hoveredTrackKey = hoveredKey;
                    self->hoverFrames = frames;
                    self->hoverLoad = load;
                    self->hoverSave = save;
                    self->hoverExport = exportVideo;
                    self->hoverScrollThumb = thumb;
                    self->hoverTheme = theme;
                    self->hoverTrackScrollThumb = trackThumb;
                    self->hoverFullscreen = full;
                    self->hoverPlay = play;
                    self->hoverPause = pause;
                    self->hoverStop = stop;
                    self->hoverLoop = loop;
                    self->hoverZoomPreset = zoomPreset;
                    self->hoveredFieldEdit = formulaField;
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                return 0;
            }
            case WM_MOUSELEAVE:
                if (self->hoveredFieldEdit != FieldEdit::NONE) {
                    self->hoveredFieldEdit = FieldEdit::NONE;
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                return 0;
            case WM_MOUSEWHEEL: {
                POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                ScreenToClient(hwnd, &point);
                if (!contains(self->timelinePanel, point) || GET_WHEEL_DELTA_WPARAM(wParam) == 0) {
                    break;
                }
                const float steps = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
                // Over the track names, and with Ctrl held anywhere on the panel, the wheel moves the rows.
                if ((GetKeyState(VK_CONTROL) & 0x8000) != 0 || point.x < self->timelineAxis.left) {
                    self->scrollTracks(static_cast<int>(-steps * static_cast<float>(sc(38))));
                } else if ((GetKeyState(VK_SHIFT) & 0x8000) != 0) {
                    self->panView(self->viewSpan() * 0.25f * steps);
                } else {
                    self->zoomView(self->viewDepthAt(point.x), std::pow(1.3f, steps));
                }
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            case WM_LBUTTONDOWN: {
                SetFocus(hwnd);
                const POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                if (contains(self->framesButton, point)) {
                    self->loadKeyframeDirectory();
                    return 0;
                }
                if (contains(self->loadButton, point)) {
                    self->loadTimeline();
                    return 0;
                }
                if (contains(self->saveButton, point)) {
                    self->saveTimeline();
                    return 0;
                }
                if (contains(self->exportButton, point)) {
                    self->openExportMenu();
                    return 0;
                }
                if (contains(self->themeButton, point)) {
                    self->toggleTheme();
                    return 0;
                }
                if (contains(self->fullscreenButton, point)) {
                    self->toggleFullscreen();
                    return 0;
                }
                if (contains(self->playButton, point)) {
                    self->setPlaying(!self->playing);
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
                if (contains(self->pauseButton, point)) {
                    self->setPlaying(false);
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
                if (contains(self->stopButton, point)) {
                    self->stopPlayback();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
                if (contains(self->loopButton, point)) {
                    self->loopPlayback = !self->loopPlayback;
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
                if (contains(self->zoomPresetButton, point)) {
                    self->openZoomMenu();
                    return 0;
                }
                // The distance, keyframe and time readouts scrub the preview when they are dragged sideways.
                if (contains(self->distanceField, point) || contains(self->keyframeField, point) ||
                    contains(self->timeField, point)) {
                    self->fieldDrag = contains(self->timeField, point) ? FieldDrag::TIME : FieldDrag::DEPTH;
                    self->pendingFieldEdit = contains(self->distanceField, point) ? FieldEdit::DISTANCE
                                             : contains(self->keyframeField, point) ? FieldEdit::KEYFRAME
                                                                                   : FieldEdit::NONE;
                    self->fieldDragMoved = false;
                    self->fieldDragOriginX = point.x;
                    self->fieldDragDepth = self->previewDepth;
                    SetCapture(hwnd);
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
                if (contains(self->trackScrollThumb, point)) {
                    self->draggingTrackScrollThumb = true;
                    self->trackScrollGrabOffset = point.y - static_cast<int>(self->trackScrollThumb.top);
                    SetCapture(hwnd);
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
                if (contains(self->trackScrollTrack, point)) {
                    self->pageScrollTracks(point);
                    return 0;
                }
                if (contains(self->scrollThumb, point)) {
                    self->draggingScrollThumb = true;
                    self->scrollGrabOffset = point.x - static_cast<int>(self->scrollThumb.left);
                    SetCapture(hwnd);
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
                if (contains(self->scrollTrack, point)) {
                    self->pageScrollView(point);
                    return 0;
                }
                // The ruler drags the view sideways, the way the depth axis itself would be grabbed.
                if (contains(self->rulerStrip, point)) {
                    self->draggingRuler = true;
                    self->rulerGrabDepth = self->viewDepthAt(point.x);
                    SetCapture(hwnd);
                    return 0;
                }
                // A row is selected from its name as readily as from one of its keys.
                if (const uint16_t labelTarget = self->hitTrackLabel(point); labelTarget != UINT16_MAX) {
                    // Ctrl adds the row to the ones picked or drops it, Shift takes the run up to it.
                    self->selectTrackRow(labelTarget, (GetKeyState(VK_CONTROL) & 0x8000) != 0,
                                         (GetKeyState(VK_SHIFT) & 0x8000) != 0);
                    // The name cell is the handle the rows picked are carried by, all of them at once.
                    if (self->trackRowSelected(labelTarget)) {
                        self->carriedRows.clear();
                        for (const uint16_t id: self->reorderRowTargets) {
                            if (self->trackRowSelected(id)) {
                                self->carriedRows.push_back(id);
                            }
                        }
                        self->draggingTrackRow = true;
                        self->trackRowDragMoved = false;
                        self->trackRowDragOriginY = point.y;
                        self->trackRowDropIndex = -1;
                        SetCapture(hwnd);
                        // Carried past an end of the stack, the rows go on to the ones below it.
                        SetTimer(hwnd, EDGE_SCROLL_TIMER, EDGE_SCROLL_INTERVAL, nullptr);
                    }
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
                const KeyHit hit = self->hitTrackKey(point);
                if (hit.valid()) {
                    self->selectedTrackTarget = hit.targetId;
                    self->selectedTrackKey = hit.keyIndex;
                    if (const VidTimelineTrack *current = self->track(hit.targetId); current != nullptr) {
                        self->previewDepth = current->keys[hit.keyIndex].depth;
                        self->syncPlaybackClock();
                    }
                    self->draggingTrackKey = true;
                    if (const TrackLayout *item = self->layout(hit.targetId); item != nullptr) {
                        self->dragValueMin = item->minValue;
                        self->dragValueMax = item->maxValue;
                    }
                    SetCapture(hwnd);
                } else if (contains(self->timelineAxis, point)) {
                    self->scrubbingTimeline = true;
                    self->updateScrubDepth(point);
                    SetCapture(hwnd);
                    // Held at an end of the axis the playhead goes on moving, and the view with it.
                    SetTimer(hwnd, EDGE_SCROLL_TIMER, EDGE_SCROLL_INTERVAL, nullptr);
                }
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            case WM_LBUTTONDBLCLK: {
                const POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                const KeyHit hit = self->hitTrackKey(point);
                if (self->draggingTrackKey) {
                    self->draggingTrackKey = false;
                    if (GetCapture() == hwnd) {
                        ReleaseCapture();
                    }
                }
                if (self->scrubbingTimeline) {
                    self->scrubbingTimeline = false;
                    KillTimer(hwnd, EDGE_SCROLL_TIMER);
                    if (GetCapture() == hwnd) {
                        ReleaseCapture();
                    }
                }
                if (self->draggingTrackRow) {
                    self->draggingTrackRow = false;
                    self->trackRowDragMoved = false;
                    self->trackRowDropIndex = -1;
                    self->carriedRows.clear();
                    KillTimer(hwnd, EDGE_SCROLL_TIMER);
                    if (GetCapture() == hwnd) {
                        ReleaseCapture();
                    }
                }
                if (hit.valid()) {
                    self->selectedTrackTarget = hit.targetId;
                    self->selectedTrackKey = hit.keyIndex;
                } else {
                    const uint16_t targetId = self->hitTrackRow(point);
                    if (targetId == UINT16_MAX) {
                        return 0;
                    }
                    self->addTrackKey(targetId, point);
                }
                self->openTrackKeyEditor();
                return 0;
            }
            case WM_LBUTTONUP:
                if (self->draggingTrackRow) {
                    const std::vector<uint16_t> carried = self->carriedRows;
                    const int dropIndex = self->trackRowDropIndex;
                    const bool moved = self->trackRowDragMoved;
                    self->draggingTrackRow = false;
                    self->trackRowDragMoved = false;
                    self->trackRowDropIndex = -1;
                    self->carriedRows.clear();
                    KillTimer(hwnd, EDGE_SCROLL_TIMER);
                    if (GetCapture() == hwnd) {
                        ReleaseCapture();
                    }
                    if (moved) {
                        self->moveTrackRows(carried, dropIndex);
                    }
                    InvalidateRect(hwnd, nullptr, FALSE);
                } else if (self->fieldDrag != FieldDrag::NONE) {
                    const FieldEdit clickedField = self->pendingFieldEdit;
                    const bool dragged = self->fieldDragMoved;
                    self->fieldDrag = FieldDrag::NONE;
                    self->pendingFieldEdit = FieldEdit::NONE;
                    self->fieldDragMoved = false;
                    if (GetCapture() == hwnd) {
                        ReleaseCapture();
                    }
                    if (!dragged && clickedField != FieldEdit::NONE) {
                        self->beginFieldEdit(clickedField);
                    } else if (dragged) {
                        self->requestFramePreview();
                    }
                    InvalidateRect(hwnd, nullptr, FALSE);
                } else if (self->draggingRuler) {
                    self->draggingRuler = false;
                    if (GetCapture() == hwnd) {
                        ReleaseCapture();
                    }
                    InvalidateRect(hwnd, nullptr, FALSE);
                } else if (self->draggingTrackScrollThumb) {
                    self->draggingTrackScrollThumb = false;
                    if (GetCapture() == hwnd) {
                        ReleaseCapture();
                    }
                    InvalidateRect(hwnd, nullptr, FALSE);
                } else if (self->draggingScrollThumb) {
                    self->draggingScrollThumb = false;
                    if (GetCapture() == hwnd) {
                        ReleaseCapture();
                    }
                    InvalidateRect(hwnd, nullptr, FALSE);
                } else if (self->draggingTrackKey) {
                    self->draggingTrackKey = false;
                    if (GetCapture() == hwnd) {
                        ReleaseCapture();
                    }
                    self->requestFramePreview();
                    InvalidateRect(hwnd, nullptr, FALSE);
                } else if (self->scrubbingTimeline) {
                    self->scrubbingTimeline = false;
                    KillTimer(hwnd, EDGE_SCROLL_TIMER);
                    if (GetCapture() == hwnd) {
                        ReleaseCapture();
                    }
                    self->requestFramePreview();
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                return 0;
            case WM_CAPTURECHANGED:
                self->fieldDrag = FieldDrag::NONE;
                self->pendingFieldEdit = FieldEdit::NONE;
                self->fieldDragMoved = false;
                self->draggingRuler = false;
                self->draggingTrackKey = false;
                self->draggingTrackRow = false;
                self->trackRowDragMoved = false;
                self->trackRowDropIndex = -1;
                self->carriedRows.clear();
                self->scrubbingTimeline = false;
                KillTimer(hwnd, EDGE_SCROLL_TIMER);
                self->draggingScrollThumb = false;
                self->draggingTrackScrollThumb = false;
                return 0;
            case WM_RBUTTONDOWN: {
                const POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                if (point.y >= self->timelineAxis.top && point.y <= self->timelineAxis.bottom &&
                    point.x <= self->timelineAxis.right) {
                    self->openTrackMenu(point);
                }
                return 0;
            }
            case WM_KEYDOWN:
                switch (wParam) {
                    case VK_DELETE:
                    case VK_BACK:
                        // A key is what Delete takes while one is picked. With the row picked and no
                        // key on it, what Delete takes is the parameter the row stands for.
                        if (self->selectedTrackKey >= 0) {
                            self->deleteTrackKey();
                        } else {
                            self->removeTrack(self->selectedTrackTarget);
                        }
                        return 0;
                    case VK_RETURN:
                    case VK_F2:
                        self->openTrackKeyEditor();
                        return 0;
                    case VK_F11:
                        self->toggleFullscreen();
                        return 0;
                    case VK_SPACE:
                        self->setPlaying(!self->playing);
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    case VK_ESCAPE:
                        if (self->fullscreen) {
                            self->toggleFullscreen();
                            return 0;
                        }
                        break;
                    case VK_HOME:
                        self->previewDepth = self->schedule.getStartDepth();
                        self->syncPlaybackClock();
                        self->requestFramePreview();
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    case VK_END:
                        self->previewDepth = self->schedule.getEndDepth();
                        self->syncPlaybackClock();
                        self->requestFramePreview();
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    case 'Z':
                        if (GetKeyState(VK_CONTROL) & 0x8000) {
                            self->undoTimeline();
                            return 0;
                        }
                        break;
                    case 'Y':
                        if (GetKeyState(VK_CONTROL) & 0x8000) {
                            self->redoTimeline();
                            return 0;
                        }
                        break;
                    case '0':
                        self->resetView();
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    case VK_OEM_PLUS:
                    case VK_ADD:
                        self->zoomView(std::clamp(self->previewDepth, self->viewEndDepth, self->viewStartDepth),
                                       1.5f);
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    case VK_OEM_MINUS:
                    case VK_SUBTRACT:
                        self->zoomView(std::clamp(self->previewDepth, self->viewEndDepth, self->viewStartDepth),
                                       1.0f / 1.5f);
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    case VK_LEFT:
                        self->panView(self->viewSpan() * 0.1f);
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    case VK_RIGHT:
                        self->panView(-self->viewSpan() * 0.1f);
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    case VK_UP:
                        self->scrollTracks(-sc(38));
                        return 0;
                    case VK_DOWN:
                        self->scrollTracks(sc(38));
                        return 0;
                    case VK_PRIOR:
                        self->scrollTracks(-sc(220));
                        return 0;
                    case VK_NEXT:
                        self->scrollTracks(sc(220));
                        return 0;
                    case '1':
                        self->setTrackInterpolation(VidKeyInterpolation::STEP);
                        return 0;
                    case '2':
                        self->setTrackInterpolation(VidKeyInterpolation::LINEAR);
                        return 0;
                    case '3':
                        self->setTrackInterpolation(VidKeyInterpolation::SMOOTH);
                        return 0;
                    case '4':
                        self->setTrackInterpolation(VidKeyInterpolation::CUBIC);
                        return 0;
                    default:
                        break;
                }
                break;
            case WM_TIMER:
                if (wParam == PLAYBACK_TIMER) {
                    self->advancePlayback();
                    return 0;
                }
                if (wParam == EDGE_SCROLL_TIMER) {
                    if (!self->scrubbingTimeline && !self->draggingTrackRow) {
                        KillTimer(hwnd, EDGE_SCROLL_TIMER);
                        return 0;
                    }
                    POINT point = {};
                    GetCursorPos(&point);
                    ScreenToClient(hwnd, &point);
                    if (self->draggingTrackRow) {
                        if (self->trackRowDragMoved && self->rowEdgeScroll(point)) {
                            self->trackRowDropIndex = self->trackRowDropTarget(point);
                            InvalidateRect(hwnd, nullptr, FALSE);
                        }
                        return 0;
                    }
                    // The playhead follows the view it has just pulled along, and stops where it does.
                    if (self->scrubEdgeScroll(point)) {
                        self->updateScrubDepth(point);
                    }
                    return 0;
                }
                if (wParam == PREVIEW_STATUS_TIMER) {
                    KillTimer(hwnd, PREVIEW_STATUS_TIMER);
                    if (self->previewPending) {
                        self->previewBusy = true;
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                    return 0;
                }
                break;
            case WM_PAINT: {
                PAINTSTRUCT ps = {};
                const HDC hdc = BeginPaint(hwnd, &ps);
                RECT client = {};
                GetClientRect(hwnd, &client);
                self->paint(hdc, client);
                EndPaint(hwnd, &ps);
                return 0;
            }
            case WM_CLOSE:
                DestroyWindow(hwnd);
                return 0;
            case WM_NCDESTROY:
                // The shader panels report to this window; nothing may be posted to it after here.
                if (self->renderScene != nullptr) {
                    HWND listening = hwnd;
                    self->renderScene->getRequests().shaderEditListener.compare_exchange_strong(
                        listening, nullptr);
                }
                self->stopFramePreviewWorker();
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
                self->window = nullptr;
                delete self;
                return DefWindowProcW(hwnd, message, wParam, lParam);
            default:
                return DefWindowProcW(hwnd, message, wParam, lParam);
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}
