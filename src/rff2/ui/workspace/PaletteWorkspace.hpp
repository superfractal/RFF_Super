//
// Modified by GPT-6 on 2026-09-14, 2026-09-19, 2026-09-21
//

#pragma once
#include "AppearanceForms.hpp"
#include "AttributeFormSection.hpp"
#include "PaletteSources.hpp"
#include "../../preset/shader/palette/ShdPalettePresets.h"

namespace merutilm::rff2::workspace {
    class PaletteWorkspace : public std::enable_shared_from_this<PaletteWorkspace> {
        struct Source {
            std::vector<glm::vec4> colors;
            std::vector<PaletteStop> stops;
            int32_t recipe = -1;
            uint32_t seed = 0;
            explicit Source(const ShdPaletteAttribute &p)
                : stops(p.stops), recipe(p.recipePresetId), seed(p.recipeSeed) {
                if (recipe < 0) {
                    colors = p.colors;
                }
            }
            bool operator==(const Source &other) const {
                return recipe == other.recipe && seed == other.seed && colors == other.colors &&
                       stops.size() == other.stops.size() &&
                       std::equal(stops.begin(), stops.end(), other.stops.begin(),
                                  [](const auto &a, const auto &b) {
                                      return a.position == b.position && a.color == b.color;
                                  });
            }
            void restore(ShdPaletteAttribute &p) const {
                auto restored = recipe < 0 ? colors : ShdPalettePresets::regenerateRecipeColors(recipe, seed);
                if (recipe >= 0 && restored.empty()) {
                    throw std::runtime_error("Unknown palette recipe");
                }
                p.colors = std::move(restored);
                p.stops = stops;
                p.recipePresetId = recipe;
                p.recipeSeed = seed;
            }
            size_t bytes() const {
                return colors.size() * sizeof(glm::vec4) + stops.size() * sizeof(PaletteStop);
            }
        };
        struct Edit {
            FormDraft before, after;
            uint64_t serial = 0;
            std::optional<std::pair<Source, Source>> source;
            size_t bytes() const {
                return source ? source->first.bytes() + source->second.bytes() : 0;
            }
        };
        AttributeGetter attribute;
        std::function<void()> changed;
        WorkspaceForm basic;
        std::deque<Edit> undoEntries, redoEntries;
        HistoryOrder order;
        size_t selected = 0;
        size_t libraryIndex = 0;
        uint32_t librarySeed = 1;
        std::wstring sourcePath, sourceMessage;
        bool importCorrection = false;
        std::optional<ShaderAttribute> imported;
        ShdPaletteAttribute &palette() const {
            return attribute().shader.palette;
        }
        FormDraft values() const {
            FormDraft result;
            for (const auto &field : basic.fields) {
                result[field.id] = field.read();
            }
            return result;
        }
        void clampSelection() {
            selected = palette().stops.empty() ? 0 : std::min(selected, palette().stops.size() - 1);
        }
        void record(FormDraft before, Source source) {
            Edit edit;
            const auto after = values();
            for (const auto &[id, value] : before) {
                if (after.at(id) != value) {
                    edit.before[id] = value;
                    edit.after[id] = after.at(id);
                }
            }
            Source target(palette());
            if (!(source == target)) {
                edit.source.emplace(std::move(source), std::move(target));
            }
            if (edit.before.empty() && !edit.source) {
                return;
            }
            edit.serial = order.commit();
            undoEntries.push_back(std::move(edit));
            redoEntries.clear();
            size_t bytes = 0;
            for (const auto &e : undoEntries) {
                bytes += e.bytes();
            }
            while (undoEntries.size() > 1 && (undoEntries.size() > 128 || bytes > 64 * 1024 * 1024)) {
                bytes -= undoEntries.front().bytes();
                undoEntries.pop_front();
            }
            basic.clearHistory();
            clampSelection();
            changed();
        }
        bool restore(bool redo) {
            auto &from = redo ? redoEntries : undoEntries;
            auto &to = redo ? undoEntries : redoEntries;
            if (from.empty() || (redo && !order.validRedo())) {
                return false;
            }
            const auto &edit = from.back();
            const auto current = values();
            for (const auto &[id, value] : redo ? edit.before : edit.after) {
                if (current.at(id) != value) {
                    undoEntries.clear();
                    redoEntries.clear();
                    return false;
                }
            }
            if (edit.source && !(Source(palette()) == (redo ? edit.source->first : edit.source->second))) {
                undoEntries.clear();
                redoEntries.clear();
                return false;
            }
            const auto backup = attribute().shader;
            try {
                if (edit.source) {
                    (redo ? edit.source->second : edit.source->first).restore(palette());
                }
                const auto error = basic.apply(redo ? edit.after : edit.before);
                if (!error.empty()) {
                    attribute().shader = backup;
                    return false;
                }
            } catch (...) {
                attribute().shader = backup;
                throw;
            }
            if (!redo) {
                order.prepareUndo(redoEntries);
            }
            basic.clearHistory();
            to.push_back(std::move(from.back()));
            from.pop_back();
            clampSelection();
            changed();
            return true;
        }
        std::wstring apply(const FormDraft &draft) {
            clampSelection();
            auto stops = palette().stops;
            size_t nextSelected = selected;
            FormDraft settings;
            auto nextLibrary = libraryIndex;
            auto nextSeed = librarySeed;
            auto nextPath = sourcePath;
            auto nextCorrection = importCorrection;
            if (const auto found = draft.find("palette.selectedStop"); found != draft.end()) {
                size_t value;
                if (!AttributeFormModel::parse(found->second, value) || value < 1 || value > stops.size()) {
                    return L"Select an existing color stop.";
                }
                nextSelected = value - 1;
                if (nextSelected != selected &&
                    (draft.contains("palette.stopPosition") || draft.contains("palette.stopColor"))) {
                    return L"Apply the stop selection before editing its position or color.";
                }
            }
            for (const auto &[id, value] : draft) {
                if (id == "palette.selectedStop") {
                    continue;
                }
                if (id == "source.preset") {
                    if (!AttributeFormModel::parse(value, nextLibrary) ||
                        nextLibrary >= paletteLibrary().size()) {
                        return L"Choose a palette preset.";
                    }
                    continue;
                }
                if (id == "source.seed") {
                    if (!AttributeFormModel::parse(value, nextSeed)) {
                        return L"Seed must be a whole number from 0 to 4294967295.";
                    }
                    continue;
                }
                if (id == "source.path") {
                    if (value.find(L'\0') != std::wstring::npos) {
                        return L"Enter a valid path.";
                    }
                    nextPath = value;
                    continue;
                }
                if (id == "source.correction") {
                    if (value != L"0" && value != L"1") {
                        return L"Choose whether to include color correction.";
                    }
                    nextCorrection = value == L"1";
                    continue;
                }
                if (id == "palette.stopPosition") {
                    if (stops.empty() || !AttributeFormModel::parse(value, stops[selected].position)) {
                        return L"Create color stops, then enter a valid position.";
                    }
                } else if (id == "palette.stopColor") {
                    if (stops.empty() || !AttributeFormModel::parseColor(value, stops[selected].color)) {
                        return L"Create color stops, then enter a valid color.";
                    }
                } else {
                    settings[id] = value;
                }
            }
            auto proposed = palette();
            proposed.stops = stops;
            if (!validPaletteStops(proposed)) {
                return L"Stop positions must stay distinct and ordered from 0 up to, but excluding, 1.";
            }
            auto before = values();
            Source source(palette());
            const auto backup = attribute().shader;
            try {
                const auto error = basic.apply(settings);
                if (!error.empty()) {
                    return error;
                }
                palette().stops = std::move(stops);
                if (draft.contains("palette.stopPosition") || draft.contains("palette.stopColor") ||
                    draft.contains("palette.stopEasing") || draft.contains("palette.colorInterpolation")) {
                    bakePaletteStops(palette());
                }
            } catch (...) {
                attribute().shader = backup;
                basic.clearHistory();
                throw;
            }
            if (nextPath != sourcePath) {
                imported.reset();
                sourceMessage.clear();
            }
            sourcePath = std::move(nextPath);
            libraryIndex = nextLibrary;
            librarySeed = nextSeed;
            importCorrection = nextCorrection;
            selected = nextSelected;
            record(std::move(before), std::move(source));
            return L"";
        }
        enum class StopAction { CreateEight, Add, RemoveSelected, EqualSpacing, ReverseColors };

