//
// Created by Merutilm on 2025-05-08.
// Modified by Opus 5 on 2026-08-26
// Modified by GPT-6 on 2026-09-22
// Modified by Opus 5.5 on 2026-09-23
//

#pragma once
#include <algorithm>
#include <atomic>
#include <stdexcept>
#include <vector>
#include <array>

namespace merutilm::rff2 {
    template<typename T>
    class Matrix {
        uint16_t width;
        uint16_t height;
        std::vector<T> canvas;

    public:
        // Counted in size_t: two uint16_t operands promote to int, and 65535 * 65535 overflows it.
        Matrix(const uint16_t width, const uint16_t height) : width(width), height(height),
                                                              canvas(elementCount(width, height)) {
        }

        // A shorter vector would put coordinates the clamp accepts past the end of the storage.
        Matrix(const uint16_t width, const uint16_t height, const std::vector<T> &data) : width(width), height(height),
            canvas(data) {
            if (canvas.size() != elementCount(width, height)) {
                throw std::invalid_argument("Matrix data does not match its dimensions");
            }
        }

        const T &operator[](uint32_t i) const {
            return canvas[i];
        }

        T &operator[](uint32_t i) {
            return canvas[i];
        }

        const T &operator()(const uint16_t x, const uint16_t y) const {
            return canvas[getIndex(x, y)];
        }

        T &operator()(const uint16_t x, const uint16_t y) {
            return canvas[getIndex(x, y)];
        }

        uint32_t getIndex(uint16_t x, uint16_t y) const {
            // An empty axis clamps to 0 rather than letting width - 1 wrap to 65535.
            x = std::min(x, static_cast<uint16_t>(width == 0 ? 0 : width - 1));
            y = std::min(y, static_cast<uint16_t>(height == 0 ? 0 : height - 1));
            return static_cast<uint32_t>(width) * y + x;
        }

        std::array<uint16_t, 2> getLocation(const uint32_t i) const {
            const uint16_t px = i % width;
            const uint16_t py = i / width;
            return {px, py};
        }

        uint16_t getWidth() const {
            return width;
        }

        uint16_t getHeight() const {
            return height;
        }

        uint32_t getLength() const {
            return static_cast<uint32_t>(width) * height;
        }

        // Element access for a matrix one thread is filling while another reads it as it fills, as
        // the progressive preview does. Relaxed is all that is wanted - each element stands alone
        // and no other memory is being published through it - so on the targets this runs on these
        // compile to the same plain load and store. What they buy is that the concurrent access is
        // defined instead of a race the optimizer is entitled to assume never happens.
        T loadRelaxed(const uint32_t i) const {
            return std::atomic_ref(const_cast<T &>(canvas[i])).load(std::memory_order_relaxed);
        }

        void storeRelaxed(const uint32_t i, const T value) {
            std::atomic_ref(canvas[i]).store(value, std::memory_order_relaxed);
        }

        void storeRelaxed(const uint16_t x, const uint16_t y, const T value) {
            storeRelaxed(getIndex(x, y), value);
        }

        const std::vector<T> &getCanvas() const {
            return canvas;
        }

    private:
        static size_t elementCount(const uint16_t width, const uint16_t height) {
            return static_cast<size_t>(width) * height;
        }
    };
}