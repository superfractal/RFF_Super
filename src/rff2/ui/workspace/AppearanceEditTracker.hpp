//
// Modified by GPT-6 on 2026-09-14, 2026-09-21, 2026-09-23
//

#pragma once
#include "AppearanceState.hpp"
#include "WorkspaceForm.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace merutilm::rff2::workspace {
    class AppearanceEditTracker : public std::enable_shared_from_this<AppearanceEditTracker> {
      public:
        struct Snapshot {
            ShaderAttribute shader;
            std::wstring label;
        };

      private:
        std::function<ShaderAttribute &()> attributeGetter;
        std::optional<Snapshot> referenceSnapshot;
        std::optional<Snapshot> rollbackSnapshot;
        bool surfaceEditOpen = false;
        uint64_t version = 0;

      public:
        explicit AppearanceEditTracker(std::function<ShaderAttribute &()> getter)
            : attributeGetter(std::move(getter)) {}
        const std::optional<Snapshot> &reference() const {
            return referenceSnapshot;
        }
        uint64_t revision() const {
            return version;
        }
        ShaderAttribute current() const {
            return attributeGetter();
        }
        void clear() {
            referenceSnapshot.reset();
            rollbackSnapshot.reset();
            surfaceEditOpen = false;
            ++version;
        }
        void record(ShaderAttribute prior, std::wstring label) {
            if (AppearanceState::key(prior) == AppearanceState::key(attributeGetter())) {
                return;
            }
            referenceSnapshot = Snapshot{std::move(prior), std::move(label)};
            ++version;
        }
        void beginSurface(const ShdSlopeAttribute &prior, std::wstring label) {
            if (surfaceEditOpen) {
                return;
            }
            rollbackSnapshot = referenceSnapshot;
            surfaceEditOpen = true;
            auto priorShader = attributeGetter();
            priorShader.slope = prior;
            referenceSnapshot = Snapshot{std::move(priorShader), std::move(label)};
            ++version;
        }
        void endSurface(bool accepted) {
            if (!surfaceEditOpen) {
                return;
            }
            if (!accepted) {
                referenceSnapshot = std::move(rollbackSnapshot);
                ++version;
            }
            rollbackSnapshot.reset();
            surfaceEditOpen = false;
        }
        WorkspaceForm wrap(WorkspaceForm form) {
            auto tracker = shared_from_this();
            const auto label = form.title;
            if (form.apply) {
                form.apply = [tracker, apply = std::move(form.apply), label](const FormDraft &draft) {
                    auto priorShader = tracker->current();
                    auto error = apply(draft);
                    if (error.empty()) {
                        tracker->record(std::move(priorShader), label);
                    }
                    return error;
                };
            }
            for (auto &action : form.actions) {
                action.invoke = [tracker, invoke = std::move(action.invoke), name = action.label] {
                    auto priorShader = tracker->current();
                    invoke();
                    tracker->record(std::move(priorShader), name);
                };
            }
            return form;
        }
    };
} // namespace merutilm::rff2::workspace