        void editStops(StopAction action) {
            clampSelection();
            auto proposed = palette();
            auto &stops = proposed.stops;
            size_t nextSelected = selected;
            if (action == StopAction::CreateEight) {
                if (proposed.colors.empty()) {
                    return;
                }
                stops.clear();
                for (int i = 0; i < 8; ++i) {
                    stops.push_back({i / 8.f, proposed.colors[size_t(i) * proposed.colors.size() / 8]});
                }
                nextSelected = 0;
            } else {
                if (stops.size() < 2) {
                    return;
                }
                if (action == StopAction::Add) {
                    if (stops.size() >= 32) {
                        return;
                    }
                    const float end = selected + 1 < stops.size() ? stops[selected + 1].position
                                                                  : stops.front().position + 1;
                    const float position = std::fmod((stops[selected].position + end) * .5f, 1.f);
                    if (std::any_of(stops.begin(), stops.end(),
                                    [&](const auto &s) { return s.position == position; })) {
                        return;
                    }
                    const auto color = samplePaletteStops(proposed, position);
                    stops.push_back({position, color});
                    std::sort(stops.begin(), stops.end(),
                              [](const auto &a, const auto &b) { return a.position < b.position; });
                    nextSelected = std::find_if(stops.begin(), stops.end(),
                                                [&](const auto &s) { return s.position == position; }) -
                                   stops.begin();
                } else if (action == StopAction::RemoveSelected) {
                    if (stops.size() <= 2) {
                        return;
                    }
                    stops.erase(stops.begin() + selected);
                } else if (action == StopAction::EqualSpacing) {
                    for (size_t i = 0; i < stops.size(); ++i) {
                        stops[i].position = float(i) / stops.size();
                    }
                } else if (action == StopAction::ReverseColors) {
                    for (size_t i = 0; i < stops.size() / 2; ++i) {
                        std::swap(stops[i].color, stops[stops.size() - 1 - i].color);
                    }
                }
            }
            if (!validPaletteStops(proposed)) {
                return;
            }
            bakePaletteStops(proposed);
            auto before = values();
            Source source(palette());
            palette() = std::move(proposed);
            selected = nextSelected;
            record(std::move(before), std::move(source));
        }

