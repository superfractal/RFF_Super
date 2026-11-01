//
// Created by Opus 5.5 on 2026-10-26.
//

#include "BuddhabrotPerturbator.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>
#include <vector>

namespace merutilm::rff2 {
    namespace {
        constexpr uint64_t CHUNK_SAMPLES = 1 << 14;
        // Starting points cover [-2, 2] x [0, 2]; the lower half is drawn as their mirror image.
        constexpr double SAMPLE_AREA = 8.0;
        // Brings the brightest filaments of a default view to about one 64-iteration palette cycle.
        constexpr double DENSITY_SCALE = 4.0;
        constexpr auto PUBLISH_INTERVAL = std::chrono::milliseconds(100);

        // SplitMix64 output for a counter: Sebastiano Vigna, splitmix64.c (2015), CC0; see NOTICE.
        uint64_t splitMix64(const uint64_t counter) {
            uint64_t z = (counter + 1) * 0x9E3779B97F4A7C15ULL;
            z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
            z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
            return z ^ (z >> 31);
        }

        double unitInterval(const uint64_t bits) {
            return static_cast<double>(bits >> 11) * 0x1.0p-53;
        }

        // Closed-form tests for the main cardioid and the period-2 disc, whose points never escape.
        bool insideMainComponents(const double cr, const double ci) {
            const double xr = cr - 0.25;
            const double q = xr * xr + ci * ci;
            if (q * (q + xr) <= 0.25 * ci * ci) return true;
            const double br = cr + 1.0;
            return br * br + ci * ci <= 0.0625;
        }
    }

    BuddhabrotPerturbator::BuddhabrotPerturbator(ParallelRenderState &state, const FractalAttribute &calc,
                                                 const double dcMax)
        : MandelbrotPerturbator(state, calc), dummyReference(std::make_unique<DummyReference>()), dcMax(dcMax) {
    }

    const MandelbrotReference *BuddhabrotPerturbator::getReference() const {
        return dummyReference.get();
    }

    dex BuddhabrotPerturbator::getDcMaxAsDoubleExp() const {
        return dex::value(dcMax);
    }

    double BuddhabrotPerturbator::iterate(const dex &, const dex &, float *escapeAngle, OrbitTrap *trap) const {
        if (escapeAngle != nullptr) *escapeAngle = 0.0f;
        if (trap != nullptr) *trap = OrbitTrap{};
        return static_cast<double>(calc.maxIteration);
    }

    bool BuddhabrotPerturbator::render(const BuddhabrotPlane &plane, Matrix<double> &matrix, const uint32_t threads,
                                       std::atomic<int> &renderedPixels) const {
        const uint16_t w = matrix.getWidth();
        const uint16_t h = matrix.getHeight();
        const uint32_t length = matrix.getLength();
        const double det = plane.axisXReal * plane.axisYImag - plane.axisYReal * plane.axisXImag;
        if (length == 0 || !std::isfinite(det) || det == 0.0) return true;

        const double centerReal = calc.center.real.edit().double_value() + plane.originReal;
        const double centerImag = calc.center.imag.edit().double_value() + plane.originImag;
        const uint64_t maxIteration = calc.maxIteration;
        const uint64_t minIteration = calc.buddhabrotMinIteration;
        const uint64_t total = static_cast<uint64_t>(calc.buddhabrotSamples) * length;
        const uint64_t chunks = (total + CHUNK_SAMPLES - 1) / CHUNK_SAMPLES;
        // A value at Max Iteration draws as the set's interior, so the brightest pixels stop just short of it.
        const double ceiling = std::nextafter(static_cast<double>(maxIteration), 0.0);

        std::vector<std::atomic<uint32_t>> hits(length);
        std::atomic<uint64_t> nextChunk = 0;
        std::atomic<uint64_t> doneChunks = 0;
        std::atomic<int64_t> lastPublish = std::chrono::steady_clock::now().time_since_epoch().count();

        const auto record = [&](const double zr, const double zi) {
            const double dr = zr - centerReal;
            const double di = zi - centerImag;
            const double x = std::round((dr * plane.axisYImag - plane.axisYReal * di) / det);
            const double y = std::round((plane.axisXReal * di - dr * plane.axisXImag) / det);
            if (x >= 0.0 && x < w && y >= 0.0 && y < h) {
                hits[static_cast<uint32_t>(y) * w + static_cast<uint32_t>(x)].fetch_add(1, std::memory_order_relaxed);
            }
        };

        // Visits per unit of plane area over all starting points: the same at any resolution or sample count.
        const auto publish = [&](const uint64_t samples) {
            const double scale = DENSITY_SCALE * SAMPLE_AREA / (static_cast<double>(samples) * std::abs(det));
            for (uint32_t i = 0; i < length; ++i) {
                const double density = scale * hits[i].load(std::memory_order_relaxed);
                matrix.storeRelaxed(i, std::min(1.0 + density, ceiling));
            }
        };

        const auto worker = [&] {
            for (uint64_t chunk = nextChunk.fetch_add(1); chunk < chunks; chunk = nextChunk.fetch_add(1)) {
                if (state.interruptRequested()) return;
                const uint64_t end = std::min(total, (chunk + 1) * CHUNK_SAMPLES);
                for (uint64_t sample = chunk * CHUNK_SAMPLES; sample < end; ++sample) {
                    const double cr = -2.0 + 4.0 * unitInterval(splitMix64(2 * sample));
                    const double ci = 2.0 * unitInterval(splitMix64(2 * sample + 1));
                    if (cr * cr + ci * ci > 4.0 || insideMainComponents(cr, ci)) continue;

                    double zr = 0.0;
                    double zi = 0.0;
                    uint64_t escape = 0;
                    for (uint64_t i = 1; i <= maxIteration; ++i) {
                        const double nr = zr * zr - zi * zi + cr;
                        zi = 2.0 * zr * zi + ci;
                        zr = nr;
                        if (zr * zr + zi * zi > 4.0) {
                            escape = i;
                            break;
                        }
                        if ((i & 0xFFFF) == 0 && state.interruptRequested()) return;
                    }
                    if (escape == 0 || escape <= minIteration) continue;

                    // The second pass retraces the orbit up to the step before it left the radius-2 disc.
                    zr = 0.0;
                    zi = 0.0;
                    for (uint64_t i = 1; i < escape; ++i) {
                        const double nr = zr * zr - zi * zi + cr;
                        zi = 2.0 * zr * zi + ci;
                        zr = nr;
                        record(zr, zi);
                        record(zr, -zi);
                        if ((i & 0xFFFF) == 0 && state.interruptRequested()) return;
                    }
                }

                const uint64_t done = doneChunks.fetch_add(1) + 1;
                renderedPixels.store(static_cast<int>(static_cast<double>(done) / static_cast<double>(chunks) * length),
                                     std::memory_order_relaxed);
                const int64_t now = std::chrono::steady_clock::now().time_since_epoch().count();
                int64_t last = lastPublish.load(std::memory_order_relaxed);
                if (now - last >= std::chrono::steady_clock::duration(PUBLISH_INTERVAL).count() &&
                    lastPublish.compare_exchange_strong(last, now)) {
                    publish(std::min(total, done * CHUNK_SAMPLES));
                }
            }
        };

        {
            std::vector<std::jthread> pool;
            pool.reserve(std::max(1u, threads));
            for (uint32_t t = 0; t < std::max(1u, threads); ++t) {
                pool.emplace_back(worker);
            }
        }
        if (state.interruptRequested()) return false;
        publish(total);
        renderedPixels.store(static_cast<int>(length), std::memory_order_relaxed);
        return true;
    }
}
