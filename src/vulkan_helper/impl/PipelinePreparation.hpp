//
// Modified by GPT-6 on 2026-09-18, 2026-09-20, 2026-09-22, 2026-09-23
// Modified by Opus 5.5 on 2026-10-13
//

#pragma once
#include <algorithm>
#include <atomic>
#include <chrono>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include "GraphicsContextWindow.hpp"

namespace merutilm::vkh {
    struct PipelinePreparation {
        struct Progress {
            HWND window;
            std::string shader;
            std::string device;
            double seconds = 0;
            bool finished = false;
            bool succeeded = false;
            // The wait may be cut short here: what is left can finish behind a working window.
            bool skippable = false;
            // The wait was cut short, so the window goes away and no time is recorded for this one.
            bool skipped = false;
            // Finished after a skip, with no window up for it: only its time is kept.
            bool background = false;
        };
        inline static std::function<void(const Progress &)> observer;
        // Raised by the preparation window's Skip. It stays up until the outermost batch has handed
        // its deferrable jobs off, so batches nested in it (the video warm-up) skip as well.
        inline static bool skipRequested = false;
        // The preparation window: the one window whose clicks get through while a batch waits.
        inline static HWND interactiveWindow = nullptr;
        // Set at shutdown, once the windows are gone: a compile the user skipped is then waited for
        // unseen, rather than putting the preparation window back up after they closed RFF_Super.
        inline static bool quietWaits = false;

        // While one is open on this thread, a pipeline that can draw without its final shader may
        // mark itself deferrable, which lets Skip leave it compiling.
        struct DeferralScope {
            inline static thread_local bool open = false;
            bool previous = open;
            DeferralScope() { open = true; }
            ~DeferralScope() { open = previous; }
            DeferralScope(const DeferralScope &) = delete;
            DeferralScope &operator=(const DeferralScope &) = delete;
        };

        static void notify(const Progress &progress) noexcept {
            try {
                if (observer)
                    observer(progress);
            } catch (...) {
            }
        }

        struct Job {
            Progress progress;
            std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
            std::shared_future<double> result;
            // Its owner draws with a stand-in until it is done, so a skip may leave it compiling.
            // Atomic only for the test delay below, which reads it from the compile thread.
            std::atomic<bool> deferrable = false;
            // A skip did leave it compiling; its owner collects it once it is ready.
            bool deferred = false;
        };

        // Jobs left compiling by a skip, kept here until their time has been recorded.
        inline static std::vector<std::shared_ptr<Job>> deferredJobs;
        // Objects such a compile may still be reading, like a render pass a resize replaced.
        inline static std::vector<std::shared_ptr<void>> keepAlive;

        [[nodiscard]] static bool deferredPending() { return !deferredJobs.empty(); }

        // With the test switch below set, appends a timed line to pipeline-skip-trace.txt beside the exe,
        // so a stall after Skip can be traced without a debugger. Does nothing otherwise.
        static void trace(const std::string &event) {
            if (testDeferredDelay() <= 0) {
                return;
            }
            static const auto origin = std::chrono::steady_clock::now();
            static std::mutex mutex;
            const double seconds =
                std::chrono::duration<double>(std::chrono::steady_clock::now() - origin).count();
            std::scoped_lock lock(mutex);
            wchar_t buffer[MAX_PATH]{};
            GetModuleFileNameW(nullptr, buffer, MAX_PATH);
            std::ofstream out(std::filesystem::path(buffer).parent_path() / L"pipeline-skip-trace.txt", std::ios::app);
            out << std::format("{:9.3f}s  [{}] {}\n", seconds,
                               GetCurrentThreadId() == mainThread() ? "ui" : "worker", event);
        }

        static void keepAliveWhileDeferred(std::shared_ptr<void> object) {
            if (deferredPending() && object)
                keepAlive.push_back(std::move(object));
        }

