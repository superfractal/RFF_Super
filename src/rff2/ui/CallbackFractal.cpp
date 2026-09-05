//
// Created by Merutilm on 2025-05-14.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21.
// Modified by Opus 5 on 2026-08-13, 2026-08-16, 2026-08-31, 2026-09-04
//

#include "CallbackFractal.hpp"

#include "Callback.hpp"
#include "SettingsMenu.hpp"
#include "../formula/Perturbator.h"

namespace merutilm::rff2 {
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackFractal::REFERENCE = [
            ](SettingsMenu &settingsMenu, RenderScene &scene) {
        auto &calc = scene.getAttribute().fractal;
        auto window = std::make_unique<SettingsWindow>(L"Reference");

        auto centerPtr = std::make_shared<std::array<std::string, 2> >();
        *centerPtr = {calc.center.real.to_string(), calc.center.imag.to_string()};

        auto zoomPtr = std::make_shared<float>(calc.logZoom);
        auto rotationPtr = std::make_shared<float>(calc.rotation);
        // Which of the three the user actually typed in. The panel opens on a copy while the canvas
        // keeps moving under it, so writing all of them back would put the zoom and the rotation the
        // view had when the panel opened over the ones it has now. Only what was typed goes back.
        auto centerEdited = std::make_shared<bool>(false);
        auto zoomEdited = std::make_shared<bool>(false);
        auto rotationEdited = std::make_shared<bool>(false);
        const auto angleUnparser = [](const float &value) {
            return Unparser::floatFixed(0)(value) + L"\u00B0";
        };
        const auto angleParser = [](std::wstring &value) {
            std::erase(value, L'\u00B0');
            return Parser::FLOAT(value);
        };

        window->registerSectionHeader(L"View", false);
        window->registerTextInput<std::string>(L"Real", &(*centerPtr)[0],
                                               Unparser::STRING,
                                               Parser::STRING, [](const std::string &v) {
                                                   mpf_t t;
                                                   const bool valid = mpf_init_set_str(t, v.data(), 10) == 0;
                                                   mpf_clear(t);
                                                   return valid;
                                               }, [centerPtr, centerEdited] {
                                                   *centerEdited = true;
                                               }, L"Real", L"Real coordinate of the view center.");
        window->registerTextInput<std::string>(L"Imag", &(*centerPtr)[1],
                                               Unparser::STRING,
                                               Parser::STRING, [](const std::string &v) {
                                                   mpf_t t;
                                                   const bool valid = mpf_init_set_str(t, v.data(), 10) == 0;
                                                   mpf_clear(t);
                                                   return valid;
                                               }, [centerPtr, centerEdited] {
                                                   *centerEdited = true;
                                               }, L"Imag", L"Imaginary coordinate of the view center.");
        window->registerTextInput<float>(L"Log Zoom (e)", zoomPtr.get(), Unparser::floatTrim(3),
                                         Parser::FLOAT, ValidCondition::POSITIVE_FLOAT,
                                         [zoomPtr, zoomEdited] {
                                             *zoomEdited = true;
                                         }, L"Log zoom (e)", L"Zoom magnification on a natural-log scale (e). Higher = deeper zoom.");
        window->registerTextInput<float>(L"Rotation", rotationPtr.get(), angleUnparser,
                                         angleParser, [](const float &) { return true; },
                                         [rotationPtr, rotationEdited] {
                                             *rotationEdited = true;
                                         }, L"Rotation", L"Sets the rotation angle in degrees.");

        window->registerSectionHeader(L"Reference Cache");
        window->registerRadioButtonInput<FrtReuseReferenceMethod>(L"Reuse Reference", &calc.reuseReferenceMethod,
                                                               Callback::NOTHING, L"Reuse Reference method",
                                                               L"Sets the reuse reference method.");
        window->registerTextInput<uint32_t>(L"Reference Compression Criteria",
                                            &calc.referenceCompAttribute.compressCriteria,
                                            Unparser::U_LONG, Parser::U_LONG,
                                            ValidCondition::ALL_U_LONG, Callback::NOTHING,
                                            L"Reference Compression Criteria",
                                            L"When compressing references, sets the minimum amount of references to compress at one time.\n"
                                            L"Reference compression slows down the calculation but frees up memory space.\n"
                                            L"Set to 0 to disable.");
        window->registerTextInput<uint8_t>(L"Reference Compression Threshold",
                                           &calc.referenceCompAttribute.compressionThresholdPower,
                                           Unparser::U_CHAR, Parser::U_CHAR,
                                           ValidCondition::ALL_U_CHAR, Callback::NOTHING,
                                           L"Reference Compression Threshold Power",
                                           L"When compressing references, sets the negative power of ten of the minimum error treated as equal.\n"
                                           L"Reference compression slows down the calculation but frees up memory space.\n"
                                           L"Set to 0 to disable.");
        window->registerCheckboxInput(L"Disable Compressor Normalization",
                                  &calc.referenceCompAttribute.noCompressorNormalization, Callback::NOTHING,
                                  L"Disable Compressor Normalization",
                                  L"Do not use normalization when compressing references. "
                                  L"This accelerates table creation, but may cause it to fail at specific locations.");
        window->registerHelpButton(
            L"Reference Guide",
            {
                {L"Use these values when you want an exact location.",
                 L"Normal mouse navigation is easier for exploration; this panel is best for precise saved coordinates."},
                {L"Advanced reference settings can usually stay unchanged.",
                 L"They mainly affect memory use and reuse behavior during deep zoom rendering."},
            });
        window->setWindowCloseFunction(
            [centerPtr, zoomPtr, rotationPtr, centerEdited, zoomEdited, rotationEdited, &settingsMenu, &scene, &calc] {
                if (*centerEdited || *zoomEdited || *rotationEdited) {
                    if (*zoomEdited) {
                        calc.logZoom = *zoomPtr;
                    }
                    if (*rotationEdited) {
                        calc.rotation = *rotationPtr;
                    }
                    if (*centerEdited) {
                        // Read at the precision the zoom now asks for: the one just typed when it
                        // was typed, and the one the canvas is at when it was not.
                        calc.center = fp_complex((*centerPtr)[0], (*centerPtr)[1],
                                                 Perturbator::logZoomToExp10(calc.logZoom));
                    }
                    scene.getRequests().requestRecompute();
                }
                settingsMenu.setCurrentActiveSettingsWindow(nullptr);
            });
        settingsMenu.setCurrentActiveSettingsWindow(std::move(window));
    };
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackFractal::ITERATIONS = [
            ](SettingsMenu &settingsMenu, RenderScene  &scene) {
        auto &calc = scene.getAttribute().fractal;
        auto window = std::make_unique<SettingsWindow>(L"Iterations");

        window->registerSectionHeader(L"Basic Limits", false);
        window->registerTextInput<uint64_t>(L"Max Iteration", &calc.maxIteration,
                                                Unparser::U_LONG_LONG,
                                                Parser::U_LONG_LONG,
                                                ValidCondition::ALL_U_LONG_LONG,
                                                Callback::NOTHING, L"Set Max Iteration",
                                                L"Set maximum iteration. It is disabled when Automatic Iterations is enabled.");
        window->registerTextInput<uint16_t>(L"Auto Iteration Multiplier", &calc.autoIterationMultiplier,
                                                        Unparser::U_SHORT,
                                                        Parser::U_SHORT,
                                                        ValidCondition::ALL_U_SHORT,
                                                        Callback::NOTHING, L"Set Auto Iteration Multiplier",
                                                        L"Set auto iteration multiplier. It is disabled when Automatic Iterations is disabled.");

        window->registerTextInput<float>(L"Bailout", &calc.bailout, Unparser::FLOAT_SCIENTIFIC,
                                         Parser::FLOAT, [](const float &v) { return v >= 2 && v <= 1e38f; },
                                         Callback::NOTHING, L"Set Bailout",
                                         L"Sets the bailout radius. Valid range: 2 to 1e38.\n"
                                         L"The smoothed iteration divides by log(bailout), so raising it does not "
                                         L"change a single band width: every value shifts by the constant "
                                         L"log2(log(new) / log(old)), which reads as a palette offset. Going from "
                                         L"1e6 to 1e30 shifts by 2.32 iterations and costs about that many more "
                                         L"steps per escaping pixel.\n"
                                         L"A larger radius does make the smoothing itself more exact, since the "
                                         L"error of the potential falls off as 1/bailout."
        );
        window->registerSectionHeader(L"Color Detail");
        window->registerRadioButtonInput<FrtDecimalizeIterationMethod>(L"Decimalize Iteration",
                                                                    &calc.decimalizeIterationMethod,
                                                                    Callback::NOTHING, L"Decimalize Iteration Method",
                                                                    L"Sets the decimalization method of iterations.");
        window->registerHelpButton(
            L"Iterations Guide",
            {
                {L"Automatic Iterations is recommended for most views.",
                 L"When it is enabled from the Fractal menu, Max Iteration is managed automatically."},
                {L"Raise quality gradually.",
                 L"Very high iteration counts can make exploration slower without improving every image."},
            });
        window->setWindowCloseFunction([&settingsMenu] {
            settingsMenu.setCurrentActiveSettingsWindow(nullptr);
        });
        settingsMenu.setCurrentActiveSettingsWindow(std::move(window));
    };
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackFractal::MPA = [
            ](SettingsMenu &settingsMenu, RenderScene  &scene) {
        auto &[minSkipReference, maxMultiplierBetweenLevel, epsilonPower, mpaSelectionMethod, mpaCompressionMethod] =
                scene.getAttribute().fractal.mpaAttribute;
        auto window = std::make_unique<SettingsWindow>(L"MP-Approximation");
        window->registerSectionHeader(L"Skip Table", false);
        window->registerTextInput<uint16_t>(L"Min Skip Reference", &minSkipReference, Unparser::U_SHORT,
                                            Parser::U_SHORT, [](const unsigned short &v) { return v >= 4; },
                                            Callback::NOTHING, L"Min Skip Reference",
                                            L"Set minimum skipping reference iteration when creating a table.");
        window->registerTextInput<uint8_t>(L"Max Multiplier Between Levels", &maxMultiplierBetweenLevel,
                                           Unparser::U_CHAR, Parser::U_CHAR,
                                           ValidCondition::POSITIVE_U_CHAR, Callback::NOTHING,
                                           L"Max Multiplier Between Levels",
                                           L"The maximum ratio between two adjacent periods for the new period inserted between them.\n"
                                           L"The worst-case ratio between two periods may be the square of this value."
        );
        window->registerTextInput<float>(L"Precision Level", &epsilonPower, Unparser::floatFixed(1),
                                         Parser::FLOAT, [](const float &v) { return v >= -15.0f && v <= -3.0f; },
                                         Callback::NOTHING,
                                         L"Precision Level",
                                         L"Useful for glitch reduction. Valid range: -15 to -3.\n"
                                         L"Smaller values render glitch-free but slowly; larger values are faster but may show visible glitches."
        );
        window->registerSectionHeader(L"Strategy");
        window->registerRadioButtonInput<FrtMPASelectionMethod>(L"Selection Method", &mpaSelectionMethod,
                                                             Callback::NOTHING, L"Selection Method",
                                                             L"The first target PA is always the front element."
        );

        window->registerRadioButtonInput<FrtMPACompressionMethod>(L"Compression Method", &mpaCompressionMethod,
                                                               Callback::NOTHING, L"Compression Method",
                                                               L"\"Little Compression\" may slow down table creation, but allocates memory efficiently.\n"
                                                               L"\"Strongest\" works based on the Reference Compressor, so if it is disabled, it will behave the same as \"Little Compression\".\n"
                                                               L"It uses acceleration when possible, and can accelerate table creation by 10x~100x."
        );
        window->registerHelpButton(
            L"MP-Approximation Guide",
            {
                {L"Most users should leave these values as-is.",
                 L"MPA helps the renderer skip through periodic structure efficiently during deep zooms."},
                {L"Lower precision levels reduce glitches but cost speed.",
                 L"If a view looks unstable, adjust Precision Level first."},
            });
        window->setWindowCloseFunction([&settingsMenu] {
            settingsMenu.setCurrentActiveSettingsWindow(nullptr);
        });
        settingsMenu.setCurrentActiveSettingsWindow(std::move(window));
    };

