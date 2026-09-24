//
// Modified by GPT-6 on 2026-09-18, 2026-09-20, 2026-09-22, 2026-09-23
//

#pragma once
#include <chrono>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <string>
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
        };
        inline static std::function<void(const Progress &)> observer;

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
        };

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
                GraphicsContextWindowImpl::setPipelineCompilationPending(previousPending);
            }
            Batch(const Batch &) = delete;
            Batch &operator=(const Batch &) = delete;

            void finish() {
                std::exception_ptr error;
                for (const auto &job : jobs) {
                    double lastUpdate = 0;
                    while (job->result.wait_for(std::chrono::milliseconds(16)) != std::future_status::ready) {
                        pumpPendingPaintMessages();
                        reportProgressIfDue(job->progress, job->start, lastUpdate);
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
                std::async(std::launch::async, [create = std::move(create)] {
                    const auto start = std::chrono::steady_clock::now();
                    create();
                    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
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
        }
    };
}
