//
// Modified by GPT-5 on 2026-08-18, 2026-08-23, 2026-08-24, 2026-08-26, 2026-08-27, 2026-08-31
// Modified by Opus 5 on 2026-08-19, 2026-08-20, 2026-08-21, 2026-08-22, 2026-08-23, 2026-08-25, 2026-08-26, 2026-08-31, 2026-09-01, 2026-09-03
// Modified by GPT-6 on 2026-09-08, 2026-09-14, 2026-09-15, 2026-09-18, 2026-09-19, 2026-09-20, 2026-09-21, 2026-09-22, 2026-09-23, 2026-09-24, 2026-09-25, 2026-09-26
//

#include "UiLanguage.hpp"
#include "NativeDialogs.hpp"
#include "TimelineWindow.hpp"
#include "../video/ZoomOverlay.hpp"
#include "workspace/TimelineTransportLayout.hpp"
#include "UiDpi.hpp"
#include "SettingsTheme.hpp"
#include "Utilities.h"
#include "workspace/AccessibleControl.hpp"
#include "workspace/AttributeFormModel.hpp"
#include "workspace/FormWorkspace.hpp"
#include "../video/VideoCameraSource.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <cwchar>
#include <format>
#include <limits>
#include <ranges>
#include <string>
#include <sstream>
#include <tuple>
#include <unordered_set>
#include <vector>
#include <commctrl.h>
#include <windowsx.h>