      public:
        PaletteWorkspace(AttributeGetter attribute, std::function<void()> changed)
            : attribute(std::move(attribute)), changed(std::move(changed)) {
            auto model = std::make_shared<AttributeFormModel>(this->attribute, [] {});
            using S = ShdPaletteAttribute;
            AttributeFormSection s(model, [](auto &a) -> auto & { return a.shader.palette; }, "palette.");
            constexpr const wchar_t *channels[] = {L"Cycle Length (R)", L"Cycle Length (G)",
                                                   L"Cycle Length (B)"};
            for (int i = 0; i < 3; ++i) {
                model->numeric(
                    "palette.cycle" + std::to_string(i), 0, channels[i],
                    L"1 to 1e18 iterations per channel cycle.",
                    [i](auto &a) -> auto & { return a.shader.palette.iterationInterval[i]; }, 1.f, 1e18f);
            }
            s.choice("iterationColoring", &S::iterationColoring, L"Iteration Coloring");
            s.choice("colorSmoothing", &S::colorSmoothing, L"Color Smoothing");
            s.choice("colorInterpolation", &S::colorInterpolation, L"Color Interpolation");
            s.number("offsetRatio", &S::offsetRatio, L"Start Offset", 0.f, 1.f);
            s.number("cycleBias", &S::cycleBias, L"Cycle Bias", .1f, 4.f);
            s.choice("cycleCurve", &S::cycleCurve, L"Cycle Curve");
            s.choice("seamless", &S::seamless, L"Seamless (Mirror)");
            s.choice("enableGloss", &S::enableGloss, L"Palette Gloss");
            s.color("glossColor", &S::glossColor, L"Gloss Color");
            s.color("mandelbrotColor", &S::mandelbrotColor, L"Mandelbrot Color");
            s.group = 1;
            s.choice("bandLineEnabled", &S::bandLineEnabled, L"Band Lines");
            s.number("bandLineCount", &S::bandLineCount, L"Lines per Cycle", uint32_t(1), uint32_t(256));
            s.number("bandLineWidth", &S::bandLineWidth, L"Line Width", 0.f, 1.f);
            s.number("bandLineOpacity", &S::bandLineOpacity, L"Line Opacity", 0.f, 1.f);
            s.number("bandLineSoftness", &S::bandLineSoftness, L"Line Softness", 0.f, 1.f);
            s.color("bandLineColor", &S::bandLineColor, L"Line Color");
            s.choice("bandLineGroove", &S::bandLineGroove, L"Recessed Grooves");
            s.choice("grooveAuto", &S::grooveAuto, L"Auto Groove");
            s.number("grooveDepth", &S::grooveDepth, L"Groove Depth", 0.f, 5.f);
            s.number("grooveWidth", &S::grooveWidth, L"Groove Width", 0.f, .5f);
            s.group = 2;
            s.number("stopEasing", &S::stopEasing, L"Stop Easing", uint32_t(0), uint32_t(2),
                     L"0 Linear, 1 Smoothstep, 2 Smootherstep.");
            s.group = -1;
            s.number("animationSpeed", &S::animationSpeed, L"Animation Speed", -formMaximum, formMaximum);
            s.choice("animationMode", &S::animationMode, L"Animation Mode");
            s.number("animationFlowAmount", &S::animationFlowAmount, L"Flow Amount", 0.f, formMaximum);
            s.number("animationFlowScale", &S::animationFlowScale, L"Flow Scale", 0.f, 12.f);
            s.number("animationFlowSpeed", &S::animationFlowSpeed, L"Flow Speed", -2.f, 2.f);
            s.number("animationFlowSwirl", &S::animationFlowSwirl, L"Flow Swirl", -2.f, 2.f);
            s.number("staticColorTolerance", &S::staticColorTolerance, L"Frozen Color Tolerance", 0.f, 1.f);
            model->numeric(
                "palette.cycleAlpha", -1, L"Cycle Alpha", L"",
                [](auto &a) -> auto & { return a.shader.palette.iterationInterval.a; }, -formMaximum,
                formMaximum);
            model->text(
                "palette.frozen", -1, L"Frozen Colors", L"",
                [](const Attribute &a) {
                    std::wstring value;
                    for (double v : a.shader.palette.staticColorIterations) {
                        if (!value.empty()) {
                            value += L",";
                        }
                        value += AttributeFormModel::number(v);
                    }
                    return value;
                },
                [](Attribute &a, const std::wstring &text) {
                    std::vector<double> values;
                    size_t start = 0;
                    while (start < text.size()) {
                        const auto end = text.find(L',', start);
                        double value;
                        if (!AttributeFormModel::parse(
                                std::wstring_view(text).substr(
                                    start, end == std::wstring::npos ? text.size() - start : end - start),
                                value) ||
                            value < 0 || values.size() >= 16) {
                            return false;
                        }
                        values.push_back(value);
                        if (end == std::wstring::npos) {
                            break;
                        }
                        start = end + 1;
                        if (start == text.size()) {
                            return false;
                        }
                    }
                    a.shader.palette.staticColorIterations = std::move(values);
                    return true;
                });
            AttributeFormSection grading(
                model, [](auto &a) -> auto & { return a.shader.color; }, "importColor.", -1);
            using C = ShdColorAttribute;
            grading.number("gamma", &C::gamma, L"Gamma", -formMaximum, formMaximum);
            grading.number("exposure", &C::exposure, L"Exposure", -formMaximum, formMaximum);
            grading.number("hue", &C::hue, L"Hue", -formMaximum, formMaximum);
            grading.number("saturation", &C::saturation, L"Saturation", -formMaximum, formMaximum);
            grading.number("brightness", &C::brightness, L"Brightness", -formMaximum, formMaximum);
            grading.number("contrast", &C::contrast, L"Contrast", -1.f, 1.f);
            s.group = 1;
            s.number("bandSpineAmount", &S::bandSpineAmount, L"Spine Amount", 0.0f, 1.0f,
                     L"Uses Band Line color, width and opacity. Independent of Studio and materials.");
            s.number("bandSpineLength", &S::bandSpineLength, L"Spine Length", 0.0f, 3.0f,
                     L"Uses Band Line color, width and opacity. Independent of Studio and materials.");
            s.number("bandSpineDensity", &S::bandSpineDensity, L"Spine Density", 0.25f, 3.0f,
                     L"Uses Band Line color, width and opacity. Independent of Studio and materials.");
            s.number("bandBranchAmount", &S::bandBranchAmount, L"Branch Amount", 0.0f, 1.0f,
                     L"Uses Band Line color, width and opacity. Independent of Studio and materials.");
            s.number("bandOrnamentAmount", &S::bandOrnamentAmount, L"Ornament Amount", 0.0f, 1.0f,
                     L"Uses Band Line color, width and opacity. Independent of Studio and materials.");
            s.number("bandOrnamentSize", &S::bandOrnamentSize, L"Ornament Size", 0.25f, 3.0f,
                     L"Uses Band Line color, width and opacity. Independent of Studio and materials.");
            s.number("bandOrnamentDensity", &S::bandOrnamentDensity, L"Ornament Density", 0.25f, 2.0f,
                     L"Uses Band Line color, width and opacity. Independent of Studio and materials.");
            s.number("bandOrnamentInset", &S::bandOrnamentInset, L"Ornament Inset", 0.0f, 0.25f,
                     L"Uses Band Line color, width and opacity. Independent of Studio and materials.");
            AttributeFormSection glow(
                model, [](auto &a) -> auto & { return a.shader.slope; }, "bandGlow.", 1);
            glow.number("strength", &ShdSlopeAttribute::styleRimStrength, L"Line Glow", 0.f, 4.f);
            glow.number("width", &ShdSlopeAttribute::styleRimWidth, L"Glow Width", .1f, 32.f);
            glow.color("color", &ShdSlopeAttribute::styleRimColor, L"Glow Color");
            basic = model->form(L"Palette", {L"Color Cycle", L"Band Line", L"Color Stops", L"Arrange Stops",
                                             L"Preset Library", L"Import Colors"});
        }
        void usePreset(size_t index, uint32_t seed) {
            auto next = paletteFromLibrary(index, seed);
            auto before = values();
            Source source(palette());
            palette() = std::move(next);
            selected = 0;
            record(std::move(before), std::move(source));
            sourceMessage = L"Palette preset applied. Undo restores the previous palette.";
        }
        bool readImport(const std::filesystem::path &path) {
            imported.reset();
            ShaderAttribute candidate;
            if (!readPaletteSource(path, attribute().shader, candidate, sourceMessage)) {
                return false;
            }
            imported = std::move(candidate);
            sourceMessage = L"Loaded " + path.filename().wstring() + L". Ready to apply imported colors.";
            return true;
        }
        bool applyImport(bool includeCorrection) {
            if (!imported) {
                sourceMessage = L"Read a color file before applying it.";
                return false;
            }
            auto next = imported->palette;
            auto before = values();
            Source source(palette());
            palette() = std::move(next);
            if (includeCorrection) {
                attribute().shader.color = imported->color;
            }
            selected = 0;
            record(std::move(before), std::move(source));
            sourceMessage = L"Imported colors applied. Undo restores the previous settings.";
            return true;
        }
        WorkspaceForm form() {
            auto self = shared_from_this();
            auto form = basic;
            std::erase_if(form.fields, [](const auto &field) { return field.group < 0; });
            form.fields.push_back({"palette.selectedStop",
                                   2,
                                   L"Selected Stop",
                                   L"Apply the selection before editing its color or position.",
                                   [self] {
                                       self->clampSelection();
                                       return std::to_wstring(
                                           self->palette().stops.empty() ? 0 : self->selected + 1);
                                   },
                                   {}});
            form.fields.push_back({"palette.stopPosition",
                                   2,
                                   L"Stop Position",
                                   L"Ordered position from 0 up to, but excluding, 1.",
                                   [self] {
                                       self->clampSelection();
                                       return AttributeFormModel::number(
                                           self->palette().stops.empty()
                                               ? 0.f
                                               : self->palette().stops[self->selected].position);
                                   },
                                   {}});
            form.fields.push_back({"palette.stopColor",
                                   2,
                                   L"Stop Color",
                                   L"#RRGGBB or R, G, B, A in 0–1.",
                                   [self] {
                                       self->clampSelection();
                                       return AttributeFormModel::colorText(
                                           self->palette().stops.empty()
                                               ? glm::vec4(0, 0, 0, 1)
                                               : self->palette().stops[self->selected].color);
                                   },
                                   {},
                                   FormField::Editor::COLOR});
            std::vector<FormChoice> presets;
            for (size_t i = 0; i < paletteLibrary().size(); ++i) {
                presets.push_back({std::to_wstring(i), Unparser::STRING(paletteLibrary()[i]->getName())});
            }
            form.fields.push_back(
                {"source.preset", 4, L"Palette Preset", L"Applies the preset's complete palette settings.",
                 [self] { return std::to_wstring(self->libraryIndex); }, std::move(presets)});
            form.fields.push_back({"source.seed",
                                   4,
                                   L"Seed",
                                   L"Whole number from 0 to 4294967295 for repeatable generation.",
                                   [self] { return std::to_wstring(self->librarySeed); },
                                   {}});
            form.fields.push_back({"source.path",
                                   5,
                                   L"Color Settings File",
                                   L"RFC, RFSP, KFR or KFP. Read first, then apply.",
                                   [self] { return self->sourcePath; },
                                   {},
                                   FormField::Editor::FILE});
            form.fields.push_back({"source.correction",
                                   5,
                                   L"Include Color Correction",
                                   L"Also imports Gamma, Exposure, Hue, Saturation, Brightness and Contrast.",
                                   [self] { return self->importCorrection ? L"1" : L"0"; },
                                   {{L"0", L"Off"}, {L"1", L"On"}}});
            for (auto &field : form.fields) {
                if (field.id == "source.seed") {
                    field.validate = AttributeFormModel::rangeValidation<uint32_t>(
                        0, std::numeric_limits<uint32_t>::max());
                }
                if (field.id == "palette.selectedStop") {
                    field.validate = [self](const std::wstring &text) -> std::wstring {
                        size_t value{};
                        return AttributeFormModel::parse(text, value) && value > 0 &&
                                       value <= self->palette().stops.size()
                                   ? L""
                                   : L"Select an existing color stop, starting at 1.";
                    };
                }
                if (field.id == "palette.stopPosition") {
                    field.validate = [](const std::wstring &text) -> std::wstring {
                        float value{};
                        return AttributeFormModel::parse(text, value) && value >= 0 && value < 1
                                   ? L""
                                   : L"Enter a position from 0 up to, but excluding, 1.";
                    };
                }
            }
            for (auto &field : form.fields) {
                if (field.id.starts_with("source.") || field.id == "palette.selectedStop") {
                    field.persisted = false;
                }
            }
            form.apply = [self](const auto &draft) { return self->apply(draft); };
            form.canUndo = [self] { return !self->undoEntries.empty(); };
            form.canRedo = [self] { return self->order.validRedo() && !self->redoEntries.empty(); };
            form.bindHistory = [self](auto domain) {
                self->undoEntries.clear();
                self->redoEntries.clear();
                self->order.bind(std::move(domain));
            };
            form.undoOrder = [self] {
                return self->undoEntries.empty() ? 0 : self->undoEntries.back().serial;
            };
            form.redoOrder = [self] {
                return !self->order.validRedo() || self->redoEntries.empty()
                           ? 0
                           : self->redoEntries.back().serial;
            };
            form.undo = [self] { return self->restore(false); };
            form.redo = [self] { return self->restore(true); };
            form.clearHistory = [self] {
                self->undoEntries.clear();
                self->redoEntries.clear();
                self->basic.clearHistory();
                self->selected = 0;
                self->imported.reset();
                self->sourceMessage.clear();
            };
            form.actions = {{2, L"Create 8 Stops (Lossy)", [self] { self->editStops(StopAction::CreateEight); }},
                            {3, L"Add Stop", [self] { self->editStops(StopAction::Add); }},
                            {3, L"Remove Selected Stop", [self] { self->editStops(StopAction::RemoveSelected); }},
                            {3, L"Equal Spacing", [self] { self->editStops(StopAction::EqualSpacing); }},
                            {3, L"Reverse Colors", [self] { self->editStops(StopAction::ReverseColors); }}};
            form.actions.push_back({4, L"Apply Palette Preset",
                                    [self] { self->usePreset(self->libraryIndex, self->librarySeed); }});
            form.actions.push_back({5, L"Read Color File", [self] { self->readImport(self->sourcePath); }});
            form.actions.push_back(
                {5, L"Apply Imported Colors", [self] { self->applyImport(self->importCorrection); }});
            form.status = [self] {
                if (!self->sourceMessage.empty()) {
                    return self->sourceMessage;
                }
                const auto &p = self->palette();
                return p.stops.empty()
                           ? std::wstring(p.recipePresetId < 0 ? L"Original color array. "
                                                               : L"Generated recipe. ") +
                                 L"Create Stops explicitly replaces it."
                           : std::to_wstring(p.stops.size()) + L" color stops. Selected: " +
                                 std::to_wstring(std::min(self->selected, p.stops.size() - 1) + 1);
            };
            return form;
        }
    };
} // namespace merutilm::rff2::workspace
