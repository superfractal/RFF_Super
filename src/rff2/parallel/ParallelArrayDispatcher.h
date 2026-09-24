//
// Created by Merutilm on 2025-05-09.
// Modified by Opus 5 on 2026-08-26
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
#include "../data/Matrix.h"
namespace merutilm::rff2 {
    template<typename T>
    using ParallelArrayRenderer = std::function<T(uint16_t x, uint16_t y, uint16_t xRes, uint16_t yRes, float xRat, float yRat, uint32_t index,
                                                  T value)>;


    template<typename T>
    class ParallelArrayDispatcher {
        ParallelRenderState &state;
        Matrix<T> &matrix;
        ParallelArrayRenderer<T> renderer;
        uint32_t threads;

    public:
        ParallelArrayDispatcher(ParallelRenderState &state, Matrix<T> &matrix, uint32_t threads,
                                ParallelArrayRenderer<T> renderer);


        void dispatch();

    private:
        static std::vector<uint16_t> getRenderPriority(uint16_t rowsPerWorker);


        void renderForward(uint16_t xRes, uint16_t yRes, uint16_t y, std::vector<std::atomic<bool> > &rendered);


        void renderBackward(uint16_t xRes, uint16_t yRes, uint32_t len, std::vector<std::atomic<bool> > &rendered);
    };

    // DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER
    // DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER
    // DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER
    // DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER
    // DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER  DEFINITION OF PARALLEL ARRAY DISPATCHER


    template<typename T>
    ParallelArrayDispatcher<T>::ParallelArrayDispatcher(ParallelRenderState &state, Matrix<T> &matrix,
                                                        const uint32_t threads, ParallelArrayRenderer<T> renderer)
        : state(state), matrix(matrix), renderer(std::move(renderer)), threads(threads) {
    }

    template<typename T>
    void ParallelArrayDispatcher<T>::dispatch() {
        const uint16_t rowsPerWorker = matrix.getHeight() / threads + 1;
        if (state.interruptRequested()) {
            return;
        }

        const std::vector<uint16_t> rowPriority = getRenderPriority(rowsPerWorker);
        const auto xRes = matrix.getWidth();
        const auto yRes = matrix.getHeight();
        const auto pixelCount = matrix.getLength();
        std::vector<std::atomic<bool>> rendered(pixelCount);
        std::vector<std::jthread> workers;
        workers.reserve(threads);

        for (uint16_t startRow = 0; startRow < matrix.getHeight(); startRow += rowsPerWorker) {
            workers.emplace_back([startRow, &rowPriority, xRes, yRes, this, &rendered, pixelCount] {
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

    template<typename T>
    std::vector<uint16_t> ParallelArrayDispatcher<T>::getRenderPriority(const uint16_t rowsPerWorker) {
        std::vector<uint16_t> priority(rowsPerWorker, 0);
        uint16_t offset = rowsPerWorker >> 1;
        uint16_t repetitionCount = 1;
        uint16_t writeIndex = 1;

        while (offset > 0) {
            for (uint16_t j = 0; j < repetitionCount; ++j) {
                priority[writeIndex] = priority[j] + offset;
                ++writeIndex;
            }

            repetitionCount <<= 1;
            offset >>= 1;
        }

        auto sortedPriority = priority;
        sortedPriority.resize(writeIndex);
        std::ranges::sort(sortedPriority);

        uint16_t sortedIndex = 0;
        while (writeIndex < priority.size()) {
            const uint16_t missing = sortedIndex + offset;
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


    template<typename T>
    void ParallelArrayDispatcher<T>::renderForward(const uint16_t xRes, const uint16_t yRes, const uint16_t y,
                                                   std::vector<std::atomic<bool> > &rendered) {
        if (y >= yRes) {
            return;
        }

        for (uint16_t x = 0; x < xRes; ++x) {
            if (x % Constants::Fractal::EXIT_CHECK_INTERVAL == 0 && state.interruptRequested()) {
                return;
            }

            const uint32_t index = static_cast<uint32_t>(xRes) * y + x;

            if (!rendered[index].exchange(true)) {
                // Relaxed: the preview reads this matrix as it fills, so the elements are written
                // the same way it reads them.
                matrix.storeRelaxed(index, renderer(x, y, xRes, yRes, static_cast<float>(x) / xRes,
                                                    static_cast<float>(y) / yRes, index,
                                                    matrix.loadRelaxed(index)));
            }
        }
    }


    template<typename T>
    void ParallelArrayDispatcher<T>::renderBackward(const uint16_t xRes, const uint16_t yRes, const uint32_t len,
                                                    std::vector<std::atomic<bool> > &rendered) {
        for (uint32_t i = len - 1; i > 0; --i) {
            if (i % Constants::Fractal::EXIT_CHECK_INTERVAL == 0 && state.interruptRequested()) {
                return;
            }
            const auto [px, py] = matrix.getLocation(i);

            if (!rendered[i].exchange(true)) {
                const T value = renderer(px, py, xRes, yRes, static_cast<float>(px) / xRes,
                                         static_cast<float>(py) / yRes, i, matrix.loadRelaxed(i));
                matrix.storeRelaxed(i, value);
            }
        }
    }
}