#include "../attr/VidTimelineTarget.h"
#include "../constants/Constants.hpp"
#include "../io/PreferencesIO.h"
#include "../io/TimelineIO.h"
#include "../io/TimelineJsonIO.hpp"
#include "../io/AudioTimelineIO.hpp"
#include "../video/AudioClipEdit.hpp"
#include "../video/TimelineTime.hpp"
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
        constexpr UINT WM_TIMELINE_CACHE_PROGRESS = WM_APP + 0x25F;
        constexpr UINT_PTR PLAYBACK_TIMER = 1;
        constexpr UINT_PTR PREVIEW_STATUS_TIMER = 2;
        constexpr UINT_PTR EDGE_SCROLL_TIMER = 3;
        constexpr int WORKSPACE_DOCK_TOGGLE = 4700;
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
            COLORREF selectedText;
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
        };

        constexpr TimelineTheme timelineThemeFrom(const SettingsThemeColors &shared, const bool light) {
            return {
                .background = shared.background,
                .panel = shared.background,
                .panelRaised = shared.buttonFace,
                .previewBackground = shared.background,
                .border = shared.sectionFrame,
                .grid = shared.sectionFrame,
                .text = shared.text,
                .mutedText = shared.rangeText,
                .accent = shared.primaryButton,
                .accentHover = shared.primaryButtonPressed,
                .accentPressed = shared.primaryButtonPressed,
                .accentSoft = shared.radioSelectedBackground,
                .selectedText = shared.radioSelectedText,
                .accentBorder = shared.radioSelectedBorder,
                .focusRing = shared.cardNoteAccent,
                .accentText = shared.cardNoteAccent,
                .activeText = shared.primaryButtonText,
                .buttonHoverBorder = shared.textFieldBorder,
                .hold = light ? RGB(217, 119, 6) : RGB(245, 158, 11),
                .selected = light ? RGB(234, 88, 12) : RGB(249, 115, 22),
                .buttonHover = shared.buttonFacePressed,
                .toggleOff = shared.sliderTrack,
                .disabledTrack = shared.textDisabled,
                .linkedTrack = shared.text,
            };
        }
        TimelineTheme timelineTheme(const bool lightMode) {
            auto result = timelineThemeFrom(settingsTheme(!lightMode), lightMode);
            if (highContrastSettingsMode()) {
                result.hold = result.text;
                result.selected = result.accentText;
            }
            return result;
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
            ShaderPanel{L"Camera / Rotation / 360\u00B0", &CallbackVideo::CAMERA_SETTINGS, L"Camera"},
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
        constexpr uint16_t COLOR_ANIMATION_TARGET =
            vidTimelineTargetId(VidTimelineTarget::PALETTE_ANIMATION_SPEED);
        constexpr uint16_t FOG_OPACITY_TARGET = vidTimelineTargetId(VidTimelineTarget::FOG_OPACITY);
        constexpr uint16_t CYCLE_R_TARGET = vidTimelineTargetId(VidTimelineTarget::PALETTE_INTERVAL_R);
        constexpr uint16_t CYCLE_G_TARGET = vidTimelineTargetId(VidTimelineTarget::PALETTE_INTERVAL_G);
        constexpr uint16_t CYCLE_B_TARGET = vidTimelineTargetId(VidTimelineTarget::PALETTE_INTERVAL_B);
        constexpr wchar_t FORMULA_FIELD_HINT[] =
            L"Click: enter a value or formula (+ \u2212 \u00d7 \u00f7 parentheses) \u00b7 Drag: scrub";

        thread_local UINT timelineDpi = 0;
        struct TimelineDpiScope {
            UINT previous = timelineDpi;
            explicit TimelineDpiScope(UINT dpi) {
                timelineDpi = dpi;
            }
            ~TimelineDpiScope() {
                timelineDpi = previous;
            }
        };
        int sc(const int value) {
            if (value <= 0) {
                return value;
            }
            return timelineDpi
                       ? std::max(
                             1,
                             int(value * Constants::Win32::SETTINGS_UI_BASE_SCALE * timelineDpi / 96.0 + .5))
                       : Constants::Win32::settingsScaled(value);
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

        void drawText(const HDC hdc, const std::wstring &text, RECT rect, const COLORREF color,
                      const UINT format, const HFONT font) {
            const HGDIOBJ previous = SelectObject(hdc, font);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, color);
            try {
                UiLanguage::drawText(hdc, text.c_str(), -1, &rect, format);
            } catch (...) {
                SelectObject(hdc, previous);
                throw;
            }
            SelectObject(hdc, previous);
        }

        std::wstring durationText(const double value) {
            return TimelineTime::display(value);
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
            const float nice = normalized <= 1.0f   ? 1.0f
                               : normalized <= 2.0f ? 2.0f
                               : normalized <= 5.0f ? 5.0f
                                                    : 10.0f;
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

        constexpr uint16_t AUDIO_ROW_TARGET = UINT16_MAX - 1;
        // The R row stands for all three channels while they are linked.
        std::wstring rowName(const uint16_t targetId, const bool linkedRgb) {
            if (targetId == AUDIO_ROW_TARGET) return L"Audio";
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
            return param != nullptr &&
                           (param->kind == TimelineParamKind::BOOL || param->kind == TimelineParamKind::ENUM)
                       ? VidKeyInterpolation::STEP
                       : VidKeyInterpolation::SMOOTH;
        }

        float evaluateDisplayedTrack(const VidTimelineTrack &track, const uint16_t targetId,
                                     const float depth, const float fallback) {
            if (const TimelineParamDesc *param = TimelineParams::find(targetId); param != nullptr) {
                return TimelineSchedule::evaluateTrack(track, depth, fallback, param->minValue,
                                                       param->maxValue);
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

        bool colorCycleTracksLinked(const VidTimelineAttribute &timeline, const bool linkWhenAbsent) {
            const auto findTrack = [&timeline](const uint16_t targetId) -> const VidTimelineTrack * {
                const auto found = std::ranges::find(timeline.tracks, targetId, &VidTimelineTrack::targetId);
                return found == timeline.tracks.end() ? nullptr : &*found;
            };
            const VidTimelineTrack *red = findTrack(CYCLE_R_TARGET);
            const VidTimelineTrack *green = findTrack(CYCLE_G_TARGET);
            const VidTimelineTrack *blue = findTrack(CYCLE_B_TARGET);
            if (red == nullptr && green == nullptr && blue == nullptr) {
                return linkWhenAbsent;
            }
            return red != nullptr && green != nullptr && blue != nullptr &&
                   red->enabled == green->enabled && red->enabled == blue->enabled &&
                   sameKeys(red->keys, green->keys) && sameKeys(red->keys, blue->keys);
        }

        COLORREF trackColor(const uint16_t targetId, const bool active, const bool lightMode) {
            if (highContrastSettingsMode()) {
                return active ? timelineTheme(lightMode).text : timelineTheme(lightMode).disabledTrack;
            }
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
                return timelineTheme(lightMode).accentText;
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
                return timelineTheme(lightMode).accentText;
            }
        }

        int valueY(const float value, const float minValue, const float maxValue, const RECT &row) {
            const float span = std::max(maxValue - minValue, 1e-6f);
            const float ratio = std::clamp((value - minValue) / span, 0.0f, 1.0f);
            return row.bottom - sc(8) - static_cast<int>((row.bottom - row.top - sc(16)) * ratio);
        }

        void drawButton(const HDC hdc, const RECT &rect, const std::wstring &label, const bool hovered,
                        const bool active, const HFONT font, const TimelineTheme &theme) {
            COLORREF fill = theme.panelRaised;
            COLORREF border = theme.border;
            if (active) {
                fill = hovered ? theme.accentHover : theme.accent;
                border = theme.accentPressed;
            } else if (hovered) {
                fill = theme.buttonHover;
                border = theme.buttonHoverBorder;
            }
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
            const auto translated = UiLanguage::text(text);
            const HGDIOBJ previous = SelectObject(hdc, font);
            SIZE size = {};
            GetTextExtentPoint32W(hdc, translated.c_str(), static_cast<int>(translated.size()), &size);
            SelectObject(hdc, previous);
            return size.cx;
        }

        int wrappedTextHeight(HDC dc, const std::wstring &text, HFONT font, int width) {
            const auto previous = SelectObject(dc, font);
            RECT measured{0, 0, std::max(1, width), 0};
            try {
                UiLanguage::drawText(dc, text.c_str(), int(text.size()), &measured,
                                     DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
            } catch (...) {
                SelectObject(dc, previous);
                throw;
            }
            SelectObject(dc, previous);
            return measured.bottom;
        }

        std::wstring fittingText(HDC dc, HFONT font, int width, std::initializer_list<std::wstring> choices) {
            for (const auto &text : choices) {
                if (textWidth(dc, text, font) <= width) {
                    return text;
                }
            }
            return {};
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
        RECT drawCaptionBox(const HDC hdc, const RECT &box, const std::wstring &caption,
                            const COLORREF background, const HFONT captionFont, const bool hovered,
                            const bool active, const TimelineTheme &theme) {
            fillRoundRect(hdc, box, hovered ? theme.buttonHover : theme.panelRaised,
                          active ? theme.accent : theme.border, sc(10));
            const int captionWidth = textWidth(hdc, caption, captionFont) + sc(10);
            const RECT plate = {box.left + sc(12), box.top - sc(11), box.left + sc(12) + captionWidth,
                                box.top + sc(11)};
            fillRect(hdc, plate, background);
            drawText(hdc, caption, plate, theme.mutedText, DT_CENTER | DT_VCENTER | DT_SINGLELINE,
                     captionFont);
            return {box.left + sc(12), box.top + sc(10), box.right - sc(12), box.bottom - sc(4)};
        }

        int fontHeight(HDC dc, HFONT font) {
            const auto previous = SelectObject(dc, font);
            TEXTMETRICW metrics{};
            GetTextMetricsW(dc, &metrics);
            SelectObject(dc, previous);
            return int(metrics.tmHeight);
        }

        RECT drawTransportReadout(HDC dc, const RECT &box, const std::wstring &caption, HFONT captionFont,
                                  bool editable, bool hovered, bool active, const TimelineTheme &theme) {
            const auto dip = [](int value) { return UiDpi::pixels(value, timelineDpi ? timelineDpi : 96); };
            if (active || (editable && hovered)) {
                fillRect(dc, box, active ? theme.accentSoft : theme.buttonHover);
            }
            const int labelBottom = box.top + dip(2) + std::max(dip(14), fontHeight(dc, captionFont));
            RECT label{box.left + dip(8), box.top + dip(2), box.right - dip(8), labelBottom};
            drawText(dc, caption, label,
                     active && highContrastSettingsMode() ? theme.selectedText : theme.mutedText,
                     DT_LEFT | DT_VCENTER | DT_SINGLELINE, captionFont);
            if (editable) {
                fillRect(dc, {box.left, box.bottom - 1, box.right, box.bottom},
                         active ? theme.accentText : theme.border);
            }
            return {box.left + dip(8), labelBottom + dip(2), box.right - dip(8), box.bottom - dip(3)};
        }

        void drawToggle(const HDC hdc, const RECT &rect, const bool on, const TimelineTheme &theme) {
            const int radius = static_cast<int>(rect.bottom - rect.top) / 2;
            fillRoundRect(hdc, rect, on ? theme.accent : theme.toggleOff,
                          on ? theme.accentBorder : theme.border, radius * 2);
            const int knob = std::max(radius - sc(4), sc(3));
            const int centerX =
                on ? static_cast<int>(rect.right) - radius : static_cast<int>(rect.left) + radius;
            const int centerY = static_cast<int>(rect.top + rect.bottom) / 2;
            COLORREF knobFill = RGB(245, 246, 248);
            if (highContrastSettingsMode()) {
                knobFill = on ? theme.activeText : theme.panel;
            }
            const HBRUSH brush = CreateSolidBrush(knobFill);
            COLORREF knobBorder = RGB(226, 228, 232);
            if (highContrastSettingsMode()) {
                knobBorder = on ? theme.activeText : theme.text;
            }
            const HPEN pen = CreatePen(PS_SOLID, 1, knobBorder);
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
        void drawTransportButton(const HDC hdc, const RECT &rect, const TransportGlyph glyph,
                                 const bool hovered, const bool active, const TimelineTheme &theme) {
            COLORREF fill = theme.panelRaised;
            if (active) {
                fill = theme.accent;
            } else if (hovered) {
                fill = theme.buttonHover;
            }
            fillRoundRect(hdc, rect, fill, active ? theme.accentBorder : theme.border, sc(8));
            const COLORREF ink = active ? theme.activeText : theme.text;
            const int cx = static_cast<int>(rect.left + rect.right) / 2;
            const int cy = static_cast<int>(rect.top + rect.bottom) / 2;
            const int size = std::max(
                static_cast<int>(std::min(rect.right - rect.left, rect.bottom - rect.top)) / 4, sc(4));
            const HBRUSH brush = CreateSolidBrush(ink);
            const HPEN pen = CreatePen(PS_SOLID, sc(2), ink);
            const HGDIOBJ oldBrush = SelectObject(hdc, brush);
            const HGDIOBJ oldPen = SelectObject(hdc, pen);
            switch (glyph) {
            case TransportGlyph::PLAY: {
                const POINT points[3] = {
                    {cx - size + sc(2), cy - size}, {cx + size, cy}, {cx - size + sc(2), cy + size}};
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
                static_cast<int>(std::min(rect.right - rect.left, rect.bottom - rect.top)) / 2 - sc(2),
                sc(4));
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
                wc.hIcon = static_cast<HICON>(
                    LoadImageW(wc.hInstance, MAKEINTRESOURCEW(1), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR));
                wc.lpfnWndProc = TimelineWindow::windowProc;
                wc.lpszClassName = TIMELINE_WINDOW_CLASS;
                return RegisterClassExW(&wc) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
            }();
            (void)registered;
        }
    } // namespace

    TimelineWindow::TimelineWindow(SettingsMenu &menu, RenderScene &scene)
        : engine(scene.engine), sourceAttribute(&scene.getAttribute()), attribute(scene.getAttribute()),
          settingsMenu(&menu), renderScene(&scene),
          schedule(TimelineSchedule::create(
              scene.getAttribute().video.timeline, scene.getAttribute().video.timeline.estimateKeyframes,
              -scene.getAttribute().video.animation.overZoom, scene.getAttribute().video.animation.mps)) {
        previewScheduleSnapshot = std::make_shared<TimelineSchedule>(schedule);
        lightMode = timelineLightMode();
        updateDpi(UiDpi::forWindow(nullptr));
        fieldEditBrush = CreateSolidBrush(timelineTheme(lightMode).panelRaised);
        ensureEditableTracks();
        linkColorCycle = colorCycleTracksLinked(attribute.video.timeline, true);
        undoBaseline = sourceAttribute->video.timeline;
        undoBaselineStatic = sourceAttribute->video.data.isStatic;
        previewDepth = schedule.getStartDepth();
        resetView();
    }

    VidTimelineTrack *TimelineWindow::track(const uint16_t targetId) {
        for (auto &track : attribute.video.timeline.tracks) {
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
        VidTimelineTrack track = {.targetId = targetId,
                                  .enabled = true,
                                  .keys = {{.depth = attribute.video.timeline.estimateKeyframes,
                                            .value = value,
                                            .color = glm::vec4(1.0f),
                                            .out = out},
                                           {.depth = -attribute.video.animation.overZoom,
                                            .value = value,
                                            .color = glm::vec4(1.0f),
                                            .out = out}}};
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
        (void)ensureScalarTrack(SPEED_TARGET);
        // The color animation is the palette's, which a PNG source never runs, so a timeline opened
        // on one starts with the pacing alone rather than a track that could not move a picture.
        if (fresh && !attribute.video.data.isStatic) {
            (void)ensureScalarTrack(COLOR_ANIMATION_TARGET);
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
        const float depth =
            std::clamp(snapDepth(previewDepth), schedule.getEndDepth(), schedule.getStartDepth());
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
            for (auto &key : current.keys) {
                key.value = clamped;
            }
            selectedTrackTarget = targetId;
            selectedTrackKey = -1;
            hoveredTrackKey = {};
            (void)syncLinkedColorCycle(targetId);
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
            current.keys.push_back({.depth = depth,
                                    .value = clamped,
                                    .color = glm::vec4(1.0f),
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
        (void)syncLinkedColorCycle(targetId);
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
        const float depth =
            std::clamp(snapDepth(previewDepth), schedule.getEndDepth(), schedule.getStartDepth());
        if (created) {
            trackScrollOffset = std::numeric_limits<int>::max() / 2;
        }
        // A color track holding one color throughout is not a curve yet, as a number's is not.
        if (created || flatColorTrack(current)) {
            for (auto &key : current.keys) {
                key.color = color;
            }
            hoveredTrackKey = {};
            commitTimeline();
            return;
        }
        for (auto &key : current.keys) {
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
        current.keys.push_back(
            {.depth = depth, .value = 0.0f, .color = color, .out = VidKeyInterpolation::SMOOTH});
        std::ranges::stable_sort(
            current.keys, [](const VidTimelineKey &a, const VidTimelineKey &b) { return a.depth > b.depth; });
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
        std::erase_if(attribute.video.timeline.tracks,
                      [&isRemoved](const VidTimelineTrack &current) { return isRemoved(current.targetId); });
        std::erase_if(selectedTrackTargets, isRemoved);
        if (isRemoved(selectedTrackTarget)) {
            selectedTrackTarget = SPEED_TARGET;
            selectedTrackKey = -1;
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
        for (const uint16_t targetId : {CYCLE_R_TARGET, CYCLE_G_TARGET, CYCLE_B_TARGET}) {
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
        refreshTheme();
    }

    void TimelineWindow::refreshTheme() {
        const auto &shared = settingsTheme(!lightMode);
        inspectorTheme = {shared.background,
                          shared.text,
                          shared.rangeText,
                          shared.cardNoteAccent,
                          shared.radioSelectedBackground,
                          shared.radioSelectedText,
                          shared.textFieldBackground,
                          shared.sliderTrack,
                          shared.textError};
        if (inspector) {
            inspector->applyTheme();
        }
        applyPanelTheme();
        hoverTheme = false;
        if (fieldEditBrush != nullptr) {
            DeleteObject(fieldEditBrush);
        }
        fieldEditBrush = CreateSolidBrush(timelineTheme(lightMode).panelRaised);
        if (fieldEdit != nullptr) {
            applyDarkThemeClass(fieldEdit, false, !lightMode);
            InvalidateRect(fieldEdit, nullptr, TRUE);
        }
        if (fieldTooltip) {
            applyDarkThemeClass(fieldTooltip, false, !lightMode);
            SendMessageW(fieldTooltip, TTM_SETTIPBKCOLOR, settingsTheme(!lightMode).tooltipBackground, 0);
            SendMessageW(fieldTooltip, TTM_SETTIPTEXTCOLOR, settingsTheme(!lightMode).tooltipText, 0);
        }
        if (window != nullptr) {
            if (!embedded || floatingWorkspace) {
                applyDarkWindowFrame(window, !lightMode);
            }
            InvalidateRect(window, nullptr, FALSE);
        }
    }

    void TimelineWindow::applyWorkspaceTheme(HWND handle) {
        auto *self = reinterpret_cast<TimelineWindow *>(GetWindowLongPtrW(handle, GWLP_USERDATA));
        if (!self) {
            return;
        }
        if (self->embedded) {
            self->lightMode = !darkSettingsMode();
        }
        self->refreshTheme();
    }

    void TimelineWindow::adoptPanel(SettingsWindow &panel) {
        std::erase_if(themedPanels, [](const HWND open) { return !IsWindow(open); });
        themedPanels.push_back(panel.getWindow());
        panel.setDarkOverride(!lightMode);
    }

    void TimelineWindow::applyPanelTheme() const {
        for (const HWND panel : themedPanels) {
            if (SettingsWindow *opened = SettingsWindow::of(panel); opened != nullptr) {
                opened->setDarkOverride(!lightMode);
            }
        }
    }

    void TimelineWindow::toggleFullscreen() {
        if (window == nullptr || embedded) {
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
                     monitor.rcMonitor.bottom - monitor.rcMonitor.top, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        InvalidateRect(window, nullptr, FALSE);
    }

    std::pair<float, float> TimelineWindow::valueRange(const uint16_t targetId) const {
        const VidTimelineTrack *current = track(targetId);
        const float fallback = baseValue(targetId);
        if (targetId == SPEED_TARGET) {
            float peak = std::max(fallback, VidTimelineAttribute::MIN_SPEED);
            if (current != nullptr) {
                for (const auto &key : current->keys) {
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
                for (const auto &key : current->keys) {
                    peak = std::max(peak, key.value);
                }
            }
            return {0.0f, std::max(peak * 1.2f, fallback * 2.0f)};
        }
        float magnitude = std::max(1.0f, std::abs(fallback));
        if (current != nullptr) {
            for (const auto &key : current->keys) {
                magnitude = std::max(magnitude, std::abs(key.value));
            }
        }
        magnitude *= 1.2f;
        return {-magnitude, magnitude};
    }

    const TimelineWindow::TrackLayout *TimelineWindow::layout(const uint16_t targetId) const {
        for (const auto &item : trackLayouts) {
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
        const float ratio = std::clamp(static_cast<float>(point.x - scrollGrabOffset - scrollTrack.left) /
                                           static_cast<float>(travel),
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
        const float ratio =
            std::clamp(static_cast<float>(point.y - trackScrollGrabOffset - trackScrollTrack.top) /
                           static_cast<float>(travel),
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
        for (auto &track : attribute.video.timeline.tracks) {
            for (auto &key : track.keys) {
                key.depth = key.depth >= previousStartDepth - 1e-3f
                                ? startDepth
                                : std::clamp(key.depth, endDepth, startDepth);
            }
            std::ranges::stable_sort(track.keys, [](const VidTimelineKey &a, const VidTimelineKey &b) {
                return a.depth > b.depth;
            });
            // Keys the move brought onto one depth are one key, so only the first of them is kept.
            const auto duplicates =
                std::ranges::unique(track.keys, [](const VidTimelineKey &a, const VidTimelineKey &b) {
                    return std::abs(a.depth - b.depth) < 1e-3f;
                });
            track.keys.erase(duplicates.begin(), duplicates.end());
        }
        selectedTrackKey = -1;
        hoveredTrackKey = {};
    }

    void TimelineWindow::rebuildSchedule() {
        const bool wasFullView = viewSpan() >= schedule.getStartDepth() - schedule.getEndDepth() - 1e-3f;
        schedule =
            TimelineSchedule::create(attribute.video.timeline, attribute.video.timeline.estimateKeyframes,
                                     -attribute.video.animation.overZoom, attribute.video.animation.mps);
        previewScheduleSnapshot = std::make_shared<TimelineSchedule>(schedule);
        previewDepth = std::clamp(previewDepth, schedule.getEndDepth(), schedule.getStartDepth());
        if (wasFullView) {
            resetView();
        } else {
            clampView();
        }
        syncPlaybackClock();
    }

    void TimelineWindow::commitTimeline() {
        accessibilityDirty = true;
        attribute.video.timeline.enabled = true;
        recordUndoStep();
        rebuildSchedule();
        if (sourceAttribute != nullptr) {
            sourceAttribute->video.timeline = attribute.video.timeline;
            rememberWorkspaceSource();
        }
        if (!draggingTrackKey) {
            requestFramePreview();
        }
        if (window != nullptr) {
            InvalidateRect(window, nullptr, FALSE);
        }
    }

    namespace {
        std::string timelineBytes(const VidTimelineAttribute &value) {
            std::ostringstream out(std::ios::out | std::ios::binary);
            TimelineIO::writeTimeline(out, value);
            AudioTimelineIO::write(out, value.audio);
            TimelineIO::writeOverlayPrecision(out, value.zoomOverlay);
            return std::move(out).str();
        }
    } // namespace

    void TimelineWindow::recordUndoStep() {
        const ULONGLONG now = GetTickCount64();
        // A key being dragged, or a value scrubbed in a settings panel, reports its change on every
        // mouse move. One step for the whole gesture is what Undo is asked to take back, so a change
        // arriving on the heels of the last one extends that step rather than opening another.
        if (timelineBytes(undoBaseline) == timelineBytes(attribute.video.timeline) &&
            undoBaselineStatic == attribute.video.data.isStatic && undoBaselineAudioRow == audioRowIndex) {
            return;
        }
        if (const bool extend = !undoSteps.empty() && historyOrder.latest(undoSteps.back().serial) &&
                                timelineBytes(undoSteps.back().after) == timelineBytes(undoBaseline) &&
                                undoSteps.back().afterStatic == undoBaselineStatic &&
                                (draggingTrackKey ? dragHasUndoStep : now - lastUndoStep < UNDO_COALESCE_MS);
            !extend) {
            undoSteps.push_back({undoBaseline, attribute.video.timeline, historyOrder.commit(),
                                 undoBaselineStatic, attribute.video.data.isStatic, undoBaselineAudioRow, audioRowIndex});
            if (undoSteps.size() > MAX_UNDO_STEPS) {
                undoSteps.erase(undoSteps.begin());
            }
            // The steps taken back are what a new change branches away from, and are gone with it.
            redoSteps.clear();
        }
        undoSteps.back().after = attribute.video.timeline;
        undoSteps.back().afterStatic = attribute.video.data.isStatic;
        undoSteps.back().afterAudioRow = audioRowIndex;
        undoBaselineAudioRow = audioRowIndex;
        undoBaselineStatic = attribute.video.data.isStatic;
        if (draggingTrackKey) {
            dragHasUndoStep = true;
        }
        lastUndoStep = now;
        undoBaseline = attribute.video.timeline;
    }

    bool TimelineWindow::undoTimeline() {
        if (undoSteps.empty()) {
            return false;
        }
        const auto &current = sourceAttribute ? sourceAttribute->video.timeline : attribute.video.timeline;
        if (timelineBytes(current) != timelineBytes(undoSteps.back().after) ||
            (sourceAttribute ? sourceAttribute->video.data.isStatic : attribute.video.data.isStatic) !=
                undoSteps.back().afterStatic) {
            undoSteps.clear();
            redoSteps.clear();
            return false;
        }
        historyOrder.prepareUndo(redoSteps);
        audioRowIndex = undoSteps.back().beforeAudioRow;
        attribute.video.data.isStatic = undoSteps.back().beforeStatic;
        if (sourceAttribute) {
            sourceAttribute->video.data.isStatic = attribute.video.data.isStatic;
        }
        auto restored = undoSteps.back().before;
        redoSteps.push_back(std::move(undoSteps.back()));
        undoSteps.pop_back();
        applyRestoredTimeline(std::move(restored));
        return true;
    }

    bool TimelineWindow::redoTimeline() {
        if (redoSteps.empty() || !historyOrder.validRedo()) {
            return false;
        }
        const auto &current = sourceAttribute ? sourceAttribute->video.timeline : attribute.video.timeline;
        if (timelineBytes(current) != timelineBytes(redoSteps.back().before) ||
            (sourceAttribute ? sourceAttribute->video.data.isStatic : attribute.video.data.isStatic) !=
                redoSteps.back().beforeStatic) {
            undoSteps.clear();
            redoSteps.clear();
            return false;
        }
        audioRowIndex = redoSteps.back().afterAudioRow;
        attribute.video.data.isStatic = redoSteps.back().afterStatic;
        if (sourceAttribute) {
            sourceAttribute->video.data.isStatic = attribute.video.data.isStatic;
        }
        auto restored = redoSteps.back().after;
        undoSteps.push_back(std::move(redoSteps.back()));
        redoSteps.pop_back();
        applyRestoredTimeline(std::move(restored));
        return true;
    }

    void TimelineWindow::applyRestoredTimeline(VidTimelineAttribute &&restored) {
        undoBaselineAudioRow = audioRowIndex;
        attribute.video.timeline = std::move(restored);
        attribute.video.animation.showText = attribute.video.timeline.zoomOverlay.visible;
        linkColorCycle = colorCycleTracksLinked(attribute.video.timeline, linkColorCycle);
        PostMessageW(window, WM_APP + 0x266, 0, 0);
        // A key the step being put back had added is not there to stay picked, edited or hovered.
        selectedTrackKey = -1;
        selectedTrackTargets.clear();
        hoveredTrackKey = {};
        // The step just put back is where the next change is measured from, and the gesture
        // clock is cleared so that change opens a step of its own however quickly it follows.
        undoBaseline = attribute.video.timeline;
        undoBaselineStatic = attribute.video.data.isStatic;
        lastUndoStep = 0;
        if (sourceAttribute) {
            sourceAttribute->video.timeline = attribute.video.timeline;
            sourceAttribute->video.animation.showText = attribute.video.animation.showText;
            rememberWorkspaceSource();
        }
        ensureEditableTracks();
        rebuildSchedule();
        if (embedded) {
            syncWorkspace(window);
        } else {
            requestFramePreview();
        }
        InvalidateRect(window, nullptr, FALSE);
    }

    TimelineWindow::KeyHit TimelineWindow::hitTrackKey(const POINT point) const {
        const int radius = sc(9);
        KeyHit best = {};
        int bestDistance = radius * radius + 1;
        for (const auto &item : trackLayouts) {
            const VidTimelineTrack *current = track(item.targetId);
            if (current == nullptr) {
                continue;
            }
            for (int i = 0; i < static_cast<int>(current->keys.size()); ++i) {
                const auto &key = current->keys[i];
                const int x = depthX(key.depth, viewStartDepth, viewEndDepth, timelineAxis);
                if (!visibleX(x, timelineAxis, radius)) {
                    continue;
                }
                const int y = item.editable ? valueY(key.value, item.minValue, item.maxValue, item.row)
                                            : (item.row.top + item.row.bottom) / 2;
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
        for (const auto &item : trackLayouts) {
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
        for (const auto &item : trackLayouts) {
            if (contains(item.label, point)) {
                return item.targetId;
            }
        }
        return UINT16_MAX;
    }

    int TimelineWindow::trackRowDropTarget(const POINT point) const {
        int index = -1;
        int firstVisible = -1;
        for (const auto &item : trackLayouts) {
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
        if (targetId == AUDIO_ROW_TARGET) inspectorSectionRequest = 1;
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
        for (const uint16_t id : order) {
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
        audioRowIndex = int(std::ranges::find(order, AUDIO_ROW_TARGET) - order.begin());
        if (audioRowIndex == int(order.size()) - 1) audioRowIndex = -1;
        lastUndoStep = 0;
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
        for (const uint16_t id : order) {
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
            rememberWorkspaceSource();
        }
        if (window != nullptr) {
            InvalidateRect(window, nullptr, FALSE);
        }
    }

    void TimelineWindow::updateTrackKey(const POINT point) {
        VidTimelineTrack *current = track(selectedTrackTarget);
        if (current == nullptr || selectedTrackKey < 0 ||
            selectedTrackKey >= static_cast<int>(current->keys.size())) {
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
        const float valueRatio =
            std::clamp(static_cast<float>(item->row.bottom - sc(8) - point.y) / plotHeight, 0.0f, 1.0f);
        float value = dragValueMin + (dragValueMax - dragValueMin) * valueRatio;
        if (selectedTrackTarget == SPEED_TARGET) {
            value = std::max(value, VidTimelineAttribute::MIN_SPEED);
        } else if (const TimelineParamDesc *param = TimelineParams::find(selectedTrackTarget);
                   param != nullptr) {
            value = std::clamp(value, param->minValue, param->maxValue);
            if (param->kind == TimelineParamKind::BOOL || param->kind == TimelineParamKind::ENUM) {
                value = std::round(value);
            }
        }
        current->keys[selectedTrackKey].depth = std::clamp(depth, endDepth, startDepth);
        if (item->editable) {
            current->keys[selectedTrackKey].value = value;
        }
        previewDepth = current->keys[selectedTrackKey].depth;
        (void)syncLinkedColorCycle(selectedTrackTarget);
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
                          : evaluateDisplayedTrack(current, targetId, depth, baseValue(targetId));
        if (targetId == SPEED_TARGET) {
            value = std::max(value, VidTimelineAttribute::MIN_SPEED);
        }
        current.keys.push_back({.depth = depth,
                                .value = value,
                                .color = glm::vec4(1.0f),
                                .out = defaultInterpolation(targetId)});
        std::ranges::stable_sort(
            current.keys, [](const VidTimelineKey &a, const VidTimelineKey &b) { return a.depth > b.depth; });
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
        (void)syncLinkedColorCycle(targetId);
        commitTimeline();
    }

    void TimelineWindow::deleteTrackKey() {
        VidTimelineTrack *current = track(selectedTrackTarget);
        if (current == nullptr || selectedTrackKey < 0 ||
            selectedTrackKey >= static_cast<int>(current->keys.size())) {
            return;
        }
        if (current->keys.size() <= 1) {
            MessageBeep(MB_ICONWARNING);
            return;
        }
        current->enabled = true;
        current->keys.erase(current->keys.begin() + selectedTrackKey);
        selectedTrackKey = std::min(selectedTrackKey, static_cast<int>(current->keys.size()) - 1);
        (void)syncLinkedColorCycle(selectedTrackTarget);
        commitTimeline();
    }

    void TimelineWindow::setTrackInterpolation(const VidKeyInterpolation interpolation) {
        VidTimelineTrack *current = track(selectedTrackTarget);
        if (current == nullptr || selectedTrackKey < 0 ||
            selectedTrackKey >= static_cast<int>(current->keys.size())) {
            return;
        }
        current->enabled = true;
        current->keys[selectedTrackKey].out = interpolation;
        if (const auto *p = TimelineParams::find(selectedTrackTarget);
            p && (p->kind == TimelineParamKind::BOOL || p->kind == TimelineParamKind::ENUM)) {
            current->keys[selectedTrackKey].out = VidKeyInterpolation::STEP;
        }
        (void)syncLinkedColorCycle(selectedTrackTarget);
        commitTimeline();
    }

    void TimelineWindow::openTrackKeyEditor() {
        showInspectorSection(0);
    }

    void TimelineWindow::loadTimeline() {
        const auto path =
            IOUtilities::ioFileDialogMulti(L"Open Video Timeline", IOUtilities::OPEN_FILE,
                {{std::wstring(Constants::Extension::DESC_TIMELINE), std::wstring(Constants::Extension::TIMELINE)},
                 {L"Timeline JSON", L"json"}});
        if (path == nullptr) {
            return;
        }
        if (_wcsicmp(path->extension().c_str(), L".json") == 0) {
            try {
                if (std::filesystem::file_size(*path) > TimelineJsonIO::maximumBytes)
                    throw std::runtime_error("JSON exceeds 16 MiB");
                std::ifstream input(*path, std::ios::binary);
                if (!input) throw std::runtime_error("Cannot open JSON file");
                std::string text((std::istreambuf_iterator<char>(input)), {});
                if (input.bad()) throw std::runtime_error("Cannot read JSON file");
                importTimelineJson(text, *path);
            } catch (const std::exception &e) {
                NativeDialogs::message(window, e.what(), "Timeline JSON", MB_OK | MB_ICONERROR);
            }
            return;
        }
        VidTimelineAttribute loaded = {};
        loaded.zoomOverlay.visible = attribute.video.timeline.zoomOverlay.visible;
        if (!TimelineIO::load(*path, loaded)) {
            NativeDialogs::message(window, L"The selected .rfvt file could not be loaded.",
                                   L"Timeline Editor", MB_OK | MB_ICONERROR);
            return;
        }
        attribute.video.timeline = std::move(loaded);
        PostMessageW(window, WM_APP + 0x266, 0, 0);
        // The keys of the file were written against the keyframe count it carries, so they are moved
        // onto this folder's axis when its own count takes over.
        const float fileStartDepth = attribute.video.timeline.estimateKeyframes;
        if (frameSource != nullptr) {
            attribute.video.timeline.estimateKeyframes = static_cast<float>(frameSource->getFrameCount());
            retargetTrackDepths(fileStartDepth);
        }
        ensureEditableTracks();
        // Three channels a file holds apart are not linked ones, so the link follows what it carries.
        linkColorCycle = colorCycleTracksLinked(attribute.video.timeline, false);
        selectedTrackTarget = SPEED_TARGET;
        selectedTrackKey = -1;
        hoveredTrackKey = {};
        rebuildSchedule();
        if (sourceAttribute != nullptr) {
            recordUndoStep();
            sourceAttribute->video.timeline = attribute.video.timeline;
            rememberWorkspaceSource();
        }
        requestFramePreview();
        InvalidateRect(window, nullptr, FALSE);
    }

    void TimelineWindow::saveTimeline() const {
        const auto path =
            IOUtilities::ioFileDialogMulti(L"Save Video Timeline", IOUtilities::SAVE_FILE,
                {{std::wstring(Constants::Extension::DESC_TIMELINE), std::wstring(Constants::Extension::TIMELINE)},
                 {L"Timeline JSON", L"json"}});
        if (path && _wcsicmp(path->extension().c_str(), L".json") == 0) {
            const auto temporary = IOUtilities::temporaryFilePath(*path);
            try {
                auto timeline = attribute.video.timeline;
                AudioTimelineIO::relativePaths(timeline.audio, *path);
                const auto text = TimelineJsonIO::document(timeline).dump(2);
                if (text.size() > TimelineJsonIO::maximumBytes) throw std::runtime_error("JSON exceeds 16 MiB");
                std::ofstream out(temporary, std::ios::binary);
                out.write(text.data(), static_cast<std::streamsize>(text.size()));
                out.close();
                if (!out || !IOUtilities::commitTemporaryFile(temporary, *path))
                    throw std::runtime_error("Cannot save timeline JSON");
            } catch (const std::exception &e) {
                IOUtilities::discardTemporaryFile(temporary);
                NativeDialogs::message(window, e.what(), "Timeline JSON", MB_OK | MB_ICONERROR);
            }
            return;
        }
        if (path != nullptr && !TimelineIO::save(*path, attribute.video.timeline)) {
            NativeDialogs::message(window, L"The .rfvt file could not be saved.", L"Timeline Editor",
                                   MB_OK | MB_ICONERROR);
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
                &exportation.fps,         &exportation.bitrate,     &exportation.lossless,
                &exportation.keyframeAA,  &exportation.colorAA,     &exportation.pauseMainPreview,
                &exportation.hdrTransfer, &exportation.hdrPeakNits, &exportation.showExportPreview,
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
        AppendMenuW(menu, MF_STRING, CMD_EXPORT_VIDEO, UiLanguage::label(L"Export Video"));
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, CMD_EXPORT_SETTINGS, UiLanguage::label(L"Export Settings"));
        MENUITEMINFOW item = {sizeof(item)};
        item.fMask = MIIM_STATE;
        item.fState = MFS_DEFAULT;
        SetMenuItemInfoW(menu, CMD_EXPORT_VIDEO, FALSE, &item);
        POINT at = {exportButton.left, exportButton.bottom + sc(2)};
        ClientToScreen(window, &at);
        const int chosen = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN, at.x, at.y, 0,
                                          window, nullptr);
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
            std::unique_ptr<VideoFrameSource> opened =
                VideoFrameSource::open(*directory, attribute.video.data.isStatic, error);
            if (opened == nullptr) {
                NativeDialogs::message(window, error.c_str(), L"Timeline Export", MB_OK | MB_ICONERROR);
                return;
            }
            frameSource = std::move(opened);
            attribute.video.data.isStatic = frameSource->isStatic();
            const float previousStartDepth = attribute.video.timeline.estimateKeyframes;
            attribute.video.timeline.estimateKeyframes = static_cast<float>(frameSource->getFrameCount());
            retargetTrackDepths(previousStartDepth);
            rebuildSchedule();
            sourceAttribute->video.data.isStatic = attribute.video.data.isStatic;
            recordUndoStep();
            sourceAttribute->video.timeline = attribute.video.timeline;
            rememberWorkspaceSource();
        }

        const bool lossless = sourceAttribute->video.exportation.lossless;
        const auto save = IOUtilities::ioFileDialog(
            L"Save Video Location", Constants::Extension::DESC_VIDEO, IOUtilities::SAVE_FILE,
            lossless ? Constants::Extension::VIDEO_LOSSLESS : Constants::Extension::VIDEO);
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
        exportStopSource = std::stop_source{};
        hoverExport = false;
        setPlaying(false);
        InvalidateRect(window, nullptr, FALSE);

        try {
            scene->getBackgroundThreads().createThread([engine = &engine, scene, exportAttribute, directory,
                                                        save = *save, notifyWindow,
                                                        owner = this, exportStop = exportStopSource](const BackgroundThread &thread) {
                std::stop_callback shutdownStop(thread.stopToken(), [exportStop]() mutable {
                    exportStop.request_stop();
                });
                struct ExportActivity final {
                    RenderScene *scene;
                    HWND notifyWindow;
                    TimelineWindow *owner;
                    ExportActivity(RenderScene *scene, HWND notifyWindow, TimelineWindow *owner)
                        : scene(scene), notifyWindow(notifyWindow), owner(owner) {
                        scene->setVideoExportActive(true);
                    }
                    ~ExportActivity() {
                        PostMessageW(notifyWindow, WM_TIMELINE_EXPORT_FINISHED,
                                     reinterpret_cast<WPARAM>(owner), 0);
                        scene->setVideoExportActive(false);
                    }
                } activity(scene, notifyWindow, owner);
                VideoWindow::createVideo(*engine, exportAttribute, directory, save, {}, exportStop.get_token());
            });
        } catch (const std::exception &) {
            PostMessageW(notifyWindow, WM_TIMELINE_EXPORT_FINISHED, reinterpret_cast<WPARAM>(this), 0);
            NativeDialogs::message(window, L"The video export worker could not start.", L"Timeline Export",
                                   MB_OK | MB_ICONERROR);
        }
    }

    void TimelineWindow::loadKeyframeDirectory() {
        const auto directory = IOUtilities::ioDirectoryDialog(L"Open Video Keyframes");
        if (directory == nullptr) {
            return;
        }
        loadKeyframeDirectory(*directory);
    }

    void TimelineWindow::loadKeyframeDirectory(const std::filesystem::path &directory) {
        std::wstring error;
        std::unique_ptr<VideoFrameSource> opened =
            VideoFrameSource::open(directory, attribute.video.data.isStatic, error);
        if (opened == nullptr) {
            NativeDialogs::message(window, error.c_str(), L"Timeline Preview", MB_OK | MB_ICONERROR);
            return;
        }
        if (engine.isValidWindowContext(Constants::VulkanWindow::VIDEO_WINDOW_ATTACHMENT_INDEX) &&
            !previewWorker.joinable() && !previewContextAttached.load()) {
            NativeDialogs::message(window,
                                   L"The video renderer is already being used by another preview or export.",
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
            recordUndoStep();
            sourceAttribute->video.timeline = attribute.video.timeline;
            rememberWorkspaceSource();
        }
        (void)initializeFramePreview();
    }

    bool TimelineWindow::initializeFramePreview() {
        if (frameSource == nullptr || window == nullptr) {
            return false;
        }
        previewRenderWindow =
            CreateWindowExW(0, Constants::Win32::CLASS_VIDEO_RENDER_WINDOW, nullptr, WS_CHILD, 0, 0, sc(64),
                            sc(64), window, nullptr, GetModuleHandleW(nullptr), nullptr);
        if (previewRenderWindow == nullptr) {
            NativeDialogs::message(window, L"Could not create the keyframe renderer.", L"Timeline Preview",
                                   MB_OK | MB_ICONERROR);
            return false;
        }
        {
            std::scoped_lock lock(previewBitmapMutex);
            previewMessage = L"Building the keyframe renderer... (the first time can take minutes)";
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
            Attribute previewAttribute = cameraSourceAttribute(initialAttribute, frameSource->getDirectory());
            const VkExtent2D sourceExtent{frameSource->getWidth(), frameSource->getHeight()};
            const double scale = std::min({0.5 / std::max(1u, previewAttribute.render.ssaa),
                                           1280.0 / sourceExtent.width, 720.0 / sourceExtent.height});
            const VkExtent2D previewExtent{std::max(1u, static_cast<uint32_t>(sourceExtent.width * scale)),
                                           std::max(1u, static_cast<uint32_t>(sourceExtent.height * scale))};
            previewAttribute.render.ssaa = 1;
            previewAttribute.video.exportation.keyframeAA = 1;
            previewAttribute.video.exportation.colorAA = 1;
            frameSource->setPreviewSize(previewExtent.width, previewExtent.height);
            previewScene =
                std::make_unique<VideoRenderScene>(engine, *context, previewExtent, previewAttribute);
        } catch (const std::exception &e) {
            previewWorkerFailed.store(true);
            const std::string text = e.what();
            {
                std::scoped_lock lock(previewBitmapMutex);
                previewMessage =
                    std::format(L"Keyframe renderer failed: {}", std::wstring(text.begin(), text.end()));
            }
            PostMessageW(window, WM_TIMELINE_PREVIEW_READY, 0, 0);
            return false;
        }
        return true;
    }

    void TimelineWindow::startFramePreviewWorker() {
        uint64_t initialGeneration;
        {
            std::scoped_lock lock(previewRequestMutex);
            initialGeneration = previewRequestGeneration;
        }
        setPlaying(false);
        cacheStop = std::stop_source{};
        cachePreloading.store(true);
        {
            std::scoped_lock lock(previewBitmapMutex);
            cacheMessage = UiLanguage::text(L"Preparing RAM preload...");
        }
        previewWorker = std::jthread([this, initialGeneration](const std::stop_token stopToken) {
            uint64_t processedGeneration = initialGeneration;
            MEMORYSTATUSEX memory{sizeof(memory)};
            const uint64_t temporaryBytes = uint64_t(frameSource->getWidth()) * frameSource->getHeight() * 32;
            const uint64_t budget =
                GlobalMemoryStatusEx(&memory) && memory.ullAvailPhys > temporaryBytes
                    ? std::min<uint64_t>(memory.ullAvailPhys / 2, memory.ullAvailPhys - temporaryBytes)
                    : 0;
            std::wstring error;
            ULONGLONG lastProgress = 0;
            const bool cached = frameSource->preload(
                budget, cacheStop.get_token(),
                [this, &lastProgress, &processedGeneration,
                 stopToken](const uint32_t done, const uint32_t count, const uint64_t bytes) {
                    const ULONGLONG now = GetTickCount64();
                    if (done != 0 && done != count && now - lastProgress < 100) {
                        return;
                    }
                    lastProgress = now;
                    processFramePreviewRequest(processedGeneration, stopToken, false);
                    {
                        std::scoped_lock lock(previewBitmapMutex);
                        cacheMessage = std::format(L"{} {}/{}  ({:.1f} MiB)\n{}",
                                                   UiLanguage::text(L"Preloading previews to RAM:"), done,
                                                   count, static_cast<double>(bytes) / (1024 * 1024),
                                                   UiLanguage::text(L"Stop cancels preloading."));
                    }
                    PostMessageW(window, WM_TIMELINE_CACHE_PROGRESS, 0, 0);
                },
                error);
            {
                std::scoped_lock lock(previewBitmapMutex);
                cacheMessage =
                    cached
                        ? std::format(L"{} {}  ({:.1f} MiB)", UiLanguage::text(L"RAM preload complete:"),
                                      frameSource->getFrameCount(),
                                      static_cast<double>(frameSource->previewCacheBytes()) / (1024 * 1024))
                        : UiLanguage::text(error);
                if (!cached && error != L"Not enough free RAM for all previews. Using on-demand loading." &&
                    error != L"RAM preload canceled. Using on-demand loading." &&
                    error != L"RAM preload failed. Using on-demand loading.") {
                    cacheMessage += L" " + UiLanguage::text(L"RAM preload failed. Using on-demand loading.");
                }
            }
            cachePreloading.store(false);
            PostMessageW(window, WM_TIMELINE_CACHE_PROGRESS, 0, 0);
            while (!stopToken.stop_requested()) {
                processFramePreviewRequest(processedGeneration, stopToken, true);
            }
        });
    }

    void TimelineWindow::processFramePreviewRequest(uint64_t &processedGeneration,
                                                    const std::stop_token stopToken, const bool wait) {
        int imageSide = 0;
        uint32_t imagePage = 0;
        std::shared_ptr<const AiBundleRequest> imageBundle;
        float depth = 0.0f;
        double sec = 0.0;
        VidTimelineAttribute timeline = {};
        ShaderAttribute shader = {};
        std::shared_ptr<const TimelineSchedule> timelineSchedule;
        {
            std::unique_lock lock(previewRequestMutex);
            if (wait) {
                previewRequestCondition.wait(lock, [this, &stopToken, &processedGeneration] {
                    return stopToken.stop_requested() || previewRequestGeneration != processedGeneration;
                });
            }
            if (stopToken.stop_requested() || previewRequestGeneration == processedGeneration) {
                return;
            }
            processedGeneration = previewRequestGeneration;
            depth = requestedPreviewDepth;
            sec = requestedPreviewSec;
            timeline = requestedPreviewTimeline;
            shader = requestedPreviewShader;
            timelineSchedule = requestedPreviewSchedule;
            imageSide = std::exchange(requestedAiImageSide, 0);
            imagePage = requestedAiImagePage;
            imageBundle = std::exchange(requestedAiBundle, {});
        }
        previewWorkerFailed.store(false);
        if (imageSide != 0) {
            renderAiImages(imageSide, imagePage, timeline, shader, *timelineSchedule, processedGeneration, stopToken, imageBundle);
            if (stopToken.stop_requested()) return;
        }
        try {
            (void)renderFramePreview(depth, sec, timeline, shader, *timelineSchedule, processedGeneration);
        } catch (const std::exception &e) {
            // What went wrong is carried into the editor, where it can be read and reported.
            const std::string text = e.what();
            {
                std::scoped_lock lock(previewBitmapMutex);
                previewMessage =
                    std::format(L"Keyframe preview failed: {}", std::wstring(text.begin(), text.end()));
            }
            previewWorkerFailed.store(true);
            PostMessageW(window, WM_TIMELINE_PREVIEW_READY, static_cast<WPARAM>(processedGeneration), 0);
        } catch (...) {
            {
                std::scoped_lock lock(previewBitmapMutex);
                previewMessage = L"The keyframe preview could not be rendered";
            }
            previewWorkerFailed.store(true);
            PostMessageW(window, WM_TIMELINE_PREVIEW_READY, static_cast<WPARAM>(processedGeneration), 0);
        }
    }

    void TimelineWindow::stopFramePreviewWorker() {
        cacheStop.request_stop();
        // Whatever was outstanding ends with the worker, so the rendering line must not outlive it.
        previewPending = false;
        previewBusy = false;
        if (window != nullptr) {
            KillTimer(window, PREVIEW_STATUS_TIMER);
        }
        if (!previewWorker.joinable()) {
            return;
        }
        {
            std::scoped_lock lock(previewRequestMutex);
            previewWorker.request_stop();
        }
        previewRequestCondition.notify_all();
        previewWorker.join();
        {
            std::scoped_lock lock(previewRequestMutex);
            requestedAiImageSide = 0;
            requestedAiBundle.reset();
        }
        aiImagesBusy.store(false);
        {
            std::scoped_lock lock(previewBitmapMutex);
            aiImageSheet.release();
            aiImageError.clear();
            aiImageGeneration = 0;
            aiSavedDirectory.clear();
        }
        cachePreloading.store(false);
    }

    double TimelineWindow::previewSeconds() const {
        if (playSeconds >= 0.0f && playSeconds <= schedule.getTotalSeconds() &&
            schedule.depthAt(playSeconds) == previewDepth) {
            return playSeconds;
        }
        return schedule.timeAt(previewDepth);
    }

    void TimelineWindow::requestFramePreview(const double seconds) {
        // Read here rather than held from when the editor opened: the Shader menu stays usable while
        // it is, and a fog or color changed there belongs in the next preview. Read before the
        // return below as well, since the track rows are drawn against it whether a keyframe folder
        // has been chosen or not.
        if (sourceAttribute != nullptr) {
            attribute.shader = sourceAttribute->shader;
        }
        if (frameSource == nullptr || !previewWorker.joinable()) {
            return;
        }
        {
            std::scoped_lock lock(previewRequestMutex);
            requestedPreviewDepth = previewDepth;
            requestedPreviewSec = seconds >= 0.0f ? seconds : previewSeconds();
            requestedPreviewTimeline = attribute.video.timeline;
            requestedPreviewShader = attribute.shader;
            requestedPreviewSchedule = previewScheduleSnapshot;
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

    bool TimelineWindow::renderFramePreview(const float depth, const double sec,
                                            const VidTimelineAttribute &timeline,
                                            const ShaderAttribute &shader, const TimelineSchedule &timelineSchedule,
                                            const uint64_t generation, cv::Mat *capture) {
        if (frameSource == nullptr || previewScene == nullptr) {
            return false;
        }
        std::wstring error;
        if (!frameSource->load(depth, error)) {
            {
                std::scoped_lock lock(previewBitmapMutex);
                previewMessage = error;
            }
            PostMessageW(window, WM_TIMELINE_PREVIEW_READY, static_cast<WPARAM>(generation), 0);
            return false;
        }

        const float sampledDepth = frameSource->getSampledDepth();
        previewScene->updateBase(shader, timeline);
        previewScene->setStatic(frameSource->isStatic());
        previewScene->setTimelineSchedule(timelineSchedule);
        previewScene->setCurrentFrame(sampledDepth);
        previewScene->setTime(sec);
        if (frameSource->isStatic()) {
            auto &normal = frameSource->getNormalStatic();
            auto &zoomed = frameSource->getZoomedStatic();
            previewScene->setMap(&normal, &zoomed);
            previewScene->applyCurrentStaticImage(frameSource->getNormalImage(),
                                                  frameSource->getZoomedImage());
        } else {
            auto &normal = frameSource->getNormalDynamic();
            auto &zoomed = frameSource->getZoomedDynamic();
            previewScene->setMap(&normal, &zoomed);
            previewScene->applyCurrentDynamicMap(normal, zoomed, sampledDepth);
            const uint64_t normalMax = normal.getMaxIteration();
            const uint64_t zoomedMax = zoomed.getMaxIteration();
            previewScene->setMaxIterationDynamic(static_cast<double>(std::max(normalMax, zoomedMax)),
                                                 static_cast<double>(normalMax),
                                                 static_cast<double>(zoomedMax));
        }
        previewScene->applyTimelineShader(depth, sec);
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
            PostMessageW(window, WM_TIMELINE_PREVIEW_READY, static_cast<WPARAM>(generation), 0);
            return false;
        }
        if (capture) {
            *capture = buffer->image.clone();
            return true;
        }
        SIZE size = {};
        const HBITMAP bitmap = createPreviewBitmap(window, buffer->image, size);
        if (bitmap == nullptr) {
            {
                std::scoped_lock lock(previewBitmapMutex);
                previewMessage = L"The rendered keyframe has an unsupported image format";
            }
            PostMessageW(window, WM_TIMELINE_PREVIEW_READY, static_cast<WPARAM>(generation), 0);
            return false;
        }
        {
            std::scoped_lock lock(previewBitmapMutex);
            if (previewBitmap != nullptr) {
                DeleteObject(previewBitmap);
            }
            previewBitmap = bitmap;
            publishedPreviewZoom = buffer->zoom;
            previewSize = size;
            publishedPreviewGeneration = generation;
            previewMessage = std::format(L"{} keyframes", frameSource->getFrameCount());
        }
        PostMessageW(window, WM_TIMELINE_PREVIEW_READY, static_cast<WPARAM>(generation), 0);
        return true;
    }

    // The zoom keyframe `id` was rendered at, as its own RFM/RFMZ or RFSM file records it.
    float TimelineWindow::keyframeLogZoom(const uint32_t id) {
        const float increment = std::log10(std::max(attribute.video.data.defaultZoomIncrement, 1.000001f));
        // Without a keyframe to read, the zoom being explored is the only anchor the editor has.
        const float estimated =
            attribute.fractal.logZoom - static_cast<float>(static_cast<int64_t>(id) - 1) * increment;
        if (frameSource == nullptr || id == 0 || id > frameSource->getFrameCount()) {
            return estimated;
        }
        const uint32_t count = frameSource->getFrameCount();
        if (keyframeLogZooms.size() != static_cast<size_t>(count)) {
            keyframeLogZooms.assign(count, std::nullopt);
        }
        // Not a NaN kept in the vector: -ffast-math is on, and a test for one there is folded away.
        std::optional<float> &cached = keyframeLogZooms[id - 1];
        if (!cachePreloading.load()) {
            if (const auto zoom = frameSource->cachedLogZoom(id)) {
                cached = zoom;
                return *zoom;
            }
        }
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
        float sampled = depth;
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
            const double shownSeconds = schedule.timeAt(viewEndDepth) - schedule.timeAt(viewStartDepth);
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

        RECT box;
        const wchar_t *accessibleName;
        const wchar_t *accessibleId;
        long focusItem;
        switch (field) {
        case FieldEdit::DISTANCE:
            box = distanceField;
            accessibleName = L"Timeline Distance";
            accessibleId = L"timeline.distance";
            focusItem = workspace::TimelineItems::distance;
            break;
        case FieldEdit::TIME:
            box = timeField;
            accessibleName = L"Timeline Time";
            accessibleId = L"timeline.time";
            focusItem = workspace::TimelineItems::time;
            break;
        default:
            box = keyframeField;
            accessibleName = L"Timeline Keyframe";
            accessibleId = L"timeline.keyframe";
            focusItem = workspace::TimelineItems::keyframe;
            break;
        }
        if (box.right <= box.left || box.bottom <= box.top) {
            return;
        }
        const std::wstring value =
            field == FieldEdit::DISTANCE
                ? Unparser::floatTrim(4)(displayDistance(previewDepth))
                : (field == FieldEdit::TIME ? std::format(L"{:.6f}", previewSeconds()) : Unparser::floatTrim(4)(previewDepth));
        const RECT editRect = {box.left + sc(8), box.top + sc(9), box.right - sc(8), box.bottom - sc(4)};
        activeFieldEdit = field;
        fieldEdit = CreateWindowExW(
            0, WC_EDITW, value.c_str(), WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_CENTER, editRect.left,
            editRect.top, editRect.right - editRect.left, editRect.bottom - editRect.top, window, nullptr,
            GetModuleHandleW(nullptr), nullptr);
        if (fieldEdit == nullptr) {
            activeFieldEdit = FieldEdit::NONE;
            return;
        }
        SendMessageW(fieldEdit, WM_SETFONT, reinterpret_cast<WPARAM>(valueFont), TRUE);
        SendMessageW(fieldEdit, EM_SETLIMITTEXT, 128, 0);
        SendMessageW(fieldEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(sc(4), sc(4)));
        SetWindowSubclass(fieldEdit, fieldEditProc, 1, reinterpret_cast<DWORD_PTR>(this));
        workspace::AccessibleControl::describe(
            fieldEdit, accessibleName,
            L"Enter a number or formula. Press Enter to apply or Escape to cancel.", accessibleId);
        keyboardFocus = focusItem;
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
            workspace::AccessibleControl::validation(fieldEdit,
                                                     L"Enter a valid formula with a finite result.");
            MessageBeep(MB_ICONWARNING);
            SendMessageW(fieldEdit, EM_SETSEL, 0, -1);
            return false;
        }
        if (activeFieldEdit == FieldEdit::TIME &&
            text == std::format(L"{:.6f}", previewSeconds())) {
            closeFieldEdit();
            return true;
        }

        const double depth =
            activeFieldEdit == FieldEdit::DISTANCE ? depthFromDistance(static_cast<float>(*value))
            : activeFieldEdit == FieldEdit::TIME   ? schedule.depthAt(static_cast<float>(*value))
                                                   : *value;
        previewDepth =
            std::clamp(static_cast<float>(depth), schedule.getEndDepth(), schedule.getStartDepth());
        syncPlaybackClock();
        if (activeFieldEdit == FieldEdit::TIME) {
            playSeconds = std::clamp(*value, 0.0, schedule.getTotalSeconds());
            restartAudioPreview();
        }
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
        const int past =
            point.x < timelineAxis.left + margin    ? point.x - static_cast<int>(timelineAxis.left) - margin
            : point.x > timelineAxis.right - margin ? point.x - static_cast<int>(timelineAxis.right) + margin
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
        const int delta = point.y < timelineAxis.top + margin      ? -sc(6)
                          : point.y > timelineAxis.bottom - margin ? sc(6)
                                                                   : 0;
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
        const float held = lowest <= highest ? std::clamp(snapped, lowest, highest)
                                             : std::clamp(snapped, viewEndDepth, viewStartDepth);
        previewDepth = std::clamp(held, schedule.getEndDepth(), schedule.getStartDepth());
        syncPlaybackClock();
        InvalidateRect(window, nullptr, FALSE);
    }

    void TimelineWindow::syncPlaybackClock() {
        playSeconds = schedule.timeAt(previewDepth);
        if (playing) {
            playTick = GetTickCount64();
            restartAudioPreview();
        }
    }

    TimelineWindow::~TimelineWindow() {
        audioPreview.stop();
        exportStopSource.request_stop();
        inspector.reset();
        inspectorSplitter.reset();
        accessibility.reset();
        dockSplitter.reset();
        // The Shader panels this editor opened stand on rows held to what a track can carry, and
        // they write into a timeline that is going away, so they close with it. A panel opened from
        // the Shader menu is untouched and keeps every row it has.
        for (const HWND panel : recordingPanels) {
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

    void TimelineWindow::rememberWorkspaceSource() {
        if (embedded && sourceAttribute) {
            workspaceSource = timelineBytes(sourceAttribute->video.timeline);
        }
    }

    HWND TimelineWindow::createWorkspace(SettingsMenu &menu, RenderScene &scene, HWND parent, bool floating) {
        registerTimelineWindowClass();
        auto *timeline = new TimelineWindow(menu, scene);
        timeline->embedded = true;
        timeline->floatingWorkspace = floating;
        timeline->rememberWorkspaceSource();
        if (!timeline->create(parent)) {
            delete timeline;
            return nullptr;
        }
        timeline->initializeWorkspaceDock();
        return timeline->window;
    }

    void TimelineWindow::initializeWorkspaceDock() {
        if (dockToggle) {
            return;
        }
        dockPreferences = Utilities::getDefaultPath() / L"timeline-layout.txt";
        dockState.load(dockPreferences);
        dockToggle = CreateWindowExW(0, L"BUTTON", UiLanguage::label(L"Hide Tracks"),
                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW, 0, 0, 1, 1, window,
                                     reinterpret_cast<HMENU>(WORKSPACE_DOCK_TOGGLE),
                                     GetModuleHandleW(nullptr), nullptr);
        SendMessageW(dockToggle, WM_SETFONT, reinterpret_cast<WPARAM>(smallFont), FALSE);
        SetWindowSubclass(dockToggle, dockToggleProc, 1, reinterpret_cast<DWORD_PTR>(this));
        TOOLINFOW hint{sizeof(hint)};
        hint.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
        hint.hwnd = window;
        hint.uId = reinterpret_cast<UINT_PTR>(dockToggle);
        hint.lpszText = const_cast<wchar_t *>(L"Show or hide tracks while playback continues.");
        SendMessageW(fieldTooltip, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&hint));
        dockSplitter = std::make_unique<workspace::PaneSplitter>(
            window, true, L"Resize timeline tracks",
            [this] {
                if (exporting || draggingOverlay || draggingTrackKey || draggingTrackRow ||
                    fieldDrag != FieldDrag::NONE || !commitFieldEdit()) {
                    return false;
                }
                dockResizeStart = dockState;
                dockResizeHeight =
                    int(std::round((dockLayout.tracks.bottom - dockLayout.tracks.top) * 96.0 / uiDpi));
                return true;
            },
            [this](int delta) {
                const int maximum = std::clamp(int(dockLayout.maximumTracksHeight * 96.0 / uiDpi),
                                               workspace::TimelineDockState::minimumHeight,
                                               workspace::TimelineDockState::maximumHeight);
                const int height = std::clamp(dockResizeHeight - int(std::round(delta * 96.0 / uiDpi)),
                                              workspace::TimelineDockState::minimumHeight, maximum);
                dockState.height = delta == 0 || height == dockResizeHeight ? dockResizeStart.height : height;
                dockState.previewPercent =
                    delta == 0
                        ? dockResizeStart.previewPercent
                        : std::clamp(
                              int(std::lround(100.0 * (1.0 - UiDpi::pixels(height, uiDpi) /
                                                                 double(std::max(1, dockLayout.paneSpace))))),
                              10, 90);
                layoutWorkspaceDock();
            },
            [this](bool cancel) {
                if (cancel) {
                    dockState = dockResizeStart;
                    layoutWorkspaceDock();
                } else if (dockState != dockResizeStart) {
                    saveWorkspaceDock();
                }
            },
            [this] {
                if (!exporting) {
                    dockState = {};
                    layoutWorkspaceDock();
                    saveWorkspaceDock();
                }
            },
            [this](int direction) {
                keyboardFocus = workspace::TimelineItems::divider;
                tabItem(direction);
            },
            true);
        initializeInspector();
        layoutWorkspaceDock();
    }

    void TimelineWindow::layoutWorkspaceDock() {
        if (!dockToggle) {
            return;
        }
        accessibilityDirty = true;
        RECT client;
        GetClientRect(window, &client);
        client.right = layoutInspector(client.right, client.bottom);
        dockLayout =
            workspace::TimelineDockLayout::arrange(client.right, client.bottom, uiDpi, dockState,
                                                   embedded ? 0 : int(std::lround(sc(100) * 96.0 / uiDpi)),
                                                   embedded ? 160 : int(std::lround(sc(256) * 96.0 / uiDpi)));
        if (dockState.inspectorLeft) {
            for (RECT *rect : {&dockLayout.preview, &dockLayout.transport, &dockLayout.tracks,
                               &dockLayout.divider, &dockLayout.toggle}) {
                if (!IsRectEmpty(rect)) {
                    OffsetRect(rect, inspectorReservedWidth, 0);
                }
            }
        }
        dockSplitter->layout(dockLayout.divider, uiDpi / 96.f);
        const auto &button = dockLayout.toggle;
        SetWindowPos(dockToggle, HWND_TOP, button.left, button.top, std::max(1L, button.right - button.left),
                     std::max(1L, button.bottom - button.top), SWP_NOACTIVATE);
        const auto *caption = UiLanguage::label(dockLayout.tracksVisible ? L"Hide Tracks" : L"Show Tracks");
        wchar_t currentCaption[128]{};
        GetWindowTextW(dockToggle, currentCaption, 128);
        if (wcscmp(currentCaption, caption) != 0) {
            SetWindowTextW(dockToggle, caption);
        }
        const bool enabled = dockLayout.tracksAvailable && !exporting;
        if ((IsWindowEnabled(dockToggle) != FALSE) != enabled) {
            EnableWindow(dockToggle, enabled);
        }
        TOOLINFOW hint{sizeof(hint)};
        hint.hwnd = window;
        hint.uId = reinterpret_cast<UINT_PTR>(dockToggle);
        hint.lpszText = const_cast<wchar_t *>(
            !dockLayout.tracksAvailable ? L"Hide Settings or increase the window height to show tracks."
            : IsRectEmpty(&dockLayout.preview) ? L"Hide tracks to show the preview at this window height."
                                               : L"Show or hide tracks while playback continues.");
        SendMessageW(fieldTooltip, TTM_UPDATETIPTEXTW, 0, reinterpret_cast<LPARAM>(&hint));
        if (!dockLayout.tracksVisible) {
            timelineAxis = {};
            timelinePanel = {};
            rulerStrip = {};
            scrollTrack = {};
            scrollThumb = {};
            trackScrollTrack = {};
            trackScrollThumb = {};
            zoomPresetButton = {};
            trackLayouts.clear();
            reorderRowTargets.clear();
        }
        RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
    }

    void TimelineWindow::saveWorkspaceDock() {
        dockPreferencesFailed = !dockState.save(dockPreferences);
        InvalidateRect(window, nullptr, FALSE);
    }

    void TimelineWindow::toggleWorkspaceDock() {
        if (exporting) {
            return;
        }
        dockSplitter->cancel();
        dockState.collapsed = !dockState.collapsed;
        if (dockState.collapsed) {
            dockState.previewHidden = false;
        }
        layoutWorkspaceDock();
        saveWorkspaceDock();
    }

    LRESULT TimelineWindow::dockToggleProc(HWND hwnd, UINT message, WPARAM w, LPARAM l, UINT_PTR,
                                           DWORD_PTR data) {
        auto &self = *reinterpret_cast<TimelineWindow *>(data);
        // Owner drawing covers the button, so resizing must not erase it with the native background.
        if (message == WM_ERASEBKGND) {
            return 1;
        }
        if (message == WM_GETDLGCODE) {
            return DLGC_WANTTAB | DLGC_WANTCHARS;
        }
        if (message == WM_KEYDOWN && w == VK_TAB) {
            self.keyboardFocus = workspace::TimelineItems::toggle;
            self.tabItem(GetKeyState(VK_SHIFT) < 0 ? -1 : 1);
            return 0;
        }
        if (message == WM_KEYDOWN && w == VK_RETURN) {
            SendMessageW(hwnd, BM_CLICK, 0, 0);
            return 0;
        }
        if (message == WM_LBUTTONDOWN || message == WM_LBUTTONDBLCLK) {
            if (IsWindowEnabled(hwnd)) {
                if (GetFocus() != self.fieldEdit) {
                    SetFocus(hwnd);
                }
                SetCapture(hwnd);
                SendMessageW(hwnd, BM_SETSTATE, TRUE, 0);
            }
            return 0;
        }
        if ((message == WM_MOUSEMOVE || message == WM_LBUTTONUP) && GetCapture() == hwnd) {
            RECT rect;
            GetClientRect(hwnd, &rect);
            const POINT point{GET_X_LPARAM(l), GET_Y_LPARAM(l)};
            const bool inside = PtInRect(&rect, point) != FALSE;
            if (message == WM_MOUSEMOVE) {
                SendMessageW(hwnd, BM_SETSTATE, inside, 0);
                return 0;
            }
            ReleaseCapture();
            SendMessageW(hwnd, BM_SETSTATE, FALSE, 0);
            if (inside) {
                SendMessageW(self.window, WM_COMMAND, MAKEWPARAM(WORKSPACE_DOCK_TOGGLE, BN_CLICKED),
                             reinterpret_cast<LPARAM>(hwnd));
            }
            return 0;
        }
        if (message == WM_NCDESTROY) {
            RemoveWindowSubclass(hwnd, dockToggleProc, 1);
        }
        return DefSubclassProc(hwnd, message, w, l);
    }

    void TimelineWindow::applyWorkspaceDpi(HWND handle, UINT dpi) {
        auto *self = reinterpret_cast<TimelineWindow *>(GetWindowLongPtrW(handle, GWLP_USERDATA));
        if (self && dpi && !self->floatingWorkspace) {
            self->updateDpi(dpi);
        }
    }

    void TimelineWindow::updateDpi(UINT dpi) {
        if (!dpi || (dpi == uiDpi && titleFont && fontsEmbedded == embedded)) {
            return;
        }
        if (dockSplitter) {
            dockSplitter->cancel();
        }
        const TimelineDpiScope scope(dpi);
        HFONT *fontTargets[] = {&titleFont, &bodyFont, &smallFont, &captionFont, &valueFont};
        const int standaloneFontSizes[] = {30, 26, 22, 20, 25},
                  fontWeights[] = {FW_SEMIBOLD, FW_MEDIUM, FW_NORMAL, FW_NORMAL, FW_SEMIBOLD};
        const int workspaceFontSizes[] = {17, 14, 13, 12, 14};
        HFONT replacementFonts[5]{};
        for (int i = 0; i < 5; ++i) {
            const int height =
                embedded ? -UiDpi::pixels(workspaceFontSizes[i], dpi) : sc(standaloneFontSizes[i]);
            replacementFonts[i] =
                CreateFontW(height, 0, 0, 0, fontWeights[i], FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                            DEFAULT_PITCH | FF_SWISS, Constants::Win32::uiFontFace());
            if (!replacementFonts[i]) {
                for (auto font : replacementFonts) {
                    if (font) {
                        DeleteObject(font);
                    }
                }
                return;
            }
        }
        if (fieldEdit) {
            SendMessageW(fieldEdit, WM_SETFONT, reinterpret_cast<WPARAM>(replacementFonts[4]), FALSE);
            SendMessageW(fieldEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(sc(4), sc(4)));
        }
        for (int i = 0; i < 5; ++i) {
            if (*fontTargets[i]) {
                DeleteObject(*fontTargets[i]);
            }
            *fontTargets[i] = replacementFonts[i];
        }
        uiDpi = dpi;
        fontsEmbedded = embedded;
        if (inspector) {
            inspector->applyMetrics(bodyFont, uiDpi / 96.f);
        }
        if (dockToggle) {
            SendMessageW(dockToggle, WM_SETFONT, reinterpret_cast<WPARAM>(smallFont), FALSE);
            layoutWorkspaceDock();
        }
        if (window) {
            InvalidateRect(window, nullptr, FALSE);
        }
    }

    void TimelineWindow::syncWorkspace(HWND handle) {
        auto *self = reinterpret_cast<TimelineWindow *>(GetWindowLongPtrW(handle, GWLP_USERDATA));
        if (!self || !self->embedded || !self->sourceAttribute) {
            return;
        }
        const auto &source = *self->sourceAttribute;
        self->attribute.shader = source.shader;
        self->attribute.video.animation = source.video.animation;
        self->attribute.video.exportation = source.video.exportation;
        if (self->workspaceSource != timelineBytes(source.video.timeline) ||
            source.video.timeline.tracks.empty()) {
            self->attribute.video.timeline = source.video.timeline;
            self->ensureEditableTracks();
            self->linkColorCycle = colorCycleTracksLinked(self->attribute.video.timeline, true);
            self->undoBaseline = source.video.timeline;
            self->lastUndoStep = 0;
            self->selectedTrackKey = -1;
            self->hoveredTrackKey = {};
            self->rememberWorkspaceSource();
        }
        if (self->frameSource && self->frameSource->isStatic() != source.video.data.isStatic) {
            self->setPlaying(false);
            self->stopFramePreviewWorker();
            self->destroyFramePreview();
            self->frameSource.reset();
            std::scoped_lock lock(self->previewBitmapMutex);
            if (self->previewBitmap) {
                DeleteObject(self->previewBitmap);
                self->previewBitmap = nullptr;
            }
            self->previewMessage = L"Select a keyframe folder to enable scrubbing";
        }
        self->undoBaselineStatic = source.video.data.isStatic;
        self->attribute.video.data = source.video.data;
        self->attribute.fractal = source.fractal;
        self->recordBaseline = source.shader;
        self->rebuildSchedule();
        if (IsWindowVisible(handle)) {
            self->requestFramePreview();
        }
        InvalidateRect(handle, nullptr, FALSE);
    }

    void TimelineWindow::showWorkspace(HWND handle, const RECT &rect, bool visible) {
        auto *self = reinterpret_cast<TimelineWindow *>(GetWindowLongPtrW(handle, GWLP_USERDATA));
        if (!self || !self->embedded) {
            return;
        }
        const bool light = !darkSettingsMode();
        if (self->lightMode != light) {
            self->lightMode = light;
            self->refreshTheme();
        }
        if (!self->floatingWorkspace) {
            RECT current;
            GetWindowRect(handle, &current);
            MapWindowPoints(nullptr, GetParent(handle), reinterpret_cast<POINT *>(&current), 2);
            if (!EqualRect(&current, &rect)) {
                SetWindowPos(handle, HWND_TOP, rect.left, rect.top, std::max(1L, rect.right - rect.left),
                             std::max(1L, rect.bottom - rect.top), SWP_NOACTIVATE);
            }
        }
        const auto show = [&] {
            int displayCommand = SW_HIDE;
            if (visible) {
                if (!self->floatingWorkspace) {
                    displayCommand = SW_SHOWNA;
                } else if (IsIconic(handle)) {
                    displayCommand = SW_RESTORE;
                } else {
                    displayCommand = SW_SHOW;
                }
            }
            ShowWindow(handle, displayCommand);
            if (visible && self->floatingWorkspace) {
                SetForegroundWindow(handle);
            }
        };
        if (visible == self->mainPreviewPauseClaimed) {
            show();
            return;
        }
        if (visible) {
            syncWorkspace(handle);
            ++openTimelineWindows;
            self->mainPreviewPauseClaimed = true;
            self->renderScene->getRequests().shaderEditListener.store(handle, std::memory_order_release);
            show();
            if (self->frameSource && !self->previewContextAttached && !self->exporting) {
                (void)self->initializeFramePreview();
            }
        } else {
            self->setPlaying(false);
            self->closeFieldEdit();
            self->stopFramePreviewWorker();
            self->destroyFramePreview();
            for (HWND panel : self->recordingPanels) {
                if (IsWindow(panel)) {
                    DestroyWindow(panel);
                }
            }
            self->recordingPanels.clear();
            HWND listening = handle;
            self->renderScene->getRequests().shaderEditListener.compare_exchange_strong(listening, nullptr);
            --openTimelineWindows;
            self->mainPreviewPauseClaimed = false;
            ShowWindow(handle, SW_HIDE);
        }
    }

    uint64_t TimelineWindow::workspaceHistoryOrder(HWND handle, bool redo) {
        auto *self = reinterpret_cast<TimelineWindow *>(GetWindowLongPtrW(handle, GWLP_USERDATA));
        if (!self || !self->embedded) {
            return 0;
        }
        const auto &entries = redo ? self->redoSteps : self->undoSteps;
        return entries.empty() || (redo && !self->historyOrder.validRedo()) ? 0 : entries.back().serial;
    }

    void TimelineWindow::bindWorkspaceHistory(HWND handle, std::shared_ptr<workspace::HistoryDomain> domain,
                                              std::function<void(bool)> request) {
        auto *self = reinterpret_cast<TimelineWindow *>(GetWindowLongPtrW(handle, GWLP_USERDATA));
        if (!self || !self->embedded) {
            return;
        }
        self->undoSteps.clear();
        self->redoSteps.clear();
        self->historyOrder.bind(std::move(domain));
        self->historyRequest = std::move(request);
    }

    bool TimelineWindow::workspaceHistory(HWND handle, bool redo, bool execute) {
        if (!workspaceHistoryOrder(handle, redo)) {
            return false;
        }
        auto *self = reinterpret_cast<TimelineWindow *>(GetWindowLongPtrW(handle, GWLP_USERDATA));
        if (!execute) {
            return true;
        }
        return redo ? self->redoTimeline() : self->undoTimeline();
    }

    void TimelineWindow::workspaceAction(HWND handle, int action) {
        auto *self = reinterpret_cast<TimelineWindow *>(GetWindowLongPtrW(handle, GWLP_USERDATA));
        if (!self || !self->embedded) {
            return;
        }
        if (action == 0) {
            self->setPlaying(!self->playing);
        }
        if (action == 1) {
            self->stopPlayback();
        }
        if (action == 2) {
            self->loadKeyframeDirectory();
        }
    }

    bool TimelineWindow::loadWorkspaceKeyframes(HWND handle, const std::filesystem::path &directory) {
        auto *self = reinterpret_cast<TimelineWindow *>(GetWindowLongPtrW(handle, GWLP_USERDATA));
        if (!self || !self->embedded) {
            return false;
        }
        self->loadKeyframeDirectory(directory);
        return self->frameSource && self->frameSource->getDirectory() == directory;
    }

    bool TimelineWindow::workspacePreviewReady(HWND handle) {
        auto *self = reinterpret_cast<TimelineWindow *>(GetWindowLongPtrW(handle, GWLP_USERDATA));
        if (!self || !self->embedded) {
            return false;
        }
        std::scoped_lock lock(self->previewRequestMutex, self->previewBitmapMutex);
        return self->previewBitmap && !self->previewPending && !self->previewWorkerFailed &&
               self->publishedPreviewGeneration == self->previewRequestGeneration;
    }

    std::wstring TimelineWindow::workspaceStatus(HWND handle) {
        auto *self = reinterpret_cast<TimelineWindow *>(GetWindowLongPtrW(handle, GWLP_USERDATA));
        if (!self || !self->embedded) {
            return L"Timeline is unavailable.";
        }
        if (self->exporting) {
            return L"Exporting video...";
        }
        if (self->aiImagesBusy.load()) return self->aiTotalPages.load()
            ? std::format(L"{} {}/{}", UiLanguage::text(L"Saving image pages:"), self->aiSavedPages.load(), self->aiTotalPages.load())
            : UiLanguage::text(L"Preparing image sheet...");
        if (self->cachePreloading.load()) {
            std::scoped_lock lock(self->previewBitmapMutex);
            return self->cacheMessage;
        }
        if (self->previewBusy) {
            return L"Rendering keyframe preview...";
        }
        if (!self->frameSource) {
            return L"Load keyframes to preview the timeline.";
        }
        return std::wstring(self->playing ? L"Timeline playing. " : L"Timeline paused. ") +
               durationText(self->previewSeconds());
    }

    bool TimelineWindow::create(const HWND owner) {
        const UiDpi::AwarenessScope awareness(true);
        updateDpi(UiDpi::forWindow(owner));
        const TimelineDpiScope dpiScope(uiDpi);
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
        const bool childWindow = embedded && !floatingWorkspace;
        window = CreateWindowExW(childWindow ? 0 : WS_EX_APPWINDOW, TIMELINE_WINDOW_CLASS,
                                 UiLanguage::label(L"RFF_Super - Timeline Editor"),
                                 (childWindow ? WS_CHILD | WS_CLIPSIBLINGS : WS_OVERLAPPEDWINDOW) |
                                     WS_CLIPCHILDREN,
                                 childWindow ? 0 : x, childWindow ? 0 : y, width, height, owner, nullptr,
                                 GetModuleHandleW(nullptr), this);
        if (window == nullptr) {
            return false;
        }
        updateDpi(UiDpi::forWindow(window));
        const TimelineDpiScope windowDpiScope(uiDpi);
        fieldTooltip = CreateWindowExW(
            WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, window, nullptr, GetModuleHandleW(nullptr), nullptr);
        if (fieldTooltip != nullptr) {
            SetWindowPos(fieldTooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            SendMessageW(fieldTooltip, TTM_SETMAXTIPWIDTH, 0, sc(520));
            SendMessageW(fieldTooltip, TTM_SETDELAYTIME, TTDT_INITIAL, 350);
            for (UINT_PTR id = 1; id <= 3; ++id) {
                TOOLINFOW tool = {};
                tool.cbSize = sizeof(tool);
                tool.uFlags = TTF_SUBCLASS;
                tool.hwnd = window;
                tool.uId = id;
                tool.lpszText = const_cast<wchar_t *>(FORMULA_FIELD_HINT);
                SendMessageW(fieldTooltip, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&tool));
            }
        }
        accessibility = std::make_unique<workspace::AccessibleItems>(
            window, [] { return L"Timeline"; }, [this] { return accessibleItems(); },
            [this](long id, bool activate) { return activate ? activateItem(id) : focusItem(id); },
            [this](long id, std::wstring_view value) { return writeItem(id, value); });
        initializeWorkspaceDock();
        if (embedded) {
            return true;
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

    void TimelineWindow::restartAudioPreview() {
        if (!playing) return;
        playOriginSeconds = playSeconds;
        playTick = GetTickCount64();
        if (!audioPreview.start(attribute.video.timeline.audio, playSeconds, schedule.getTotalSeconds())) {
            setPlaying(false);
            NativeDialogs::message(window, L"Cannot start audio preview. Check the audio files and FFmpeg installation.", L"Audio Preview", MB_OK | MB_ICONERROR);
        }
    }

    void TimelineWindow::setPlaying(const bool play) {
        if (play && inspector && !inspector->applyPending()) return;
        if (play && cachePreloading.load()) {
            return;
        }
        if (play == playing) {
            return;
        }
        playing = play;
        accessibilityDirty = true;
        if (!playing) {
            audioPreview.stop();
            KillTimer(window, PLAYBACK_TIMER);
            return;
        }
        // Starting at the end replays from the top rather than sitting still on the last frame.
        playSeconds = previewSeconds();
        if (playSeconds >= schedule.getTotalSeconds() - 1e-3f) {
            playSeconds = 0.0f;
            previewDepth = schedule.getStartDepth();
        }
        playTick = GetTickCount64();
        SetTimer(window, PLAYBACK_TIMER, PLAYBACK_INTERVAL, nullptr);
        restartAudioPreview();
    }

    void TimelineWindow::stopPlayback() {
        cacheStop.request_stop();
        accessibilityDirty = true;
        setPlaying(false);
        playSeconds = 0.0f;
        previewDepth = schedule.getStartDepth();
        requestFramePreview();
    }

    void TimelineWindow::advancePlayback() {
        if (audioPreview.takeFailure()) {
            setPlaying(false);
            const auto message = L"Audio preview stopped. Check the audio file, FFmpeg and the Windows output device.\n\nLog: " + audioPreview.diagnosticLog().wstring();
            NativeDialogs::message(window, message.c_str(), L"Audio Preview", MB_OK | MB_ICONERROR);
            return;
        }
        const ULONGLONG now = GetTickCount64();
        const double total = std::max(schedule.getTotalSeconds(), 1e-3);
        playSeconds = TimelineTime::elapsed(playOriginSeconds, playTick, now);
        if (playSeconds >= total) {
            if (loopPlayback) {
                playSeconds = std::fmod(playSeconds, total);
                restartAudioPreview();
            } else {
                playSeconds = total;
                setPlaying(false);
            }
        }
        previewDepth = schedule.depthAt(playSeconds);
        requestFramePreview(playSeconds);
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
            const std::wstring item = PERCENTS[i] == 100 ? std::format(L"{}%  (fit)", PERCENTS[i])
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

    void TimelineWindow::cancelAudioDrag() {
        if (!draggingAudio) return;
        auto &clips = attribute.video.timeline.audio.clips;
        const auto clip = std::ranges::find(clips, audioDragBefore.id, &VidAudioClip::id);
        if (clip != clips.end()) *clip = audioDragBefore;
        draggingAudio = false;
        InvalidateRect(window, nullptr, FALSE);
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

        if (rowTarget == AUDIO_ROW_TARGET) {
            selectTrackRow(AUDIO_ROW_TARGET, false, false);
            showInspectorSection(1);
            return;
        }
        const HMENU menu = CreatePopupMenu();
        if (menu == nullptr) {
            return;
        }
        constexpr int CMD_ADD_KEY = 1;
        constexpr int CMD_DELETE_KEY = 2;
        constexpr int CMD_REMOVE_TRACK = 3;
        constexpr int CMD_AUDIO = 4;
        constexpr int CMD_INTERPOLATION = 10;
        constexpr int CMD_PARAMETER = 100;
        constexpr int INTERPOLATION_COUNT = 4;

        const VidTimelineTrack *current = rowTarget == UINT16_MAX ? nullptr : track(rowTarget);
        if (hit.valid() && current != nullptr) {
            const HMENU interpolations = CreatePopupMenu();
            for (int i = 0; i < INTERPOLATION_COUNT; ++i) {
                const auto mode = static_cast<VidKeyInterpolation>(i);
                AppendMenuW(interpolations,
                            MF_STRING | (current->keys[hit.keyIndex].out == mode ? MF_CHECKED : 0),
                            CMD_INTERPOLATION + i, UiLanguage::text(interpolationName(mode)).c_str());
            }
            AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(interpolations),
                        UiLanguage::label(L"To Next Key"));
            AppendMenuW(menu, MF_STRING, CMD_DELETE_KEY, UiLanguage::label(L"Delete Key"));
            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        } else if (rowTarget != UINT16_MAX && editableTarget(rowTarget) && layout(rowTarget) != nullptr &&
                   contains(layout(rowTarget)->row, point)) {
            AppendMenuW(menu, MF_STRING, CMD_ADD_KEY, UiLanguage::label(L"Add Key Here"));
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
                        UiLanguage::label(SHADER_PANELS[i].name));
        }
        AppendMenuW(parameters, MF_STRING, CMD_AUDIO, UiLanguage::label(L"Audio"));
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(parameters), UiLanguage::label(L"Parameters"));
        const bool removable = current != nullptr && rowTarget != SPEED_TARGET;
        const std::wstring removeItem = rowTarget == UINT16_MAX
                                            ? std::wstring(L"Remove Parameter")
                                            : L"Remove " + rowName(rowTarget, linkColorCycle);
        AppendMenuW(menu, MF_STRING | (removable ? 0 : MF_GRAYED), CMD_REMOVE_TRACK,
                    UiLanguage::text(removeItem).c_str());

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
        case CMD_AUDIO:
            showInspectorSection(1);
            break;
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

    void TimelineWindow::openControlsGuide() {
        if (!commitFieldEdit()) {
            return;
        }
        controlsGuide.reset();
        controlsGuide = std::make_unique<SettingsWindow>(L"Timeline Controls", 620);
        controlsGuide->registerNotesCard(
            L"Navigate",
            {{L"Preview",
              L"Drag an empty track area to scrub. Space plays or pauses when a track or key is focused."},
             {L"View",
              L"Wheel zooms. Shift+Wheel pans. Ctrl+Wheel scrolls tracks. Press 0 to fit the full timeline."},
             {L"Keyboard", L"Tab moves between controls. Up/Down selects tracks; Left/Right selects keys. F6 "
                           L"moves between workspace panes."}});
        controlsGuide->registerNotesCard(
            L"Edit", {{L"Keys", L"Double-click a track or press Insert to add a key. Enter or F2 edits the "
                                L"selected key. Delete removes the selected key or track."},
                      {L"Tracks", L"Drag a track name or press Alt+Up/Down to reorder. Shift+Up/Down extends "
                                  L"the selection. Right-click or Shift+F10 opens the track menu."},
                      {L"Values", L"Click Distance, Keyframe or Time to enter a formula. Enter applies it; "
                                  L"Esc cancels. Zoom is read-only."}});
        std::wstring status;
        {
            std::scoped_lock lock(previewBitmapMutex);
            status = cacheMessage.empty() ? previewMessage : cacheMessage;
        }
        if (previewBusy && !cachePreloading.load()) {
            status = std::format(L"Rendering distance {:.2f}.", displayDistance(previewBusyDepth));
        }
        const std::wstring source =
            frameSource ? frameSource->getDirectory().filename().wstring() : L"No keyframe folder selected.";
        controlsGuide->registerNotesCard(L"Preview", {{L"Status", status}, {L"Source folder", source}});
        controlsGuide->setWindowCloseFunction([] {});
        adoptPanel(*controlsGuide);
    }

    void TimelineWindow::openShaderPanel(const size_t index) {
        if (index < SHADER_PANELS.size()) {
            showParameterCatalog(SHADER_PANELS[index].groupPrefix);
        }
    }

    bool TimelineWindow::recording() const {
        return std::ranges::any_of(recordingPanels, [](const HWND open) { return IsWindow(open) != FALSE; });
    }

    void TimelineWindow::recordShaderEdits() {
        cancelAudioDrag();
        if (sourceAttribute == nullptr) {
            return;
        }
        if (timelineBytes(attribute.video.timeline) != timelineBytes(sourceAttribute->video.timeline)) {
            attribute.video.timeline = sourceAttribute->video.timeline;
            commitTimeline();
        }
        const ShaderAttribute &now = sourceAttribute->shader;
        // The editor draws its rows against its own copy, so it follows the settings either way.
        attribute.shader = now;
        if (!recording()) {
            // Nothing of this editor's is open on the shader, so an edit made elsewhere is only
            // read by the next preview, as it always was.
            recordBaseline = now;
            requestFramePreview();
            return;
        }
        std::vector<const TimelineParamDesc *> changed;
        for (const auto &param : TimelineParams::all()) {
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
            if (!changed.empty()) {
                requestFramePreview();
            }
            return;
        }
        for (const TimelineParamDesc *param : changed) {
            if (param->kind == TimelineParamKind::COLOR) {
                setParameterColor(param->id, param->getColor(now));
            } else {
                setParameterValue(param->id, param->getValue(now));
            }
        }
    }

    void TimelineWindow::paint(const HDC target, const RECT &client) {
        audioClipLayouts.clear();
        audioLane = {};
        const TimelineDpiScope dpiScope(uiDpi);
        const TimelineTheme &theme = timelineTheme(lightMode);
        const int width = std::max(1, int(client.right - client.left) - inspectorReservedWidth);
        const int height = client.bottom - client.top;
        const HDC canvas = paintBuffer.begin(target, client.right - client.left, height);
        if (!canvas) {
            return;
        }
        fillRect(canvas, client, theme.background);

        // One spacing value for the window edges, the header inset and the gaps between the three
        // panels, so nothing sits closer to its neighbour than anything else does.
        const auto dip = [this](int value) { return UiDpi::pixels(value, uiDpi); };
        const bool narrow = width < dip(700);
        const bool narrowHeader = embedded && width < dip(656);
        const int margin = embedded ? dip(12) : sc(12);
        const int headerHeight = embedded ? dip(narrowHeader ? 96 : 48) : sc(88);
        const int transportHeight = sc(72);
        const int rulerRowHeight = embedded ? dip(24) : sc(34);
        const int axisTopInset = rulerRowHeight * 2 + sc(8);
        const int footerHeight = embedded ? dip(32) : sc(46);
        const int zoomRowHeight = embedded ? dip(28) : sc(38);
        const int scrollHeight = sc(12);
        const int minRowHeight = embedded ? dip(36) : sc(52);
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

        const int contentLeft = dockState.inspectorLeft ? inspectorReservedWidth : 0;
        RECT header = {contentLeft, 0, contentLeft + width, headerHeight};
        fillRect(canvas, header, theme.panel);
        const int boxTop = embedded ? dip(8) : sc(30);
        const int boxBottom = headerHeight - (embedded ? dip(8) : sc(16));
        const int buttonTop = boxTop + (embedded ? 0 : sc(2));
        const int buttonBottom = boxBottom - (embedded ? 0 : sc(2));
        const int headerInset = margin + sc(20);
        aiButton = {width - headerInset - sc(96), buttonTop, width - headerInset, buttonBottom};
        exportButton = {aiButton.left - sc(10) - sc(112), buttonTop, aiButton.left - sc(10), buttonBottom};
        saveButton = {exportButton.left - sc(10) - sc(112), buttonTop, exportButton.left - sc(10),
                      buttonBottom};
        loadButton = {saveButton.left - sc(10) - sc(96), buttonTop, saveButton.left - sc(10), buttonBottom};
        fullscreenButton = embedded ? RECT{}
                                    : RECT{loadButton.left - sc(20) - sc(104), buttonTop,
                                           loadButton.left - sc(20), buttonBottom};
        const int framesRight = (embedded ? loadButton.left : fullscreenButton.left) - sc(20);
        framesButton = {framesRight - sc(146), boxTop, framesRight, boxBottom};
        themeButton = embedded ? RECT{}
                               : RECT{framesButton.left - sc(20) - sc(126), boxTop,
                                      framesButton.left - sc(20), boxBottom};
        if (narrowHeader) {
            const int right = width - margin;
            aiButton = {right - dip(76), dip(56), right, dip(84)};
            exportButton = {aiButton.left - dip(84), dip(56), aiButton.left - dip(8), dip(84)};
            saveButton = {exportButton.left - dip(84), dip(56), exportButton.left - dip(8), dip(84)};
            loadButton = {saveButton.left - dip(84), dip(56), saveButton.left - dip(8), dip(84)};
            framesButton = {right - dip(136), dip(8), right, dip(40)};
        }

        for (RECT *rect :
             {&aiButton, &exportButton, &saveButton, &loadButton, &fullscreenButton, &framesButton, &themeButton}) {
            if (!IsRectEmpty(rect)) {
                OffsetRect(rect, contentLeft, 0);
            }
        }
        if (!embedded) {
            const RECT inner = drawCaptionBox(canvas, themeButton, L"Theme", theme.panel, captionFont,
                                              hoverTheme, lightMode, theme);
            const int mid = static_cast<int>(inner.top + inner.bottom) / 2;
            const RECT toggle = {inner.right - sc(46), mid - sc(11), inner.right, mid + sc(11)};
            drawToggle(canvas, toggle, lightMode, theme);
            drawText(canvas, lightMode ? L"Light" : L"Dark",
                     {inner.left, inner.top, toggle.left - sc(8), inner.bottom},
                     lightMode ? theme.accentText : theme.text, DT_LEFT | DT_VCENTER | DT_SINGLELINE,
                     bodyFont);
        }
        {
            const bool loaded = frameSource != nullptr;
            const RECT inner = drawCaptionBox(canvas, framesButton, L"Keyframes", theme.panel, captionFont,
                                              hoverFrames, loaded, theme);
            drawText(canvas,
                     loaded ? std::format(L"{} loaded", frameSource->getFrameCount()) : L"Select folder",
                     inner, loaded ? theme.accentText : theme.text,
                     DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, bodyFont);
        }
        if (!embedded) {
            drawButton(canvas, fullscreenButton, fullscreen ? L"WINDOW" : L"FULL", hoverFullscreen,
                       fullscreen, smallFont, theme);
        }
        drawButton(canvas, loadButton, L"Load", hoverLoad, false, smallFont, theme);
        drawButton(canvas, saveButton, L"Save", hoverSave, false, smallFont, theme);
        drawButton(canvas, aiButton, L"AI Edit", hoverAi, aiImagesBusy.load(), smallFont, theme);
        drawButton(canvas, exportButton, exporting ? L"Exporting" : L"Export", hoverExport, true, smallFont,
                   theme);

        RECT previewPanel = {margin, contentTop, width - margin, contentTop + previewHeight};
        if (dockToggle) {
            previewPanel = dockLayout.preview;
        }
        if (!IsRectEmpty(&previewPanel)) {
            fillRect(canvas, previewPanel, theme.previewBackground);
            frameRect(canvas, previewPanel, theme.border);
        }
        overlayImageRect = {};
        RECT preview = previewPanel;
        preview.left += sc(10);
        preview.right -= sc(10);
        preview.top += sc(10);
        preview.bottom -= sc(10);
        if (!IsRectEmpty(&preview)) {
            std::scoped_lock lock(previewBitmapMutex);
            if (!cacheMessage.empty()) {
                RECT notice = preview;
                notice.bottom = std::min(preview.bottom, preview.top + sc(cachePreloading.load() ? 48 : 24));
                drawText(canvas, cacheMessage, notice, theme.mutedText, DT_CENTER | DT_WORDBREAK, smallFont);
                preview.top = notice.bottom;
            }
            if (shortsGuide.visible && shortsGuide.showDescriptions &&
                preview.bottom - preview.top > sc(100)) {
                RECT description = preview;
                description.bottom = description.top + sc(20);
                drawText(canvas, L"Top: UI display area", description, theme.mutedText,
                         DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS, smallFont);
                preview.top = description.bottom;
                description.top = preview.bottom - sc(60);
                description.bottom = description.top + sc(20);
                drawText(canvas, L"Bottom: title / description area", description, theme.mutedText,
                         DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS, smallFont);
                preview.bottom = description.top;
                description.top += sc(20);
                description.bottom += sc(20);
                drawText(canvas, L"Right: controls", description, theme.mutedText,
                         DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS, smallFont);
                description.top += sc(20);
                description.bottom += sc(20);
                drawText(canvas, L"YouTube Shorts guide (approximate)", description, theme.mutedText,
                         DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS, smallFont);
            }
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
                overlayImageRect = image;
                if (!overlayRenderer) {
                    overlayRenderer = std::make_unique<ZoomOverlay>();
                }
                overlayRenderer->paint(canvas, image, publishedPreviewZoom,
                                       attribute.video.timeline.zoomOverlay);
                shortsGuide.paint(canvas, image);
                if (overlayPositionMode && attribute.video.timeline.zoomOverlay.visible) {
                    frameRect(canvas, image, theme.accentText);
                    const auto b = overlayRenderer->bounds();
                    const int saved = SaveDC(canvas);
                    if (saved != 0) {
                        IntersectClipRect(canvas, image.left, image.top, image.right, image.bottom);
                        frameRect(canvas,
                                  {image.left + b.x, image.top + b.y, image.left + b.x + b.width,
                                   image.top + b.y + b.height},
                                  theme.accentText);
                        RestoreDC(canvas, saved);
                    }
                }
                if (!overlayRenderer->status().empty()) {
                    drawText(canvas, overlayRenderer->status(),
                             {image.left, image.bottom - dip(22), image.right, image.bottom}, theme.text,
                             DT_SINGLELINE | DT_END_ELLIPSIS, smallFont);
                }
            } else {
                // An empty panel named for a render read as one already loaded, so with no keyframe
                // folder behind it the panel says that instead of naming what it would hold.
                if (frameSource == nullptr) {
                    overlayImageRect = preview;
                    if (!overlayRenderer) {
                        overlayRenderer = std::make_unique<ZoomOverlay>();
                    }
                    overlayRenderer->paint(canvas, preview, 100, attribute.video.timeline.zoomOverlay);
                    shortsGuide.paint(canvas, preview);
                }
                drawText(canvas,
                         frameSource == nullptr ? L"SAMPLE OVERLAY - NO KEYFRAMES LOADED"
                                                : L"CURRENT RENDER PREVIEW",
                         preview, theme.mutedText, DT_CENTER | DT_VCENTER | DT_SINGLELINE, bodyFont);
            }
        }

        RECT transport = {margin, previewPanel.bottom + margin, width - margin,
                          previewPanel.bottom + margin + transportHeight};
        if (dockToggle) {
            transport = dockLayout.transport;
        }
        if (dockToggle && (dockPreferencesFailed || !dockLayout.tracksAvailable) &&
            preview.bottom - preview.top >= dip(24)) {
            RECT notice = preview;
            notice.bottom = notice.top + dip(24);
            fillRect(canvas, notice, theme.panel);
            drawText(canvas,
                     dockPreferencesFailed ? L"Timeline layout was not saved."
                                           : L"Hide Settings or increase the window height to show tracks.",
                     notice, theme.text, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, smallFont);
        }
        fillRect(canvas, transport, theme.panel);
        frameRect(canvas, transport, theme.border);
        const float startDepth = schedule.getStartDepth();
        const float zoomExponent = zoomExponentAt(previewDepth);
        const std::wstring fields[4][2] = {
            {L"Distance", std::format(L"{:.1f}", displayDistance(previewDepth))},
            {L"Keyframe", std::format(L"{:.1f}", previewDepth)},
            // Read off the keyframe files; with none open the depth axis is a placeholder and the
            // zoom extrapolated along it says nothing, so the readout stands empty until one is.
            {L"Zoom",
             frameSource == nullptr ? std::wstring(L"\u2014") : std::format(L"1E{:.1f}", zoomExponent)},
            {L"Time", std::format(L"{} / {}", durationText(previewSeconds()),
                                  durationText(schedule.getTotalSeconds()))},
        };
        RECT *const fieldRects[4] = {&distanceField, &keyframeField, &zoomField, &timeField};
        // The readouts share one width and one gap, so the row is spaced as evenly as it reads.
        std::array<int, 4> fieldOrder = narrow ? std::array{0, 3, 1, 2} : std::array{0, 1, 2, 3};
        if (narrow && activeFieldEdit == FieldEdit::KEYFRAME) {
            fieldOrder = {1, 0, 3, 2};
        }
        if (narrow && activeFieldEdit == FieldEdit::TIME) {
            fieldOrder = {3, 0, 1, 2};
        }
        const int transportRight =
            dockToggle && !narrow ? dockLayout.toggle.left - dip(8) : transport.right - dip(12);
        const auto transportLayout = workspace::TimelineTransportLayout::arrange(
            transport, transportRight, uiDpi, narrow, textWidth(canvas, fields[3][1], valueFont), fieldOrder,
            std::max(dip(14), fontHeight(canvas, captionFont)) + fontHeight(canvas, valueFont) + dip(7));
        playButton = transportLayout.buttons[0];
        pauseButton = transportLayout.buttons[1];
        stopButton = transportLayout.buttons[2];
        loopButton = transportLayout.buttons[3];
        drawTransportButton(canvas, playButton, TransportGlyph::PLAY, hoverPlay, playing, theme);
        drawTransportButton(canvas, pauseButton, TransportGlyph::PAUSE, hoverPause, false, theme);
        drawTransportButton(canvas, stopButton, TransportGlyph::STOP, hoverStop, false, theme);
        drawTransportButton(canvas, loopButton, TransportGlyph::LOOP, hoverLoop, loopPlayback, theme);
        fillRect(canvas, transportLayout.separator, theme.border);
        RECT activeEditRect{};
        for (int i = 0; i < 4; ++i) {
            *fieldRects[i] = transportLayout.fields[i];
        }
        for (int i : fieldOrder) {
            const RECT box = transportLayout.fields[i];
            if (IsRectEmpty(&box)) {
                continue;
            }
            // The boxes that carry the playhead light up while one of them is being dragged.
            const bool edited = activeFieldEdit == FieldEdit::DISTANCE ? i == 0
                                : activeFieldEdit == FieldEdit::TIME
                                    ? i == 3
                                    : activeFieldEdit == FieldEdit::KEYFRAME && i == 1;
            const bool dragged =
                fieldDrag == FieldDrag::DEPTH ? i == 0 || i == 1 : fieldDrag == FieldDrag::TIME && i == 3;
            const bool hovered = hoveredFieldEdit == FieldEdit::DISTANCE ? i == 0
                                 : hoveredFieldEdit == FieldEdit::TIME
                                     ? i == 3
                                     : hoveredFieldEdit == FieldEdit::KEYFRAME && i == 1;
            const RECT inner = drawTransportReadout(canvas, box, fields[i][0], captionFont, i != 2, hovered,
                                                    dragged || edited, theme);
            if (edited) {
                activeEditRect = inner;
            } else {
                drawText(canvas, fields[i][1], inner, dragged ? theme.selectedText : theme.text,
                         DT_CENTER | DT_VCENTER | DT_SINGLELINE, valueFont);
            }
        }
        std::wstring status;
        if (activeFieldEdit != FieldEdit::NONE) {
            status = L"Enter / Esc";
        } else if (hoveredFieldEdit != FieldEdit::NONE) {
            status = L"Click to edit";
        } else if (dockToggle && !dockLayout.tracksAvailable) {
            status = L"Increase the window height to show tracks.";
        } else if (dockPreferencesFailed) {
            status = L"Timeline layout was not saved.";
        } else if (aiImagesBusy.load()) {
            status = aiTotalPages.load()
                ? std::format(L"{} {}/{}", UiLanguage::text(L"Saving image pages:"), aiSavedPages.load(), aiTotalPages.load())
                : UiLanguage::text(L"Preparing image sheet...");
        } else if (cachePreloading.load()) {
            status = L"Preloading previews to RAM:";
        } else if (previewBusy) {
            status = L"Rendering";
        } else {
            std::scoped_lock lock(previewBitmapMutex);
            status = previewMessage;
        }
        const auto visibleStatus =
            fittingText(canvas, smallFont, transportLayout.status.right - transportLayout.status.left,
                        {status, L"See Controls"});
        drawText(canvas, visibleStatus, transportLayout.status, theme.mutedText,
                 DT_RIGHT | DT_VCENTER | DT_SINGLELINE, smallFont);

        RECT timeline = {margin, transport.bottom + margin, width - margin,
                         transport.bottom + margin + timelineHeight};
        if (dockToggle) {
            if (!dockLayout.tracksVisible) {
                controlsButton = {};
                overlayButton = {};
                presentPaint(target, activeEditRect);
                return;
            }
            timeline = dockLayout.tracks;
        }
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
        for (const auto &track : attribute.video.timeline.tracks) {
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
        const int trackValueWidth = embedded ? dip(52) : sc(66);
        const int nameInset = rowTextLeft - timeline.left;
        const int nameGutters = nameInset + sc(20) + sc(10) + (narrow ? 0 : sc(12) + trackValueWidth);
        int desiredLabelWidth = embedded ? dip(240) : sc(360);
        for (const auto *track : displayedTracks) {
            desiredLabelWidth =
                std::max(desiredLabelWidth,
                         nameGutters + textWidth(canvas, rowName(track->targetId, linkColorCycle), bodyFont));
        }
        const int labelLimit =
            embedded ? std::max(nameGutters + dip(84), std::min(dip(340), width * 2 / 5)) : sc(440);
        const int labelWidth = std::min(desiredLabelWidth, labelLimit);
        const int trackNameWidth = std::max(dip(32), labelWidth - nameGutters);
        int readableRowHeight = minRowHeight;
        for (const auto *track : displayedTracks) {
            readableRowHeight = std::max(readableRowHeight,
                                         wrappedTextHeight(canvas, rowName(track->targetId, linkColorCycle),
                                                           bodyFont, trackNameWidth) +
                                             sc(16) + (narrow ? fontHeight(canvas, smallFont) + dip(2) : 0));
        }
        const size_t audioPosition = audioRowIndex < 0 ? displayedTracks.size() : std::min<size_t>(audioRowIndex, displayedTracks.size());
        displayedTracks.insert(displayedTracks.begin() + audioPosition, nullptr);
        const int trackCount = static_cast<int>(displayedTracks.size());
        const int guideTop = timeline.bottom - footerHeight;
        const int zoomRowTop = guideTop - zoomRowHeight;
        const int axisTop = timeline.top + axisTopInset;
        const int axisBottom = zoomRowTop - sc(6);
        const int viewHeight = std::max(axisBottom - axisTop, 1);
        // The rows share the view four ways at the most: past that a row is too short to read the
        // value or to drag a key on, so the stack scrolls at a quarter of the view rather than
        // thinning every row further towards nothing.
        constexpr int maxVisibleTracks = 4;
        const int trackRowHeight = std::max(readableRowHeight, viewHeight / maxVisibleTracks);
        const bool tracksScroll = trackCount * trackRowHeight > viewHeight;
        // The axis stops short of the right column whether the rows scroll or not, so the bar that
        // column holds keeps its place and widening the window never drops it.
        timelineAxis = {timeline.left + labelWidth, axisTop, rightColumnLeft - sc(16), axisBottom};
        RECT &axis = timelineAxis;
        trackLayouts.clear();
        trackScrollTrack = {};
        trackScrollThumb = {};
        if (axis.right <= axis.left || axis.bottom <= axis.top) {
            presentPaint(target, activeEditRect);
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
        drawText(canvas, L"Distance (d)",
                 {rowIconLeft, distanceRowTop, axis.left - sc(12), distanceRowTop + rulerRowHeight},
                 theme.mutedText, DT_LEFT | DT_VCENTER | DT_SINGLELINE, smallFont);
        drawText(canvas, L"Time", {rowIconLeft, timeRowTop, axis.left - sc(12), timeRowTop + rulerRowHeight},
                 theme.mutedText, DT_LEFT | DT_VCENTER | DT_SINGLELINE, smallFont);
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
                const int minorX =
                    depthX(depthFromDistance(distance + tickStep * static_cast<float>(minor) / 5.0f),
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
                     {x - sc(62), distanceRowTop, x + sc(62), timeRowTop - sc(4)}, theme.mutedText,
                     DT_CENTER | DT_VCENTER | DT_SINGLELINE, smallFont);
            drawText(canvas, durationText(schedule.timeAt(depth)),
                     {x - sc(58), timeRowTop, x + sc(58), timeRowTop + rulerRowHeight}, theme.mutedText,
                     DT_CENTER | DT_VCENTER | DT_SINGLELINE, smallFont);
        }

        // Every row is carried by its name cell to any place in the stack, the Speed row included.
        reorderRowTargets.clear();
        for (const VidTimelineTrack *item : displayedTracks) {
            reorderRowTargets.push_back(item ? item->targetId : AUDIO_ROW_TARGET);
        }
        // Every track reads the same, so every row gets the same height.
        const int rowHeight = tracksScroll ? trackRowHeight : viewHeight / trackCount;
        const int contentHeight = trackCount * rowHeight;
        trackScrollRange = std::max(contentHeight - viewHeight, 0);
        trackScrollOffset = std::clamp(trackScrollOffset, 0, trackScrollRange);
        // Rows are drawn at their own height wherever the scroll puts them, and cut at the view.
        const int trackClipRight = static_cast<int>(axis.right) + sc(10);
        const HRGN trackClip = CreateRectRgn(timeline.left + sc(2), axis.top, trackClipRight, axis.bottom);
        if (trackClip == nullptr) {
            return;
        }
        if (SelectClipRgn(canvas, trackClip) == ERROR) {
            DeleteObject(trackClip);
            return;
        }
        try {
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
                const VidTimelineTrack *track =
                    row < static_cast<int>(displayedTracks.size()) ? displayedTracks[row] : nullptr;
                if (!track) {
                    const RECT labelCell{timeline.left + sc(8), top + sc(4), axis.left - sc(20), bottom - sc(4)};
                    if (trackRowSelected(AUDIO_ROW_TARGET)) fillRoundRect(canvas, labelCell, theme.panelRaised, theme.accentText, sc(8));
                    if (draggingTrackRow && trackRowDragMoved && std::ranges::find(carriedRows, AUDIO_ROW_TARGET) != carriedRows.end()) {
                        fillRoundRect(canvas, labelCell, theme.accentSoft, theme.accentBorder, sc(8));
                    }
                    trackLayouts.push_back({.targetId = AUDIO_ROW_TARGET, .row = {axis.left, top, axis.right, bottom},
                                            .label = labelCell, .editable = false, .minValue = 0, .maxValue = 1, .order = row});
                    audioLane = {axis.left, std::max<LONG>(top, axis.top), axis.right, std::min<LONG>(bottom, axis.bottom)};
                    drawText(canvas, L"Audio", {rowTextLeft, top, axis.left - sc(20), bottom}, theme.accentText,
                             DT_LEFT | DT_VCENTER | DT_SINGLELINE, bodyFont);
                    const auto &audio = attribute.video.timeline.audio;
                    const int audioDc = SaveDC(canvas);
                    IntersectClipRect(canvas, axis.left, std::max<LONG>(top, axis.top), axis.right, std::min<LONG>(bottom, axis.bottom));
                    if (audio.clips.empty()) {
                        drawText(canvas, L"Click to add audio", {axis.left + sc(8), top, axis.right, bottom}, theme.mutedText,
                                 DT_LEFT | DT_VCENTER | DT_SINGLELINE, smallFont);
                    }
                    for (const auto &clip : audio.clips) {
                        const double startSeconds = double(clip.start) / 1000000;
                        const double endSeconds = double(clip.start + clip.duration()) / 1000000;
                        const double firstSecond = schedule.timeAt(viewStart), lastSecond = schedule.timeAt(viewEnd);
                        if (endSeconds < firstSecond || startSeconds > lastSecond || startSeconds >= schedule.getTotalSeconds()) continue;
                        const int left = depthX(schedule.depthAt(startSeconds), viewStart, viewEnd, axis);
                        const int right = depthX(schedule.depthAt(endSeconds), viewStart, viewEnd, axis);
                        RECT bounds{std::max<LONG>(axis.left, left), top + sc(7),
                                    std::min<LONG>(axis.right, std::max(right, left + sc(12))), bottom - sc(7)};
                        if (bounds.right <= bounds.left) continue;
                        const bool selected = clip.id == selectedAudioClip;
                        const COLORREF color = !audio.exportEnabled || clip.muted ? theme.mutedText : theme.accentText;
                        fillRoundRect(canvas, bounds, theme.panelRaised, selected ? theme.accentText : theme.grid, sc(4));
                        const auto file = std::filesystem::path(std::u8string(clip.path.begin(), clip.path.end())).filename().wstring();
                        drawText(canvas, file, {bounds.left + sc(10), bounds.top, bounds.right - sc(10), bounds.bottom}, color,
                                 DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, smallFont);
                        const HPEN pen = CreatePen(PS_SOLID, sc(2), color);
                        const auto old = SelectObject(canvas, pen);
                        for (int x : {int(bounds.left + sc(4)), int(bounds.right - sc(4))}) {
                            MoveToEx(canvas, x, bounds.top + sc(6), nullptr);
                            LineTo(canvas, x, bounds.bottom - sc(6));
                        }
                        SelectObject(canvas, old);
                        DeleteObject(pen);
                        RECT hit{};
                        IntersectRect(&hit, &bounds, &audioLane);
                        if (hit.bottom > hit.top) audioClipLayouts.push_back({clip.id, hit});
                    }
                    RestoreDC(canvas, audioDc);


                    const HPEN divider = CreatePen(PS_SOLID, 1, theme.grid);
                    const auto oldDivider = SelectObject(canvas, divider);
                    MoveToEx(canvas, timeline.left + sc(8), bottom, nullptr);
                    LineTo(canvas, axis.right, bottom);
                    SelectObject(canvas, oldDivider);
                    DeleteObject(divider);
                    if (draggingTrackRow && trackRowDragMoved) {
                        const int dropY = trackRowDropIndex == row ? top :
                            trackRowDropIndex == trackCount && row == trackCount - 1 ? bottom : -1;
                        if (dropY >= 0) {
                            const HPEN pen = CreatePen(PS_SOLID, sc(2), theme.selected);
                            const auto old = SelectObject(canvas, pen);
                            MoveToEx(canvas, timeline.left + sc(8), dropY, nullptr);
                            LineTo(canvas, axis.right, dropY);
                            SelectObject(canvas, old);
                            DeleteObject(pen);
                        }
                    }
                    continue;
                }
                const uint16_t targetId =
                    track != nullptr ? track->targetId : vidTimelineTargetId(VidTimelineTarget::SPEED);
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
                    fillRoundRect(canvas, labelCell, theme.panelRaised, theme.accentText, sc(8));
                }
                if (carriedRow) {
                    fillRoundRect(canvas, labelCell, theme.accentSoft, theme.accentBorder, sc(8));
                }
                const int labelMid = static_cast<int>(labelCell.top + labelCell.bottom) / 2;
                const RECT iconBox = {rowIconLeft, labelMid - sc(12), rowIconLeft + rowIconWidth,
                                      labelMid + sc(12)};
                drawTrackIcon(canvas, iconBox, targetId,
                              carriedRow && highContrastSettingsMode() ? theme.selectedText : rowColor);
                const float shownValue =
                    track != nullptr ? evaluateDisplayedTrack(*track, targetId, previewDepth, baseValue(targetId))
                                     : baseValue(targetId);
                const std::wstring valueLabel =
                    fittingText(canvas, smallFont, trackValueWidth,
                                {std::format(L"{:.3f}", shownValue), std::format(L"{:.3g}", shownValue)});
                const int valueLeft = labelCell.right - trackValueWidth - sc(10);
                const auto name = rowName(targetId, linkColorCycle);
                const int nameHeight = wrappedTextHeight(canvas, name, bodyFont, trackNameWidth);
                const int nameTop =
                    labelMid - (nameHeight + (narrow ? fontHeight(canvas, smallFont) + dip(2) : 0)) / 2;
                drawText(canvas, name, {rowTextLeft, nameTop, rowTextLeft + trackNameWidth, nameTop + nameHeight},
                         carriedRow    ? theme.selectedText
                         : trackActive ? theme.text
                                       : theme.mutedText,
                         DT_LEFT | DT_WORDBREAK, bodyFont);
                const RECT valueRect =
                    narrow ? RECT{rowTextLeft, nameTop + nameHeight + dip(2), labelCell.right - sc(10),
                                  labelCell.bottom}
                           : RECT{valueLeft, labelCell.top, labelCell.right - sc(10), labelCell.bottom};
                drawText(canvas, valueLabel, valueRect,
                         carriedRow && highContrastSettingsMode() ? theme.selectedText
                         : trackActive                            ? theme.mutedText
                                                                  : theme.disabledTrack,
                         (narrow ? DT_LEFT | DT_TOP : DT_RIGHT | DT_VCENTER) | DT_SINGLELINE, smallFont);

                const HPEN divider = CreatePen(PS_SOLID, 1, theme.grid);
                const HGDIOBJ oldPen = SelectObject(canvas, divider);
                MoveToEx(canvas, timeline.left + sc(8), bottom, nullptr);
                LineTo(canvas, timeline.right - sc(8), bottom);
                SelectObject(canvas, oldPen);
                DeleteObject(divider);

                if (draggingTrackRow && trackRowDragMoved && rowOrder >= 0) {
                    const int lastRow = static_cast<int>(reorderRowTargets.size());
                    // The line stands where the carried row lands: over this row, or under the last one.
                    const int dropY = trackRowDropIndex == rowOrder                             ? top
                                      : trackRowDropIndex == lastRow && rowOrder == lastRow - 1 ? bottom
                                                                                                : -1;
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
                trackLayouts.push_back({.targetId = targetId,
                                        .row = {axis.left, top, axis.right, bottom},
                                        .label = labelCell,
                                        .editable = editable,
                                        .minValue = minValue,
                                        .maxValue = maxValue,
                                        .order = rowOrder});
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
                        const int y =
                            editable ? valueY(key.value, minValue, maxValue, {axis.left, top, axis.right, bottom})
                                     : (top + bottom) / 2;
                        const bool selected = targetId == selectedTrackTarget && keyIndex == selectedTrackKey;
                        const bool hovered =
                            targetId == hoveredTrackKey.targetId && keyIndex == hoveredTrackKey.keyIndex;
                        const int radius = selected ? sc(7) : hovered ? sc(6) : sc(5);
                        const HBRUSH keyBrush = CreateSolidBrush(selected ? theme.selected : rowColor);
                        const HPEN keyPen = CreatePen(PS_SOLID, selected ? sc(2) : 1,
                                                      selected ? RGB(255, 237, 213) : theme.focusRing);
                        const HGDIOBJ oldBrush = SelectObject(canvas, keyBrush);
                        const HGDIOBJ oldKeyPen = SelectObject(canvas, keyPen);
                        // A keyframe is drawn as the diamond every editor draws it as.
                        const POINT diamond[4] = {
                            {x, y - radius}, {x + radius, y}, {x, y + radius}, {x - radius, y}};
                        Polygon(canvas, diamond, 4);
                        SelectObject(canvas, oldKeyPen);
                        SelectObject(canvas, oldBrush);
                        DeleteObject(keyPen);
                        DeleteObject(keyBrush);
                    }
                }
            }
        } catch (...) {
            SelectClipRgn(canvas, nullptr);
            DeleteObject(trackClip);
            throw;
        }
        SelectClipRgn(canvas, nullptr);
        DeleteObject(trackClip);

        // The right column is the bar's own width, so it stands clear of the axis without a gutter.
        trackScrollTrack = {rightColumnLeft, axis.top, rightColumnLeft + rightColumnWidth, axis.bottom};
        fillRect(canvas, trackScrollTrack, theme.panelRaised);
        frameRect(canvas, trackScrollTrack, theme.border);
        {
            const int barHeight = static_cast<int>(trackScrollTrack.bottom - trackScrollTrack.top);
            const int thumbHeight =
                std::clamp(static_cast<int>(static_cast<float>(barHeight) * static_cast<float>(viewHeight) /
                                            static_cast<float>(std::max(contentHeight, 1))),
                           std::min(sc(28), barHeight), barHeight);
            const float scrolled = trackScrollRange > 0 ? static_cast<float>(trackScrollOffset) /
                                                              static_cast<float>(trackScrollRange)
                                                        : 0.0f;
            const int thumbTop = trackScrollTrack.top +
                                 static_cast<int>(static_cast<float>(barHeight - thumbHeight) * scrolled);
            trackScrollThumb = {trackScrollTrack.left, thumbTop, trackScrollTrack.right,
                                thumbTop + thumbHeight};
            // With every row on screen the bar stays, filled and quiet, rather than leaving the column empty.
            const bool thumbHeld =
                trackScrollRange > 0 && (draggingTrackScrollThumb || hoverTrackScrollThumb);
            fillRect(canvas, trackScrollThumb,
                     trackScrollRange == 0 ? theme.toggleOff
                     : thumbHeld           ? theme.accentText
                                           : theme.mutedText);
            frameRect(canvas, trackScrollThumb, thumbHeld ? theme.accentText : theme.border);
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
                  zoomPresetButton.bottom},
                 theme.text, DT_LEFT | DT_VCENTER | DT_SINGLELINE, smallFont);
        drawText(canvas, L"\x25BE",
                 {zoomPresetButton.left, zoomPresetButton.top, zoomPresetButton.right - sc(9),
                  zoomPresetButton.bottom},
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
        fillRect(canvas, scrollThumb, thumbActive ? theme.accentText : theme.mutedText);
        frameRect(canvas, scrollThumb, thumbActive ? theme.accentText : theme.border);

        const HPEN guideDivider = CreatePen(PS_SOLID, 1, theme.grid);
        const HGDIOBJ oldGuideDivider = SelectObject(canvas, guideDivider);
        MoveToEx(canvas, timeline.left + sc(8), guideTop, nullptr);
        LineTo(canvas, timeline.right - sc(8), guideTop);
        SelectObject(canvas, oldGuideDivider);
        DeleteObject(guideDivider);
        const int guideMiddle = (guideTop + timeline.bottom) / 2;
        controlsButton = {timeline.right - rowRightPad - dip(84), guideMiddle - dip(14),
                          timeline.right - rowRightPad, guideMiddle + dip(14)};
        drawButton(canvas, controlsButton, L"Controls", hoverControls, false, smallFont, theme);
        overlayButton = {controlsButton.left - dip(130), controlsButton.top, controlsButton.left - dip(8),
                         controlsButton.bottom};
        drawButton(canvas, overlayButton, L"Zoom Overlay", false, overlayPositionMode, smallFont, theme);
        RECT guidance = {rowIconLeft, guideTop, overlayButton.left - dip(12), timeline.bottom};
        const int guidanceWidth = guidance.right - guidance.left;
        std::wstring guideText;
        const VidTimelineTrack *selectedTrack = track(selectedTrackTarget);
        if (selectedTrack != nullptr && selectedTrackKey >= 0 &&
            selectedTrackKey < static_cast<int>(selectedTrack->keys.size())) {
            const auto &key = selectedTrack->keys[selectedTrackKey];
            const std::wstring unit = selectedTrackTarget == SPEED_TARGET ? L" kf/s" : L"";
            guideText =
                fittingText(canvas, smallFont, guidanceWidth,
                            {std::format(L"Key {}   |   d {:.3f}   |   {:.4f}{}   |   {}   |   F2: edit key",
                                         selectedTrackKey + 1, displayDistance(key.depth), key.value, unit,
                                         interpolationName(key.out)),
                             std::format(L"Key {}   |   {}   |   F2: edit key", selectedTrackKey + 1,
                                         interpolationName(key.out)),
                             std::format(L"Key {}   |   F2: edit", selectedTrackKey + 1), L"F2: edit key"});
        } else {
            guideText = fittingText(canvas, smallFont, guidanceWidth,
                                    {L"Double-click a track: add key    |    Drag: scrub preview    |    "
                                     L"Wheel: zoom    |    F1: all controls",
                                     L"Double-click: add key    |    Drag: scrub    |    F1: controls",
                                     L"Double-click: add key    |    F1: controls", L"F1: all controls"});
        }
        drawText(canvas, guideText, guidance, theme.mutedText, DT_LEFT | DT_VCENTER | DT_SINGLELINE,
                 smallFont);

        const int playheadX = depthX(previewDepth, viewStart, viewEnd, axis);
        if (visibleX(playheadX, axis, 0)) {
            const HPEN playhead = CreatePen(PS_SOLID, sc(2), theme.accentText);
            const HGDIOBJ oldPlayhead = SelectObject(canvas, playhead);
            MoveToEx(canvas, playheadX, timeRowTop, nullptr);
            LineTo(canvas, playheadX, axis.bottom);
            SelectObject(canvas, oldPlayhead);
            DeleteObject(playhead);
            // The head of the playhead, so where the preview stands is visible at a glance.
            const HBRUSH headBrush = CreateSolidBrush(theme.accentText);
            const HPEN headPen = CreatePen(PS_SOLID, 1, theme.accentText);
            const HGDIOBJ oldHeadBrush = SelectObject(canvas, headBrush);
            const HGDIOBJ oldHeadPen = SelectObject(canvas, headPen);
            const POINT head[3] = {{playheadX - sc(7), timeRowTop - sc(9)},
                                   {playheadX + sc(7), timeRowTop - sc(9)},
                                   {playheadX, timeRowTop + sc(1)}};
            Polygon(canvas, head, 3);
            SelectObject(canvas, oldHeadPen);
            SelectObject(canvas, oldHeadBrush);
            DeleteObject(headPen);
            DeleteObject(headBrush);
        }

        for (const auto &hold : attribute.video.timeline.holds) {
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

        presentPaint(target, activeEditRect);
    }

    std::vector<workspace::AccessibleItem> TimelineWindow::accessibleItems() {
        using Id = workspace::TimelineItems;
        using Item = workspace::AccessibleItem;
        const TimelineDpiScope dpiScope(uiDpi);
        std::vector<Item> items;
        const auto add = [&](long id, const wchar_t *name, RECT bounds, const wchar_t *help = L"",
                             long role = ROLE_SYSTEM_PUSHBUTTON) {
            if (IsRectEmpty(&bounds)) {
                return;
            }
            items.push_back({id, name, help, L"Activate", bounds, role, STATE_SYSTEM_FOCUSABLE});
        };
        add(Id::frames, L"Keyframe folder", framesButton, L"Select the source keyframe folder.");
        if (frameSource && !items.empty()) {
            items.back().help += L" Current source: " + frameSource->getDirectory().wstring();
        }
        add(Id::load, L"Load timeline", loadButton);
        add(Id::save, L"Save timeline", saveButton);
        add(Id::exportVideo, L"Export", exportButton);
        add(Id::ai, L"AI Edit", aiButton);
        add(Id::theme, L"Theme", themeButton);
        add(Id::fullscreen, L"Fullscreen", fullscreenButton);
        add(Id::play, L"Play / Pause", playButton, L"Play or pause at the current time.",
            ROLE_SYSTEM_CHECKBUTTON);
        add(Id::pause, L"Pause", pauseButton);
        add(Id::stop, L"Stop", stopButton, L"Stop playback and return to the beginning.");
        add(Id::loop, L"Loop", loopButton, L"Repeat playback.", ROLE_SYSTEM_CHECKBUTTON);
        add(Id::distance, L"Timeline Distance", distanceField,
            L"Distance from the first frame. Enter opens a formula; Left and Right scrub.", ROLE_SYSTEM_TEXT);
        add(Id::keyframe, L"Timeline Keyframe", keyframeField,
            L"Source keyframe depth. Enter opens a formula; Left and Right scrub.", ROLE_SYSTEM_TEXT);
        add(Id::zoom, L"Source zoom", zoomField, L"Zoom read from the source keyframes.",
            ROLE_SYSTEM_STATICTEXT);
        add(Id::time, L"Timeline Time", timeField,
            L"Elapsed seconds. Enter opens a formula; Left and Right scrub.", ROLE_SYSTEM_TEXT);
        const auto native = [&](long id, HWND control, const wchar_t *name) {
            if (!control || !IsWindowVisible(control)) {
                return;
            }
            RECT bounds;
            GetWindowRect(control, &bounds);
            MapWindowPoints(nullptr, window, reinterpret_cast<POINT *>(&bounds), 2);
            add(id, name, bounds);
            items.back().native = control;
        };
        native(Id::divider, dockSplitter ? dockSplitter->handle() : nullptr, L"Resize timeline tracks");
        native(Id::toggle, dockToggle, L"Show or hide tracks");
        const int rowHeight =
            trackLayouts.empty() ? 0 : int(trackLayouts.front().row.bottom - trackLayouts.front().row.top);
        for (int row = 0; row < int(reorderRowTargets.size()) && rowHeight > 0; ++row) {
            const auto target = reorderRowTargets[row];
            const auto *current = track(target);
            if (!current && target != AUDIO_ROW_TARGET) {
                continue;
            }
            const int top = timelineAxis.top - trackScrollOffset + row * rowHeight;
            RECT label{timelinePanel.left + sc(8), top + sc(4), timelineAxis.left - sc(20),
                       top + rowHeight - sc(4)};
            Item item{Id::track(target),
                      rowName(target, linkColorCycle),
                      L"Up and Down select tracks; Left and Right select keys; Insert adds a key; "
                      L"Alt+Up/Down reorders; Shift+F10 opens the track menu.",
                      L"Select track",
                      label,
                      ROLE_SYSTEM_LISTITEM,
                      STATE_SYSTEM_FOCUSABLE | STATE_SYSTEM_SELECTABLE};
            item.value = target == AUDIO_ROW_TARGET ? std::to_wstring(attribute.video.timeline.audio.clips.size()) + L" clips" : workspace::AttributeFormModel::number(playheadValue(target));
            if (trackRowSelected(target)) {
                item.state |= STATE_SYSTEM_SELECTED;
            }
            items.push_back(std::move(item));
            if (target == AUDIO_ROW_TARGET) continue;
            const auto limits = valueRange(target);
            const RECT plot{timelineAxis.left, top, timelineAxis.right, top + rowHeight};
            for (int key = 0; key < int(current->keys.size()); ++key) {
                const auto &value = current->keys[key];
                const int x = depthX(value.depth, viewStartDepth, viewEndDepth, timelineAxis),
                          y = editableTarget(target) ? valueY(value.value, limits.first, limits.second, plot)
                                                     : top + rowHeight / 2;
                Item keyItem{
                    itemIds.key(target, key),
                    rowName(target, linkColorCycle) + L" key " + std::to_wstring(key + 1),
                    L"Distance " + workspace::AttributeFormModel::number(displayDistance(value.depth)) +
                        L". Enter or F2 edits this key; Delete removes it; 1-4 change interpolation.",
                    L"Edit key",
                    {x - sc(8), y - sc(8), x + sc(8), y + sc(8)},
                    ROLE_SYSTEM_LISTITEM,
                    STATE_SYSTEM_FOCUSABLE | STATE_SYSTEM_SELECTABLE};
                keyItem.value = workspace::AttributeFormModel::number(value.value);
                keyItem.writable = editableTarget(target);
                if (selectedTrackTarget == target && selectedTrackKey == key) {
                    keyItem.state |= STATE_SYSTEM_SELECTED;
                }
                items.push_back(std::move(keyItem));
            }
        }
        add(Id::viewZoom, L"Timeline zoom", zoomPresetButton, L"Choose the visible timeline range.");
        add(Id::overlay, L"Zoom Overlay", overlayButton, L"Configure the zoom ratio shown in the video.");
        add(Id::controls, L"Timeline controls", controlsButton,
            L"F1 shows the controls, preview status and source folder.");
        native(Id::editor, fieldEdit, L"Timeline formula");
        RECT client;
        GetClientRect(window, &client);
        for (auto &item : items) {
            if ((item.id == Id::play && playing) || (item.id == Id::loop && loopPlayback)) {
                item.state |= STATE_SYSTEM_CHECKED;
            }
            if (item.id == Id::distance || item.id == Id::keyframe || item.id == Id::time) {
                item.writable = true;
                item.value = workspace::AttributeFormModel::number(item.id == Id::distance
                                                                       ? displayDistance(previewDepth)
                                                                   : item.id == Id::time ? previewSeconds()
                                                                                         : previewDepth);
            }
            if (item.id == Id::zoom) {
                item.state = STATE_SYSTEM_READONLY;
                item.action.clear();
                item.value =
                    frameSource ? std::format(L"1E{:.1f}", zoomExponentAt(previewDepth)) : L"No keyframes";
            }
            if ((item.native && GetFocus() == item.native) ||
                (!item.native && GetFocus() == window && keyboardFocus == item.id)) {
                item.state |= STATE_SYSTEM_FOCUSED;
            }
            if (exporting || (item.native && !IsWindowEnabled(item.native))) {
                item.state |= STATE_SYSTEM_UNAVAILABLE;
            }
            if (!IsWindowVisible(window)) {
                item.state |= STATE_SYSTEM_INVISIBLE;
            }
            RECT clip = client;
            if (Id::isTrack(item.id) || Id::isKey(item.id)) {
                clip.top = timelineAxis.top;
                clip.bottom = timelineAxis.bottom;
                if (Id::isKey(item.id)) {
                    clip.left = timelineAxis.left;
                    clip.right = timelineAxis.right;
                }
            }
            RECT intersection;
            if (!IntersectRect(&intersection, &clip, &item.bounds)) {
                item.bounds = {};
                item.state |= STATE_SYSTEM_OFFSCREEN;
            } else {
                item.bounds = intersection;
            }
        }
        return items;
    }

    bool TimelineWindow::focusItem(long id) {
        using Id = workspace::TimelineItems;
        const TimelineDpiScope dpiScope(uiDpi);
        if (!IsWindowVisible(window) || exporting) {
            return false;
        }
        const auto items = accessibleItems();
        const auto found = std::ranges::find(items, id, &workspace::AccessibleItem::id);
        if (found == items.end() || (found->state & STATE_SYSTEM_UNAVAILABLE) ||
            !(found->state & STATE_SYSTEM_FOCUSABLE)) {
            return false;
        }
        if (id == Id::editor && fieldEdit) {
            SetFocus(fieldEdit);
            return true;
        }
        if (!commitFieldEdit()) {
            SetFocus(fieldEdit);
            return false;
        }
        keyboardFocus = id;
        if (found->native) {
            SetFocus(found->native);
            accessibilityDirty = true;
            return true;
        }
        if (Id::isTrack(id) || Id::isKey(id)) {
            const auto target = itemIds.target(id);
            const auto row = std::ranges::find(reorderRowTargets, target);
            if (row == reorderRowTargets.end() || trackLayouts.empty()) {
                return false;
            }
            const int height = trackLayouts.front().row.bottom - trackLayouts.front().row.top;
            const int top = int(row - reorderRowTargets.begin()) * height, bottom = top + height,
                      viewport = timelineAxis.bottom - timelineAxis.top;
            if (top < trackScrollOffset) {
                trackScrollOffset = top;
            } else if (bottom > trackScrollOffset + viewport) {
                trackScrollOffset = std::min(top, bottom - viewport);
            }
            selectTrackRow(target, false, false);
            if (Id::isKey(id)) {
                auto *current = track(target);
                const int key = itemIds.index(id);
                if (!current || key >= int(current->keys.size())) {
                    return false;
                }
                selectedTrackKey = key;
                previewDepth = current->keys[key].depth;
                syncPlaybackClock();
                if (previewDepth < viewEndDepth || previewDepth > viewStartDepth) {
                    const float span = viewSpan();
                    viewStartDepth = previewDepth + span * .5f;
                    viewEndDepth = viewStartDepth - span;
                    clampView();
                }
                requestFramePreview();
            }
        }
        SetFocus(window);
        accessibilityDirty = true;
        RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
        return true;
    }

    bool TimelineWindow::activateItem(long id) {
        using Id = workspace::TimelineItems;
        if (!focusItem(id)) {
            return false;
        }
        if (Id::isTrack(id) && itemIds.target(id) == AUDIO_ROW_TARGET) {
            selectTrackRow(AUDIO_ROW_TARGET, false, false);
            showInspectorSection(1);
            return true;
        }
        if (Id::isTrack(id)) {
            return true;
        }
        if (Id::isKey(id)) {
            openTrackKeyEditor();
            return true;
        }
        switch (id) {
        case Id::frames:
            loadKeyframeDirectory();
            break;
        case Id::load:
            loadTimeline();
            break;
        case Id::save:
            saveTimeline();
            break;
        case Id::exportVideo:
            openExportMenu();
            break;
        case Id::ai:
            openAiExchangeMenu();
            break;
        case Id::theme:
            toggleTheme();
            break;
        case Id::fullscreen:
            toggleFullscreen();
            break;
        case Id::play:
            setPlaying(!playing);
            break;
        case Id::pause:
            setPlaying(false);
            break;
        case Id::stop:
            stopPlayback();
            break;
        case Id::loop:
            loopPlayback = !loopPlayback;
            break;
        case Id::distance:
            beginFieldEdit(FieldEdit::DISTANCE);
            break;
        case Id::keyframe:
            beginFieldEdit(FieldEdit::KEYFRAME);
            break;
        case Id::time:
            beginFieldEdit(FieldEdit::TIME);
            break;
        case Id::viewZoom:
            openZoomMenu();
            break;
        case Id::overlay:
            openOverlaySettings();
            break;
        case Id::controls:
            openControlsGuide();
            break;
        case Id::toggle:
            toggleWorkspaceDock();
            break;
        case Id::divider:
        case Id::editor:
            return true;
        default:
            return false;
        }
        accessibilityDirty = true;
        InvalidateRect(window, nullptr, FALSE);
        return true;
    }

    bool TimelineWindow::writeItem(long id, std::wstring_view text) {
        using Id = workspace::TimelineItems;
        const auto value = NumericExpression::evaluate(std::wstring(text));
        if (!value || std::abs(*value) > std::numeric_limits<float>::max() || exporting) {
            return false;
        }
        const float parsed = static_cast<float>(*value);
        if (Id::isKey(id)) {
            const auto target = itemIds.target(id);
            const int key = itemIds.index(id);
            const auto *current = track(target);
            if (!current || key >= int(current->keys.size()) || !editableTarget(target)) {
                return false;
            }
            const auto *parameter = TimelineParams::find(target);
            if (parameter &&
                (parameter->kind == TimelineParamKind::BOOL || parameter->kind == TimelineParamKind::ENUM) &&
                std::round(parsed) != parsed) {
                return false;
            }
            if (target == SPEED_TARGET
                    ? parsed < VidTimelineAttribute::MIN_SPEED
                    : !parameter || parsed < parameter->minValue || parsed > parameter->maxValue) {
                return false;
            }
            if (!focusItem(id)) {
                return false;
            }
            current = track(target);
            if (current->keys[key].value == parsed) {
                return true;
            }
            auto *edited = track(target);
            lastUndoStep = 0;
            edited->keys[key].value = parsed;
            edited->enabled = true;
            (void)syncLinkedColorCycle(target);
            commitTimeline();
        } else {
            if (id != Id::distance && id != Id::keyframe && id != Id::time) {
                return false;
            }
            const double minimum = id == Id::keyframe ? schedule.getEndDepth() : 0.f,
                        maximum = id == Id::distance ? schedule.getStartDepth() - schedule.getEndDepth()
                                  : id == Id::time   ? schedule.getTotalSeconds()
                                                     : schedule.getStartDepth();
            if (*value < minimum || *value > maximum || !focusItem(id)) {
                return false;
            }
            previewDepth = id == Id::distance ? depthFromDistance(*value)
                           : id == Id::time   ? schedule.depthAt(*value)
                                              : *value;
            syncPlaybackClock();
            if (id == Id::time) {
                playSeconds = *value;
                restartAudioPreview();
            }
            requestFramePreview();
        }
        accessibilityDirty = true;
        InvalidateRect(window, nullptr, FALSE);
        return true;
    }

    void TimelineWindow::tabItem(int direction) {
        using Id = workspace::TimelineItems;
        if (!commitFieldEdit()) {
            return;
        }
        std::vector<long> order;
        const auto items = accessibleItems();
        long row = 0, key = 0;
        const auto focusTarget = (Id::isTrack(keyboardFocus) || Id::isKey(keyboardFocus))
                                     ? itemIds.target(keyboardFocus)
                                     : selectedTrackTarget;
        for (const auto &item : items) {
            if (item.state & (STATE_SYSTEM_UNAVAILABLE | STATE_SYSTEM_INVISIBLE) ||
                !(item.state & STATE_SYSTEM_FOCUSABLE)) {
                continue;
            }
            if (Id::isTrack(item.id)) {
                if (!row || itemIds.target(item.id) == focusTarget) {
                    row = item.id;
                }
                continue;
            }
            if (Id::isKey(item.id)) {
                if (itemIds.target(item.id) == focusTarget &&
                    (!key || itemIds.index(item.id) == selectedTrackKey)) {
                    key = item.id;
                }
                continue;
            }
            if (item.id == Id::editor) {
                continue;
            }
            if (item.id == Id::viewZoom) {
                if (row) {
                    order.push_back(row);
                }
                if (key) {
                    order.push_back(key);
                }
            }
            if (!(item.state & STATE_SYSTEM_OFFSCREEN)) {
                order.push_back(item.id);
            }
        }
        if (order.empty()) {
            return;
        }
        const auto found = std::ranges::find(order, keyboardFocus);
        int index = found == order.end() ? (direction > 0 ? -1 : 0) : int(found - order.begin());
        (void)focusItem(order[(index + direction + int(order.size())) % order.size()]);
    }

    bool TimelineWindow::keyItem(WPARAM key) {
        using Id = workspace::TimelineItems;
        const bool ctrl = GetKeyState(VK_CONTROL) < 0, shift = GetKeyState(VK_SHIFT) < 0,
                   alt = GetKeyState(VK_MENU) < 0;
        if (key == VK_F1) {
            openControlsGuide();
            return true;
        }
        if (key == VK_TAB) {
            tabItem(shift ? -1 : 1);
            return true;
        }
        if (exporting) {
            return true;
        }
        if (key == VK_SPACE && (Id::isTrack(keyboardFocus) || Id::isKey(keyboardFocus))) {
            setPlaying(!playing);
            InvalidateRect(window, nullptr, FALSE);
            return true;
        }
        if (key == VK_RETURN || key == VK_SPACE) {
            (void)activateItem(keyboardFocus);
            return true;
        }
        if (keyboardFocus == Id::distance || keyboardFocus == Id::keyframe || keyboardFocus == Id::time) {
            if (key == VK_LEFT || key == VK_RIGHT) {
                const float step = (key == VK_RIGHT ? 1.f : -1.f) * (shift ? 10.f : 1.f);
                const double seconds = std::clamp(previewSeconds() + step, 0.0, schedule.getTotalSeconds());
                previewDepth =
                    keyboardFocus == Id::time
                        ? schedule.depthAt(seconds)
                        : std::clamp(previewDepth - step, schedule.getEndDepth(), schedule.getStartDepth());
                syncPlaybackClock();
                if (keyboardFocus == Id::time) {
                    playSeconds = seconds;
                    restartAudioPreview();
                }
                requestFramePreview();
                InvalidateRect(window, nullptr, FALSE);
                return true;
            }
        }
        if (!Id::isTrack(keyboardFocus) && !Id::isKey(keyboardFocus)) {
            return key == VK_DELETE || key == VK_BACK || key == VK_F2 || (key >= '1' && key <= '4');
        }
        const auto target = itemIds.target(keyboardFocus);
        auto *current = track(target);
        if (!current && target != AUDIO_ROW_TARGET) {
            return false;
        }
        const auto row = std::ranges::find(reorderRowTargets, target);
        if (row == reorderRowTargets.end()) {
            return false;
        }
        const int index = int(row - reorderRowTargets.begin());
        if (target == AUDIO_ROW_TARGET && key != VK_UP && key != VK_DOWN) {
            if (key == VK_F2 || (key == VK_F10 && shift)) showInspectorSection(1);
            return !ctrl;
        }
        if (key == VK_DELETE || key == VK_BACK) {
            lastUndoStep = 0;
            if (Id::isKey(keyboardFocus)) {
                deleteTrackKey();
                if (selectedTrackKey >= 0) {
                    (void)focusItem(itemIds.key(target, selectedTrackKey));
                }
            } else {
                const auto next = reorderRowTargets[std::min(index + 1, int(reorderRowTargets.size()) - 1)];
                removeTrack(target);
                RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
                if (!reorderRowTargets.empty()) {
                    (void)focusItem(Id::track(track(next) ? next : reorderRowTargets.back()));
                }
            }
            return true;
        }
        if (key == VK_UP || key == VK_DOWN) {
            const int direction = key == VK_DOWN ? 1 : -1,
                      next = std::clamp(index + direction, 0, int(reorderRowTargets.size()) - 1);
            if (alt) {
                if (next != index) {
                    std::vector<uint16_t> carried;
                    int first = index, last = index;
                    for (int i = 0; i < int(reorderRowTargets.size()); ++i) {
                        if (trackRowSelected(reorderRowTargets[i])) {
                            carried.push_back(reorderRowTargets[i]);
                            first = std::min(first, i);
                            last = std::max(last, i);
                        }
                    }
                    lastUndoStep = 0;
                    moveTrackRows(carried, direction > 0 ? last + 2 : first - 1);
                    RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
                    (void)focusItem(Id::track(target));
                    selectedTrackTargets = std::move(carried);
                }
            } else {
                const auto nextTarget = reorderRowTargets[next];
                const auto anchor = selectedTrackTarget;
                (void)focusItem(Id::track(nextTarget));
                if (shift) {
                    selectedTrackTarget = anchor;
                    selectTrackRow(nextTarget, false, true);
                }
            }
            accessibilityDirty = true;
            InvalidateRect(window, nullptr, FALSE);
            return true;
        }
        if (!ctrl && (key == VK_LEFT || key == VK_RIGHT || key == VK_HOME || key == VK_END)) {
            if (current->keys.empty()) {
                return true;
            }
            int next = selectedTrackKey;
            if (key == VK_HOME) {
                next = 0;
            } else if (key == VK_END) {
                next = int(current->keys.size()) - 1;
            } else if (next < 0) {
                next = key == VK_RIGHT ? 0 : int(current->keys.size()) - 1;
            } else {
                next += key == VK_RIGHT ? 1 : -1;
            }
            (void)focusItem(itemIds.key(target, std::clamp(next, 0, int(current->keys.size()) - 1)));
            return true;
        }
        if (key == VK_ESCAPE && Id::isKey(keyboardFocus)) {
            (void)focusItem(Id::track(target));
            return true;
        }
        if (key == VK_INSERT) {
            if (const auto *row = layout(target); row && row->editable) {
                lastUndoStep = 0;
                const int x = std::clamp(depthX(previewDepth, viewStartDepth, viewEndDepth, timelineAxis),
                                         int(row->row.left), int(row->row.right) - 1);
                addTrackKey(target, {x, (row->row.top + row->row.bottom) / 2});
                if (selectedTrackKey >= 0) {
                    (void)focusItem(itemIds.key(target, selectedTrackKey));
                }
            }
            return true;
        }
        if (key == VK_APPS || (key == VK_F10 && shift)) {
            if (const auto *row = layout(target)) {
                POINT point{row->row.left + sc(12), (row->row.top + row->row.bottom) / 2};
                if (selectedTrackKey >= 0) {
                    point.x = depthX(current->keys[selectedTrackKey].depth, viewStartDepth, viewEndDepth,
                                     timelineAxis);
                    point.y = row->editable ? valueY(current->keys[selectedTrackKey].value, row->minValue,
                                                     row->maxValue, row->row)
                                            : (row->row.top + row->row.bottom) / 2;
                }
                openTrackMenu(point);
            }
            return true;
        }
        return false;
    }

    void TimelineWindow::paintItemFocus(HDC dc) {
        if (!dc || GetFocus() != window) {
            return;
        }
        const auto items = accessibleItems();
        const auto found = std::ranges::find(items, keyboardFocus, &workspace::AccessibleItem::id);
        if (found == items.end() || (found->state & (STATE_SYSTEM_OFFSCREEN | STATE_SYSTEM_INVISIBLE))) {
            return;
        }
        RECT bounds = found->bounds;
        InflateRect(&bounds, -2, -2);
        const auto theme = timelineTheme(lightMode);
        const auto pen = CreatePen(PS_SOLID, std::max(2, UiDpi::pixels(1, uiDpi)), theme.focusRing);
        const auto oldPen = SelectObject(dc, pen), oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
        Rectangle(dc, bounds.left, bounds.top, bounds.right, bounds.bottom);
        SelectObject(dc, oldBrush);
        SelectObject(dc, oldPen);
        DeleteObject(pen);
    }

    void TimelineWindow::presentPaint(HDC target, const RECT &activeEditRect) {
        RECT client;
        GetClientRect(window, &client);
        paintItemFocus(paintBuffer.begin(target, client.right, client.bottom));
        paintBuffer.present(target);
        if (fieldEdit != nullptr && activeEditRect.right > activeEditRect.left) {
            SetWindowPos(fieldEdit, nullptr, activeEditRect.left, activeEditRect.top,
                         activeEditRect.right - activeEditRect.left,
                         activeEditRect.bottom - activeEditRect.top, SWP_NOACTIVATE | SWP_NOZORDER);
        }
        if (fieldTooltip != nullptr) {
            const RECT tooltipRects[3] = {distanceField, keyframeField, timeField};
            for (UINT_PTR id = 1; id <= 3; ++id) {
                TOOLINFOW tool = {};
                tool.cbSize = sizeof(tool);
                tool.hwnd = window;
                tool.uId = id;
                tool.rect = tooltipRects[id - 1];
                SendMessageW(fieldTooltip, TTM_NEWTOOLRECTW, 0, reinterpret_cast<LPARAM>(&tool));
            }
        }
        const auto now = GetTickCount64();
        if (accessibility && (accessibilityDirty || (playing && now - accessibilityTick >= 500))) {
            accessibilityDirty = false;
            accessibilityTick = now;
            accessibility->changed();
        }
    }

    LRESULT TimelineWindow::fieldEditProc(const HWND hwnd, const UINT message, const WPARAM wParam,
                                          const LPARAM lParam, [[maybe_unused]] const UINT_PTR subclassId,
                                          const DWORD_PTR referenceData) {
        auto *self = reinterpret_cast<TimelineWindow *>(referenceData);
        if (self == nullptr) {
            return DefSubclassProc(hwnd, message, wParam, lParam);
        }
        const TimelineDpiScope dpiScope(self->uiDpi);
        if (message == WM_GETDLGCODE) {
            return DLGC_WANTALLKEYS;
        }
        if (message == WM_KEYDOWN) {
            MSG key{};
            key.hwnd = hwnd;
            key.message = message;
            key.wParam = wParam;
            if (workspace::PaneSplitter::handleCapturedShortcut(key)) {
                return 0;
            }
            if (wParam == VK_TAB) {
                if (self->commitFieldEdit()) {
                    self->tabItem(GetKeyState(VK_SHIFT) < 0 ? -1 : 1);
                }
                return 0;
            }
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
        if (message == WM_CHAR &&
            (wParam == VK_RETURN || wParam == L'\n' || wParam == VK_ESCAPE || wParam == VK_TAB)) {
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
            NativeDialogs::message(hwnd, e.what(), "Timeline Editor", MB_OK | MB_ICONERROR);
        } catch (...) {
            NativeDialogs::message(hwnd, L"The Timeline Editor ran into an unexpected error.",
                                   L"Timeline Editor", MB_OK | MB_ICONERROR);
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
        const TimelineDpiScope dpiScope(self->uiDpi);

        switch (message) {
        case WM_APP + 0x270:
            self->finishAiImages(static_cast<uint64_t>(wParam));
            return 0;
        case WM_APP + 0x271:
            self->openAiExchangeMenu();
            return 0;
        case WM_APP + 0x272:
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_SYSCOLORCHANGE:
        case WM_SETTINGCHANGE:
            refreshSystemSettingsTheme();
            self->refreshTheme();
            break;
        case WM_GETOBJECT:
            if (static_cast<DWORD>(lParam) == static_cast<DWORD>(OBJID_CLIENT) && self->accessibility) {
                return self->accessibility->object(wParam);
            }
            break;
        case WM_GETDLGCODE:
            return DLGC_WANTALLKEYS;
        case WM_SETFOCUS:
        case WM_KILLFOCUS:
            self->accessibilityDirty = true;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_DPICHANGED: {
            self->updateDpi(HIWORD(wParam));
            if ((!self->embedded || self->floatingWorkspace) && lParam) {
                const auto &r = *reinterpret_cast<const RECT *>(lParam);
                SetWindowPos(hwnd, nullptr, r.left, r.top, r.right - r.left, r.bottom - r.top,
                             SWP_NOZORDER | SWP_NOACTIVATE);
            }
            return 0;
        }
        case WM_GETMINMAXINFO: {
            if (self->fullscreen || (self->embedded && !self->floatingWorkspace)) {
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
            self->accessibilityDirty = true;
            self->layoutWorkspaceDock();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_COMMAND:
            if (LOWORD(wParam) == inspectorToggleId && HIWORD(wParam) == BN_CLICKED) {
                if (self->inspector && !self->inspector->applyPending()) {
                    self->inspector->focus();
                    return 0;
                }
                self->dockState.inspectorOpen = !self->dockState.inspectorOpen;
                self->layoutWorkspaceDock();
                self->saveWorkspaceDock();
                return 0;
            }
            if (reinterpret_cast<HWND>(lParam) == self->fieldEdit && HIWORD(wParam) == EN_CHANGE) {
                workspace::AccessibleControl::validation(self->fieldEdit, L"");
                return 0;
            }
            if (LOWORD(wParam) == WORKSPACE_DOCK_TOGGLE && HIWORD(wParam) == BN_CLICKED) {
                self->toggleWorkspaceDock();
                return 0;
            }
            break;
        case WM_DRAWITEM: {
            const auto *item = reinterpret_cast<DRAWITEMSTRUCT *>(lParam);
            if (item && item->hwndItem == self->inspectorToggle) {
                const workspace::WorkspaceButton::Context context{&self->inspectorTheme, self->smallFont,
                                                                  self->uiDpi / 96.f};
                auto bufferedItem = *item;
                const auto buffer =
                    self->inspectorToggleBuffer.begin(item->hDC, item->rcItem.right, item->rcItem.bottom);
                if (buffer) {
                    bufferedItem.hDC = buffer;
                }
                workspace::WorkspaceButton::draw(bufferedItem, context);
                if (buffer) {
                    self->inspectorToggleBuffer.present(item->hDC);
                }
                return TRUE;
            }
            if (item && item->hwndItem == self->dockToggle) {
                auto theme = timelineTheme(self->lightMode);
                if (item->itemState & ODS_DISABLED) {
                    theme.text = theme.mutedText;
                }
                const auto buffered =
                    self->dockToggleBuffer.begin(item->hDC, item->rcItem.right, item->rcItem.bottom);
                const auto dc = buffered ? buffered : item->hDC;
                fillRect(dc, item->rcItem, theme.panel);
                drawButton(
                    dc, item->rcItem,
                    UiLanguage::label(self->dockLayout.tracksVisible ? L"Hide Tracks" : L"Show Tracks"),
                    (item->itemState & ODS_SELECTED) != 0, false, self->smallFont, theme);
                if (item->itemState & ODS_FOCUS) {
                    RECT focus = item->rcItem;
                    InflateRect(&focus, -3, -3);
                    DrawFocusRect(dc, &focus);
                }
                if (buffered) {
                    self->dockToggleBuffer.present(item->hDC);
                }
                return TRUE;
            }
            break;
        }
        case WM_CTLCOLOREDIT:
            if (reinterpret_cast<HWND>(lParam) == self->fieldEdit) {
                const TimelineTheme &theme = timelineTheme(self->lightMode);
                const HDC editDc = reinterpret_cast<HDC>(wParam);
                SetTextColor(editDc, theme.text);
                SetBkColor(editDc, theme.panelRaised);
                return reinterpret_cast<LRESULT>(self->fieldEditBrush);
            }
            break;
        case WM_TIMELINE_CACHE_PROGRESS:
            self->accessibilityDirty = true;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_TIMELINE_PREVIEW_READY:
            if (wParam) {
                std::scoped_lock lock(self->previewRequestMutex);
                if (static_cast<uint64_t>(wParam) != self->previewRequestGeneration &&
                    !self->previewWorkerFailed.load()) {
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
            }
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
            if (!self->embedded || IsWindowVisible(hwnd)) {
                (void)self->initializeFramePreview();
            }
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
                if (contains(self->audioLane, point)) {
                    bool overClip = false, edge = false;
                    const int grip = std::max(4, int(std::lround(7 * self->uiDpi / 96.0)));
                    for (const auto &item : self->audioClipLayouts) if (contains(item.bounds, point)) {
                        overClip = true;
                        edge = point.x < item.bounds.left + grip || point.x >= item.bounds.right - grip;
                    }
                    SetCursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(edge ? 32644 : overClip ? 32646 : 32649)));
                    return TRUE;
                }
                const bool interactive =
                    contains(self->framesButton, point) || contains(self->loadButton, point) ||
                    contains(self->saveButton, point) || contains(self->exportButton, point) || contains(self->aiButton, point) ||
                    contains(self->controlsButton, point) || contains(self->themeButton, point) ||
                    contains(self->fullscreenButton, point) || self->hitTrackKey(point).valid() ||
                    self->hitTrackLabel(point) != UINT16_MAX;
                // The readouts that carry the playhead are dragged sideways, and say so.
                const bool slider = contains(self->distanceField, point) ||
                                    contains(self->keyframeField, point) || contains(self->timeField, point);
                const int cursor = slider                                ? 32644
                                   : interactive                         ? 32649
                                   : contains(self->timelineAxis, point) ? 32515
                                                                         : 32512;
                SetCursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(cursor)));
                return TRUE;
            }
            break;
        }
        case WM_APP + 0x266:
            self->refreshOverlaySettings();
            return 0;
        case WM_MOUSEMOVE: {
            const POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            if (self->draggingAudio) {
                const auto time = int64_t(std::llround(std::clamp(double(self->schedule.timeAt(self->viewDepthAt(point.x))), 0.0, 604800.0) * 1000000));
                AudioClipEdit::apply(self->attribute.video.timeline.audio, self->audioDragBefore,
                                     self->audioDragEdge, time - self->audioDragTime);
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            if (self->draggingOverlay) {
                auto &o = self->attribute.video.timeline.zoomOverlay;
                o.x =
                    std::clamp(self->overlayDragBefore.x + float(point.x - self->overlayDragStart.x) /
                                                               std::max(1L, self->overlayImageRect.right -
                                                                                self->overlayImageRect.left),
                               0.f, 1.f);
                o.y = std::clamp(self->overlayDragBefore.y + float(point.y - self->overlayDragStart.y) /
                                                                 std::max(1L, self->overlayImageRect.bottom -
                                                                                  self->overlayImageRect.top),
                                 0.f, 1.f);
                o.custom = true;
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
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
            const bool ai = contains(self->aiButton, point);
            const bool thumb = contains(self->scrollThumb, point);
            const bool theme = contains(self->themeButton, point);
            const bool trackThumb = contains(self->trackScrollThumb, point);
            const bool full = contains(self->fullscreenButton, point);
            const bool play = contains(self->playButton, point);
            const bool pause = contains(self->pauseButton, point);
            const bool stop = contains(self->stopButton, point);
            const bool loop = contains(self->loopButton, point);
            const bool zoomPreset = contains(self->zoomPresetButton, point);
            const bool controls = contains(self->controlsButton, point);
            const FieldEdit formulaField = contains(self->distanceField, point)   ? FieldEdit::DISTANCE
                                           : contains(self->keyframeField, point) ? FieldEdit::KEYFRAME
                                           : contains(self->timeField, point)     ? FieldEdit::TIME
                                                                                  : FieldEdit::NONE;
            if (hoveredKey.targetId != self->hoveredTrackKey.targetId ||
                hoveredKey.keyIndex != self->hoveredTrackKey.keyIndex || frames != self->hoverFrames ||
                load != self->hoverLoad || save != self->hoverSave || exportVideo != self->hoverExport || ai != self->hoverAi ||
                thumb != self->hoverScrollThumb || theme != self->hoverTheme ||
                trackThumb != self->hoverTrackScrollThumb || full != self->hoverFullscreen ||
                play != self->hoverPlay || pause != self->hoverPause || stop != self->hoverStop ||
                loop != self->hoverLoop || zoomPreset != self->hoverZoomPreset ||
                formulaField != self->hoveredFieldEdit || controls != self->hoverControls) {
                self->hoveredTrackKey = hoveredKey;
                self->hoverFrames = frames;
                self->hoverLoad = load;
                self->hoverSave = save;
                self->hoverExport = exportVideo;
                self->hoverAi = ai;
                self->hoverScrollThumb = thumb;
                self->hoverTheme = theme;
                self->hoverTrackScrollThumb = trackThumb;
                self->hoverFullscreen = full;
                self->hoverPlay = play;
                self->hoverPause = pause;
                self->hoverStop = stop;
                self->hoverLoop = loop;
                self->hoverZoomPreset = zoomPreset;
                self->hoverControls = controls;
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
            if (self->fieldEdit && !self->commitFieldEdit()) {
                return 0;
            }
            SetFocus(hwnd);
            self->accessibilityDirty = true;
            const POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            if (contains(self->audioLane, point)) {
                if (self->exporting || (self->inspector && !self->inspector->applyPending())) return 0;
                for (const auto &item : self->audioClipLayouts) {
                    if (!contains(item.bounds, point)) continue;
                    auto &clips = self->attribute.video.timeline.audio.clips;
                    const auto clip = std::ranges::find(clips, item.id, &VidAudioClip::id);
                    if (clip == clips.end()) return 0;
                    self->keyboardFocus = workspace::TimelineItems::track(AUDIO_ROW_TARGET);
                    self->selectTrackRow(AUDIO_ROW_TARGET, false, false);
                    self->selectedAudioClip = item.id;
                    self->audioDragBefore = *clip;
                    const int grip = std::max(4, int(std::lround(7 * self->uiDpi / 96.0)));
                    self->audioDragEdge = point.x < item.bounds.left + grip ? -1 : point.x >= item.bounds.right - grip ? 1 : 0;
                    self->audioDragTime = int64_t(std::llround(std::clamp(double(self->schedule.timeAt(self->viewDepthAt(point.x))), 0.0, 604800.0) * 1000000));
                    if (self->audioDragEdge < 0) self->audioDragTime = clip->start;
                    if (self->audioDragEdge > 0) self->audioDragTime = clip->start + clip->duration();
                    self->draggingAudio = true;
                    SetCapture(hwnd);
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
                self->selectTrackRow(AUDIO_ROW_TARGET, false, false);
                self->showInspectorSection(1);
                if (self->attribute.video.timeline.audio.clips.empty()) self->addAudioClip();
                return 0;
            }
            for (const auto &item : self->accessibleItems()) {
                if (item.id >= workspace::TimelineItems::frames && item.id < workspace::TimelineItems::divider &&
                    (item.state & STATE_SYSTEM_FOCUSABLE) &&
                    contains(item.bounds, point)) {
                    self->keyboardFocus = item.id;
                    if (item.id < workspace::TimelineItems::distance ||
                        item.id >= workspace::TimelineItems::viewZoom) {
                        (void)self->activateItem(item.id);
                        return 0;
                    }
                    break;
                }
            }
            if (self->overlayPositionMode && contains(self->overlayImageRect, point)) {
                self->overlayDragBefore = self->attribute.video.timeline.zoomOverlay;
                self->overlayDragStart = point;
                self->draggingOverlay = true;
                SetCapture(hwnd);
                return 0;
            }
            // The distance, keyframe and time readouts scrub the preview when they are dragged sideways.
            if (contains(self->distanceField, point) || contains(self->keyframeField, point) ||
                contains(self->timeField, point)) {
                self->fieldDrag = contains(self->timeField, point) ? FieldDrag::TIME : FieldDrag::DEPTH;
                self->pendingFieldEdit = contains(self->distanceField, point)   ? FieldEdit::DISTANCE
                                         : contains(self->keyframeField, point) ? FieldEdit::KEYFRAME
                                                                                : FieldEdit::TIME;
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
                self->keyboardFocus = workspace::TimelineItems::track(labelTarget);
                // Ctrl adds the row to the ones picked or drops it, Shift takes the run up to it.
                self->selectTrackRow(labelTarget, (GetKeyState(VK_CONTROL) & 0x8000) != 0,
                                     (GetKeyState(VK_SHIFT) & 0x8000) != 0);
                // The name cell is the handle the rows picked are carried by, all of them at once.
                if (self->trackRowSelected(labelTarget)) {
                    self->carriedRows.clear();
                    for (const uint16_t id : self->reorderRowTargets) {
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
                self->keyboardFocus = self->itemIds.key(hit.targetId, hit.keyIndex);
                self->selectedTrackTarget = hit.targetId;
                self->selectedTrackKey = hit.keyIndex;
                if (const VidTimelineTrack *current = self->track(hit.targetId); current != nullptr) {
                    self->previewDepth = current->keys[hit.keyIndex].depth;
                    self->syncPlaybackClock();
                }
                self->draggingTrackKey = true;
                self->dragHasUndoStep = false;
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
                self->keyboardFocus = self->itemIds.key(hit.targetId, hit.keyIndex);
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
            if (self->draggingAudio) {
                self->draggingAudio = false;
                ReleaseCapture();
                const auto &clips = self->attribute.video.timeline.audio.clips;
                const auto clip = std::ranges::find(clips, self->selectedAudioClip, &VidAudioClip::id);
                if (clip != clips.end() && *clip != self->audioDragBefore) {
                    self->lastUndoStep = 0;
                    self->commitTimeline();
                }
                self->inspectorSectionRequest = 1;
                self->showInspectorSection(1);
                return 0;
            }
            if (self->draggingOverlay) {
                self->draggingOverlay = false;
                ReleaseCapture();
                self->lastUndoStep = 0;
                self->commitOverlay();
                PostMessageW(hwnd, WM_APP + 0x266, 0, 0);
                return 0;
            }
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
                if (self->selectedTrackTarget == AUDIO_ROW_TARGET) self->showInspectorSection(1);
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
        case WM_CAPTURECHANGED: {
            self->cancelAudioDrag();
            const bool previewChanged = (self->fieldDrag != FieldDrag::NONE && self->fieldDragMoved) ||
                                        self->draggingTrackKey || self->scrubbingTimeline;
            if (self->draggingOverlay) {
                self->attribute.video.timeline.zoomOverlay = self->overlayDragBefore;
                self->draggingOverlay = false;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
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
            if (previewChanged) {
                self->requestFramePreview();
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }
        case WM_RBUTTONDOWN: {
            const POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            if (contains(self->audioLane, point)) {
                for (const auto &item : self->audioClipLayouts) if (contains(item.bounds, point)) self->selectedAudioClip = item.id;
                self->inspectorSectionRequest = 1;
                self->showInspectorSection(1);
                return 0;
            }
            if (point.y >= self->timelineAxis.top && point.y <= self->timelineAxis.bottom &&
                point.x <= self->timelineAxis.right) {
                self->openTrackMenu(point);
            }
            return 0;
        }
        case WM_SYSKEYDOWN:
            if ((wParam == VK_UP || wParam == VK_DOWN) && (GetKeyState(VK_MENU) < 0) &&
                self->keyItem(wParam)) {
                return 0;
            }
            break;
        case WM_KEYDOWN:
            if (self->draggingAudio) {
                if (wParam == VK_ESCAPE) {
                    self->cancelAudioDrag();
                    ReleaseCapture();
                }
                return 0;
            }
            if (self->overlayPositionMode && wParam == VK_ESCAPE) {
                if (self->draggingOverlay) {
                    self->attribute.video.timeline.zoomOverlay = self->overlayDragBefore;
                    self->draggingOverlay = false;
                    ReleaseCapture();
                } else {
                    self->overlayPositionMode = false;
                }
                InvalidateRect(hwnd, nullptr, FALSE);
                PostMessageW(hwnd, WM_APP + 0x266, 0, 0);
                return 0;
            }
            if (self->overlayPositionMode && wParam >= VK_LEFT && wParam <= VK_DOWN) {
                const float step = GetKeyState(VK_SHIFT) < 0 ? 10.f : 1.f;
                const float w = self->frameSource ? float(self->frameSource->getWidth()) /
                                                        std::max(1u, self->attribute.render.ssaa)
                                                  : 1920.f;
                const float h = self->frameSource ? float(self->frameSource->getHeight()) /
                                                        std::max(1u, self->attribute.render.ssaa)
                                                  : 1080.f;
                auto &o = self->attribute.video.timeline.zoomOverlay;
                o.x = std::clamp(o.x + (wParam == VK_LEFT    ? -step
                                        : wParam == VK_RIGHT ? step
                                                             : 0) /
                                           w,
                                 0.f, 1.f);
                o.y = std::clamp(o.y + (wParam == VK_UP     ? -step
                                        : wParam == VK_DOWN ? step
                                                            : 0) /
                                           h,
                                 0.f, 1.f);
                o.custom = true;
                self->commitOverlay();
                return 0;
            }
            self->accessibilityDirty = true;
            if (self->keyItem(wParam)) {
                return 0;
            }
            if (self->dockToggle && !self->dockLayout.tracksVisible &&
                (wParam == VK_DELETE || wParam == VK_BACK || wParam == VK_RETURN || wParam == VK_F2 ||
                 (wParam >= '1' && wParam <= '4'))) {
                return 0;
            }
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
                    if (self->historyRequest) {
                        self->historyRequest(false);
                    } else {
                        self->undoTimeline();
                    }
                    return 0;
                }
                break;
            case 'Y':
                if (GetKeyState(VK_CONTROL) & 0x8000) {
                    if (self->historyRequest) {
                        self->historyRequest(true);
                    } else {
                        self->redoTimeline();
                    }
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
            if (wParam == inspectorTimerId) {
                self->refreshInspector();
                return 0;
            }
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
            try {
                RECT client = {};
                GetClientRect(hwnd, &client);
                self->paint(hdc, client);
            } catch (...) {
                EndPaint(hwnd, &ps);
                throw;
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_PRINTCLIENT: {
            RECT client;
            GetClientRect(hwnd, &client);
            self->paint(reinterpret_cast<HDC>(wParam), client);
            return 0;
        }
        case WM_CLOSE:
            if (NativeDialogs::isOpen()) {
                return 0;
            }
            if (self->floatingWorkspace) {
                showWorkspace(hwnd, {}, false);
                return 0;
            }
            if (self->embedded) {
                return 0;
            }
            DestroyWindow(hwnd);
            return 0;
        case WM_NCDESTROY:
            // The shader panels report to this window; nothing may be posted to it after here.
            if (self->renderScene != nullptr) {
                HWND listening = hwnd;
                self->renderScene->getRequests().shaderEditListener.compare_exchange_strong(listening,
                                                                                            nullptr);
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
} // namespace merutilm::rff2
