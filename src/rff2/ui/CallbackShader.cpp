//
// Created by Merutilm on 2025-05-16.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-16, 2026-08-21, 2026-08-23, 2026-08-26, 2026-08-27, 2026-08-31.
// Modified by Opus 4.8 on 2026-07-05
// Modified by Opus 5 on 2026-08-05, 2026-08-06, 2026-08-07, 2026-08-08, 2026-08-11, 2026-08-12, 2026-08-13, 2026-08-14, 2026-08-15, 2026-08-16, 2026-08-17, 2026-08-18, 2026-08-19, 2026-08-20, 2026-08-22, 2026-08-24, 2026-08-25, 2026-08-26, 2026-08-27, 2026-08-29, 2026-08-31, 2026-09-04
// Modified by ox-alpha on 2026-08-22.
// Modified by Fable 5.1 on 2026-09-02
//

#include "Callback.hpp"
#include "CallbackFile.hpp"
#include "CallbackShader.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <string>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>

#include "../attr/ShdPatternLayerSelection.h"
#include "../attr/ShdTextureLayerSelection.h"
#include "../io/KFRColorLoader.hpp"
#include "../io/ShaderPresetIO.h"
#include "IOUtilities.h"
#include "../constants/ExtensionConstants.hpp"

namespace merutilm::rff2 {

