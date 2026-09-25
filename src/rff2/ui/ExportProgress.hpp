//
// Modified by GPT-6 on 2026-09-14, 2026-09-22, 2026-09-25
//

#pragma once
#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <new>
#include <string>

namespace merutilm::rff2 {
    struct ExportProgress {
        enum class Phase { READY, QUEUED, RENDERING, WRITING, COMPLETED, CANCELLED, FAILED };
        struct Snapshot {
            Phase phase = Phase::READY;
            float ratio = 0;
            std::wstring message = L"Ready to export.";
        };
        std::atomic<bool> cancelRequested{false};

      private:
        mutable std::mutex mutex;
        Snapshot value;
        std::wstring diagnosticLog;

      public:
        void setDiagnosticLog(std::wstring path) {
            std::scoped_lock lock(mutex);
            diagnosticLog = std::move(path);
        }
        std::wstring getDiagnosticLog() const {
            std::scoped_lock lock(mutex);
            return diagnosticLog;
        }
        Snapshot snapshot() const {
            std::scoped_lock lock(mutex);
            return value;
        }
        void report(Phase phase, std::wstring message, float ratio = 0) {
            std::scoped_lock lock(mutex);
            value = {phase, std::clamp(ratio, 0.f, 1.f), std::move(message)};
        }
        bool active() const {
            std::scoped_lock lock(mutex);
            const auto phase = value.phase;
            return phase == Phase::QUEUED || phase == Phase::RENDERING || phase == Phase::WRITING;
        }
    };
    struct ExportCompletion {
        std::shared_ptr<ExportProgress> progress;
        ~ExportCompletion() {
            if (progress && progress->active()) {
                try {
                    progress->report(progress->cancelRequested ? ExportProgress::Phase::CANCELLED
                                                               : ExportProgress::Phase::FAILED,
                                     progress->cancelRequested ? L"Export cancelled."
                                                               : L"Export did not complete.");
                } catch (const std::bad_alloc &) {
                    const auto terminalPhase = progress->cancelRequested ? ExportProgress::Phase::CANCELLED
                                                                        : ExportProgress::Phase::FAILED;
                    progress->report(terminalPhase, {});
                }
            }
        }
    };
} // namespace merutilm::rff2
