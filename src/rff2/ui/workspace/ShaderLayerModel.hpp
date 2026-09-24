//
// Modified by GPT-6 on 2026-09-17, 2026-09-21, 2026-09-23
// Modified by GPT-5 on 2026-09-17
//

#pragma once
#include "WorkspaceForm.hpp"
#include "../../attr/ShaderAttribute.h"
#include "../UiLanguage.hpp"
#include <algorithm>
#include <iterator>
#include <memory>
#include <optional>

namespace merutilm::rff2::workspace {
    inline std::wstring shaderLayerName(ShdLayer layer) {
        static constexpr const wchar_t *names[] = {L"Band Line",
                                         L"Texture 1",
                                         L"Texture 2",
                                         L"Texture 3",
                                         L"Texture 4",
                                         L"Pattern 1",
                                         L"Pattern 2",
                                         L"Pattern 3",
                                         L"Pattern 4",
                                         L"Stripe",
                                         L"Lighting & Base Material",
                                         L"Metal",
                                         L"Cyber Sigilism",
                                         L"PHONK",
                                         L"Frost",
                                         L"Sea",
                                         L"Print Material",
                                         L"Material Color & Rim",
                                         L"Effect 1",
                                         L"Effect 2",
                                         L"Effect 3",
                                         L"Effect 4",
                                         L"Color Correction",
                                         L"Fog",
                                         L"Bloom",
                                         L"HDR / Tone Mapping",
                                         L"Print Color Quantization",
                                         L"VHS Finish",
                                         L"Monochrome Finish"};
        static_assert(std::size(names) == ShdLayerOrder::COUNT);
        const auto id = uint32_t(layer);
        if (id < 1 || id > ShdLayerOrder::COUNT) {
            return L"";
        }
        return UiLanguage::text(names[id - 1]);
    }

    class ShaderLayerModel : public std::enable_shared_from_this<ShaderLayerModel> {
        struct Edit {
            ShdLayerOrder before, after;
            uint64_t serial;
        };
        std::function<ShaderAttribute &()> attribute;
        std::function<void()> changed;
        std::function<void(ShaderAttribute)> record;
        std::vector<Edit> undoEntries, redoEntries;
        HistoryOrder history;

      public:
        ShdLayer selected = ShdLayer::BAND_LINE;
        ShaderLayerModel(std::function<ShaderAttribute &()> getter, std::function<void()> changed,
                         std::function<void(ShaderAttribute)> record = {})
            : attribute(std::move(getter)), changed(std::move(changed)), record(std::move(record)) {}
        const ShaderAttribute &shader() const {
            return attribute();
        }
        const ShdLayerOrder &order() const {
            return attribute().layerOrder;
        }
        int position() const {
            return int(std::find(order().layers.begin(), order().layers.end(), selected) -
                       order().layers.begin());
        }
        bool commit(const ShdLayerOrder &next) {
            const auto prior = order();
            if (!next.valid() || prior == next) {
                return false;
            }
            std::optional<ShaderAttribute> before;
            if (record) {
                before = attribute();
            }
            undoEntries.push_back({prior, next, history.commit()});
            if (undoEntries.size() > 128) {
                undoEntries.erase(undoEntries.begin());
            }
            redoEntries.clear();
            attribute().layerOrder = next;
            if (before) {
                record(std::move(*before));
            }
            changed();
            return true;
        }
        bool move(int direction) {
            const int from = position(), to = from + direction;
            if (to < 0 || to >= int(ShdLayerOrder::COUNT)) {
                return false;
            }
            auto next = order();
            next.move(from, to);
            return commit(next);
        }
        bool enable(bool value) {
            auto next = order();
            next.enabled = value;
            return commit(next);
        }
        bool toggleVisible() {
            auto next = order();
            next.setVisible(selected, !next.visible(selected));
            return commit(next);
        }
        bool moveTo(int position) {
            return move(position - this->position());
        }
        bool reset() {
            return commit({});
        }
        void clear() {
            undoEntries.clear();
            redoEntries.clear();
        }
        bool canUndo() const {
            return !undoEntries.empty() && undoEntries.back().after == order();
        }
        bool canRedo() const {
            return history.validRedo() && !redoEntries.empty() && redoEntries.back().before == order();
        }
        bool restore(bool redo) {
            if (redo ? !canRedo() : !canUndo()) {
                return false;
            }
            auto &from = redo ? redoEntries : undoEntries;
            auto &to = redo ? undoEntries : redoEntries;
            if (!redo) {
                history.prepareUndo(redoEntries);
            }
            const auto next = redo ? from.back().after : from.back().before;
            to.push_back(from.back());
            from.pop_back();
            attribute().layerOrder = next;
            changed();
            return true;
        }
        WorkspaceForm form(std::function<void()> open) {
            auto self = shared_from_this();
            WorkspaceForm form;
            form.title = L"Shader Layers";
            form.groups = {L"Layer Order"};
            form.fields.push_back(
                {"layers.enabled",
                 0,
                 L"Custom Layer Order",
                 L"Moving a layer enables custom compositing. Disabling restores the original rendering.",
                 [self] { return self->order().enabled ? L"1" : L"0"; },
                 {{L"0", L"Off"}, {L"1", L"On"}}});
            FormField choice{"layers.selected", 0, L"Selected Layer", L"Higher layers are applied later.",
                             [self] { return std::to_wstring(uint32_t(self->selected)); }};
            choice.persisted = false;
            for (auto layer : ShdLayerOrder::defaults()) {
                choice.choices.push_back({std::to_wstring(uint32_t(layer)), shaderLayerName(layer)});
            }
            form.fields.push_back(std::move(choice));
            form.apply = [self](const FormDraft &draft) -> std::wstring {
                if (const auto it = draft.find("layers.selected"); it != draft.end()) {
                    try {
                        auto id = std::stoul(it->second);
                        if (id < 1 || id > ShdLayerOrder::COUNT) {
                            return L"Invalid layer";
                        }
                        self->selected = ShdLayer(id);
                    } catch (...) {
                        return L"Invalid layer";
                    }
                }
                if (const auto it = draft.find("layers.enabled"); it != draft.end()) {
                    self->enable(it->second == L"1");
                }
                return {};
            };
            form.actions = {{0, L"Open Layer List", std::move(open)},
                            {0, L"Move Up", [self] { self->move(1); }},
                            {0, L"Move Down", [self] { self->move(-1); }},
                            {0, L"Bring to Front",
                             [self] { self->move(int(ShdLayerOrder::COUNT) - 1 - self->position()); }},
                            {0, L"Send to Back", [self] { self->move(-self->position()); }},
                            {0, L"Restore Original Order", [self] { self->reset(); }}};
            form.canUndo = [self] { return self->canUndo(); };
            form.canRedo = [self] { return self->canRedo(); };
            form.undo = [self] { return self->restore(false); };
            form.redo = [self] { return self->restore(true); };
            form.clearHistory = [self] { self->clear(); };
            form.bindHistory = [self](auto domain) {
                self->clear();
                self->history.bind(std::move(domain));
            };
            form.undoOrder = [self] { return self->canUndo() ? self->undoEntries.back().serial : 0; };
            form.redoOrder = [self] { return self->canRedo() ? self->redoEntries.back().serial : 0; };
            form.status = [self] {
                return UiLanguage::text(
                    self->order().enabled ? L"Custom order active. Top layers are applied last."
                                          : L"Original rendering active. Move a layer to try custom order.");
            };
            return form;
        }
    };
} // namespace merutilm::rff2::workspace
