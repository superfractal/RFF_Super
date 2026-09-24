//
// Created by Merutilm on 2025-05-09.
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <functional>
#include <thread>
#include <utility>
#include <vector>

#include "ParallelRenderState.h"
#include "../constants/Constants.hpp"
namespace merutilm::rff2 {
    using ParallelRenderer = std::function<void(uint32_t x, uint32_t y, uint32_t xRes, uint32_t yRes, float xRat,
                                                float yRat, uint32_t index)>;

    class ParallelDispatcher {
        ParallelRenderState &state;
        ParallelRenderer renderer;
        uint32_t xRes;
        uint32_t yRes;
        uint32_t threads;

    public:
        ParallelDispatcher(ParallelRenderState &state, uint32_t xRes, uint32_t yRes, uint32_t threads, ParallelRenderer renderer);


        void dispatch() const;

    private:
        static std::vector<uint32_t> getRenderPriority(uint32_t rowsPerWorker);


        void renderForward(uint32_t xRes, uint32_t yRes, uint32_t y, std::vector<std::atomic<bool> > &rendered) const;


        void renderBackward(uint32_t xRes, uint32_t yRes, uint32_t len, std::vector<std::atomic<bool> > &rendered) const;
    };

    // DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER
    // DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER
    // DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER
    // DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER
    // DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER


    inline ParallelDispatcher::ParallelDispatcher(ParallelRenderState &state, const uint32_t xRes,
                                                  const uint32_t yRes, const uint32_t threads,
                                                  ParallelRenderer renderer)
        : state(state), renderer(std::move(renderer)), xRes(xRes), yRes(yRes), threads(threads) {
    }

    inline void ParallelDispatcher::dispatch() const {
        const uint32_t rowsPerWorker = yRes / threads + 1;
        if (state.interruptRequested()) {
            return;
        }

        const std::vector<uint32_t> rowPriority = getRenderPriority(rowsPerWorker);
        const auto pixelCount = xRes * yRes;
        std::vector<std::atomic<bool>> rendered(pixelCount);
        std::vector<std::jthread> workers;
        workers.reserve(threads);

        for (uint32_t startRow = 0; startRow < yRes; startRow += rowsPerWorker) {
            workers.emplace_back([startRow, &rowPriority, this, &rendered, pixelCount] {
                for (const auto rowOffset : rowPriority) {
                    renderForward(xRes, yRes, startRow + rowOffset, rendered);
                }
                renderBackward(xRes, yRes, pixelCount, rendered);
            });
        }

        for (auto &worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    inline std::vector<uint32_t> ParallelDispatcher::getRenderPriority(const uint32_t rowsPerWorker) {
        std::vector<uint32_t> priority(rowsPerWorker, 0);
        uint32_t offset = rowsPerWorker >> 1;
        uint32_t repetitionCount = 1;
        uint32_t writeIndex = 1;

        while (offset > 0) {
            for (uint32_t j = 0; j < repetitionCount; ++j) {
                priority[writeIndex] = priority[j] + offset;
                ++writeIndex;
            }

            repetitionCount <<= 1;
            offset >>= 1;
        }

        auto sortedPriority = priority;
        sortedPriority.resize(writeIndex);
        std::ranges::sort(sortedPriority);

        uint32_t sortedIndex = 0;
        while (writeIndex < priority.size()) {
            const uint32_t missing = sortedIndex + offset;
            if (sortedPriority.size() <= sortedIndex || sortedPriority[sortedIndex] != missing) {
                priority[writeIndex] = missing;
                ++writeIndex;
                ++offset;
            } else {
                ++sortedIndex;
            }
        }
        return priority;
    }


    inline void ParallelDispatcher::renderForward(const uint32_t xRes, const uint32_t yRes, const uint32_t y,
                                                  std::vector<std::atomic<bool> > &rendered) const {
        if (y >= yRes) {
            return;
        }

        for (uint32_t x = 0; x < xRes; ++x) {
            if (x % Constants::Fractal::EXIT_CHECK_INTERVAL == 0 && state.interruptRequested()) {
                return;
            }

            const uint32_t index = static_cast<uint32_t>(xRes) * y + x;
            if (!rendered[index].exchange(true)) {
                renderer(x, y, xRes, yRes, static_cast<float>(x) / xRes,
                         static_cast<float>(y) / yRes, index);
            }
        }
    }


    inline void ParallelDispatcher::renderBackward(const uint32_t xRes, const uint32_t yRes, const uint32_t len,
                                                   std::vector<std::atomic<bool> > &rendered) const {
        for (uint32_t i = len - 1; i > 0; --i) {
            if (i % Constants::Fractal::EXIT_CHECK_INTERVAL == 0 && state.interruptRequested()) {
                return;
            }
            const uint32_t px = i % xRes;
            const uint32_t py = i / xRes;

            if (!rendered[i].exchange(true)) {
                renderer(px, py, xRes, yRes, static_cast<float>(px) / xRes, static_cast<float>(py) / yRes, i);
            }
        }
    }
}