    const std::function<bool*(RenderScene &, bool)> CallbackFractal::AUTOMATIC_ITERATIONS = [
            ](RenderScene  &scene, const bool executeMode) -> bool* {
        auto &calc = scene.getAttribute().fractal;
        // Custom formulas have no period structure, so auto-iteration cannot
        // estimate a max iteration. Reject the toggle (executeMode is only set
        // on an actual click, not on a menu-state refresh) and keep it disabled.
        if (executeMode && calc.formulaType == FractalFormulaType::CUSTOM) {
            MessageBox(nullptr,
                       "Automatic Iterations is not available for custom formulas.\n"
                       "The manually-set Max Iteration is used instead.",
                       "Caution", MB_OK | MB_ICONWARNING);
            // Route the caller's toggle to a throwaway flag so autoMaxIteration
            // stays off. Pre-set it so that after the caller flips it the menu
            // item still renders unchecked.
            static bool sink;
            sink = true;
            return &sink;
        }
        return &calc.autoMaxIteration;
    };

    const std::function<bool*(RenderScene &, bool)> CallbackFractal::ABSOLUTE_ITERATION_MODE = [
            ](RenderScene  &scene, bool) {
        return &scene.getAttribute().fractal.absoluteIterationMode;
    };

    const std::function<void(SettingsMenu &, RenderScene &)> CallbackFractal::FORMULA = [
            ](SettingsMenu &settingsMenu, RenderScene &scene) {
        auto &calc = scene.getAttribute().fractal;
        auto window = std::make_unique<SettingsWindow>(L"Formula");

        window->registerSectionHeader(L"Formula Mode", false);
        window->registerRadioButtonInput<FractalFormulaType>(L"Formula Type", &calc.formulaType,
                                                              [&scene] {
                                                                  // Switching the formula forces a reset so the
                                                                  // location/zoom suit the new formula. The reset
                                                                  // preserves the formula type and color settings.
                                                                  scene.getRequests().requestDefaultSettings();
                                                                  scene.getRequests().requestShader();
                                                                  scene.getRequests().requestResize();
                                                                  scene.getRequests().requestRecompute();
                                                              },
                                                              L"Formula Type",
                                                              L"Mandelbrot: Standard z^2+c\nCustom: Use custom formula string");
        
        window->registerSectionHeader(L"Custom Expression");
        auto formulaPtr = std::make_shared<std::string>(calc.customFormula);
        const HWND formulaField = window->registerTextInput<std::string>(L"Custom Formula", formulaPtr.get(),
                                               Unparser::STRING,
                                               Parser::STRING, [](const std::string &v) {
                                                   return !v.empty();
                                               }, [formulaPtr, &calc, &scene] {
                                                   calc.customFormula = *formulaPtr;
                                                   // Changing the formula forces a reset so the
                                                   // location/zoom suit the new formula. The reset
                                                   // preserves the formula string and color settings.
                                                   scene.getRequests().requestDefaultSettings();
                                                   scene.getRequests().requestShader();
                                                   scene.getRequests().requestResize();
                                                   scene.getRequests().requestRecompute();
                                               }, L"Custom Formula",
                                               L"Enter custom formula (see syntax below)");

        // Clickable chips: tap a function to insert it at the caret, then press Enter to apply.
        window->registerStaticText(L"Insert function (click to add at cursor):");
        window->registerInsertChips(formulaField, {
            L"abs", L"sqrt", L"exp", L"log",
            L"sin", L"cos", L"tan", L"asin", L"acos", L"atan",
            L"sinh", L"cosh", L"tanh",
            L"real", L"imag", L"conj", L"arg", L"norm",
            L"rabs", L"iabs", L"riabs",
            L"floor", L"ceil", L"round", L"sign"
        }, false);

        // Full-formula examples: clicking one replaces the whole field.
        window->registerStaticText(L"Examples (click to replace):");
        window->registerInsertChips(formulaField, {
            L"z^2+c", L"z^3+c", L"riabs(z)^2+conj(c)", L"conj(z)^2+c"
        }, true);

        // Display supported syntax directly in the window
        window->registerStaticText(
            L"=== Supported Syntax ===\n"
            L"Variables: z, c, i, pi, e\n"
            L"Imaginary: 1i, 2i, 3.5i (number+i)\n"
            L"Operators: + - * / ^ (power)\n"
            L"=== Examples ===\n"
            L"z^3+c (3rd Power Mandelbrot)\n"
            L"riabs(z)^2+conj(c) (Burning Ship)");
        window->registerHelpButton(
            L"Formula Guide",
            {
                {L"Mandelbrot is the easiest starting point.",
                 L"Switch to Custom only when you want to experiment with formula strings."},
                {L"Click chips to insert examples.",
                 L"After editing the formula field, press Enter to apply the new expression."},
                {L"Smooth coloring reads the formula's degree.",
                 L"A formula that grows like a power of z — z^2+c, riabs(z)^2+c, z*conj(z)+c — is colored exactly. One that grows faster, such as exp(z)+c or sin(z)+c, has no single degree to read, so its bands are only placed approximately and can sit up to several iterations out. Decimalize Iteration set to None colors those by whole iterations instead."},
            });
        
        window->setWindowCloseFunction([&settingsMenu, formulaPtr, &calc, &scene] {
            calc.customFormula = *formulaPtr;
            settingsMenu.setCurrentActiveSettingsWindow(nullptr);
        });
        settingsMenu.setCurrentActiveSettingsWindow(std::move(window));
    };