        // Records the deferred jobs that have finished, and lets go of what they were holding on to.
        static void pollDeferred() {
            std::erase_if(deferredJobs, [](const std::shared_ptr<Job> &job) {
                if (job->result.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
                    return false;
                trace("background compile done: " + job->progress.shader);
                settle(*job, true);
                return true;
            });
            if (deferredJobs.empty())
                keepAlive.clear();
        }

        // Waits for one deferred job with the preparation window up, for a caller that needs it now.
        static void await(const std::shared_ptr<Job> &job) {
            if (!job || !job->deferred)
                return;
            trace("await begins: " + job->progress.shader);
            if (!quietWaits && GetWindowThreadProcessId(job->progress.window, nullptr) == GetCurrentThreadId()) {
                job->progress.skippable = false;
                double lastUpdate = 0;
                while (job->result.wait_for(std::chrono::milliseconds(16)) != std::future_status::ready) {
                    pumpPendingPaintMessages();
                    reportProgressIfDue(job->progress, job->start, lastUpdate);
                }
            } else {
                job->result.wait();
            }
            trace("await ends: " + job->progress.shader);
            if (std::erase(deferredJobs, job) != 0)
                settle(*job, quietWaits);
            if (deferredJobs.empty())
                keepAlive.clear();
        }

        // Waits for every compile a skip left running, for a caller about to shut the device down.
        static void awaitAll() {
            for (const auto pending = deferredJobs; const auto &job : pending)
                await(job);
            keepAlive.clear();
        }

        struct Batch;
        inline static thread_local Batch *activeBatch = nullptr;

        struct Batch {
            Batch *previous = activeBatch;
            bool previousPending = GraphicsContextWindowImpl::isPipelineCompilationPending();
            std::vector<std::shared_ptr<Job>> jobs;
            explicit Batch() {
                activeBatch = this;
                GraphicsContextWindowImpl::setPipelineCompilationPending(true);
            }
            ~Batch() {
                for (const auto &job : jobs)
                    job->result.wait();
                activeBatch = previous;
                if (previous == nullptr)
                    skipRequested = false;
                GraphicsContextWindowImpl::setPipelineCompilationPending(previousPending);
            }
            Batch(const Batch &) = delete;
            Batch &operator=(const Batch &) = delete;

            void finish() {
                trace(std::format("batch finish begins ({} jobs)", jobs.size()));
                std::exception_ptr error;
                bool dismissed = false;
                for (size_t index = 0; index < jobs.size(); ++index) {
                    const auto &job = jobs[index];
                    const auto deferrableLeft = [&] {
                        return std::any_of(jobs.begin() + static_cast<std::ptrdiff_t>(index), jobs.end(),
                                           [](const std::shared_ptr<Job> &left) { return left->deferrable.load(); });
                    };
                    double lastUpdate = 0;
                    bool handedOff = false;
                    while (job->result.wait_for(std::chrono::milliseconds(16)) != std::future_status::ready) {
                        if (skipRequested && job->deferrable) {
                            handedOff = true;
                            break;
                        }
                        job->progress.skippable = !skipRequested && deferrableLeft();
                        pumpPendingPaintMessages();
                        reportProgressIfDue(job->progress, job->start, lastUpdate);
                    }
                    if (handedOff) {
                        trace("handed off to the background: " + job->progress.shader);
                        job->deferred = true;
                        deferredJobs.push_back(job);
                        if (!dismissed) {
                            dismissed = true;
                            Progress closing = job->progress;
                            closing.skipped = true;
                            notify(closing);
                        }
                        continue;
                    }
                    try {
                        job->progress.seconds = job->result.get();
                        job->progress.succeeded = true;
                    } catch (...) {
                        if (!error)
                            error = std::current_exception();
                    }
                    job->progress.finished = true;
                    notify(job->progress);
                }
                jobs.clear();
                activeBatch = previous;
                if (previous == nullptr)
                    skipRequested = false;
                trace("batch finish ends");
                if (error)
                    std::rethrow_exception(error);
            }
        };

        template <class Create>
        static std::shared_ptr<Job> enqueue(HWND window, std::string shader, std::string device,
                                            Create create) {
            if (!activeBatch || GetWindowThreadProcessId(window, nullptr) != GetCurrentThreadId()) {
                create();
                return {};
            }
            auto job = std::make_shared<Job>();
            job->progress = {window, std::move(shader), std::move(device)};
            job->result =
                std::async(std::launch::async, [create = std::move(create), target = job.get()] {
                    const auto start = std::chrono::steady_clock::now();
                    create();
                    const double seconds =
                        std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
                    // Checked after the compile, by which time the owner has long marked the job.
                    if (const double delay = testDeferredDelay(); delay > 0 && target->deferrable) {
                        std::this_thread::sleep_for(std::chrono::duration<double>(delay));
                    }
                    // The delay is left out, so the recorded time stays the real compile's.
                    return seconds;
                }).share();
            activeBatch->jobs.push_back(job);
            return job;
        }

        template <class Create>
        static VkResult run(HWND window, std::string shader, std::string device, Create create) {
            if (GetWindowThreadProcessId(window, nullptr) != GetCurrentThreadId())
                return create();
            struct Scope {
                bool previous = GraphicsContextWindowImpl::isPipelineCompilationPending();
                Progress progress;
                std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
                explicit Scope(Progress value) : progress(std::move(value)) {
                    GraphicsContextWindowImpl::setPipelineCompilationPending(true);
                }
                ~Scope() {
                    progress.seconds =
                        std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
                    progress.finished = true;
                    notify(progress);
                    GraphicsContextWindowImpl::setPipelineCompilationPending(previous);
                }
            } scope({window, std::move(shader), std::move(device)});
            auto compilation = std::async(std::launch::async, std::move(create));
            double lastUpdate = 0;
            while (compilation.wait_for(std::chrono::milliseconds(16)) != std::future_status::ready) {
                // Pump paint only so scene edits cannot reenter pipeline construction.
                pumpPendingPaintMessages();
                reportProgressIfDue(scope.progress, scope.start, lastUpdate);
            }
            const VkResult result = compilation.get();
            scope.progress.succeeded = result == VK_SUCCESS;
            return result;
        }

      private:
        // Test switch: RFF_TEST_DEFERRED_PIPELINE_DELAY=<seconds> holds every deferrable pipeline back
        // that long after its compile, so Skip can be tried when the caches make preparation instant.
        static DWORD mainThread() {
            static const DWORD id = GetCurrentThreadId();
            return id;
        }

        static double testDeferredDelay() {
            static const double delay = [] {
                wchar_t value[32]{};
                const DWORD length = GetEnvironmentVariableW(L"RFF_TEST_DEFERRED_PIPELINE_DELAY", value, 32);
                if (length == 0 || length >= 32) {
                    return 0.0;
                }
                try {
                    return std::clamp(std::stod(value), 0.0, 600.0);
                } catch (...) {
                    return 0.0;
                }
            }();
            return delay;
        }

        static void settle(Job &job, bool background) {
            try {
                job.progress.seconds = job.result.get();
                job.progress.succeeded = true;
            } catch (...) {
                // The owner rethrows this when it collects the pipeline; here only the time is lost.
                job.progress.succeeded = false;
            }
            job.progress.finished = true;
            job.progress.background = background;
            notify(job.progress);
        }

        static void reportProgressIfDue(Progress &progress,
                                        std::chrono::steady_clock::time_point start,
                                        double &lastUpdate) {
            progress.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
            if (progress.seconds >= 0.5 && progress.seconds - lastUpdate >= 0.1) {
                notify(progress);
                lastUpdate = progress.seconds;
            }
        }

        static void pumpPendingPaintMessages() {
            MSG message;
            for (int count = 0; count < 32; ++count) {
                if (!PeekMessageW(&message, nullptr, WM_PAINT, WM_PAINT, PM_REMOVE))
                    break;
                if (message.message == WM_QUIT) {
                    PostQuitMessage(static_cast<int>(message.wParam));
                    break;
                }
                DispatchMessageW(&message);
            }
            // Mouse input for the preparation window alone, so its Skip can be clicked; every other
            // window keeps waiting, as scene edits cannot reenter pipeline construction.
            if (interactiveWindow != nullptr) {
                for (int count = 0; count < 32; ++count) {
                    if (!PeekMessageW(&message, interactiveWindow, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE))
                        break;
                    DispatchMessageW(&message);
                }
            }
        }
    };
}