    static void drawPalettePreview(const HDC hdc, const RECT &rc, const ShdPaletteAttribute &pal,
                                   const ShdPalColorSmoothingMethod method,
                                   const ShdPalColorInterpolationMethod interp) {
        const int w = rc.right - rc.left;
        const int h = rc.bottom - rc.top;
        if (w <= 0 || h <= 0) {
            return;
        }
        const int n = static_cast<int>(pal.colors.size());
        if (n == 0) {
            FillRect(hdc, &rc, static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
            return;
        }
        // Stock DC_BRUSH + SetDCBrushColor lets us fill one 1px column per color without
        // allocating a brush per pixel.
        const auto oldBrush = SelectObject(hdc, GetStockObject(DC_BRUSH));
        for (int x = 0; x < w; ++x) {
            const float t = w == 1 ? 0.0f : static_cast<float>(x) / static_cast<float>(w - 1); // 0..1
            glm::vec4 c;
            switch (method) {
                case ShdPalColorSmoothingMethod::NONE: {
                    const int bands = std::max(1, std::min(n, w / 10));
                    const int band = std::min(static_cast<int>(t * static_cast<float>(bands)), bands - 1);
                    const int i = std::min(band * n / bands, n - 1);
                    c = pal.colors[i];
                    break;
                }
                case ShdPalColorSmoothingMethod::REVERSED: {
                    const int bands = std::max(1, std::min(n, w / 10));
                    const float scaled = t * static_cast<float>(bands);
                    const int band = std::min(static_cast<int>(scaled), bands - 1);
                    const float local = scaled - static_cast<float>(band); // 0..1 within the band
                    const float p = (static_cast<float>(band) + (1.0f - local)) / static_cast<float>(bands);
                    const float f = p * static_cast<float>(n);
                    const int i0 = static_cast<int>(f) % n;
                    const int i1 = (i0 + 1) % n;
                    const float d = f - std::floor(f);
                    c = blendPaletteColors(pal.colors[i0], pal.colors[i1], d, interp);
                    break;
                }
                case ShdPalColorSmoothingMethod::NORMAL: {
                    const float f = t * static_cast<float>(n);
                    const int i0 = static_cast<int>(f) % n;
                    const int i1 = (i0 + 1) % n;
                    const float d = f - std::floor(f);
                    c = blendPaletteColors(pal.colors[i0], pal.colors[i1], d, interp);
                    break;
                }
            }
            // Drawn over the blend, as the palette upload lays it over the colors it sends.
            if (const float lineCoverage = pal.bandLineCoverage(t); lineCoverage > 0.0f) {
                c = glm::mix(c, pal.bandLineColor, lineCoverage);
            }
            const COLORREF rgb = RGB(
                static_cast<BYTE>(std::round(std::clamp(c.r, 0.0f, 1.0f) * 255.0f)),
                static_cast<BYTE>(std::round(std::clamp(c.g, 0.0f, 1.0f) * 255.0f)),
                static_cast<BYTE>(std::round(std::clamp(c.b, 0.0f, 1.0f) * 255.0f)));
            SetDCBrushColor(hdc, rgb);
            RECT col = {rc.left + x, rc.top, rc.left + x + 1, rc.bottom};
            FillRect(hdc, &col, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
        }
        SelectObject(hdc, oldBrush);
    }

    // Approximate display color of a frozen iteration value. Computed in double precision so a
    // large iteration value keeps the correct cycle phase (float getMidColor would lose it).
    static COLORREF freezeSwatchColor(const ShdPaletteAttribute &pal, const double iter) {
        const int n = static_cast<int>(pal.colors.size());
        if (n == 0) return RGB(0, 0, 0);
        auto chan = [&](const int ch, const float interval) -> float {
            double ratio = std::fmod(iter / static_cast<double>(interval) + pal.offsetRatio, 1.0);
            if (ratio < 0.0) ratio += 1.0;
            const double f = ratio * static_cast<double>(n);
            const int i0 = static_cast<int>(f) % n;
            const int i1 = (i0 + 1) % n;
            const float d = static_cast<float>(f - std::floor(f));
            return std::lerp(pal.colors[i0][ch], pal.colors[i1][ch], d);
        };
        return RGB(
            static_cast<BYTE>(std::round(std::clamp(chan(0, pal.iterationInterval.r), 0.0f, 1.0f) * 255.0f)),
            static_cast<BYTE>(std::round(std::clamp(chan(1, pal.iterationInterval.g), 0.0f, 1.0f) * 255.0f)),
            static_cast<BYTE>(std::round(std::clamp(chan(2, pal.iterationInterval.b), 0.0f, 1.0f) * 255.0f)));
    }

    static constexpr int FREEZE_SWATCH_SIZE = 22;
    static constexpr int FREEZE_SWATCH_GAP = 5;

    // Grid rect of the i-th frozen-color swatch; shared by the panel painter and click hit-test.
    static RECT freezeSwatchRect(const RECT &client, const int i) {
        const int size = Constants::Win32::settingsScaled(FREEZE_SWATCH_SIZE);
        const int gap = Constants::Win32::settingsScaled(FREEZE_SWATCH_GAP);
        const int avail = client.right - client.left - gap;
        const int perRow = std::max(1, avail / (size + gap));
        const int row = i / perRow;
        const int col = i % perRow;
        RECT r;
        r.left = client.left + gap + col * (size + gap);
        r.top = client.top + gap + row * (size + gap);
        r.right = r.left + size;
        r.bottom = r.top + size;
        return r;
    }

    struct FreezeSwatchCtx {
        ShdPaletteAttribute *palette;
        RenderScene *scene;
    };

    // Subclass for the frozen-color swatch panel: claims clicks (statics are click-through) and
    // removes the clicked swatch in place, repainting without rebuilding the window.
    static LRESULT CALLBACK freezeSwatchProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                             UINT_PTR id, DWORD_PTR ref) {
        auto *ctx = reinterpret_cast<FreezeSwatchCtx *>(ref);
        switch (msg) {
            case WM_NCHITTEST:
                return HTCLIENT;
            case WM_LBUTTONDOWN: {
                const int mx = GET_X_LPARAM(lp);
                const int my = GET_Y_LPARAM(lp);
                RECT client;
                GetClientRect(hwnd, &client);
                auto &iters = ctx->palette->staticColorIterations;
                for (int i = 0; i < static_cast<int>(iters.size()); ++i) {
                    RECT s = freezeSwatchRect(client, i);
                    POINT p{mx, my};
                    if (PtInRect(&s, p)) {
                        iters.erase(iters.begin() + i);
                        ctx->scene->getRequests().requestShader();
                        InvalidateRect(hwnd, nullptr, TRUE);
                        break;
                    }
                }
                return 0;
            }
            case WM_NCDESTROY:
                RemoveWindowSubclass(hwnd, freezeSwatchProc, id);
                delete ctx;
                break;
            default:
                break;
        }
        return DefSubclassProc(hwnd, msg, wp, lp);
    }

    const std::function<void(SettingsMenu &, RenderScene &)> CallbackShader::PALETTE = [
            ](SettingsMenu &settingsMenu, RenderScene &scene) {
        auto &palette = scene.getAttribute().shader.palette;
        auto window = std::make_unique<SettingsWindow>(L"Palette");
        constexpr float intervalSliderMin = 1.0f;
        constexpr float intervalSliderMax = 1.0e18f;
        // Use %.2e for large values (consistent width, e.g. 1.00e+05); %g for small ones.
        auto intervalUnparser = Unparser::FLOAT_SCIENTIFIC;
        // Cycle Length stops at the slider's own range: anything beyond it (or a non-finite value
        // from an overflowing entry) is refused with the standard "Invalid value!" error.
        auto intervalValidCondition = ValidCondition::floatInRange(intervalSliderMin, intervalSliderMax);
        auto shortFloatUnparser = [](const float &v) -> std::wstring {
            std::wstring s = std::to_wstring(v);
            if (s.find(L'.') != std::wstring::npos) {
                s.erase(s.find_last_not_of(L'0') + 1);
                if (!s.empty() && s.back() == L'.') {
                    s.push_back(L'0');
                }
            }
            return s;
        };
        // G/B text-field HWNDs, filled in after those rows exist; used to mirror the R
        // interval onto G and B (see applySyncGBtoR).
        auto gbIntervalFields = std::make_shared<std::array<HWND, 2>>();
        static auto syncGBtoR = std::make_shared<bool>(true);
        // Copies the R interval onto G and B and refreshes their text fields + sliders.
        auto applySyncGBtoR = [&palette, winPtr = window.get(), gbIntervalFields] {
            palette.iterationInterval.g = palette.iterationInterval.r;
            palette.iterationInterval.b = palette.iterationInterval.r;
            winPtr->setFloatValueByField((*gbIntervalFields)[0], palette.iterationInterval.r);
            winPtr->setFloatValueByField((*gbIntervalFields)[1], palette.iterationInterval.r);
        };
        window->registerSectionHeader(L"Color Cycle", false);
        window->registerSliderInput(L"Cycle Length (R)", &palette.iterationInterval.r,
                                    intervalSliderMin, intervalSliderMax, intervalUnparser, Parser::FLOAT,
                                    intervalValidCondition, [&scene, applySyncGBtoR] {
                                        if (*syncGBtoR) {
                                            applySyncGBtoR();
                                        }
                                        scene.getRequests().requestShader();
                                    }, L"Set R Interval", L"Iterations for Red channel cycle");
        (*gbIntervalFields)[0] = window->registerSliderInput(L"Cycle Length (G)", &palette.iterationInterval.g,
                                    intervalSliderMin, intervalSliderMax, intervalUnparser, Parser::FLOAT,
                                    intervalValidCondition, [&scene] {
                                        scene.getRequests().requestShader();
                                    }, L"Set G Interval", L"Iterations for Green channel cycle");
        (*gbIntervalFields)[1] = window->registerSliderInput(L"Cycle Length (B)", &palette.iterationInterval.b,
                                    intervalSliderMin, intervalSliderMax, intervalUnparser, Parser::FLOAT,
                                    intervalValidCondition, [&scene] {
                                        scene.getRequests().requestShader();
                                    }, L"Set B Interval", L"Iterations for Blue channel cycle");
        window->registerSelectionInput<ShdPalIterationColoringMode>(L"Iteration Coloring", &palette.iterationColoring, [&scene] {
            scene.getRequests().requestShader();
        }, L"Set Iteration Coloring",
        L"The curve the iteration count is read through before it lands on the color cycle.\nLinear is the straight count: every band is one Cycle Length wide, so the bands crowd into nothing where the count runs away near the set. Square root, Cube root, Log and LogLog widen each band as the count climbs, holding the deep ones apart - in that order, from the gentlest to the most compressed.\nSmoothstep and Smootherstep leave the band widths alone and re-space the colors inside every band instead: the count is eased through each cycle, so a color holds at the band edges and sweeps through the middle, drawing the palette as broad plateaus divided by narrow transitions. Smootherstep flattens the plateaus further. Where the compressing curves change how much depth a band covers, these change where the color sits within it.\nEvery curve reaches the end of its first cycle at exactly one Cycle Length, so that setting keeps its meaning when the mode changes; only the bands past the first are re-spaced.\nThe curve is attached to the iteration bands rather than to the palette itself: Palette Start Offset and the color animation still slide the colors straight through it, and the texture, pattern and warp layers ride the same re-spaced bands.");
        window->registerTextInput<float>(L"Palette Start Offset", &palette.offsetRatio, shortFloatUnparser, Parser::FLOAT,
                                         ValidCondition::FLOAT_ZERO_TO_ONE, [&scene] {
                                             scene.getRequests().requestShader();
                                         }, L"Set Palette Start Offset",
                                         L"Start offset of the cycling palette. 0.0 ~ 1.0 value required.\nUp/Down arrows nudge by 0.01 (Shift = 0.1).", 0.01);
        auto colorSmoothingRadio = std::make_shared<std::vector<HWND>>();
        window->registerTextInput<float>(
                                         L"Color Animation Speed", &palette.animationSpeed, shortFloatUnparser,
                                         Parser::FLOAT, ValidCondition::ALL_FLOAT,
                                         [&scene, winPtr = window.get(), colorSmoothingRadio] {
                                             // Force-enable color smoothing whenever the animation speed is changed.
                                             // Animating an unsmoothed palette looks stepped, so switch NONE -> NORMAL.
                                             auto &pal = scene.getAttribute().shader.palette;
                                             if (pal.colorSmoothing == ShdPalColorSmoothingMethod::NONE) {
                                                 pal.colorSmoothing = ShdPalColorSmoothingMethod::NORMAL;
                                                 winPtr->setRadioValueByGroup(*colorSmoothingRadio,
                                                                              std::any(pal.colorSmoothing));
                                             }
                                             scene.getRequests().requestShader();
                                         }, L"Set Animation Speed",
                                         L"Color Animation Speed, The colors' offset(iterations) per second.\nUp/Down arrows nudge by 1 (Shift = 10).", 1.0);
        window->registerSliderInput(L"Cycle Bias", &palette.cycleBias, 0.10f, 4.00f,
                                    Unparser::floatFixed(2), Parser::FLOAT,
                                    ValidCondition::floatInRange(0.10f, 4.00f), [&scene] {
                                        scene.getRequests().requestShader();
                                    }, L"Set Cycle Bias",
                                    L"A curve on the cycle position the iterations map to, reshaping where each color cycle spends its colors. Above 1.00 a cycle lingers on its first colors and rushes its last ones; below 1.00 it rushes the first and lingers the last. 1.00 is the straight mapping.\nThe curve is attached to the iteration bands rather than to the palette itself: Palette Start Offset and the animation still slide the colors straight through it, and the texture, pattern and warp layers ride the same curved bands.\nThe previews beside Color Smoothing show the palette's own sequence of colors, which the bias does not rearrange - it only re-maps where the iterations land on that sequence.");
        window->registerRadioButtonInput<ShdPaletteCycleCurve>(L"Cycle Curve", &palette.cycleCurve, [&scene] {
            scene.getRequests().requestShader();
        }, L"Set Cycle Curve",
        L"The curve Cycle Bias runs the cycle position through.\nPower bends it by pow(ratio, bias). It can pile nearly the whole cycle onto a narrow stretch of iterations - dramatic on a still, but under color animation the boundaries there crawl while others race across the view, because the curve's own slope fades to nothing at one end of the cycle.\nWave tilts the density sinusoidally towards whichever end the bias names instead. Every band width stays within a factor of Cycle Bias of another, so animated boundaries all move inside that one speed range - the even choice while the colors are animating.");

        window->registerSectionHeader(L"Animation Shape");
        struct AnimationShapeControls {
            std::vector<HWND> mode;
            HWND amount = nullptr;
            HWND scale = nullptr;
            HWND speed = nullptr;
            HWND swirl = nullptr;
        };
        auto animationControls = std::make_shared<AnimationShapeControls>();
        auto updateAnimationShapeRows = std::make_shared<std::function<void()>>();
        // Tracks the mode the flow values currently belong to, so switching shapes reseeds them
        // while editing a flow value (or reclicking the active mode) leaves them alone.
        auto lastAnimationMode = std::make_shared<ShdPaletteAnimationMode>(palette.animationMode);
        auto requestAnimationShape = [&scene, winPtr = window.get(), colorSmoothingRadio, animationControls, updateAnimationShapeRows, lastAnimationMode] {
            auto &pal = scene.getAttribute().shader.palette;
            if (pal.animationMode != ShdPaletteAnimationMode::LINEAR &&
                pal.colorSmoothing == ShdPalColorSmoothingMethod::NONE) {
                pal.colorSmoothing = ShdPalColorSmoothingMethod::NORMAL;
                winPtr->setRadioValueByGroup(*colorSmoothingRadio, std::any(pal.colorSmoothing));
            }
            if (pal.animationMode != *lastAnimationMode) {
                *lastAnimationMode = pal.animationMode;
                switch (pal.animationMode) {
                    using enum ShdPaletteAnimationMode;
                    case BREATHING:
                        // Breathing reads Flow Speed as its oscillation rate, so it keeps a non-zero one.
                        pal.animationFlowAmount = 1.0f;
                        pal.animationFlowSpeed = 0.5f;
                        break;
                    case TURBULENCE:
                    case PSYCHEDELIC:
                        // A still field that the linear Color Animation Speed then drifts through.
                        pal.animationFlowAmount = 3.0f;
                        pal.animationFlowScale = 1.0f;
                        pal.animationFlowSpeed = 0.0f;
                        pal.animationFlowSwirl = 0.0f;
                        break;
                    default:
                        break;
                }
                if (animationControls->amount != nullptr) {
                    winPtr->setFloatValueByField(animationControls->amount, pal.animationFlowAmount);
                    winPtr->setFloatValueByField(animationControls->scale, pal.animationFlowScale);
                    winPtr->setFloatValueByField(animationControls->speed, pal.animationFlowSpeed);
                    winPtr->setFloatValueByField(animationControls->swirl, pal.animationFlowSwirl);
                }
            }
            if (*updateAnimationShapeRows) {
                (*updateAnimationShapeRows)();
            }
            scene.getRequests().requestShader();
        };
        animationControls->mode = window->registerRadioButtonInput<ShdPaletteAnimationMode>(
                L"Animation Mode", &palette.animationMode, requestAnimationShape,
                L"Set Animation Mode",
                L"Linear drifts the palette. Breathing oscillates it. Turbulence creates layered fluid motion. Psychedelic creates radial and spiral motion.");
        animationControls->amount = window->registerTextInput<float>(
                L"Flow Amount", &palette.animationFlowAmount, shortFloatUnparser, Parser::FLOAT,
                ValidCondition::POSITIVE_FLOAT_ZERO, requestAnimationShape, L"Set Flow Amount",
                L"Maximum displacement of the animated color cycle, measured in iterations.\nUp/Down arrows nudge by 10 (Shift = 100).", 10.0);
        animationControls->scale = window->registerSliderInput(
                L"Flow Scale", &palette.animationFlowScale, 0.0f, 12.0f, Unparser::floatFixed(2),
                Parser::FLOAT, ValidCondition::floatInRange(0.0f, 12.0f), requestAnimationShape,
                L"Set Flow Scale", L"Density of the turbulence or liquid bands. Higher values make tighter, busier motion.");
        window->setSliderFractionalSteps(animationControls->scale);
        animationControls->speed = window->registerSliderInput(
                L"Flow Speed", &palette.animationFlowSpeed, -2.0f, 2.0f, Unparser::floatFixed(2),
                Parser::FLOAT, ValidCondition::floatInRange(-2.0f, 2.0f), requestAnimationShape,
                L"Set Flow Speed", L"Speed of the selected motion. Negative values reverse its direction.");
        animationControls->swirl = window->registerSliderInput(
                L"Swirl", &palette.animationFlowSwirl, -2.0f, 2.0f, Unparser::floatFixed(2),
                Parser::FLOAT, ValidCondition::floatInRange(-2.0f, 2.0f), requestAnimationShape,
                L"Set Swirl", L"Twist around the screen center. Negative values rotate the bend the other way.");
        *updateAnimationShapeRows = [winPtr = window.get(), animationControls, &palette] {
            const auto mode = palette.animationMode;
            const bool breathing = mode == ShdPaletteAnimationMode::BREATHING;
            const bool turbulence = mode == ShdPaletteAnimationMode::TURBULENCE;
            const bool psychedelic = mode == ShdPaletteAnimationMode::PSYCHEDELIC;
            winPtr->setRowEnabled(animationControls->amount, breathing || turbulence || psychedelic);
            winPtr->setRowEnabled(animationControls->scale, turbulence || psychedelic);
            winPtr->setRowEnabled(animationControls->speed, breathing || turbulence || psychedelic);
            winPtr->setRowEnabled(animationControls->swirl, psychedelic);
        };
        (*updateAnimationShapeRows)();

        window->registerSectionHeader(L"Freeze Colors (Eyedropper)");
        const HWND freezePanel = window->registerOwnerDrawnPanel(
                Constants::Win32::settingsScaled(64), [&palette](const HDC hdc, const RECT &rc) {
            const HBRUSH bg = CreateSolidBrush(settingsTheme().textFieldBackground);
            FillRect(hdc, &rc, bg);
            DeleteObject(bg);
            const auto &iters = palette.staticColorIterations;
            if (iters.empty()) {
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, settingsTheme().text);
                RECT t = rc;
                const int textInset = Constants::Win32::settingsScaled(FREEZE_SWATCH_GAP + 2);
                t.left += textInset;
                t.right -= textInset;
                DrawTextW(hdc, L"No frozen colors. Press Pick Color, then click the fractal.", -1, &t,
                          DT_LEFT | DT_VCENTER | DT_WORDBREAK);
                return;
            }
            const HPEN pen = CreatePen(PS_SOLID, 1, settingsTheme().buttonBorder);
            const auto oldPen = SelectObject(hdc, pen);
            const auto oldBrush = SelectObject(hdc, GetStockObject(DC_BRUSH));
            for (int i = 0; i < static_cast<int>(iters.size()); ++i) {
                const RECT s = freezeSwatchRect(rc, i);
                SetDCBrushColor(hdc, freezeSwatchColor(palette, iters[i]));
                Rectangle(hdc, s.left, s.top, s.right, s.bottom);
            }
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(pen);
        });
        RECT freezePanelRect;
        GetWindowRect(freezePanel, &freezePanelRect);
        MapWindowPoints(nullptr, window->getWindow(), reinterpret_cast<POINT *>(&freezePanelRect), 2);
        const int freezePanelLeft = Constants::Win32::settingsScaled(
                                        Constants::Win32::SETTINGS_LABEL_LEFT_PADDING);
        SetWindowPos(freezePanel, nullptr, freezePanelLeft, freezePanelRect.top,
                     freezePanelRect.right - freezePanelLeft, freezePanelRect.bottom - freezePanelRect.top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowSubclass(freezePanel, freezeSwatchProc, 1,
                          reinterpret_cast<DWORD_PTR>(new FreezeSwatchCtx{&palette, &scene}));
        window->registerButton(L"Pick Color", L"Eyedropper",
                               [&scene, freezePanel] {
                                   scene.beginColorFreezePick(freezePanel, [freezePanel] {
                                       InvalidateRect(freezePanel, nullptr, TRUE);
                                   });
                               }, L"Pick Color to Freeze",
                               L"Press this, then click a spot on the fractal: that color stops animating and is held static.\nClick a swatch above to remove it. The same color is frozen everywhere it appears.");
        window->registerButton(L"Frozen Colors", L"Clear All",
                               [&scene, &palette, freezePanel] {
                                   palette.staticColorIterations.clear();
                                   scene.getRequests().requestShader();
                                   InvalidateRect(freezePanel, nullptr, TRUE);
                               }, L"Clear All Frozen Colors",
                               L"Removes every frozen color so the whole palette animates again.");
        window->registerTextInput<float>(L"Freeze Match Tolerance", &palette.staticColorTolerance,
                                         shortFloatUnparser, Parser::FLOAT, ValidCondition::FLOAT_ZERO_TO_ONE,
                                         [&scene] { scene.getRequests().requestShader(); },
                                         L"Set Freeze Match Tolerance",
                                         L"How close a color must be (fraction of one cycle, 0..1) to count as a frozen color.\nLarger freezes a wider band around each picked color; very small (e.g. 0.00001) freezes only flat areas and lets thin lines keep animating.\nUp/Down arrows nudge by 0.01 (Shift = 0.1).", 0.01);

        // Each group's previews are drawn with the other group's value, so a change in either leaves the other's stale.
        auto colorInterpolationRadio = std::make_shared<std::vector<HWND>>();
        auto repaintPreviews = [](const std::shared_ptr<std::vector<HWND>> &group) {
            for (const HWND item : *group) {
                InvalidateRect(item, nullptr, TRUE);
            }
        };

        window->registerSectionHeader(L"Color Smoothing");
        *colorSmoothingRadio = window->registerRadioButtonInput<ShdPalColorSmoothingMethod>(L"Color Smoothing", &palette.colorSmoothing, [&scene, colorInterpolationRadio, repaintPreviews] {
            scene.getRequests().requestShader();
            repaintPreviews(colorInterpolationRadio);
        }, L"Color Smoothing", L"Color Smoothing method");
        {
            const auto smoothingValues = Selectable::values<ShdPalColorSmoothingMethod>();
            for (size_t i = 0; i < colorSmoothingRadio->size() && i < smoothingValues.size(); ++i) {
                const ShdPalColorSmoothingMethod method = smoothingValues[i];
                window->setRowPreview((*colorSmoothingRadio)[i], [&palette, method](const HDC hdc, const RECT &rc) {
                    drawPalettePreview(hdc, rc, palette, method, palette.colorInterpolation);
                });
            }
        }

        window->registerSectionHeader(L"Color Interpolation");
        *colorInterpolationRadio = window->registerRadioButtonInput<ShdPalColorInterpolationMethod>(
            L"Interpolation", &palette.colorInterpolation, [&scene, colorSmoothingRadio, repaintPreviews] {
                scene.getRequests().requestShader();
                repaintPreviews(colorSmoothingRadio);
            }, L"Color Interpolation",
            L"Color space the blend between two palette colors runs in.\nRGB mixes the channels directly. OKLab mixes what the eye sees, so a blend between two strong colors keeps its color instead of passing through gray, and the hue no longer drifts across the blend.");
        {
            const auto interpolationValues = Selectable::values<ShdPalColorInterpolationMethod>();
            for (size_t i = 0; i < colorInterpolationRadio->size() && i < interpolationValues.size(); ++i) {
                const ShdPalColorInterpolationMethod interp = interpolationValues[i];
                window->setRowPreview((*colorInterpolationRadio)[i], [&palette, interp](const HDC hdc, const RECT &rc) {
                    drawPalettePreview(hdc, rc, palette, palette.colorSmoothing, interp);
                });
            }
        }

        // Opens the native Windows color picker (a window with RGB fields) for the
        // given color, writing the chosen RGB back and refreshing the shader on OK.
        auto openColorPicker = [&scene, winPtr = window.get()](glm::vec4 *color) {
            static COLORREF customColors[16] = {};
            CHOOSECOLORW cc = {};
            cc.lStructSize = sizeof(cc);
            cc.hwndOwner = winPtr->getWindow();
            cc.lpCustColors = customColors;
            cc.rgbResult = RGB(
                static_cast<BYTE>(std::round(std::clamp(color->r, 0.0f, 1.0f) * 255.0f)),
                static_cast<BYTE>(std::round(std::clamp(color->g, 0.0f, 1.0f) * 255.0f)),
                static_cast<BYTE>(std::round(std::clamp(color->b, 0.0f, 1.0f) * 255.0f)));
            cc.Flags = CC_FULLOPEN | CC_RGBINIT;
            if (ChooseColorW(&cc)) {
                color->r = static_cast<float>(GetRValue(cc.rgbResult)) / 255.0f;
                color->g = static_cast<float>(GetGValue(cc.rgbResult)) / 255.0f;
                color->b = static_cast<float>(GetBValue(cc.rgbResult)) / 255.0f;
                scene.getRequests().requestShader();
            }
        };

        window->registerSectionHeader(L"Options");
        window->registerCheckboxInput(L"Link G & B to R", syncGBtoR.get(),
                                      [&scene, applySyncGBtoR] {
                                          if (*syncGBtoR) {
                                              applySyncGBtoR();
                                              scene.getRequests().requestShader();
                                          }
                                      }, L"Link G & B to R",
                                      L"When checked, the Green and Blue iteration intervals follow the Red interval.");
        // Default is checked, so mirror R onto G/B immediately when the panel is built - but only
        // when they have actually drifted apart. Mirroring unconditionally re-ran the shader on
        // every open of this panel, and rewrote two fields that were already correct.
        if (*syncGBtoR && (palette.iterationInterval.g != palette.iterationInterval.r ||
                           palette.iterationInterval.b != palette.iterationInterval.r)) {
            applySyncGBtoR();
            // The mirror just changed the attribute, so the image on screen no longer matches it.
            scene.getRequests().requestShader();
        }

        window->registerCheckboxInput(L"Seamless (Mirror)", &palette.seamless, [&scene] {
             scene.getRequests().requestShader();
        }, L"Seamless", L"Mirrors the palette to eliminate seams at the cycle boundary.");

        window->registerCheckboxInput(L"Enable Gloss", &palette.enableGloss, [&scene] {
             scene.getRequests().requestShader();
        }, L"Enable Gloss", L"Applies a periodic shiny reflection effect to the palette.");

        // Converts a stored linear RGB color into a COLORREF for the preview swatch.
        auto colorToColorRef = [](const glm::vec4 *color) {
            return RGB(
                static_cast<BYTE>(std::round(std::clamp(color->r, 0.0f, 1.0f) * 255.0f)),
                static_cast<BYTE>(std::round(std::clamp(color->g, 0.0f, 1.0f) * 255.0f)),
                static_cast<BYTE>(std::round(std::clamp(color->b, 0.0f, 1.0f) * 255.0f)));
        };

        window->registerColorButton(L"Gloss Color", L"Set Color...",
                               [colorToColorRef, colorPtr = &palette.glossColor] {
                                   return colorToColorRef(colorPtr);
                               },
                               [openColorPicker, colorPtr = &palette.glossColor] {
                                   openColorPicker(colorPtr);
                               }, L"Set Gloss Color", L"Opens a color picker for the gloss highlight color.", &palette.glossColor);

        window->registerColorButton(L"Mandelbrot Color", L"Set Color...",
                               [colorToColorRef, colorPtr = &palette.mandelbrotColor] {
                                   return colorToColorRef(colorPtr);
                               },
                               [openColorPicker, colorPtr = &palette.mandelbrotColor] {
                                   openColorPicker(colorPtr);
                               }, L"Set Mandelbrot Color", L"Opens a color picker for the Mandelbrot set interior color.", &palette.mandelbrotColor);

        // The lines are laid over the colors on their way to the card, so every row here moves them
        // without touching the palette itself.
        struct BandLineControls {
            HWND count = nullptr;
            HWND width = nullptr;
            HWND opacity = nullptr;
            HWND softness = nullptr;
            HWND color = nullptr;
        };
        auto bandControls = std::make_shared<BandLineControls>();
        auto updateBandEnabled = std::make_shared<std::function<void()>>();
        auto refreshBandLine = [&scene, colorSmoothingRadio, colorInterpolationRadio, repaintPreviews] {
            scene.getRequests().requestShader();
            repaintPreviews(colorSmoothingRadio);
            repaintPreviews(colorInterpolationRadio);
        };

        window->registerSectionHeader(L"Band Line");
        window->registerCheckboxInput(L"Enable Band Line", &palette.bandLineEnabled,
                                      [refreshBandLine, updateBandEnabled] {
                                          refreshBandLine();
                                          (*updateBandEnabled)();
                                      }, L"Enable Band Line",
                                      L"Draws a line across the palette at even points of its cycle, separating the color bands instead of letting them run into one another.\nThe colors themselves are left as they are - the lines are laid over them, so lifting this brings the palette back unchanged.");

        bandControls->count = window->registerTextInput<uint32_t>(L"Lines per Cycle", &palette.bandLineCount,
                                         Unparser::U_LONG, Parser::U_LONG,
                                         [](const uint32_t &v) { return v >= 1 && v <= 256; },
                                         refreshBandLine, L"Set Lines per Cycle",
                                         L"How many lines one full color cycle carries. Setting it to the number of colors in the palette puts one line on every color boundary, which is where the colors meet.\nThe cycle is Cycle Length iterations long, so the lines land that many iterations apart divided by this. 1 to 256.\nUp/Down arrows nudge by 1 (Shift = 10).", 1.0);

        bandControls->width = window->registerSliderInput(L"Line Width", &palette.bandLineWidth, 0.0f, 1.0f,
                                         Unparser::floatFixed(3), Parser::FLOAT,
                                         ValidCondition::FLOAT_ZERO_TO_ONE, refreshBandLine,
                                         L"Set Line Width",
                                         L"Thickness, as the fraction of one band the line takes up. It is measured against the band rather than in pixels or iterations, so the lines keep their proportion as Cycle Length changes and as the zoom deepens.\n0.000 leaves no line at all, 0.500 makes the line as wide as the color left beside it, and 1.000 leaves nothing but line.");

        bandControls->opacity = window->registerSliderInput(L"Line Opacity", &palette.bandLineOpacity, 0.0f, 1.0f,
                                         Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::FLOAT_ZERO_TO_ONE, refreshBandLine,
                                         L"Set Line Opacity",
                                         L"How far the line covers the color under it. 1.00 replaces it, and lower values tint the boundary instead of cutting it.");

        bandControls->softness = window->registerSliderInput(L"Line Softness", &palette.bandLineSoftness, 0.0f, 1.0f,
                                         Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::FLOAT_ZERO_TO_ONE, refreshBandLine,
                                         L"Set Line Softness",
                                         L"How far the line feathers into the colors around it. 0.00 is the hard-edged separator; raising it keeps the bright core compact while lengthening the colored transition, and at 1.00 the line has no edge left and reads as one smooth swell of its color.\nA wide, soft, white line is the smooth highlight Enable Gloss draws, except that it is placed and sized here, and it is mixed into the color rather than added to it - so it still shows on a palette already too bright for Gloss to add anything to.");

        bandControls->color = window->registerColorButton(L"Line Color", L"Set Color...",
                               [colorToColorRef, colorPtr = &palette.bandLineColor] {
                                   return colorToColorRef(colorPtr);
                               },
                               [openColorPicker, colorPtr = &palette.bandLineColor, refreshBandLine] {
                                   openColorPicker(colorPtr);
                                   refreshBandLine();
                               }, L"Set Line Color", L"Opens a color picker for the line. Black is the separator; white or a color of its own outlines the bands instead.", &palette.bandLineColor);

        *updateBandEnabled = [winPtr = window.get(), bandControls, &palette] {
            for (const HWND control : {bandControls->count, bandControls->width, bandControls->opacity,
                                       bandControls->softness, bandControls->color}) {
                if (control != nullptr) {
                    winPtr->setRowEnabled(control, palette.bandLineEnabled);
                }
            }
        };
        (*updateBandEnabled)();

        window->setWindowCloseFunction([&settingsMenu] {
            settingsMenu.setCurrentActiveSettingsWindow(nullptr);
        });
        settingsMenu.setCurrentActiveSettingsWindow(std::move(window),
                                                    SettingsMenu::SettingsWindowKind::SHADER_PALETTE);
    };
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackShader::TEXTURE = [
            ](SettingsMenu &settingsMenu, RenderScene &scene) {
        // Every control binds to a raw address, so the window edits one layer through a staging copy
        // and swaps the values behind the controls when the layer changes. Hiding one layer's rows to
        // show another's would not work here: a hidden row keeps its slot, and three hidden layers
        // would leave the panel mostly empty space.
        struct TextureEdit {
            ShdTextureLayerSelection layer = ShdTextureLayerSelection::LAYER_1;
            ShdTextureAttribute editing = {};
            HWND enabled = nullptr;
            HWND opacity = nullptr;
            HWND period = nullptr;
            HWND size = nullptr;
            HWND keepAspect = nullptr;
            HWND repeatU = nullptr;
            HWND repeatV = nullptr;
            HWND paletteFollow = nullptr;
            HWND scrollU = nullptr;
            HWND scrollV = nullptr;
            HWND pathPanel = nullptr;
            std::vector<HWND> uvMode = {};
            std::vector<HWND> blendMode = {};
        };
        auto &textures = scene.getAttribute().shader.textures;
        const auto edit = std::make_shared<TextureEdit>();
        edit->editing = textures[0];
        auto &texture = edit->editing;

        auto window = std::make_unique<SettingsWindow>(L"Texture", 640, 250);

        // Push the staged layer back into the stack. Every control's callback ends here, so the
        // layer the window is pointed at is the only one an edit can reach.
        const auto commitTexture = [&scene, &textures, edit] {
            textures[static_cast<size_t>(edit->layer)] = edit->editing;
            scene.getRequests().requestShader();
        };

        window->registerSectionHeader(L"Layer", false);
        window->registerRadioButtonInput<ShdTextureLayerSelection>(
            L"Edit Layer", &edit->layer, [winPtr = window.get(), &textures, edit] {
                // The radio has already written the new layer; load it into the controls.
                edit->editing = textures[static_cast<size_t>(edit->layer)];
                const auto &t = edit->editing;
                winPtr->setCheckboxValue(edit->enabled, t.enabled);
                winPtr->setRadioValueByGroup(edit->uvMode, std::any(t.uvMode));
                winPtr->setRadioValueByGroup(edit->blendMode, std::any(t.blendMode));
                winPtr->setFloatValueByField(edit->opacity, t.opacity);
                winPtr->setFloatValueByField(edit->period, t.periodIterations);
                winPtr->setFloatValueByField(edit->size, t.size);
                winPtr->setCheckboxValue(edit->keepAspect, t.keepAspect);
                winPtr->setFloatValueByField(edit->repeatU, t.scaleU);
                winPtr->setFloatValueByField(edit->repeatV, t.scaleV);
                winPtr->setFloatValueByField(edit->paletteFollow, t.paletteFollow);
                winPtr->setFloatValueByField(edit->scrollU, t.scrollU);
                winPtr->setFloatValueByField(edit->scrollV, t.scrollV);
                InvalidateRect(edit->pathPanel, nullptr, TRUE);
            }, L"Set Edited Layer",
            L"Which layer of the texture stack the rows below edit. All four render at once, blended bottom-up: layer 1 sits under layer 2, and so on.\nSwitching layers only changes what is shown here; it turns nothing off.");

        window->registerSectionHeader(L"Source Image");
        // Laid out as a row (0 = one standard row height): the box reads as a field, so it belongs
        // in the value column with the fields under it rather than spanning the whole panel.
        edit->pathPanel = window->registerOwnerDrawnPanel(
                0, [edit](const HDC hdc, const RECT &rc) {
                    const auto &texture = edit->editing;
                    const HBRUSH bg = CreateSolidBrush(settingsTheme().textFieldBackground);
                    FillRect(hdc, &rc, bg);
                    DeleteObject(bg);
                    SetBkMode(hdc, TRANSPARENT);
                    SetTextColor(hdc, settingsTheme().text);
                    RECT t = rc;
                    // The same horizontal padding the text fields set with EM_SETMARGINS.
                    t.left += Constants::Win32::settingsScaled(8);
                    t.right -= Constants::Win32::settingsScaled(8);
                    const std::wstring label = texture.path.empty()
                                                   ? std::wstring(L"No image selected.")
                                                   : Unparser::STRING(texture.path);
                    // DT_PATH_ELLIPSIS keeps the file name readable when the directory is long.
                    DrawTextW(hdc, label.c_str(), -1, &t, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_PATH_ELLIPSIS);
                }, L"Selected Image", L"Selected Image",
                L"The image file this layer samples. Choose one with the button below; Clear drops it.");
        window->registerButton(L"Image File", L"Choose...",
                               [winPtr = window.get(), commitTexture, edit] {
                                   const auto picked = IOUtilities::ioFileDialogMulti(
                                       L"Open Texture Image", IOUtilities::OPEN_FILE,
                                       {
                                           {L"PNG image", L"png"},
                                           {L"JPEG image", L"jpg"},
                                           {L"Bitmap image", L"bmp"},
                                           {L"Targa image", L"tga"}
                                       });
                                   if (picked == nullptr) {
                                       return;
                                   }
                                   edit->editing.path = Parser::STRING(picked->wstring());
                                   // Picking a file is the intent to use it; no separate enable step.
                                   edit->editing.enabled = true;
                                   winPtr->setCheckboxValue(edit->enabled, true);
                                   InvalidateRect(edit->pathPanel, nullptr, TRUE);
                                   commitTexture();
                               }, L"Choose Texture Image",
                               L"Picks the image painted over the escaping region. The Mandelbrot interior keeps its own color.");
        window->registerButton(L"Clear Image", L"Clear",
                               [winPtr = window.get(), commitTexture, edit] {
                                   edit->editing.path.clear();
                                   edit->editing.enabled = false;
                                   winPtr->setCheckboxValue(edit->enabled, false);
                                   InvalidateRect(edit->pathPanel, nullptr, TRUE);
                                   commitTexture();
                               }, L"Clear Texture Image",
                               L"Drops this layer's texture; the layers below it keep painting.");
        edit->enabled = window->registerCheckboxInput(L"Enabled", &texture.enabled, commitTexture,
            L"Enable Texture", L"Turns this layer on and off without forgetting the chosen image.");

        window->registerSectionHeader(L"Mapping");
        edit->uvMode = window->registerRadioButtonInput<ShdTextureUVMode>(L"UV Source", &texture.uvMode,
            commitTexture, L"Set UV Source",
           L"Cycle x Band turns the texture with the bands and still rides the color cycle.\nCycle x Angle wraps the image around the viewport center; it stretches on spirals.\nCycle x Screen runs it along the bands vertically.\nScreen pins it to the viewport like a masked overlay.");
        edit->blendMode = window->registerRadioButtonInput<ShdTextureBlendMode>(L"Blend Mode", &texture.blendMode,
            commitTexture, L"Set Blend Mode",
           L"Multiply tints the palette with the image. Overlay keeps the palette's light and dark. Replace shows the image alone.\nEach layer blends over the layers below it, so Replace on an upper layer hides them.");
        edit->opacity = window->registerSliderInput(L"Opacity", &texture.opacity, 0.0f, 1.0f, Unparser::floatFixed(2),
                                    Parser::FLOAT, ValidCondition::FLOAT_ZERO_TO_ONE, commitTexture,
                                    L"Set Texture Opacity", L"Blend weight between the layers below and this layer over them.");
        // %.2e above 1000 keeps the width steady the way the palette's Cycle Length field does.
        auto periodUnparser = [](const float &v) -> std::wstring {
            wchar_t buf[32];
            if (v >= 1e3f)
                swprintf(buf, 32, L"%.2e", v);
            else
                swprintf(buf, 32, L"%.3g", v);
            return buf;
        };
        edit->period = window->registerTextInput<float>(L"Texture Period", &texture.periodIterations, periodUnparser,
                                         Parser::FLOAT, ValidCondition::POSITIVE_FLOAT_ZERO, commitTexture,
                                         L"Set Texture Period",
                                         L"Iterations spanned by one texture tile.\n0 follows the palette's Cycle Length, which shrinks the texture whenever you shorten the color cycle. Set a value here to size the texture independently.\nUp/Down arrows step by the current order of magnitude (Shift = x10).",
                                         0.0, 1.0, 100000.0);
        // Sized inside the repeats rather than against them, so the picture is resized without the count changing with it.
        edit->size = window->registerSliderInput(
            L"Size", &texture.size, 0.1f, 20.0f, Unparser::floatFixed(2),
            Parser::FLOAT, ValidCondition::floatInRange(0.1f, 20.0f), commitTexture,
            L"Set Texture Size",
            L"How much of the space one repeat spans the image fills, drawn in the middle of it. 0.50 draws it half the size with the picture under it standing around it, and 2.00 enlarges it until that space crops it.\nHow many of the image there are is Repeat U and Repeat V, so this resizes the picture without multiplying it.");
        window->setSliderFractionalSteps(edit->size);
        edit->keepAspect = window->registerCheckboxInput(L"Keep Aspect", &texture.keepAspect, commitTexture,
            L"Keep Image Aspect",
            L"Draws the image at its own width and height, fitted inside the space one repeat spans rather than stretched to fill it, so a wide or tall picture is not squashed. What is under the layer stands in the rest of that space.\nRepeat U and Repeat V are untouched, so turning this on reshapes the picture without moving it.");
        // Fractional steps: the default whole-number stepping would round every drag to an integer,
        // which puts the whole 0 - 1 end of the range out of reach and drops the half-repeats the
        // field itself displays and accepts. The zero stop puts 0 on the bar's left end; the shader
        // only ever multiplies by this, so 0 flattens that axis to a single line of the image.
        edit->repeatU = window->registerSliderInput(
            L"Repeat U", &texture.scaleU, 0.1f, 20.0f, Unparser::floatFixed(2),
            Parser::FLOAT, ValidCondition::floatInRange(0.0f, 20.0f), commitTexture,
            L"Set U Repeat", L"Texture repeats across one period. 0 stretches one line of the image over the whole cycle.");
        window->setSliderFractionalSteps(edit->repeatU);
        window->setSliderZeroStop(edit->repeatU);
        edit->repeatV = window->registerSliderInput(
            L"Repeat V", &texture.scaleV, 0.1f, 20.0f, Unparser::floatFixed(2),
            Parser::FLOAT, ValidCondition::floatInRange(0.0f, 20.0f), commitTexture,
            L"Set V Repeat", L"Texture repeats across the second coordinate, which is the direction the bands run under Cycle x Band, the angle around the viewport center under Cycle x Angle, and screen height under the other two. 0 stretches one line of the image over it.\nUnder Cycle x Band and Cycle x Angle that coordinate runs around a circle and closes on itself, so it is taken to the nearest whole number of repeats there; a fractional one would leave a hard seam along one line of the picture.");
        window->setSliderFractionalSteps(edit->repeatV);
        window->setSliderZeroStop(edit->repeatV);

        window->registerSectionHeader(L"Animation");
        edit->paletteFollow = window->registerSliderInput(L"Palette Follow", &texture.paletteFollow, -2.0f, 2.0f,
                                    Unparser::floatFixed(2), Parser::FLOAT, ValidCondition::floatInRange(-2.0f, 2.0f),
                                    commitTexture, L"Set Palette Follow",
                                    L"How much of the palette's Color Animation the texture rides.\n1 = moves exactly with the colors. 0 = texture holds still while they animate. -1 = runs against them.");
        edit->scrollU = window->registerSliderInput(L"Scroll U", &texture.scrollU, -2.0f, 2.0f, Unparser::floatFixed(2),
                                    Parser::FLOAT, ValidCondition::floatInRange(-2.0f, 2.0f), commitTexture,
                                    L"Set U Scroll", L"Texture repeats scrolled per second along U. Negative reverses.");
        edit->scrollV = window->registerSliderInput(L"Scroll V", &texture.scrollV, -2.0f, 2.0f, Unparser::floatFixed(2),
                                    Parser::FLOAT, ValidCondition::floatInRange(-2.0f, 2.0f), commitTexture,
                                    L"Set V Scroll", L"Texture repeats scrolled per second along V. Negative reverses.");
        window->registerNotesCard(L"Stacking and animating",
                                  {
                                      {
                                          L"Layers",
                                          L"All four layers render together, bottom-up. Give each one its own Blend Mode and Opacity to build a material out of them."
                                      },
                                      {L"Scroll U / V", L"Moves the image itself, in texture repeats per second."},
                                      {
                                          L"Palette Follow",
                                          L"U is built from the palette cycle, so color animation drags the texture with it. Set 0 to keep the texture still, which is what a solid material wants."
                                      },
                                      {
                                          L"Video export",
                                          L"Both are driven by frame time, not wall clock, so the exported video matches the preview."
                                      }
                                  });

        window->setWindowCloseFunction([&settingsMenu] {
            settingsMenu.setCurrentActiveSettingsWindow(nullptr);
        });
        settingsMenu.setCurrentActiveSettingsWindow(std::move(window), SettingsMenu::SettingsWindowKind::SHADER);
    };
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackShader::PATTERN = [
            ](SettingsMenu &settingsMenu, RenderScene &scene) {
        // Same staging copy as the Texture window, and for the same reason: every control binds to a
        // raw address, so the window edits one layer through a copy and swaps the values behind the
        // controls when the layer changes.
        struct PatternEdit {
            ShdPatternLayerSelection layer = ShdPatternLayerSelection::LAYER_1;
            ShdPatternAttribute editing = {};
            HWND enabled = nullptr;
            HWND sharpness = nullptr;
            HWND paletteShift = nullptr;
            HWND inkColor = nullptr;
            HWND edgeEnabled = nullptr;
            HWND edgeColor = nullptr;
            HWND edgeWidth = nullptr;
            HWND edgeOpacity = nullptr;
            HWND edgeRelative = nullptr;
            HWND opacity = nullptr;
            HWND period = nullptr;
            HWND repeatU = nullptr;
            HWND repeatV = nullptr;
            HWND paletteFollow = nullptr;
            HWND scrollU = nullptr;
            HWND scrollV = nullptr;
            std::vector<HWND> type = {};
            std::vector<HWND> uvMode = {};
            std::vector<HWND> blendMode = {};
            std::vector<HWND> inkMode = {};
        };
        auto &patterns = scene.getAttribute().shader.patterns;
        const auto edit = std::make_shared<PatternEdit>();
        edit->editing = patterns[0];
        auto &pattern = edit->editing;

        auto window = std::make_unique<SettingsWindow>(L"Pattern", 640, 250);

        auto updateEnabled = std::make_shared<std::function<void()>>();
        // Push the staged layer back into the stack. Every control's callback ends here, so the
        // layer the window is pointed at is the only one an edit can reach.
        const auto commitPattern = [&scene, &patterns, edit, updateEnabled] {
            patterns[static_cast<size_t>(edit->layer)] = edit->editing;
            scene.getRequests().requestShader();
            if (*updateEnabled) {
                (*updateEnabled)();
            }
        };

        auto colorToColorRef = [](const glm::vec4 *color) {
            return RGB(
                static_cast<BYTE>(std::round(std::clamp(color->r, 0.0f, 1.0f) * 255.0f)),
                static_cast<BYTE>(std::round(std::clamp(color->g, 0.0f, 1.0f) * 255.0f)),
                static_cast<BYTE>(std::round(std::clamp(color->b, 0.0f, 1.0f) * 255.0f)));
        };
        auto openColorPicker = [commitPattern, winPtr = window.get()](glm::vec4 *color) {
            static COLORREF customColors[16] = {};
            CHOOSECOLORW cc = {};
            cc.lStructSize = sizeof(cc);
            cc.hwndOwner = winPtr->getWindow();
            cc.lpCustColors = customColors;
            cc.rgbResult = RGB(
                static_cast<BYTE>(std::round(std::clamp(color->r, 0.0f, 1.0f) * 255.0f)),
                static_cast<BYTE>(std::round(std::clamp(color->g, 0.0f, 1.0f) * 255.0f)),
                static_cast<BYTE>(std::round(std::clamp(color->b, 0.0f, 1.0f) * 255.0f)));
            cc.Flags = CC_FULLOPEN | CC_RGBINIT;
            if (ChooseColorW(&cc)) {
                color->r = static_cast<float>(GetRValue(cc.rgbResult)) / 255.0f;
                color->g = static_cast<float>(GetGValue(cc.rgbResult)) / 255.0f;
                color->b = static_cast<float>(GetBValue(cc.rgbResult)) / 255.0f;
                commitPattern();
            }
        };

        window->registerSectionHeader(L"Layer", false);
        window->registerRadioButtonInput<ShdPatternLayerSelection>(
            L"Edit Layer", &edit->layer, [winPtr = window.get(), &patterns, edit, updateEnabled] {
                // The radio has already written the new layer; load it into the controls.
                edit->editing = patterns[static_cast<size_t>(edit->layer)];
                const auto &p = edit->editing;
                winPtr->setCheckboxValue(edit->enabled, p.enabled);
                winPtr->setCheckboxValue(edit->edgeEnabled, p.edgeEnabled);
                winPtr->setCheckboxValue(edit->edgeRelative, p.edgeRelative);
                winPtr->setRadioValueByGroup(edit->type, std::any(p.type));
                winPtr->setRadioValueByGroup(edit->uvMode, std::any(p.uvMode));
                winPtr->setRadioValueByGroup(edit->blendMode, std::any(p.blendMode));
                winPtr->setRadioValueByGroup(edit->inkMode, std::any(p.inkMode));
                winPtr->setFloatValueByField(edit->sharpness, p.sharpness);
                winPtr->setFloatValueByField(edit->paletteShift, p.paletteShift);
                winPtr->setFloatValueByField(edit->edgeWidth, p.edgeWidth);
                winPtr->setFloatValueByField(edit->edgeOpacity, p.edgeOpacity);
                winPtr->setFloatValueByField(edit->opacity, p.opacity);
                winPtr->setFloatValueByField(edit->period, p.periodIterations);
                winPtr->setFloatValueByField(edit->repeatU, p.scaleU);
                winPtr->setFloatValueByField(edit->repeatV, p.scaleV);
                winPtr->setFloatValueByField(edit->paletteFollow, p.paletteFollow);
                winPtr->setFloatValueByField(edit->scrollU, p.scrollU);
                winPtr->setFloatValueByField(edit->scrollV, p.scrollV);
                // Both swatches are owner-drawn from the staged copy, so they repaint on demand.
                InvalidateRect(edit->inkColor, nullptr, TRUE);
                InvalidateRect(edit->edgeColor, nullptr, TRUE);
                // The new layer's Ink Mode and Edge Enabled grey out a different set of rows below.
                if (*updateEnabled) {
                    (*updateEnabled)();
                }
            }, L"Set Edited Layer",
            L"Which layer of the pattern stack the rows below edit. All four render at once, blended bottom-up: layer 1 sits under layer 2, and so on.\nSwitching layers only changes what is shown here; it turns nothing off.\nOne layer draws one shape at one Edge Width, and those two settle a single scale between them - a width that leaves a thin seam draws the shape as a tile, a width that swallows it leaves only grain. Put the two on separate layers to have both at once.");

        window->registerSectionHeader(L"Shape");
        edit->enabled = window->registerCheckboxInput(L"Enabled", &pattern.enabled, [commitPattern] {
            commitPattern();
        }, L"Enable Pattern",
           L"Draws a generated pattern over the escaping region. Needs no image file, and stacks on top of the Texture layer and of the pattern layers below it.");
        edit->type = window->registerRadioButtonInput<ShdPatternType>(L"Pattern", &pattern.type, [commitPattern] {
            commitPattern();
        }, L"Set Pattern",
           L"Shape drawn in the pattern's ink color. Every shape tiles seamlessly across the UV cell, so none of them seam where the color cycle wraps.");
        edit->sharpness = window->registerSliderInput(L"Sharpness", &pattern.sharpness, 0.0f, 1.0f, Unparser::floatFixed(2),
                                    Parser::FLOAT, ValidCondition::FLOAT_ZERO_TO_ONE, [commitPattern] {
                                        commitPattern();
                                    }, L"Set Sharpness",
                                    L"Edge hardness of the shape. 0 leaves it a soft gradient, 1 cuts it to a hard border.");

        window->registerSectionHeader(L"Ink");
        edit->inkMode = window->registerRadioButtonInput<ShdPatternInkMode>(L"Ink Mode", &pattern.inkMode, [commitPattern] {
            commitPattern();
        }, L"Set Ink Mode",
           L"Palette Shift reads the covered area from your own palette, further along the cycle, so the pattern comes out as colored as the fractal.\nSolid Color paints one flat color instead.");
        edit->paletteShift = window->registerSliderInput(L"Palette Shift", &pattern.paletteShift, 0.0f, 1.0f,
                                    Unparser::floatFixed(2), Parser::FLOAT, ValidCondition::FLOAT_ZERO_TO_ONE,
                                    [commitPattern] {
                                        commitPattern();
                                    }, L"Set Palette Shift",
                                    L"How far around the palette cycle the covered area is read, in cycles. 0.5 lands on the opposite side, the strongest contrast the palette offers.\nAt 0 the ink is the color already under it, which makes the pattern vanish under Replace. Multiply and Overlay still fold that color into itself there, so the shapes stay visible as darkening and lightening.");
        edit->inkColor = window->registerColorButton(L"Ink Color", L"Pick Color",
                               [colorToColorRef, colorPtr = &pattern.color] {
                                   return colorToColorRef(colorPtr);
                               },
                               [openColorPicker, colorPtr = &pattern.color] {
                                   openColorPicker(colorPtr);
                               }, L"Set Ink Color",
                               L"Opens a color picker for the ink used in Solid Color mode. The uncovered part of the pattern is left untouched.", &pattern.color);

        window->registerSectionHeader(L"Edge");
        edit->edgeEnabled = window->registerCheckboxInput(L"Edge Enabled", &pattern.edgeEnabled, [commitPattern] {
            commitPattern();
        }, L"Enable Edge",
           L"Draws a line along the border between the pattern and the color beside it. The line straddles that border, so it separates the two instead of outlining either one.");
        edit->edgeColor = window->registerColorButton(L"Edge Color", L"Pick Color",
                               [colorToColorRef, colorPtr = &pattern.edgeColor] {
                                   return colorToColorRef(colorPtr);
                               },
                               [openColorPicker, colorPtr = &pattern.edgeColor] {
                                   openColorPicker(colorPtr);
                               }, L"Set Edge Color",
                               L"Opens a color picker for the edge. White or black separates the two sides most strongly; the edge is painted flat and does not go through the Blend Mode.", &pattern.edgeColor);
        edit->edgeWidth = window->registerSliderInput(L"Edge Width", &pattern.edgeWidth, -0.5f, 1.0f,
                                    Unparser::floatFixed(2), Parser::FLOAT, ValidCondition::floatInRange(-0.5f, 1.0f),
                                    [commitPattern] {
                                        commitPattern();
                                    }, L"Set Edge Width",
                                    L"Thickness of the edge. It is measured against the shape itself, so raising Repeat U or V shrinks the shapes and the edge with them.\nHigh values eventually swallow the thinner side of the pattern entirely.\nAt 0 the edge is still as wide as the shape's own soft border, so go below 0 for a thinner line. It thins away to nothing at -0.5 with Sharpness at 0, and just under 0 with Sharpness at 1.");
        edit->edgeOpacity = window->registerSliderInput(L"Edge Opacity", &pattern.edgeOpacity, 0.0f, 1.0f,
                                    Unparser::floatFixed(2), Parser::FLOAT, ValidCondition::FLOAT_ZERO_TO_ONE,
                                    [commitPattern] {
                                        commitPattern();
                                    }, L"Set Edge Opacity",
                                    L"Blend weight of the edge. Lower it to let the colors underneath show through the line.");

        edit->edgeRelative = window->registerCheckboxInput(L"Relative Width", &pattern.edgeRelative,
                                    [commitPattern] {
                                        commitPattern();
                                    }, L"Relative Edge Width",
                                    L"Measures Edge Width against each shape's own height instead of in raw units. Every shape rises to a different height, and the two sides of one shape rarely match, so the width that swallows a shape whole is a different number for each of them and has to be hunted for.\nOn, that number is the same everywhere: just under 1.00 the line has eaten all of the pattern but the very peaks, which survive as isolated grain scattered through the color.\nHow much survives up there is the shape's own business. Stripes and Waves peak along a ridge and still hold a thin line of color at 0.99; Dots and Diamond peak at a point and are down to a few dozen pixels of the frame by 0.95, so 0.85 to 0.92 is where their grain is still there to see.");

        window->registerSectionHeader(L"Mapping");
        edit->uvMode = window->registerRadioButtonInput<ShdTextureUVMode>(L"UV Source", &pattern.uvMode, [commitPattern] {
            commitPattern();
        }, L"Set UV Source",
           L"Cycle x Band turns the pattern with the bands and still rides the color cycle.\nCycle x Angle wraps it around the viewport center; it stretches on spirals.\nCycle x Screen runs it along the bands vertically.\nScreen pins it to the viewport like a masked overlay.\nOnly the last three vary in two directions, so only they draw the shape as a lattice - see the notes below.");
        edit->blendMode = window->registerRadioButtonInput<ShdTextureBlendMode>(L"Blend Mode", &pattern.blendMode, [commitPattern] {
            commitPattern();
        }, L"Set Blend Mode",
           L"Multiply tints the palette with the ink. Overlay keeps the palette's light and dark. Replace paints the ink flat.");
        edit->opacity = window->registerSliderInput(L"Opacity", &pattern.opacity, 0.0f, 1.0f, Unparser::floatFixed(2),
                                    Parser::FLOAT, ValidCondition::FLOAT_ZERO_TO_ONE, [commitPattern] {
                                        commitPattern();
                                    }, L"Set Pattern Opacity", L"Blend weight of the pattern where it covers.");
        // %.2e above 1000 keeps the width steady the way the palette's Cycle Length field does.
        auto periodUnparser = [](const float &v) -> std::wstring {
            wchar_t buf[32];
            if (v >= 1e3f)
                swprintf(buf, 32, L"%.2e", v);
            else
                swprintf(buf, 32, L"%.3g", v);
            return buf;
        };
        edit->period = window->registerTextInput<float>(L"Pattern Period", &pattern.periodIterations, periodUnparser,
                                         Parser::FLOAT, ValidCondition::POSITIVE_FLOAT_ZERO, [commitPattern] {
                                             commitPattern();
                                         }, L"Set Pattern Period",
                                         L"Iterations spanned by one pattern tile.\n0 follows the palette's Cycle Length, which shrinks the pattern whenever you shorten the color cycle. Set a value here to size the pattern independently.\nUp/Down arrows step by the current order of magnitude (Shift = x10).",
                                         0.0, 1.0, 100000.0);
        // See the Texture window: without these the drag rounds to whole cells, losing the 0 - 1 end
        // of the range and the half-cell values the field shows, and 0 stays off the bar.
        edit->repeatU = window->registerSliderInput(
            L"Repeat U", &pattern.scaleU, 0.1f, 200.0f, Unparser::floatFixed(2),
                                    Parser::FLOAT, ValidCondition::floatInRange(0.0f, 200.0f), [commitPattern] {
                                        commitPattern();
                                    }, L"Set U Repeat",
                                    L"Pattern cells packed into one period. Raise it to shrink the shapes. 0 holds the pattern at one position across the whole cycle.");
        window->setSliderFractionalSteps(edit->repeatU);
        window->setSliderZeroStop(edit->repeatU);
        edit->repeatV = window->registerSliderInput(
            L"Repeat V", &pattern.scaleV, 0.1f, 200.0f, Unparser::floatFixed(2),
                                    Parser::FLOAT, ValidCondition::floatInRange(0.0f, 200.0f), [commitPattern] {
                                        commitPattern();
                                    }, L"Set V Repeat",
                                    L"Pattern cells across the second coordinate. Under Cycle x Band that coordinate is the direction the bands run, not distance along them, so this only subdivides where the bands turn. Use Cycle x Screen or Screen for a true grid.\nUnder Cycle x Band and Cycle x Angle that coordinate runs around a circle and closes on itself, so it is taken to the nearest whole number of repeats there; a fractional one would leave a hard seam along one line of the picture.");
        window->setSliderFractionalSteps(edit->repeatV);
        window->setSliderZeroStop(edit->repeatV);

        window->registerSectionHeader(L"Animation");
        edit->paletteFollow = window->registerSliderInput(L"Palette Follow", &pattern.paletteFollow, -2.0f, 2.0f,
                                    Unparser::floatFixed(2), Parser::FLOAT, ValidCondition::floatInRange(-2.0f, 2.0f), [commitPattern] {
                                        commitPattern();
                                    }, L"Set Palette Follow",
                                    L"How much of the palette's Color Animation the pattern rides.\n1 = moves exactly with the colors. 0 = pattern holds still while they animate. -1 = runs against them.");
        edit->scrollU = window->registerSliderInput(L"Scroll U", &pattern.scrollU, -2.0f, 2.0f, Unparser::floatFixed(2),
                                    Parser::FLOAT, ValidCondition::floatInRange(-2.0f, 2.0f), [commitPattern] {
                                        commitPattern();
                                    }, L"Set U Scroll", L"Pattern cells scrolled per second along U. Negative reverses.");
        edit->scrollV = window->registerSliderInput(L"Scroll V", &pattern.scrollV, -2.0f, 2.0f, Unparser::floatFixed(2),
                                    Parser::FLOAT, ValidCondition::floatInRange(-2.0f, 2.0f), [commitPattern] {
                                        commitPattern();
                                    }, L"Set V Scroll", L"Pattern cells scrolled per second along V. Negative reverses.");
        window->registerNotesCard(L"Using the pattern",
                                  {
                                      {
                                          L"Cycle x Band",
                                          L"U rides the color cycle and V turns with the bands, so the pattern follows the fractal's own shape instead of lying flat on the screen. It is the mode worth trying first."
                                      },
                                      {
                                          L"Drawing a lattice",
                                          L"Cycle x Band cannot. Its V is the direction the iteration field climbs and nothing else, and where the bands run parallel that direction is the same all along them, so the shape is constant along a band and comes out as stripes running across it. Repeat V does not help. Use Cycle x Screen, Screen or Cycle x Angle, whose two axes move independently."
                                      },
                                      {
                                          L"Repeat U / V",
                                          L"Raise these to pack more cells into one color band. Very high values eventually alias into noise in the deep filigree."
                                      },
                                      {
                                          L"Stacking",
                                          L"Four pattern layers render at once, after the image texture, each over the result of the ones below it. A tile on one layer and grain on another is the usual reason to reach for a second one, since a layer's Edge Width can only be set for one of the two."
                                      },
                                      {
                                          L"Edge",
                                          L"A white or black edge reads as a drawn outline and holds the shapes apart where the two colors are close in brightness. Raise Sharpness with it to keep the line crisp."
                                      }
                                  });

        *updateEnabled = [winPtr = window.get(), edit] {
            const auto &pattern = edit->editing;
            const bool paletteInk = pattern.inkMode == ShdPatternInkMode::PALETTE_SHIFT;
            auto enable = [winPtr](const HWND control, const bool enabled) {
                if (control != nullptr) {
                    winPtr->setRowEnabled(control, enabled);
                }
            };
            enable(edit->paletteShift, paletteInk);
            enable(edit->inkColor, !paletteInk);
            enable(edit->edgeColor, pattern.edgeEnabled);
            enable(edit->edgeWidth, pattern.edgeEnabled);
            enable(edit->edgeOpacity, pattern.edgeEnabled);
            enable(edit->edgeRelative, pattern.edgeEnabled);
        };
        (*updateEnabled)();

        window->setWindowCloseFunction([&settingsMenu] {
            settingsMenu.setCurrentActiveSettingsWindow(nullptr);
        });
        settingsMenu.setCurrentActiveSettingsWindow(std::move(window), SettingsMenu::SettingsWindowKind::SHADER);
    };
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackShader::WARP = [
            ](SettingsMenu &settingsMenu, RenderScene &scene) {
        auto &warp = scene.getAttribute().shader.warp;
        auto window = std::make_unique<SettingsWindow>(L"Warp", 640, 250);

        struct WarpControls {
            std::vector<HWND> source;
            HWND amount = nullptr;
            HWND octaves = nullptr;
            std::vector<HWND> uvMode;
            HWND repeatU = nullptr;
            HWND repeatV = nullptr;
            HWND period = nullptr;
            HWND paletteFollow = nullptr;
            HWND scrollU = nullptr;
            HWND scrollV = nullptr;
        };
        auto controls = std::make_shared<WarpControls>();
        auto updateEnabled = std::make_shared<std::function<void()>>();
        auto requestWarpShader = [&scene, updateEnabled] {
            scene.getRequests().requestShader();
            if (*updateEnabled) {
                (*updateEnabled)();
            }
        };

        window->registerSectionHeader(L"Field", false);
        window->registerCheckboxInput(L"Enabled", &warp.enabled, [requestWarpShader] {
            requestWarpShader();
        }, L"Enable Warp",
           L"Bends the color bands themselves instead of painting over them. Each pixel is colored from a little further along or back the palette cycle, by an amount a smooth field decides, so the bands swirl and marble while the fractal underneath is untouched.");
        controls->source = window->registerRadioButtonInput<ShdWarpSource>(L"Warp Source", &warp.source,
            [requestWarpShader] {
                requestWarpShader();
            }, L"Set Warp Source",
            L"Field the displacement is read from.\nNoise generates its own and needs no image file.\nA Texture choice reads the brightness of that layer's image, weighted the way a black-and-white copy of it would be, so the picture drives the swirl. A strong blue reads dark and a strong green reads light, well apart from how light they look. Give that layer an image first; the layer itself may stay switched off, and lowering its Opacity to 0 warps without painting it.");
        controls->amount = window->registerSliderInput(L"Warp Amount", &warp.amount, 0.0f, 2.0f,
                                    Unparser::floatFixed(2), Parser::FLOAT, ValidCondition::floatInRange(0.0f, 2.0f),
                                    [requestWarpShader] {
                                        requestWarpShader();
                                    }, L"Set Warp Amount",
                                    L"Palette cycles the coloring is pushed by where the field is at its strongest. Small values ripple the bands, 0.5 and up fold them over one another into marbling. 0 leaves the coloring alone.");
        controls->octaves = window->registerSliderInput(L"Warp Detail", &warp.octaves, 1.0f, 6.0f,
                                    Unparser::floatFixed(0), Parser::FLOAT, ValidCondition::floatInRange(1.0f, 6.0f),
                                    [requestWarpShader] {
                                        requestWarpShader();
                                    }, L"Set Warp Detail",
                                    L"Layers of detail in the generated field. 1 is a single smooth swirl; higher adds finer turbulence inside it. Only used by the Noise source.");

        window->registerSectionHeader(L"Mapping");
        controls->uvMode = window->registerRadioButtonInput<ShdTextureUVMode>(L"UV Source", &warp.uvMode,
            [requestWarpShader] {
                requestWarpShader();
            }, L"Set UV Source",
            L"Cycle x Band turns the field with the bands, which folds them along their own length.\nCycle x Angle wraps it around the viewport center.\nCycle x Screen runs it along the bands vertically.\nScreen pins it to the viewport, so the warp stays put while the fractal moves through it.");
        // Matching the Texture and Pattern windows: without these the drag rounds to whole cells and
        // 0 stays off the bar.
        controls->repeatU = window->registerSliderInput(L"Repeat U", &warp.scaleU, 0.1f, 200.0f,
                                    Unparser::floatFixed(2), Parser::FLOAT, ValidCondition::floatInRange(0.0f, 200.0f),
                                    [requestWarpShader] {
                                        requestWarpShader();
                                    }, L"Set U Repeat",
                                    L"Warp cells packed into one period. Raise it for a tighter, busier swirl. 0 holds the field at one position across the whole cycle.");
        window->setSliderFractionalSteps(controls->repeatU);
        window->setSliderZeroStop(controls->repeatU);
        controls->repeatV = window->registerSliderInput(L"Repeat V", &warp.scaleV, 0.1f, 200.0f,
                                    Unparser::floatFixed(2), Parser::FLOAT, ValidCondition::floatInRange(0.0f, 200.0f),
                                    [requestWarpShader] {
                                        requestWarpShader();
                                    }, L"Set V Repeat",
                                    L"Warp cells across the second coordinate. Under Cycle x Band that coordinate is the direction the bands run, so use Cycle x Screen or Screen for an evenly spread field.\nUnder Cycle x Band and Cycle x Angle that coordinate runs around a circle and closes on itself, so it is taken to the nearest whole number of repeats there; a fractional one would leave a hard seam along one line of the picture.");
        window->setSliderFractionalSteps(controls->repeatV);
        window->setSliderZeroStop(controls->repeatV);
        // %.2e above 1000 keeps the width steady the way the palette's Cycle Length field does.
        auto warpPeriodUnparser = [](const float &v) -> std::wstring {
            wchar_t buf[32];
            if (v >= 1e3f)
                swprintf(buf, 32, L"%.2e", v);
            else
                swprintf(buf, 32, L"%.3g", v);
            return buf;
        };
        controls->period = window->registerTextInput<float>(L"Warp Period", &warp.periodIterations, warpPeriodUnparser,
                                         Parser::FLOAT, ValidCondition::POSITIVE_FLOAT_ZERO, [requestWarpShader] {
                                             requestWarpShader();
                                         }, L"Set Warp Period",
                                         L"Iterations spanned by one warp tile.\n0 follows the palette's Cycle Length, which resizes the swirl whenever you shorten the color cycle. Set a value here to size it independently.\nUp/Down arrows step by the current order of magnitude (Shift = x10).",
                                         0.0, 1.0, 100000.0);

        window->registerSectionHeader(L"Animation");
        controls->paletteFollow = window->registerSliderInput(L"Palette Follow", &warp.paletteFollow, -2.0f, 2.0f,
                                    Unparser::floatFixed(2), Parser::FLOAT, ValidCondition::floatInRange(-2.0f, 2.0f),
                                    [requestWarpShader] {
                                        requestWarpShader();
                                    }, L"Set Palette Follow",
                                    L"How much of the palette's Color Animation the warp rides.\n1 = the swirl travels with the colors, so the shape holds. 0 = the swirl stands still and the colors run through it. -1 = runs against them.");
        controls->scrollU = window->registerSliderInput(L"Scroll U", &warp.scrollU, -2.0f, 2.0f,
                                    Unparser::floatFixed(2), Parser::FLOAT, ValidCondition::floatInRange(-2.0f, 2.0f),
                                    [requestWarpShader] {
                                        requestWarpShader();
                                    }, L"Set U Scroll", L"Warp cells scrolled per second along U. Negative reverses.");
        controls->scrollV = window->registerSliderInput(L"Scroll V", &warp.scrollV, -2.0f, 2.0f,
                                    Unparser::floatFixed(2), Parser::FLOAT, ValidCondition::floatInRange(-2.0f, 2.0f),
                                    [requestWarpShader] {
                                        requestWarpShader();
                                    }, L"Set V Scroll", L"Warp cells scrolled per second along V. Negative reverses.");
        window->registerNotesCard(L"Using the warp",
                                  {
                                      {
                                          L"Where to start",
                                          L"Noise, Warp Amount around 0.3 and Repeat U around 4. Raise Warp Amount from there until the bands begin to fold into one another."
                                      },
                                      {
                                          L"Warping with a picture",
                                          L"Choose the layer in Warp Source, give that layer an image in the Texture window, and set its Opacity to 0. The image then shapes the coloring without being seen."
                                      },
                                      {
                                          L"Order",
                                          L"The warp runs before the texture and pattern layers, so those ride the bent bands rather than the straight ones."
                                      }
                                  });

        *updateEnabled = [winPtr = window.get(), controls, &warp] {
            const bool active = warp.enabled;
            auto enable = [winPtr](const HWND control, const bool enabled) {
                if (control != nullptr) {
                    winPtr->setRowEnabled(control, enabled);
                }
            };
            for (const HWND item : controls->source) {
                enable(item, active);
            }
            for (const HWND item : controls->uvMode) {
                enable(item, active);
            }
            enable(controls->amount, active);
            enable(controls->octaves, active && warp.source == ShdWarpSource::NOISE);
            enable(controls->repeatU, active);
            enable(controls->repeatV, active);
            enable(controls->period, active);
            enable(controls->paletteFollow, active);
            enable(controls->scrollU, active);
            enable(controls->scrollV, active);
        };
        (*updateEnabled)();

        window->setWindowCloseFunction([&settingsMenu] {
            settingsMenu.setCurrentActiveSettingsWindow(nullptr);
        });
        settingsMenu.setCurrentActiveSettingsWindow(std::move(window), SettingsMenu::SettingsWindowKind::SHADER);
    };
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackShader::STRIPE = [
            ](SettingsMenu &settingsMenu, RenderScene &scene) {
        auto &[stripeType, firstInterval, secondInterval, opacity, offset, animationSpeed] = scene.getAttribute().shader.stripe;
        auto window = std::make_unique<SettingsWindow>(L"Stripe");
        window->registerRadioButtonInput<ShdStripeType>(L"Stripe Type", &stripeType, [&scene] {
            scene.getRequests().requestShader();
        }, L"Set Stripe Type", L"Sets the stripe coloring algorithm; controls how the stripes are computed and overlaid.");
        window->registerTextInput<float>(L"Interval 1", &firstInterval, Unparser::floatFixed(1), Parser::FLOAT,
                                         ValidCondition::POSITIVE_FLOAT, [&scene] {
                                             scene.getRequests().requestShader();
                                         }, L"Set Interval 1", L"Sets the first Stripe Interval.\nUp/Down arrows step by the current order of magnitude (Shift = x10).", 0.0, 1.0, 100000.0);
        window->registerTextInput<float>(L"Interval 2", &secondInterval, Unparser::floatFixed(1), Parser::FLOAT,
                                         ValidCondition::POSITIVE_FLOAT, [&scene] {
                                             scene.getRequests().requestShader();
                                         }, L"Set Interval 2", L"Sets the second Stripe Interval.\nUp/Down arrows step by the current order of magnitude (Shift = x10).", 0.0, 1.0, 100000.0);
        window->registerTextInput<float>(L"Opacity", &opacity, Unparser::floatFixed(1), Parser::FLOAT,
                                         ValidCondition::FLOAT_ZERO_TO_ONE, [&scene] {
                                             scene.getRequests().requestShader();
                                         }, L"Set Opacity", L"Sets the opacity of stripes.\nUp/Down arrows nudge by 0.01 (Shift = 0.1).", 0.01);
        window->registerTextInput<float>(L"Offset", &offset, Unparser::floatFixed(1), Parser::FLOAT,
                                         ValidCondition::ALL_FLOAT, [&scene] {
                                             scene.getRequests().requestShader();
                                         }, L"Set Offset", L"Start offset iteration of stripes.\nUp/Down arrows nudge by 1 (Shift = 10).", 1.0);
        window->registerTextInput<float>(L"Animation Speed", &animationSpeed, Unparser::floatFixed(1),
                                         Parser::FLOAT,
                                         ValidCondition::ALL_FLOAT, [&scene] {
                                             scene.getRequests().requestShader();
                                         }, L"Set Animation Speed", L"Sets the stripe animation speed.\nUp/Down arrows nudge by 1 (Shift = 10).", 1.0);
        window->setWindowCloseFunction([&settingsMenu] {
            settingsMenu.setCurrentActiveSettingsWindow(nullptr);
        });
        settingsMenu.setCurrentActiveSettingsWindow(std::move(window), SettingsMenu::SettingsWindowKind::SHADER);
    };
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackShader::SLOPE = [
            ](SettingsMenu &settingsMenu, RenderScene &scene) {
        auto &slope_attr = scene.getAttribute().shader.slope;
        auto window = std::make_unique<SettingsWindow>(L"Slope", 700, 250);
        auto colorToColorRef = [](const glm::vec4 *color) {
            return RGB(
                static_cast<BYTE>(std::round(std::clamp(color->r, 0.0f, 1.0f) * 255.0f)),
                static_cast<BYTE>(std::round(std::clamp(color->g, 0.0f, 1.0f) * 255.0f)),
                static_cast<BYTE>(std::round(std::clamp(color->b, 0.0f, 1.0f) * 255.0f)));
        };
        auto openColorPicker = [&scene, winPtr = window.get()](glm::vec4 *color) {
            static COLORREF customColors[16] = {};
            CHOOSECOLORW cc = {};
            cc.lStructSize = sizeof(cc);
            cc.hwndOwner = winPtr->getWindow();
            cc.lpCustColors = customColors;
            cc.rgbResult = RGB(
                static_cast<BYTE>(std::round(std::clamp(color->r, 0.0f, 1.0f) * 255.0f)),
                static_cast<BYTE>(std::round(std::clamp(color->g, 0.0f, 1.0f) * 255.0f)),
                static_cast<BYTE>(std::round(std::clamp(color->b, 0.0f, 1.0f) * 255.0f)));
            cc.Flags = CC_FULLOPEN | CC_RGBINIT;
            if (ChooseColorW(&cc)) {
                color->r = static_cast<float>(GetRValue(cc.rgbResult)) / 255.0f;
                color->g = static_cast<float>(GetGValue(cc.rgbResult)) / 255.0f;
                color->b = static_cast<float>(GetBValue(cc.rgbResult)) / 255.0f;
                scene.getRequests().requestShader();
            }
        };
        const auto angleUnparser = [](const float &v) {
            wchar_t buf[32];
            swprintf(buf, 32, L"%.0f%c", v == 0.0f ? 0.0f : v, static_cast<wchar_t>(0x00B0));
            return std::wstring(buf);
        };
        const auto angleParser = [](std::wstring &s) {
            std::erase(s, static_cast<wchar_t>(0x00B0));
            return std::stof(s);
        };

        struct SlopeControls {
            std::vector<HWND> shadingBlend;
            HWND lightZenith = nullptr;
            HWND lightAzimuth = nullptr;
            HWND fillIntensity = nullptr;
            HWND fillZenith = nullptr;
            HWND fillDirection = nullptr;
            HWND specularPower = nullptr;
            HWND specularColor = nullptr;
            HWND specularIndependent = nullptr;
            HWND specularZenith = nullptr;
            HWND specularAzimuth = nullptr;
            HWND specularAnisotropy = nullptr;
            HWND specularAnisotropyAngle = nullptr;
            HWND reliefResponse = nullptr;
            HWND ambientSkyColor = nullptr;
            HWND ambientGroundColor = nullptr;
            HWND rimPower = nullptr;
            HWND rimColor = nullptr;
            std::vector<HWND> glossSource;
            HWND glossRelief = nullptr;
            HWND glossBands = nullptr;
            HWND glossSharpness = nullptr;
            HWND glossPhase = nullptr;
            HWND glossColor = nullptr;
            std::vector<HWND> lightBlend;
            HWND highlightKnee = nullptr;
            HWND shadowFloor = nullptr;
            HWND terminatorSoftness = nullptr;
            HWND slopeGamma = nullptr;
            HWND tintResponse = nullptr;
            HWND shadowChroma = nullptr;
            std::vector<HWND> tintBlend;
        };
        auto controls = std::make_shared<SlopeControls>();
        auto updateEnabled = std::make_shared<std::function<void()>>();
        auto requestSlopeShader = [&scene, updateEnabled] {
            scene.getRequests().requestShader();
            if (*updateEnabled) {
                (*updateEnabled)();
            }
        };

        // 0 to 10000 to six decimals, 0 being the shader's own off switch.
        auto reliefDepthValidCondition = [](const float &v) {
            return v >= 0.0f && v <= 10000.0f;
        };
        window->registerSectionHeader(L"Slope Shading", false);
        // Declared from 0.000001 so the decades above come out on a power-of-ten grid; the zero stop
        // below then turns the bottom window into a linear 0-0.00001, putting 0 on the bar's left end.
        const HWND shadingDepth = window->registerSliderInput(
                                         L"Shading Depth", &slope_attr.depth, 0.000001f, 10000.0f,
                                         Unparser::floatTrim(6), Parser::FLOAT,
                                         reliefDepthValidCondition, [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Shading Depth",
                                         L"Height of the relief the light is cast on, driving the diffuse shading and the specular highlight.\nRim Light and Cavity Shading read the same relief but scale with their own intensity, not with this.\nFrom 0 to 10000 to six decimals; 0 disables slope shading.\nAbove about 0.001 every slope of a wide view is driven past vertical, and a surface already past vertical cannot be tilted further - so the shading stops telling the fractal's steep parts from its flat ones and lights them alike. The far field of a wide view is hundreds of times flatter than the boundary, and the decades below that are where the difference comes back: the boundary keeps its relief while the field around it goes quiet.");
        // The whole-number step was the field's own display; six decimals are shown now.
        window->setSliderFractionalSteps(shadingDepth);
        window->setSliderZeroStop(shadingDepth);

        controls->shadowFloor = window->registerSliderInput(L"Shadow Floor", &slope_attr.reflectionRatio, 0.0f, 1.0f,
                                         Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::FLOAT_ZERO_TO_ONE, [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Shadow Floor",
                                         L"Lowest level the directional shading is allowed to reach, so a shadowed area never goes darker than it. Range: 0.00 to 1.00.\nThe shading is cut off at the floor rather than lifted onto it, so everything below it comes out as one flat tone; Terminator Softness eases that away.\n1.00 flattens the directional shading only; specular, rim, cavity and tint still apply.");

        window->registerSliderInput(L"Opacity", &slope_attr.opacity, 0.0f, 1.0f, Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::FLOAT_ZERO_TO_ONE, [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Opacity", L"Sets the opacity of the slope. It blends the finished slope result, including diffuse shading, brightness, tint, cavity, specular and rim.\n0 disables the whole slope pass. Range: 0.00 to 1.00.");

        controls->terminatorSoftness = window->registerSliderInput(L"Terminator Softness", &slope_attr.terminatorSoftness, 0.0f, 1.0f,
                                         Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::FLOAT_ZERO_TO_ONE, [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Terminator Softness",
                                         L"Wraps the light past the shadow edge so it arrives as a gradient instead of a crease.\n0 = plain Lambert falloff, 1 = fully wrapped and very soft.");

        window->registerSliderInput(L"Relief Lightness", &slope_attr.lumaAmount, 0.0f, 1.0f,
                                         Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::FLOAT_ZERO_TO_ONE, [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Relief Lightness",
                                         L"How much of the relief is carried by lightness. 1.00 is the ordinary shading; lower it and the light stops darkening the picture without the light itself moving.\nAt 0.00 the palette keeps its own lightness everywhere and the whole relief is left to Lit / Shadow Tint, Cavity Shading and the highlight — the form is then made of color temperature alone, which is the one composite a strongly colored palette comes through undimmed.\nShadow Floor, Terminator Softness and Slope Gamma all shape the lightness this scales, so they do nothing at 0.00.");

        controls->shadingBlend = window->registerRadioButtonInput<ShdSlopeShadingBlend>(
            L"Shading Blend", &slope_attr.shadingBlend, [requestSlopeShader] {
                requestSlopeShader();
            }, L"Shading Blend",
            L"How the shading is laid on the palette color.\nOverlay works on the red, green and blue values, which pulls the color of a shaded area away from the palette's.\nOKLab Lightness darkens only what the eye reads as lightness, so a shaded area keeps its hue and its strength of color. It shades deeper than Overlay does, so raise Slope Brightness to match.");

        window->registerSectionHeader(L"Relief Detail");
        window->registerSliderInput(L"Macro Relief", &slope_attr.macroRelief, 0.0f, 1.0f,
                                         Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::FLOAT_ZERO_TO_ONE, [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Macro Relief",
                                         L"Blends a wider-radius slope to add large-scale undulation. 0 = fine detail only (default).");

        const HWND macroRadius = window->registerSliderInput(L"Macro Radius", &slope_attr.macroRadius, 1.0f, 12.0f,
                                         Unparser::floatFixed(1), Parser::FLOAT,
                                         ValidCondition::floatInRange(1.0f, 12.0f), [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Macro Radius",
                                         L"Sampling radius of the macro slope in reference pixels. Larger = broader relief. Only matters when Macro Relief > 0.");
        window->setSliderFractionalSteps(macroRadius);

        window->registerSectionHeader(L"Light Direction");
        controls->lightZenith = window->registerSliderInput(L"Light Zenith", &slope_attr.zenith, 0.0f, 359.0f,
                                         angleUnparser, angleParser,
                                         ValidCondition::FLOAT_DEGREE, [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Light Zenith", L"Angle of the light away from straight overhead. 0 puts it directly above, 90 puts it level with the horizon, so raising this lowers the light rather than lifting it. Range: 0 <= x < 360 degrees.");

        controls->lightAzimuth = window->registerSliderInput(L"Light Direction", &slope_attr.azimuth, 0.0f, 359.0f,
                                         angleUnparser, angleParser,
                                         ValidCondition::FLOAT_DEGREE, [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Azimuth", L"Sets the azimuth of the slope. Range: 0 <= x < 360 degrees.");

        window->registerSectionHeader(L"Fill Light");
        controls->fillIntensity = window->registerSliderInput(L"Fill Intensity", &slope_attr.fillIntensity, 0.0f, 1.0f,
                                         Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::FLOAT_ZERO_TO_ONE, [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Fill Intensity",
                                         L"A second diffuse light with its own Zenith and Direction below, folded into the shading before Shadow Floor, Terminator Softness and Slope Gamma act, so those shape both lights together.\nWhere Shadow Floor holds every shadow at one flat level whichever way the surface faces, this raises only what is turned towards the fill, so the form inside a shadow survives instead of flattening out. A surface turned away from it gains nothing - the fill lifts shadows, it never casts its own. 0.00 = off.");

        controls->fillZenith = window->registerSliderInput(L"Fill Zenith", &slope_attr.fillZenith, 0.0f, 359.0f,
                                         angleUnparser, angleParser,
                                         ValidCondition::FLOAT_DEGREE, [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Fill Zenith",
                                         L"Angle of the fill light away from straight overhead, as Light Zenith is for the key. Only matters when Fill Intensity > 0.");

        controls->fillDirection = window->registerSliderInput(L"Fill Direction", &slope_attr.fillAzimuth, 0.0f, 359.0f,
                                         angleUnparser, angleParser,
                                         ValidCondition::FLOAT_DEGREE, [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Fill Azimuth",
                                         L"Azimuth of the fill light. It starts opposite Light Direction, so raising Fill Intensity opens up the key's own shadows first. Only matters when Fill Intensity > 0.");

        window->registerSectionHeader(L"Specular");
        // Specular packs the most rows of any section; give each row a few extra px of
        // breathing room so the dense list reads less cramped (reset before the next section).
        window->setExtraRowGap(3);
        window->registerSliderInput(L"Specular Intensity", &slope_attr.specularIntensity, 0.0f, 1.0f,
                                         Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::FLOAT_ZERO_TO_ONE, [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Specular Intensity", L"Sets the intensity of specular highlight. 0 = matte, 1 = full specular.");

        controls->specularPower = window->registerSliderInput(L"Specular Power", &slope_attr.specularPower, 1.0f, 100000.0f,
                                         Unparser::floatFixed(0), Parser::FLOAT,
                                         ValidCondition::floatInRange(1.0f, 100000.0f), [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Specular Power",
                                         L"Sets the sharpness of specular highlight. Higher = smaller and sharper.");

        controls->reliefResponse = window->registerSliderInput(L"Relief Response", &slope_attr.reliefResponse, 0.0f, 1.0f,
                                         Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::FLOAT_ZERO_TO_ONE, [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Relief Response",
                                         L"How much of the highlight's angle is taken from the steepness of the relief rather than from the light.\nAt 0 the highlight is angled to meet the light exactly, so it lands wherever a slope faces the light and reads as a clean highlight. This is what Shading Depth calls for: it drives the relief far past vertical, which leaves steepness with almost nothing left to say while the direction stays exact.\nRaise it only where the relief is gentle enough for its steepness to mean something. Too high on a deep relief breaks the highlight into thin filaments.");

        controls->specularColor = window->registerColorButton(L"Specular Color", L"Pick Color",
                               [colorToColorRef, colorPtr = &slope_attr.specularColor] {
                                   return colorToColorRef(colorPtr);
                               },
                               [openColorPicker, colorPtr = &slope_attr.specularColor] {
                                   openColorPicker(colorPtr);
                               }, L"Set Specular Color", L"Tints the specular highlight (e.g. warm gold for a metallic look). White = neutral.", &slope_attr.specularColor);

        // Unity-like placement: when "Independent Light" is on, the highlight uses its own
        // zenith/azimuth so it can be moved around the feature without dragging the shadows.
        controls->specularIndependent = window->registerCheckboxInput(L"Independent Light", &slope_attr.specularIndependent, [requestSlopeShader] {
            requestSlopeShader();
        }, L"Independent Specular Light",
        L"When checked, the specular highlight uses its own Zenith/Azimuth below instead of following the main Light Direction. Lets you move the gloss without moving the shadows.");

        controls->specularZenith = window->registerSliderInput(L"Specular Zenith", &slope_attr.specularZenith, 0.0f, 359.0f,
                                         angleUnparser, angleParser,
                                         ValidCondition::FLOAT_DEGREE, [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Specular Zenith",
                                         L"Zenith of the dedicated specular light. Only used when Independent Light is checked.");

        controls->specularAzimuth = window->registerSliderInput(L"Specular Azimuth", &slope_attr.specularAzimuth, 0.0f, 359.0f,
                                         angleUnparser, angleParser,
                                         ValidCondition::FLOAT_DEGREE, [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Specular Azimuth",
                                         L"Azimuth of the dedicated specular light. Only used when Independent Light is checked.");

        controls->specularAnisotropy = window->registerSliderInput(L"Specular Anisotropy", &slope_attr.specularAnisotropy, 0.0f, 1.0f,
                                         Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::FLOAT_ZERO_TO_ONE, [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Specular Anisotropy",
                                         L"Stretches the round highlight into an ellipse. 0 = round, 1 = strongly stretched.");

        controls->specularAnisotropyAngle = window->registerSliderInput(L"Anisotropy Angle", &slope_attr.specularAnisotropyAngle, 0.0f, 359.0f,
                                         angleUnparser, angleParser,
                                         ValidCondition::FLOAT_DEGREE, [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Anisotropy Angle",
                                         L"Direction the highlight is stretched along. Only matters when Anisotropy > 0.");

        window->setExtraRowGap(0);
        window->registerSectionHeader(L"Cavity Shading");
        window->registerSliderInput(L"Cavity Intensity", &slope_attr.aoIntensity, 0.0f, 1.0f,
                                         Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::FLOAT_ZERO_TO_ONE, [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Cavity Intensity",
                                         L"Darkens concave pits to deepen the 3D relief. It reads how sharply the relief curves at each pixel rather than what stands in the light's way, so a pit is darkened wherever it is and nothing casts a shadow onto anything else.\n0 = off, 1 = full.");

        window->registerSectionHeader(L"Lit / Shadow Tint");
        window->registerSliderInput(L"Tint Intensity", &slope_attr.ambientIntensity, 0.0f, 1.0f,
                                         Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::FLOAT_ZERO_TO_ONE, [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Tint Intensity",
                                         L"Carries a two-color tint across the picture, from the color below towards the one above it as a surface turns to face the light. It follows the light, not which way a surface points, so moving Light Zenith or Light Direction moves where each color lands. 0 = off.");

        controls->ambientSkyColor = window->registerColorButton(L"Light-Facing Color", L"Pick Color",
                               [colorToColorRef, colorPtr = &slope_attr.skyColor] {
                                   return colorToColorRef(colorPtr);
                               },
                               [openColorPicker, colorPtr = &slope_attr.skyColor] {
                                   openColorPicker(colorPtr);
                               }, L"Set Light-Facing Color", L"Tint applied to the areas turned towards the light. Near-white keeps them at the palette color; warm for a sunlit look.", &slope_attr.skyColor);

        controls->ambientGroundColor = window->registerColorButton(L"Shadow-Facing Color", L"Pick Color",
                               [colorToColorRef, colorPtr = &slope_attr.groundColor] {
                                   return colorToColorRef(colorPtr);
                               },
                               [openColorPicker, colorPtr = &slope_attr.groundColor] {
                                   openColorPicker(colorPtr);
                               }, L"Set Shadow-Facing Color", L"Tint applied to the areas turned away from the light. A cool tint gives the classic cool-shadow / warm-light 3D look.", &slope_attr.groundColor);

        controls->tintBlend = window->registerRadioButtonInput<ShdSlopeTintBlend>(
            L"Tint Blend", &slope_attr.tintBlend, [requestSlopeShader] {
                requestSlopeShader();
            }, L"Tint Blend",
            L"How the two tint colors meet the palette color.\nMultiply scales the red, green and blue values by the tint, as every earlier version did. Against a strongly colored palette the two hues fight and mostly what survives is the darkening, so raising the tint dims the picture rather than warming and cooling it.\nOKLab Tint splits the tint into a lightness and a color of its own: the lightness scales as Multiply does, while the color is added, so a shadow takes the tint's hue on instead of being dimmed towards it. It is also what Shadow Chroma needs to work.");

        controls->tintResponse = window->registerSliderInput(L"Tint Response", &slope_attr.tintResponse, 0.10f, 4.0f,
                                         Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::floatInRange(0.10f, 4.0f), [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Tint Response",
                                         L"Curve on the crossing from the shadow color to the light-facing one. Shading Depth saturates the relief, so the two normally swap over a very short band and meet in a hard seam.\nBelow 1.00 the light-facing color takes over early and spreads; above 1.00 it is held back to the parts most squarely facing the light, tightening the seam. 1.00 = the straight crossing. It moves nothing else about the light.");

        controls->shadowChroma = window->registerSliderInput(L"Shadow Chroma", &slope_attr.shadowChroma, 0.0f, 2.0f,
                                         Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::floatInRange(0.0f, 2.0f), [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Shadow Chroma",
                                         L"Strength of color on the shadow side, left at the lightness it already has. Above 1.00 a shadow deepens in color as it turns away from the light, which is how a shadow behaves in paint and is not something a lightness curve can imitate; below 1.00 it drains towards grey.\n1.00 = unchanged. Needs Tint Blend on OKLab Tint.");

        window->registerSectionHeader(L"Tone Mapping");
        window->registerTextInput<float>(L"Slope Brightness", &slope_attr.brightness, Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::POSITIVE_FLOAT, [&scene] {
                                             scene.getRequests().requestShader();
                                         }, L"Set Slope Brightness", L"Sets the brightness of the slope shading. >1 to brighten.\nUp/Down arrows nudge by 0.05 (Shift = 0.5).", 0.05);

        controls->slopeGamma = window->registerTextInput<float>(L"Slope Gamma", &slope_attr.gamma, Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::POSITIVE_FLOAT, [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Slope Gamma", L"Sets the gamma of the slope shading. >1 to soften shadows.\nUp/Down arrows nudge by 0.05 (Shift = 0.5).", 0.05);

        controls->lightBlend = window->registerRadioButtonInput<ShdSlopeLightBlend>(
            L"Light Blend", &slope_attr.lightBlend, [requestSlopeShader] {
                requestSlopeShader();
            }, L"Light Blend",
            L"Where the specular highlight and the rim light are added to the shaded color.\nDirect adds them to the color values as they are written out and cuts them off at white, as every earlier version did. Slope Brightness is the one part that differs: it now rolls off into Highlight Knee rather than being scaled back to white point, so a bright area sits a little lower and keeps its relief.\nLinear RGB Add adds them in proportion to real light instead, so a highlight keeps its color as it brightens rather than reaching white early and flattening into a tintless patch. It is the addition done in linear RGB, and is not the blend mode of that name in an image editor.\nLinear RGB Add is not Direct plus a highlight: Highlight Knee shapes the whole picture, so anything already brighter than the knee is pulled down a little even where there is no highlight at all.");

        controls->highlightKnee = window->registerSliderInput(L"Highlight Knee", &slope_attr.highlightKnee, 0.0f, 1.0f,
                                         Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::FLOAT_ZERO_TO_ONE, [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Highlight Knee",
                                         L"Where the roll towards white starts. Anything above this level is eased towards white on all three channels at once, so a blown highlight keeps its tint instead of clipping one channel at a time.\nIt is a curve over the whole picture, not only over the highlight: an area already brighter than the knee is eased down by it even with no highlight on it. At 0.75 a white area leaves at about 0.94. That headroom is what the highlight rolls off into, so it cannot be had without it — raise the knee to give up the roll-off and keep the brightness.\nLower = earlier and softer. 1.00 = hard clip.\nBoth Light Blend modes use it: Linear RGB Add rolls the lit result off into it, Direct rolls the brightened shading off into it before the highlight is added.");

        window->registerSectionHeader(L"Rim Light");
        window->registerSliderInput(L"Rim Intensity", &slope_attr.rimIntensity, 0.0f, 1.0f,
                                         Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::FLOAT_ZERO_TO_ONE, [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Rim Intensity", L"Sets the intensity of rim lighting (Fresnel effect).");

        controls->rimPower = window->registerSliderInput(L"Rim Power", &slope_attr.rimPower, 1.0f, 64.0f,
                                         Unparser::floatFixed(0), Parser::FLOAT,
                                         ValidCondition::floatInRange(1.0f, 64.0f), [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Rim Power", L"Sets the falloff of rim lighting. Lower = wider rim.");

        // Rim color reuses the shared color-picker helpers defined at the top of this window.
        controls->rimColor = window->registerColorButton(L"Rim Color", L"Pick Color",
                               [colorToColorRef, colorPtr = &slope_attr.rimColor] {
                                   return colorToColorRef(colorPtr);
                               },
                               [openColorPicker, colorPtr = &slope_attr.rimColor] {
                                   openColorPicker(colorPtr);
                               }, L"Set Rim Color", L"Opens a color picker for the rim lighting color.", &slope_attr.rimColor);

        window->registerSectionHeader(L"Gloss");
        window->registerSliderInput(L"Gloss Intensity", &slope_attr.glossIntensity, 0.0f, 1.0f,
                                         Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::FLOAT_ZERO_TO_ONE, [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Gloss Intensity",
                                         L"Narrow bright bands laid across the relief. It is the shape Palette Gloss draws, put on a coordinate the surface owns instead of on the palette's cycle.\nBecause it is read off the relief, the bands stay on the same surfaces while the colors animate, they are unmoved by which palette is loaded, and at a new location they arrive already placed on its form instead of landing wherever that location's iteration count happens to fall.\nIt is added as light alongside the highlight and the rim, so Light Blend and Highlight Knee shape how it rolls off, and it fades out in shadow and on flat ground rather than lighting them. 0.00 = off.");

        controls->glossSource = window->registerRadioButtonInput<ShdSlopeGlossSource>(
            L"Gloss Source", &slope_attr.glossSource, [requestSlopeShader] {
                requestSlopeShader();
            }, L"Gloss Source",
            L"Which reading of the relief the bands are laid along.\nFine Shading uses the surface's answer to the light on a relief of its own, read at Gloss Relief rather than Shading Depth, so the bands ring every curved form from its crest outward and hold their place however deep the shading is set.\nShading uses the same answer on the shaded surface itself. At the usual Shading Depth almost every point faces fully toward or away from the light, so the bands gather on the line between lit and shadow; Gloss Phase 0.25 seats one on the lit side instead.\nRelief Detail uses the fine slope with no Shading Depth on it, which is the one reading of the relief that is not driven past its range, so the bands pick out levels of detail density instead: the filigree lights and a smooth expanse stays dark. This is the one that carries between locations most nearly unchanged.\nSlope Facing uses the direction the surface points, so the bands run out radially from every spiral and swirl and read as a brushed, drawn-out sheen.");

        controls->glossRelief = window->registerSliderInput(L"Gloss Relief", &slope_attr.glossRelief, 0.0f, 16.0f,
                                         Unparser::floatFixed(1), Parser::FLOAT,
                                         ValidCondition::floatInRange(0.0f, 16.0f), [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Gloss Relief",
                                         L"How steeply the relief is read for Fine Shading, in doublings: each step of 1 doubles the slope the bands answer to, so 8 reads it 256 times steeper than the raw height and 16 reads it 65536 times steeper.\nIt is the gloss's own Shading Depth and is not moved by that control, so the bands stay in their place when the shading is deepened.\nLow values spread the bands wide over the broad forms; high values pull them in tight around the fine relief.");
        window->setSliderFractionalSteps(controls->glossRelief);

        controls->glossBands = window->registerSliderInput(L"Gloss Bands", &slope_attr.glossBands, 1.0f, 32.0f,
                                         Unparser::floatFixed(1), Parser::FLOAT,
                                         ValidCondition::floatInRange(1.0f, 32.0f), [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Gloss Bands",
                                         L"How many bright bands are laid across the whole range of the source. More bands = more lines, each covering less.\nOn Slope Facing the coordinate is a full turn that joins back onto itself, so the count is rounded to a whole number there to keep the bands from meeting in a seam.");
        window->setSliderFractionalSteps(controls->glossBands);

        controls->glossSharpness = window->registerSliderInput(L"Gloss Sharpness", &slope_attr.glossSharpness, 1.0f, 256.0f,
                                         Unparser::floatFixed(0), Parser::FLOAT,
                                         ValidCondition::floatInRange(1.0f, 256.0f), [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Gloss Sharpness",
                                         L"Width of each band. Higher pulls it into a thinner line, which is what holds the lit part of the picture down to the small fraction that leaves the rest reading as dark.\n1 leaves a broad wave rather than a band. Palette Gloss draws its own at 20.");

        controls->glossPhase = window->registerSliderInput(L"Gloss Phase", &slope_attr.glossPhase, 0.0f, 1.0f,
                                         Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::FLOAT_ZERO_TO_ONE, [requestSlopeShader] {
                                             requestSlopeShader();
                                         }, L"Set Gloss Phase",
                                         L"Slides every band along the source together. 1.00 is one whole band apart, so 0.00 and 1.00 draw the same picture.\nUse it to move the bands off a feature they happen to land on, without changing how many there are or how wide they run.");

        controls->glossColor = window->registerColorButton(L"Gloss Color", L"Pick Color",
                               [colorToColorRef, colorPtr = &slope_attr.glossColor] {
                                   return colorToColorRef(colorPtr);
                               },
                               [openColorPicker, colorPtr = &slope_attr.glossColor] {
                                   openColorPicker(colorPtr);
                               }, L"Set Gloss Color", L"Color of the gloss bands. It is the gloss's own color rather than the palette's, so a warm white here keeps the bands reading as light on metal however the coloring is animated.", &slope_attr.glossColor);

        *updateEnabled = [winPtr = window.get(), controls, &slope_attr] {
            const bool slopeActive = slope_attr.depth > 0.0f && slope_attr.opacity > 0.0f;
            const bool specularActive = slopeActive && slope_attr.specularIntensity > 0.0f;
            const bool independentSpecular = specularActive && slope_attr.specularIndependent;
            auto enable = [winPtr](const HWND control, const bool enabled) {
                if (control != nullptr) {
                    winPtr->setRowEnabled(control, enabled);
                }
            };
            for (const HWND item : controls->shadingBlend) {
                enable(item, slopeActive);
            }
            enable(controls->lightZenith, slopeActive);
            enable(controls->lightAzimuth, slopeActive);
            const bool fillActive = slopeActive && slope_attr.fillIntensity > 0.0f;
            enable(controls->fillIntensity, slopeActive);
            enable(controls->fillZenith, fillActive);
            enable(controls->fillDirection, fillActive);
            enable(controls->specularPower, specularActive);
            enable(controls->specularColor, specularActive);
            enable(controls->specularIndependent, specularActive);
            enable(controls->specularZenith, independentSpecular);
            enable(controls->specularAzimuth, independentSpecular);
            enable(controls->specularAnisotropy, specularActive);
            enable(controls->specularAnisotropyAngle, specularActive && slope_attr.specularAnisotropy > 0.0f);
            enable(controls->reliefResponse, specularActive);
            // Every one of these shapes the lightness Relief Lightness scales, so at 0 they do nothing.
            const bool lumaActive = slopeActive && slope_attr.lumaAmount > 0.0f;
            enable(controls->shadowFloor, lumaActive);
            enable(controls->terminatorSoftness, lumaActive);
            enable(controls->slopeGamma, lumaActive);
            const bool tintActive = slopeActive && slope_attr.ambientIntensity > 0.0f;
            enable(controls->ambientSkyColor, tintActive);
            enable(controls->ambientGroundColor, tintActive);
            enable(controls->tintResponse, tintActive);
            for (const HWND item : controls->tintBlend) {
                enable(item, tintActive);
            }
            // Chroma is the half of the tint only the OKLab composite keeps apart from lightness.
            enable(controls->shadowChroma, tintActive && slope_attr.tintBlend == ShdSlopeTintBlend::OKLAB);
            enable(controls->rimPower, slopeActive && slope_attr.rimIntensity > 0.0f);
            enable(controls->rimColor, slopeActive && slope_attr.rimIntensity > 0.0f);
            const bool glossActive = slopeActive && slope_attr.glossIntensity > 0.0f;
            for (const HWND item : controls->glossSource) {
                enable(item, glossActive);
            }
            enable(controls->glossRelief, glossActive && slope_attr.glossSource == ShdSlopeGlossSource::SHADING_FINE);
            enable(controls->glossBands, glossActive);
            enable(controls->glossSharpness, glossActive);
            enable(controls->glossPhase, glossActive);
            enable(controls->glossColor, glossActive);
            for (const HWND item : controls->lightBlend) {
                enable(item, slopeActive);
            }
            // Both modes end on the same shoulder: Linear Light over the lit result, Direct over the
            // brightened base, so the knee shapes either one.
            enable(controls->highlightKnee, slopeActive);
        };
        (*updateEnabled)();

        window->setWindowCloseFunction([&settingsMenu] {
            settingsMenu.setCurrentActiveSettingsWindow(nullptr);
        });
        settingsMenu.setCurrentActiveSettingsWindow(std::move(window), SettingsMenu::SettingsWindowKind::SHADER);
    };
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackShader::COLOR = [
            ](SettingsMenu &settingsMenu, RenderScene &scene) {
        auto &[gamma, exposure, hue, saturation, brightness, contrast] = scene.getAttribute().shader.color;
        auto window = std::make_unique<SettingsWindow>(L"Color");
        window->registerTextInput<float>(L"Gamma", &gamma, Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::ALL_FLOAT, [&scene] {
                                             scene.getRequests().requestShader();
                                         }, L"Set Gamma", L"Adjusts the brightness curve. <1 darkens midtones, >1 brightens them.\nUp/Down arrows nudge by 0.01 (Shift = 0.1).", 0.01);
        window->registerTextInput<float>(L"Exposure", &exposure, Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::ALL_FLOAT, [&scene] {
                                             scene.getRequests().requestShader();
                                         }, L"Set Exposure", L"Scales overall brightness multiplicatively. 0 = unchanged, >0 brighter, <0 darker.\nUp/Down arrows nudge by 0.01 (Shift = 0.1).", 0.01);
        window->registerTextInput<float>(L"Hue", &hue, Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::ALL_FLOAT, [&scene] {
                                             scene.getRequests().requestShader();
                                         }, L"Set Hue", L"Rotates all colors around the hue wheel. 0 = unchanged.\nUp/Down arrows nudge by 0.01 (Shift = 0.1).", 0.01);
        window->registerTextInput<float>(L"Saturation", &saturation, Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::ALL_FLOAT, [&scene] {
                                             scene.getRequests().requestShader();
                                         }, L"Set Saturation", L"0 = original, -1 = grayscale, >0 = more vivid.\nUp/Down arrows nudge by 0.01 (Shift = 0.1).", 0.01);
        window->registerTextInput<float>(L"Brightness", &brightness, Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::ALL_FLOAT, [&scene] {
                                             scene.getRequests().requestShader();
                                         }, L"Set Brightness", L"Adds a constant offset to all channels. 0 = unchanged.\nUp/Down arrows nudge by 0.01 (Shift = 0.1).", 0.01);
        window->registerTextInput<float>(L"Contrast", &contrast, Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::floatInRange(-1.0f, 1.0f), [&scene] {
                                             scene.getRequests().requestShader();
                                         }, L"Set Contrast", L"Stretches or compresses the color range around the midpoint. 0 = unchanged.\n-1.00 flattens the picture to one mid tone; past that it would swap black and white, so the range stops there. Range: -1.00 to 1.00.\nUp/Down arrows nudge by 0.01 (Shift = 0.1).", 0.01);
        window->setWindowCloseFunction([&settingsMenu] {
            settingsMenu.setCurrentActiveSettingsWindow(nullptr);
        });
        settingsMenu.setCurrentActiveSettingsWindow(std::move(window), SettingsMenu::SettingsWindowKind::SHADER);
    };
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackShader::HDR = [
            ](SettingsMenu &settingsMenu, RenderScene &scene) {
        auto &[use, exposure, headroom, method] = scene.getAttribute().shader.hdr;
        auto window = std::make_unique<SettingsWindow>(L"HDR");
        window->registerCheckboxInput(L"Use HDR", &use, [&scene] {
            scene.getRequests().requestShader();
        }, L"Use HDR",
        L"Lets the picture hold light brighter than white. Bloom is the pass that makes it: with HDR on its glow is kept as the value it is, up to Headroom, instead of being cut off at white, and a curve brings it back down at the very end. What changes is the top end - a blown-out core stops reading as a flat white plate and keeps its shape - and the rest of the picture comes down with it, since white is no longer the brightest thing in the frame. Off is exactly what every earlier version drew, pass for pass. It is also what an HDR video export needs, since without it there is nothing above white to encode.");
        window->registerTextInput<float>(L"Exposure", &exposure, Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::ALL_FLOAT, [&scene] {
                                             scene.getRequests().requestShader();
                                         }, L"Set Exposure",
                                         L"Stops of light applied before the curve, so the whole picture slides along it. +1 doubles the light, -1 halves it. Unlike Color > Exposure this happens after Fog and Bloom and in linear light, which is what lets it move the glow into or out of the shoulder rather than just brightening everything.\nUp/Down arrows nudge by 0.01 (Shift = 0.1).", 0.01);
        window->registerTextInput<float>(L"Headroom", &headroom, Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::POSITIVE_FLOAT, [&scene] {
                                             scene.getRequests().requestShader();
                                         }, L"Set Headroom",
                                         L"The linear value that comes out as display white, and the ceiling the glow is held at. 1 leaves nothing above white and the curve does almost nothing; 2 gives one stop of highlight to roll off, 4 gives two. Raise it while a bright core still reads as a flat plate, lower it if the picture goes grey - the whole frame is scaled into the range below it, so a high value darkens everything that is not glowing. In an HDR export this is also the value that lands on Peak Brightness.\nUp/Down arrows nudge by 0.1 (Shift = 1).", 0.1);
        window->registerRadioButtonInput<ShdToneMapMethod>(L"Tone Map", &method, [&scene] {
            scene.getRequests().requestShader();
        }, L"Set Tone Map",
        L"The shape of the curve that brings the above-white light back down. Every one of them is normalized so Headroom lands exactly on white. Clip is a straight cut with no shoulder at all - the old look, scaled. Reinhard rolls off gently and evenly, keeping colors where they were. ACES (Narkowicz fit) is a compact approximation of the ACES RRT+ODT look, not the ACES transform itself; it adds a contrastier toe and a hard shoulder that desaturates as it clips. Filmic (Hable) sits between Reinhard and that fit, with a deeper toe.");
        window->setWindowCloseFunction([&settingsMenu] {
            settingsMenu.setCurrentActiveSettingsWindow(nullptr);
        });
        settingsMenu.setCurrentActiveSettingsWindow(std::move(window), SettingsMenu::SettingsWindowKind::SHADER);
    };
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackShader::FOG = [
            ](SettingsMenu &settingsMenu, RenderScene &scene) {
        auto &fog_attr = scene.getAttribute().shader.fog;
        auto &[radius, opacity, centerStart, centerInvert, rimMask, rimMaskBoost, rimBlur,
               focusAmount, focusRatio, focusRange, focusFalloff, focusBlur, blurQuality] = fog_attr;
        auto window = std::make_unique<SettingsWindow>(L"Fog");
        window->registerTextInput<float>(L"Radius", &radius, Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::FLOAT_ZERO_TO_ONE, [&scene] {
                                             scene.getRequests().requestShader();
                                         }, L"Set Radius", L"Blur radius of the haze, as a fraction of the frame height. Higher = broader, flatter smear.\nUp/Down arrows nudge by 0.01 (Shift = 0.1).", 0.01);
        window->registerTextInput<float>(L"Opacity", &opacity, Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::FLOAT_ZERO_TO_ONE, [&scene] {
                                             scene.getRequests().requestShader();
                                         }, L"Set Opacity", L"Maximum opacity of the fog. 0 = invisible, 1 = fully opaque.\nUp/Down arrows nudge by 0.01 (Shift = 0.1).", 0.01);
        window->registerTextInput<float>(L"Center Start", &centerStart, Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::FLOAT_ZERO_TO_ONE, [&scene] {
                                             scene.getRequests().requestShader();
                                         }, L"Set Center Start", L"How far out from the screen center the fog begins, ramping to full at the frame edge.\n0 = whole frame evenly (off), 0.5 = outer half, 1 = edge only. Combines with Rim Mask.\nWith Invert Falloff on it reads the other way: the radius of the fully fogged core.\nUp/Down arrows nudge by 0.01 (Shift = 0.1).", 0.01);
        window->registerCheckboxInput(L"Invert Falloff", &centerInvert, [&scene] {
            scene.getRequests().requestShader();
        }, L"Invert Center Falloff",
        L"Flips which side of the frame the fog sits on. Off = clear center, haze building toward the edges (the shallow side of a zoom). On = haze over the center clearing outwards, so the deep side you are zooming into is the part that fades away.");
        window->registerTextInput<float>(L"Rim Mask", &rimMask, Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::FLOAT_ZERO_TO_ONE, [&scene] {
                                             scene.getRequests().requestShader();
                                         }, L"Set Rim Mask", L"Confines the fog to where Slope's rim light lands. 0 = whole frame, 1 = rim only.\nWorks even with Rim Intensity at 0; Rim Power controls how tight the masked band is.\nUp/Down arrows nudge by 0.01 (Shift = 0.1).", 0.01);
        window->registerTextInput<float>(L"Rim Mask Boost", &rimMaskBoost, Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::POSITIVE_FLOAT, [&scene] {
                                             scene.getRequests().requestShader();
                                         }, L"Set Rim Mask Boost", L"Strength of the masked band. The raw rim footprint is faint, so 1 only dims the fog;\nraise it until the masked area reaches full opacity. Higher = wider, stronger band.\nUp/Down arrows nudge by 1 (Shift = 10).", 1.0);
        window->registerTextInput<float>(L"Rim Blur", &rimBlur, Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::POSITIVE_FLOAT_ZERO, [&scene] {
                                             scene.getRequests().requestShader();
                                         }, L"Set Rim Blur", L"Radius in pixels (at 1280 wide, capped at 16) the masked band dissolves into. Unlike Radius,\nthis samples the image at full resolution, so the band softens without looking downscaled.\nUp/Down arrows nudge by 1 (Shift = 10).", 1.0);

        window->registerRadioButtonInput<ShdFogBlurQuality>(L"Blur Quality", &blurQuality, [&scene] {
            scene.getRequests().requestShader();
        }, L"Set Blur Quality",
        L"How much Rim Blur and Focus Blur are allowed to cost. Speed holds both to 16 pixels of the frame being rendered, which is what every earlier version drew: the preview and a video rendered at several times its size then blur by different fractions of the picture, and the larger the render the less of it the band covers. Appearance spends the radius the setting asks for at any render size, so a video matches the preview it was set up in, for about four times the fog pass.");

        struct FocusControls {
            HWND ratio = nullptr;
            HWND range = nullptr;
            HWND falloff = nullptr;
            HWND blur = nullptr;
        };
        auto controls = std::make_shared<FocusControls>();
        auto updateEnabled = std::make_shared<std::function<void()>>();
        auto requestFogShader = [&scene, updateEnabled] {
            scene.getRequests().requestShader();
            if (*updateEnabled) {
                (*updateEnabled)();
            }
        };

        window->registerSectionHeader(L"Focus Band");
        window->registerSliderInput(L"Focus Amount", &focusAmount, 0.0f, 1.0f, Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::FLOAT_ZERO_TO_ONE, [requestFogShader] {
                                             requestFogShader();
                                         }, L"Set Focus Amount",
                                         L"Throws everything outside one band of the fractal's own depth out of focus, the way a lens holds one distance and lets the rest go. The band is measured on the iteration count rather than on the screen, so it holds the same structure as the view zooms instead of a fixed place in the frame.\n0 = off, 1 = full defocus at the far end. The interior is left sharp.");

        controls->ratio = window->registerSliderInput(L"Focus Depth", &focusRatio, 0.0f, 1.0f, Unparser::floatFixed(2),
                                         Parser::FLOAT, ValidCondition::FLOAT_ZERO_TO_ONE, [requestFogShader] {
                                             requestFogShader();
                                         }, L"Set Focus Depth",
                                         L"Where the sharp band sits, as a fraction of the frame's maximum iteration. 0 holds the shallow outside, 1 the last band before the interior.\nIt is a fraction rather than a count so the band stays where you put it as the zoom deepens and the count grows with it. Animating it across a video sends the focus through the structure.");

        controls->range = window->registerSliderInput(L"Focus Range", &focusRange, 0.01f, 1.0f, Unparser::floatFixed(2),
                                         Parser::FLOAT, ValidCondition::floatInRange(0.01f, 1.0f), [requestFogShader] {
                                             requestFogShader();
                                         }, L"Set Focus Range",
                                         L"Half-width of the sharp band, in the same fraction of the maximum iteration. Small = a thin sheet of the structure in focus, large = most of the frame sharp with only the far ends melting.");

        controls->falloff = window->registerSliderInput(L"Focus Falloff", &focusFalloff, 0.10f, 4.0f,
                                         Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::floatInRange(0.10f, 4.0f), [requestFogShader] {
                                             requestFogShader();
                                         }, L"Set Focus Falloff",
                                         L"Curve out of the band. Above 1.00 the sharp part is held wider and gives way suddenly near the edge; below 1.00 the picture starts softening as soon as it leaves the band. 1.00 = a straight ramp.");

        controls->blur = window->registerTextInput<float>(L"Focus Blur", &focusBlur, Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::POSITIVE_FLOAT_ZERO, [requestFogShader] {
                                             requestFogShader();
                                         }, L"Set Focus Blur",
                                         L"Radius in pixels (at 1280 wide, capped at 16) at the far end of the band. It is the radius that grows with the distance out of focus, not a fade to a blurred copy, so a pixel just outside the band is softened rather than half-replaced.\nUp/Down arrows nudge by 1 (Shift = 10).", 1.0);

        *updateEnabled = [winPtr = window.get(), controls, &fog_attr] {
            const bool focusActive = fog_attr.focusAmount > 0.0f;
            auto enable = [winPtr](const HWND control, const bool enabled) {
                if (control != nullptr) {
                    winPtr->setRowEnabled(control, enabled);
                }
            };
            enable(controls->ratio, focusActive);
            enable(controls->range, focusActive);
            enable(controls->falloff, focusActive);
            enable(controls->blur, focusActive);
        };
        (*updateEnabled)();

        window->setWindowCloseFunction([&settingsMenu] {
            settingsMenu.setCurrentActiveSettingsWindow(nullptr);
        });
        settingsMenu.setCurrentActiveSettingsWindow(std::move(window), SettingsMenu::SettingsWindowKind::SHADER);
    };
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackShader::BLOOM = [
            ](SettingsMenu &settingsMenu, RenderScene &scene) {
        auto &[threshold, radius, softness, intensity, linearAdd] = scene.getAttribute().shader.bloom;
        auto window = std::make_unique<SettingsWindow>(L"Bloom");
        window->registerTextInput<float>(L"Threshold", &threshold, Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::FLOAT_ZERO_TO_ONE, [&scene] {
                                             scene.getRequests().requestShader();
                                         }, L"Set Threshold", L"Brightness level above which pixels start to glow. 0 = all pixels, 1 = only the brightest.\nUp/Down arrows nudge by 0.01 (Shift = 0.1).", 0.01);
        window->registerTextInput<float>(L"Radius", &radius, Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::floatInRange(0.0f, 1.0f), [&scene] {
                                             scene.getRequests().requestShader();
                                         }, L"Set Radius", L"Spread radius of the bloom glow effect.\nUp/Down arrows nudge by 0.01 (Shift = 0.1).", 0.01);
        window->registerTextInput<float>(L"Softness", &softness, Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::FLOAT_ZERO_TO_ONE, [&scene] {
                                             scene.getRequests().requestShader();
                                         }, L"Set Softness", L"Falloff of the bloom kernel. Higher = softer, more diffuse glow.\n0.00 is the hard glow, 1.00 the fully spread one. Range: 0.00 to 1.00.\nUp/Down arrows nudge by 0.01 (Shift = 0.1).", 0.01);
        window->registerTextInput<float>(L"Intensity", &intensity, Unparser::floatFixed(2), Parser::FLOAT,
                                         ValidCondition::ALL_FLOAT, [&scene] {
                                             scene.getRequests().requestShader();
                                         }, L"Set Intensity", L"Strength of the bloom overlay blended onto the image. 0 = off.\nUp/Down arrows nudge by 0.01 (Shift = 0.1).", 0.01);
        window->registerCheckboxInput(L"Linear Light", &linearAdd, [&scene] {
            scene.getRequests().requestShader();
        }, L"Linear Light",
        L"Adds the glow in proportion to light rather than on the encoded values. Summed on the encoded values the glow is overstated by roughly the encoding curve, which carries a bright core to white early and then leaves it there as a flat plate with a hard edge and no tint left in it; summed in linear light it keeps climbing and keeps its color. It is the sum Slope > Light Blend offers as Linear, taken over the glow instead of over the highlight. Off is exactly what every earlier version drew. The same Intensity reads darker with it on, so Intensity is raised to meet the look it had.");
        window->setWindowCloseFunction([&settingsMenu] {
            settingsMenu.setCurrentActiveSettingsWindow(nullptr);
        });
        settingsMenu.setCurrentActiveSettingsWindow(std::move(window), SettingsMenu::SettingsWindowKind::SHADER);
    };
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackShader::LOAD_KFR_PALETTE = [
            ](SettingsMenu &settingsMenu, RenderScene &scene) {
        // The Palette panel edits the very colors this replaces; it cannot stay open across the load.
        settingsMenu.closePaletteSettingsWindows();
        const auto colors = KFRColorLoader::loadPaletteSettings();
        if (colors.empty()) {
            MessageBoxW(nullptr, L"No colors found", L"Error", MB_OK | MB_ICONERROR);
            return;
        }
        scene.getAttribute().shader.palette.colors = colors;
        // Loaded colors aren't reproducible from a recipe, so force raw storage on save.
        scene.getAttribute().shader.palette.recipePresetId = -1;
        scene.getRequests().requestShader();
    };
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackShader::SAVE_PRESET = [
            ](SettingsMenu &, RenderScene &scene) {
        const auto path = IOUtilities::ioFileDialog(L"Save Shader Preset", Constants::Extension::DESC_SHADER_PRESET,
                                                    IOUtilities::SAVE_FILE, Constants::Extension::SHADER_PRESET);
        if (path == nullptr) {
            return;
        }
        if (!ShaderPresetIO::save(*path, scene.getAttribute().shader)) {
            MessageBoxW(nullptr, L"Failed to save shader preset", L"Error", MB_OK | MB_ICONERROR);
        }
    };
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackShader::LOAD_PRESET = [
            ](SettingsMenu &settingsMenu, RenderScene &scene) {
        // Every shader panel is bound to the values this replaces, so none of them can stay open.
        settingsMenu.closeShaderSettingsWindows();
        const auto path = IOUtilities::ioFileDialog(L"Load Shader Preset", Constants::Extension::DESC_SHADER_PRESET,
                                                    IOUtilities::OPEN_FILE, Constants::Extension::SHADER_PRESET);
        if (path == nullptr) {
            return;
        }
        if (ShaderPresetIO::load(*path, scene.getAttribute().shader)) {
            scene.getRequests().requestShader();
            CallbackFile::warnMissingTextureImages(scene.getAttribute().shader);
        } else {
            MessageBoxW(nullptr, L"Failed to load shader preset", L"Error", MB_OK | MB_ICONERROR);
        }
    };
}
