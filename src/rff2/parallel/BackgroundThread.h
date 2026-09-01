//
// Created by Merutilm on 2025-06-09.
// Modified by Opus 5 on 2026-08-26
// Modified by GPT-5 on 2026-09-01
//

#pragma once
#include <atomic>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <stop_token>
#include <thread>

#include "../../vulkan_helper/core/logger.hpp"

namespace merutilm::rff2 {
    class BackgroundThread {
        std::mutex mutex;
        std::condition_variable_any cv;
        // Its own source rather than the jthread's: the worker may reach waitUntil before the
        // jthread member has finished being constructed, and reading it from there is a race.
        std::stop_source stopSource;
        std::atomic<bool> finished = false;
        std::jthread thread;

    public:


        template<typename T> requires std::is_invocable_r_v<void, T, BackgroundThread &> && (!std::is_same_v<T, BackgroundThread>)
        explicit BackgroundThread(T &&func) : thread(std::jthread([this, f = std::forward<T>(func)] {
            try {
                f(*this);
            } catch (const std::exception &error) {
                try {
                    vkh::logger::log_err_silent("Background worker failed: {}", error.what());
                } catch (...) {
                }
            } catch (...) {
                try {
                    vkh::logger::log_err_silent("Background worker failed with an unknown exception");
                } catch (...) {
                }
            }
            finished.store(true, std::memory_order_release);
        })) {
        }

        // Runs before the jthread member is joined, so a worker parked in waitUntil is released
        // rather than waited on forever: once the render loop is gone nothing is left to notify it.
        ~BackgroundThread() {
            stopSource.request_stop();
            notify();
        }

        BackgroundThread(const BackgroundThread &) = delete;

        BackgroundThread &operator=(const BackgroundThread &) = delete;

        BackgroundThread(BackgroundThread &&) = delete;

        BackgroundThread &operator=(BackgroundThread &&) = delete;

        friend bool operator==(const BackgroundThread &a, const BackgroundThread &b) {
            return &a == &b;
        }


        // False means the wait gave up because the thread was asked to stop, not because the
        // condition came true: the caller has to leave rather than go on to the next step.
        template<typename P> requires (!std::is_same_v<P, BackgroundThread>)
        [[nodiscard]] bool waitUntil(P &&b) {
            std::unique_lock lock(mutex);
            return cv.wait(lock, stopSource.get_token(), std::forward<P>(b));
        }


        // Under the lock, so a condition that turned true just before this call cannot be missed by
        // a waiter that is between testing it and going to sleep.
        void notify() {
            std::scoped_lock lock(mutex);
            cv.notify_all();
        }

        [[nodiscard]] bool isStopRequested() const {
            return stopSource.stop_requested();
        }

        void tryJoin() {
            if(thread.joinable()) thread.join();
        }

        bool isFinished() const {
            return finished.load(std::memory_order_acquire);
        }
    };


}