    const std::function<void(SettingsMenu &, RenderScene &)> CallbackFractal::PROJECTION = [
            ](SettingsMenu &settingsMenu, RenderScene &scene) {
        auto &calc = scene.getAttribute().fractal;
        auto window = std::make_unique<SettingsWindow>(L"Projection");
        const auto angleUnparser = [](const float &value) {
            return Unparser::floatFixed(1)(value) + L"\u00B0";
        };
        const auto angleParser = [](std::wstring &value) {
            std::erase(value, L'\u00B0');
            return Parser::FLOAT(value);
        };

        window->registerSectionHeader(L"Projection", false);
        window->registerRadioButtonInput<FrtProjectionMethod>(L"Projection", &calc.projectionMethod,
                                                              [&scene] {
                                                                  scene.getRequests().requestRecompute();
                                                              }, L"Projection",
                                                              L"Planar: the ordinary flat view.\n"
                                                              L"360\u00B0 Camera: stand inside the scene and look around. Drag the canvas to turn, and the wheel zooms as it always does, so a location is explored in 360\u00B0 rather than only rendered in it.\n"
                                                              L"360\u00B0 Equirectangular: the whole view laid out flat, its width one turn around and its height pole to pole, which is the layout a VR viewer or a 360 player reads.");

        window->registerSectionHeader(L"Layout");
        window->registerRadioButtonInput<FrtPanoramaLayout>(L"Layout", &calc.panoramaLayout,
                                                            [&scene] {
                                                                scene.getRequests().requestRecompute();
                                                            }, L"Layout",
                                                            L"Ground and Sky: the fractal lies under the viewer, in true perspective, and everything at or above the horizon is empty sky. The whole plane is below you, so nothing of it appears twice.\n"
                                                            L"Full Sphere: the plane is wrapped onto the whole sphere, which keeps the fractal's shape exactly everywhere. The plane beyond the horizon radius then fills the upper half, turned inside out around that radius, so the set is seen a second time overhead as a deformed copy of itself. Choose Ground and Sky if that copy is not wanted.");

        window->registerSectionHeader(L"Camera");
        window->registerTextInput<float>(L"Pitch", &calc.panoramaPitch, angleUnparser,
                                         angleParser, [](const float &v) { return v >= -90.0f && v <= 90.0f; },
                                         [&scene] { scene.getRequests().requestRecompute(); },
                                         L"Pitch",
                                         L"Which way the 360\u00B0 Camera looks up or down, in degrees. Valid range: -90 to 90.\n"
                                         L"-90 faces straight down at the view center, 0 faces the horizon, and 90 faces straight up.\n"
                                         L"Dragging the canvas moves it. It is not used by 360\u00B0 Equirectangular, whose nadir stays at the bottom where a 360 player expects it.",
                                         1.0);
        window->registerTextInput<float>(L"Field of View", &calc.panoramaFov, angleUnparser,
                                         angleParser, [](const float &v) { return v >= 1.0f && v <= 179.0f; },
                                         [&scene] { scene.getRequests().requestRecompute(); },
                                         L"Field of View",
                                         L"How wide the 360\u00B0 Camera sees across the canvas, in degrees. Valid range: 1 to 179.\n"
                                         L"Narrowing it magnifies what is in front without moving the view center, so it reads as a zoom that costs nothing to compute.",
                                         5.0);
        window->registerStaticText(L"Yaw is the Rotation of the Reference panel, and dragging the canvas moves it.");

        window->registerSectionHeader(L"Distance");
        window->registerTextInput<float>(L"Panorama Range", &calc.panoramaRange, Unparser::floatFixed(1),
                                         Parser::FLOAT, [](const float &v) { return v >= 0.0f && v <= 6.0f; },
                                         [&scene] { scene.getRequests().requestRecompute(); },
                                         L"Panorama Range",
                                         L"How far out the view reaches, as a power of ten of the horizon radius. Valid range: 0 to 6.\n"
                                         L"Every 360 layout runs the plane out to infinity somewhere - at the horizon under Ground and Sky, at the zenith under Full Sphere - so the picture has to fold into a band at that line. This sets where the fold lands.\n"
                                         L"The fold is never allowed to spread wider than a pixel, so raising this past what the canvas can show changes nothing. Lowering it pulls the fold in, which is a haze at the horizon and a faster render, since the reference only has to stay valid out to where the fold sits.",
                                         0.5);
        window->registerHelpButton(
            L"Projection Guide",
            {
                {L"Explore in 360\u00B0 Camera, render in 360\u00B0 Equirectangular.",
                 L"The camera is the one to fly around and pick a view in; switching to Equirectangular then lays that same view out for export."},
                {L"A second, deformed Mandelbrot overhead means Full Sphere.",
                 L"Wrapping the whole plane onto the whole sphere puts everything beyond the horizon radius over your head, turned inside out. It is the exact, shape-preserving picture, but Ground and Sky is the one that looks like standing on the fractal."},
                {L"Looking down is the cheapest direction.",
                 L"The reference only has to hold detail out to the furthest point in sight, so a view facing the center computes faster than the flat view of the same zoom. Facing the horizon is the slowest, and Panorama Range is what bounds it."},
                {L"A 360 image wants a 2:1 size.",
                 L"Set the window or the image export to twice as wide as it is tall before rendering the equirectangular layout, otherwise the panorama is squeezed when a viewer wraps it onto a sphere."},
                {L"A player needs to be told the picture is spherical.",
                 L"RFF_Super writes the pixels; the equirectangular tag a 360 player or a video site reads is added to the file afterwards with a metadata tool."},
            });
        window->setWindowCloseFunction([&settingsMenu] {
            settingsMenu.setCurrentActiveSettingsWindow(nullptr);
        });
        settingsMenu.setCurrentActiveSettingsWindow(std::move(window));
    };
}
