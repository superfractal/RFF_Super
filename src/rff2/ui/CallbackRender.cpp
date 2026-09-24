//
// Created by Merutilm on 2025-05-14.
// Modified by AI; earlier exact modification date unavailable.
// Modified by Opus 5 on 2026-08-15, 2026-08-31
// Modified by GPT-5 on 2026-08-21
// Modified by GPT-6 on 2026-09-14, 2026-09-18, 2026-09-22, 2026-09-23, 2026-09-24
//

#include "NativeDialogs.hpp"
#include "CallbackRender.hpp"

#include "SettingsMenu.hpp"
#include "Callback.hpp"
#include "../attr/NumericSettingLimits.hpp"
#include <algorithm>
#include <thread>

namespace merutilm::rff2 {
    const std::function<bool *(RenderScene &, bool)> CallbackRender::SMOOTH_ZOOM =
        [](RenderScene &scene, bool) { return scene.smoothZoomOption(); };
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackRender::SET_CLARITY = [](SettingsMenu &
                                                                                                  settingsMenu,
                                                                                              RenderScene
                                                                                                  &scene) {
        auto window = std::make_unique<SettingsWindow>(L"Render Properties");
        auto &[clarityMultiplier, ssaa, fps, linearInterpolation, threads, boundaryTraceFill, preview2Color,
               coarsePreview, dither] = scene.getAttribute().render;
        // Last clarity/SSAA accepted within the memory budget; restored if the user declines an over-budget change.
        auto prevClarity = std::make_shared<float>(clarityMultiplier);
        auto prevSsaa = std::make_shared<uint32_t>(ssaa);
        // Warns on (and offers to revert) a clarity/SSAA change that would likely exhaust VRAM/RAM, then applies it.
        const auto applyRenderChange = [](RenderScene &scene, float *prevClarity, uint32_t *prevSsaa) {
            auto &render = scene.getAttribute().render;
            if (const std::wstring warn = scene.checkRenderMemoryBudget(scene.getAttribute());
                !warn.empty()) {
                const std::wstring text = L"This clarity / supersampling setting may run out of memory:\n\n" +
                                          warn + L"\nApply anyway?";
                if (NativeDialogs::message(nullptr, text.c_str(), L"Memory warning",
                                           MB_YESNO | MB_ICONWARNING) == IDNO) {
                    render.clarityMultiplier = *prevClarity;
                    render.ssaa = *prevSsaa;
                    return;
                }
            }
            *prevClarity = render.clarityMultiplier;
            *prevSsaa = render.ssaa;
            scene.getRequests().requestResize();
            scene.getRequests().requestRecompute();
        };
        window->registerSectionHeader(L"Basic Quality", false);
        window->registerTextInput<float>(
            L"Clarity", &clarityMultiplier, Unparser::floatFixed(2), Parser::FLOAT,
            [&scene](const float &v) {
                const auto ssaa = std::max<uint32_t>(1, scene.getAttribute().render.ssaa);
                const float maxScale = scene.getMaxInternalScale();
                return v > 0.01f && (maxScale <= 0.0f || v * static_cast<float>(ssaa) <= maxScale);
            },
            [&scene, prevClarity, prevSsaa, applyRenderChange] {
                applyRenderChange(scene, prevClarity.get(), prevSsaa.get());
            },
            L"Clarity Multiplier",
            L"Internal render resolution scale. Higher = sharper but uses more memory.");
        window->registerTextInput<uint32_t>(
            L"Supersampling (SSAA)", &ssaa, Unparser::U_LONG, Parser::U_LONG,
            [&scene](const uint32_t &v) {
                const float clarity = scene.getAttribute().render.clarityMultiplier;
                const float maxScale = scene.getMaxInternalScale();
                return v >= 1 && v <= 8 && (maxScale <= 0.0f || static_cast<float>(v) * clarity <= maxScale);
            },
            [&scene, prevClarity, prevSsaa, applyRenderChange] {
                applyRenderChange(scene, prevClarity.get(), prevSsaa.get());
            },
            L"Export Supersampling (SSAA)",
            L"Renders internally at clarity x SSAA and downsamples by SSAA at "
            L"export (image & video) for anti-aliasing. Output size is unchanged. 1 = off.");
        window->registerSectionHeader(L"Performance");
        window->registerTextInput<float>(
            L"Rendering FPS", &fps, Unparser::floatFixed(2), Parser::FLOAT, NumericSettingLimits::acceptsFps,
            [&scene] { scene.wndRequestFPS(); }, L"Rendering FPS",
            L"Limits live rendering to 1 to 1000 frames per second. Lower this to reduce rendering load. Video export uses a separate frame rate.");
        window->registerTextInput<uint32_t>(L"Threads", &threads, Unparser::U_LONG, Parser::U_LONG,
                                            [](const uint32_t &v) { return v >= 1 && v <= std::max(1u, std::thread::hardware_concurrency()); }, Callback::NOTHING, L"Threads",
                                            L"Sets the number of threads when calculating.");
        window->registerHelpButton(
            L"Render Properties Guide",
            {
                {L"Start with Clarity before changing SSAA.",
                 L"Clarity affects the live render. SSAA mainly improves exported image and video edges."},
                {L"Performance settings can affect memory use.",
                 L"If a warning appears, lower Clarity or SSAA first."},
            });
        window->setWindowCloseFunction(
            [&settingsMenu] { settingsMenu.setCurrentActiveSettingsWindow(nullptr); });
        settingsMenu.setCurrentActiveSettingsWindow(std::move(window));
    };
    const std::function<bool *(RenderScene &, bool)> CallbackRender::LINEAR_INTERPOLATION =
        [](RenderScene &scene, const bool executeMode) {
            if (executeMode) {
                scene.getRequests().requestShader();
            }
            return &scene.getAttribute().render.linearInterpolation;
        };
    const std::function<bool *(RenderScene &, bool)> CallbackRender::DITHER = [](RenderScene &scene,
                                                                                 const bool executeMode) {
        if (executeMode) {
            scene.getRequests().requestShader();
        }
        return &scene.getAttribute().render.dither;
    };
    const std::function<bool *(RenderScene &, bool)> CallbackRender::BOUNDARY_TRACE_FILL =
        [](RenderScene &scene, const bool executeMode) {
            if (executeMode) {
                scene.getRequests().requestRecompute();
            }
            return &scene.getAttribute().render.boundaryTraceFill;
        };
    const std::function<bool *(RenderScene &, bool)> CallbackRender::PREVIEW_2COLOR =
        [](RenderScene &scene, const bool executeMode) {
            if (executeMode) {
                scene.getRequests().requestRecompute();
            }
            return &scene.getAttribute().render.preview2Color;
        };
    const std::function<bool *(RenderScene &, bool)> CallbackRender::COARSE_PREVIEW =
        [](RenderScene &scene, const bool executeMode) {
            if (executeMode) {
                scene.getRequests().requestRecompute();
            }
            return &scene.getAttribute().render.coarsePreview;
        };
}
