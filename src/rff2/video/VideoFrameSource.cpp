//
// Modified by GPT-5 on 2026-08-18, 2026-09-01
// Modified by GPT-6 on 2026-09-15, 2026-09-21, 2026-09-23
// Modified by Opus 5.5 on 2026-10-10, 2026-10-13, 2026-10-16
//

#include "VideoFrameSource.hpp"
#include "../io/RFFOrbitTrapBinary.h"
#include "../formula/OrbitTrap.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <opencv2/imgproc.hpp>

#include "../constants/Constants.hpp"
#include "../ui/IOUtilities.h"

namespace merutilm::rff2 {
    namespace {
        // One interpolated sample along an axis: the two source pixels around it and the fraction between them.
        struct Tap {
            uint32_t i0 = 0;
            uint32_t i1 = 0;
            double f = 0.0;
        };

        // Where each output pixel samples the source along one axis. Output pixel o stands where source pixel
        // o * step does, both measured from the middle, as the merge shader reads it; its samples spread evenly
        // over its span, symmetric about that point.
        struct AxisSampling {
            uint32_t count;
            std::vector<Tap> taps;
            // The source pixel under each output pixel's own position.
            std::vector<uint32_t> nearest;

            AxisSampling(const uint32_t source, const uint32_t output)
                : count(std::max(1u, (source + output - 1) / output)), taps(static_cast<size_t>(output) * count),
                  nearest(output) {
                const double step = static_cast<double>(source) / output;
                for (uint32_t o = 0; o < output; ++o) {
                    nearest[o] = std::min(static_cast<uint32_t>(std::llround(o * step)), source - 1);
                    for (uint32_t k = 0; k < count; ++k) {
                        const double p = std::clamp((o + (k + 0.5) / count - 0.5) * step, 0.0, source - 1.0);
                        Tap &tap = taps[static_cast<size_t>(o) * count + k];
                        tap.i0 = static_cast<uint32_t>(p);
                        tap.i1 = std::min(tap.i0 + 1, source - 1);
                        tap.f = p - tap.i0;
                    }
                }
            }

            [[nodiscard]] const Tap &tap(const uint32_t o, const uint32_t k) const {
                return taps[static_cast<size_t>(o) * count + k];
            }
        };

        template<typename At>
        double bilinear(const At &at, const Tap &tx, const Tap &ty) {
            const double top = at(tx.i0, ty.i0) + (at(tx.i1, ty.i0) - at(tx.i0, ty.i0)) * tx.f;
            const double bottom = at(tx.i0, ty.i1) + (at(tx.i1, ty.i1) - at(tx.i0, ty.i1)) * tx.f;
            return top + (bottom - top) * ty.f;
        }

        // Escape counts are averaged over bilinear samples; a sample touching an interior or missing pixel takes its
        // nearest pixel instead, and the output pixel follows whichever side most samples land on, as the merge
        // shader's blend_height does.
        Matrix<double> reduceIterations(const Matrix<double> &source, const double maxIteration,
                                        const uint32_t outputWidth, const uint32_t outputHeight) {
            const AxisSampling xs(source.getWidth(), outputWidth);
            const AxisSampling ys(source.getHeight(), outputHeight);
            const auto at = [&source](const uint32_t px, const uint32_t py) {
                return source(static_cast<uint16_t>(px), static_cast<uint16_t>(py));
            };
            const auto escaping = [maxIteration](const double v) { return v > 0 && v < maxIteration; };
            const auto sample = [&](const Tap &tx, const Tap &ty) {
                if (escaping(at(tx.i0, ty.i0)) && escaping(at(tx.i1, ty.i0)) &&
                    escaping(at(tx.i0, ty.i1)) && escaping(at(tx.i1, ty.i1))) {
                    return bilinear(at, tx, ty);
                }
                return at(tx.f < 0.5 ? tx.i0 : tx.i1, ty.f < 0.5 ? ty.i0 : ty.i1);
            };
            Matrix<double> reduced(static_cast<uint16_t>(outputWidth), static_cast<uint16_t>(outputHeight));
            for (uint32_t y = 0; y < outputHeight; ++y) {
                for (uint32_t x = 0; x < outputWidth; ++x) {
                    uint32_t exterior = 0, interior = 0;
                    double sum = 0;
                    for (uint32_t j = 0; j < ys.count; ++j) {
                        for (uint32_t i = 0; i < xs.count; ++i) {
                            const double v = sample(xs.tap(x, i), ys.tap(y, j));
                            if (v <= 0) continue;
                            if (v >= maxIteration) {
                                ++interior;
                            } else {
                                ++exterior;
                                sum += v;
                            }
                        }
                    }
                    reduced(static_cast<uint16_t>(x), static_cast<uint16_t>(y)) =
                        exterior > 0 && exterior >= interior ? sum / exterior : interior > 0 ? maxIteration : 0.0;
                }
            }
            return reduced;
        }

        // The trap distances are averaged over the same samples, floored as the merge shader floors an exact hit;
        // the iteration has no mean, so it comes from the source pixel under the output pixel.
        std::vector<float> reduceTraps(const std::vector<float> &planes, const uint32_t width, const uint32_t height,
                                       const uint32_t outputWidth, const uint32_t outputHeight) {
            const uint64_t pixels = static_cast<uint64_t>(width) * height;
            if (pixels == 0 || (planes.size() != pixels * OrbitTrap::PLANE_COUNT &&
                                planes.size() != pixels * OrbitTrap::SHAPE_PLANE_COUNT)) {
                return {};
            }
            const auto planeCount = static_cast<uint32_t>(planes.size() / pixels);
            const AxisSampling xs(width, outputWidth);
            const AxisSampling ys(height, outputHeight);
            const uint64_t outputPixels = static_cast<uint64_t>(outputWidth) * outputHeight;
            constexpr uint32_t ITERATION_PLANE = OrbitTrap::PLANE_COUNT - 1;
            // Of the shape planes only the distance is averaged; its iteration, angle and status are picked as the iteration is.
            constexpr uint32_t SHAPE_DISTANCE_PLANE = OrbitTrap::PLANE_COUNT;
            std::vector<float> reduced(outputPixels * planeCount);
            for (uint32_t k = 0; k < planeCount; ++k) {
                const float *plane = planes.data() + k * pixels;
                float *out = reduced.data() + k * outputPixels;
                const auto at = [plane, width](const uint32_t px, const uint32_t py) {
                    return static_cast<double>(std::max(plane[static_cast<uint64_t>(py) * width + px], -4096.0f));
                };
                for (uint32_t y = 0; y < outputHeight; ++y) {
                    for (uint32_t x = 0; x < outputWidth; ++x) {
                        float &value = out[static_cast<uint64_t>(y) * outputWidth + x];
                        if (k == ITERATION_PLANE || k > SHAPE_DISTANCE_PLANE) {
                            value = plane[static_cast<uint64_t>(ys.nearest[y]) * width + xs.nearest[x]];
                            continue;
                        }
                        double sum = 0;
                        for (uint32_t j = 0; j < ys.count; ++j) {
                            for (uint32_t i = 0; i < xs.count; ++i) {
                                sum += bilinear(at, xs.tap(x, i), ys.tap(y, j));
                            }
                        }
                        value = static_cast<float>(sum / (xs.count * ys.count));
                    }
                }
            }
            return reduced;
        }
    }

    VideoFrameSource::VideoFrameSource(std::filesystem::path directory, const bool staticImages,
                                       const KeyframeRange range, const uint32_t width, const uint32_t height)
        : directory(std::move(directory)), staticImages(staticImages), range(range), frameCount(range.count),
          width(width), height(height) {}

    std::unique_ptr<VideoFrameSource> VideoFrameSource::open(const std::filesystem::path &directory,
                                                             const bool preferStatic, std::wstring &error,
                                                             const uint32_t requestedFirst,
                                                             const uint32_t requestedLast) {
        const KeyframeRange dynamicRange = keyframe_range::resolve(directory, false, requestedFirst, requestedLast);
        const KeyframeRange staticRange = keyframe_range::resolve(directory, true, requestedFirst, requestedLast);
        const bool useStatic = preferStatic ? staticRange.count > 0 : dynamicRange.count == 0 && staticRange.count > 0;
        const KeyframeRange range = useStatic ? staticRange : dynamicRange;
        if (range.count == 0) {
            error = requestedFirst || requestedLast
                        ? L"No RFM/RFMZ or RFSM/PNG keyframes were found in the selected file range."
                        : L"No numbered RFM/RFMZ or RFSM/PNG keyframes were found in this folder.";
            return nullptr;
        }

        if (useStatic) {
            const RFFStaticMapBinary first = RFFStaticMapBinary::readByID(directory, range.first);
            if (!first.hasData()) {
                error = L"The first RFSM keyframe header could not be read.";
                return nullptr;
            }
            return std::unique_ptr<VideoFrameSource>(
                new VideoFrameSource(directory, true, range, first.getWidth(), first.getHeight()));
        }

        uint16_t width = 0;
        uint16_t height = 0;
        if (!RFFDynamicMapBinary::readSizeByID(directory, range.first, width, height)) {
            error = L"The first RFM/RFMZ keyframe header could not be read.";
            return nullptr;
        }
        return std::unique_ptr<VideoFrameSource>(
            new VideoFrameSource(directory, false, range, width, height));
    }

    void VideoFrameSource::setTrapShape(const TrapShapeParams &shape) {
        if (shape == trapShape) {
            return;
        }
        trapShape = shape;
        cachedFrames.clear();
        normalFrame = std::make_shared<Frame>();
        zoomedFrame = std::make_shared<Frame>();
        loadedPair.reset();
    }

    void VideoFrameSource::setPreviewSize(const uint32_t targetWidth, const uint32_t targetHeight) {
        const auto outputWidth = std::clamp(targetWidth, 1u, width);
        const auto outputHeight = std::clamp(targetHeight, 1u, height);
        if (previewWidth == outputWidth && previewHeight == outputHeight) {
            return;
        }
        previewWidth = outputWidth;
        previewHeight = outputHeight;
        cachedFrames.clear();
        normalFrame = std::make_shared<Frame>();
        zoomedFrame = std::make_shared<Frame>();
        loadedPair.reset();
    }

    uint64_t VideoFrameSource::previewCacheBytes() const {
        if (frameCount == 0) {
            return 0;
        }
        const uint64_t pixels =
            uint64_t(previewWidth ? previewWidth : width) * (previewHeight ? previewHeight : height);
        const uint64_t overhead = sizeof(Frame) + sizeof(std::shared_ptr<Frame>) + 64;
        const uint64_t maximumBytes = std::numeric_limits<uint64_t>::max();
        // A folder whose first keyframe carries orbit traps is counted as carrying them throughout.
        std::error_code trapError;
        const bool traps = !staticImages &&
                           std::filesystem::is_regular_file(RFFOrbitTrapBinary::keyframePath(directory, range.fileNumber(1)), trapError);
        const uint64_t pixelBytes = 8 + (traps ? sizeof(float) * OrbitTrap::planeCountFor(trapShape.enabled) : 0);
        if (pixels > (maximumBytes - overhead) / pixelBytes) {
            return maximumBytes;
        }
        const uint64_t perFrameBytes = pixels * pixelBytes + overhead;
        return frameCount > maximumBytes / perFrameBytes ? maximumBytes : perFrameBytes * frameCount;
    }

    std::shared_ptr<VideoFrameSource::Frame> VideoFrameSource::readFrame(const uint32_t id,
                                                                         std::wstring &error) const {
        auto frame = std::make_shared<Frame>();
        frame->id = id;
        const uint32_t outputWidth = previewWidth ? previewWidth : width;
        const uint32_t outputHeight = previewHeight ? previewHeight : height;
        if (staticImages) {
            frame->header = RFFStaticMapBinary::readByID(directory, range.fileNumber(id));
            frame->image = RFFStaticMapBinary::loadImageByID(directory, range.fileNumber(id));
            if (!frame->header.hasData() || frame->image.empty()) {
                error = L"A required RFSM/PNG keyframe pair could not be read.";
                return nullptr;
            }
            if (frame->header.getWidth() != width || frame->header.getHeight() != height ||
                frame->image.cols != static_cast<int>(width) ||
                frame->image.rows != static_cast<int>(height)) {
                error = L"All RFSM/PNG keyframes must have the same dimensions.";
                return nullptr;
            }
            if (outputWidth != width || outputHeight != height) {
                cv::resize(frame->image, frame->image,
                           cv::Size(static_cast<int>(outputWidth), static_cast<int>(outputHeight)), 0, 0,
                           cv::INTER_AREA);
            }
        } else {
            frame->dynamic = RFFDynamicMapBinary::readByID(directory, range.fileNumber(id));
            if (!frame->dynamic.hasData()) {
                error = L"A required RFM/RFMZ keyframe pair could not be read.";
                return nullptr;
            }
            const auto &source = frame->dynamic.getMatrix();
            if (source.getWidth() != width || source.getHeight() != height) {
                error = L"All RFM/RFMZ keyframes must have the same dimensions.";
                return nullptr;
            }
            frame->traps = RFFOrbitTrapBinary::readByID(directory, range.fileNumber(id), static_cast<uint16_t>(width),
                                                        static_cast<uint16_t>(height), trapShape);
            // Averaged around each output pixel's exact center rather than picked: a picked pixel drifts across its
            // source pixel in a repeating cycle at a fractional ratio, and the relief draws that drift as a grid of dots.
            if (outputWidth != width || outputHeight != height) {
                const auto &map = frame->dynamic;
                Matrix<double> reduced = reduceIterations(source, static_cast<double>(map.getMaxIteration()),
                                                          outputWidth, outputHeight);
                frame->dynamic = RFFDynamicMapBinary(map.getLogZoom(), map.getPeriod(), map.getMaxIteration(),
                                                     std::move(reduced));
                frame->traps = reduceTraps(frame->traps, width, height, outputWidth, outputHeight);
            }
        }
        return frame;
    }

    std::shared_ptr<VideoFrameSource::Frame> VideoFrameSource::getFrame(const uint32_t id,
                                                                        std::wstring &error) const {
        if (isPreloaded()) {
            return cachedFrames[id - 1];
        }
        if (normalFrame->id == id) {
            return normalFrame;
        }
        if (zoomedFrame->id == id) {
            return zoomedFrame;
        }
        return readFrame(id, error);
    }

    bool VideoFrameSource::preload(const uint64_t byteLimit, const std::stop_token stop,
                                   const std::function<void(uint32_t, uint32_t, uint64_t)> &progress,
                                   std::wstring &error) {
        error.clear();
        const uint64_t requiredBytes = previewCacheBytes();
        if (isPreloaded()) {
            if (progress) {
                progress(frameCount, frameCount, requiredBytes);
            }
            return true;
        }
        if (requiredBytes > byteLimit) {
            error = L"Not enough free RAM for all previews. Using on-demand loading.";
            return false;
        }
        try {
            std::vector<std::shared_ptr<Frame>> pendingFrames;
            pendingFrames.reserve(frameCount);
            if (progress) {
                progress(0, frameCount, requiredBytes);
            }
            for (uint32_t index = 0; index < frameCount; ++index) {
                const uint32_t id = index + 1;
                if (stop.stop_requested()) {
                    error = L"RAM preload canceled. Using on-demand loading.";
                    return false;
                }
                auto frame = getFrame(id, error);
                if (!frame) {
                    return false;
                }
                pendingFrames.push_back(std::move(frame));
                if (progress) {
                    progress(id, frameCount, requiredBytes);
                }
            }
            if (stop.stop_requested()) {
                error = L"RAM preload canceled. Using on-demand loading.";
                return false;
            }
            cachedFrames = std::move(pendingFrames);
            return true;
        } catch (const std::bad_alloc &) {
            error = L"Not enough free RAM for all previews. Using on-demand loading.";
            return false;
        } catch (const std::exception &) {
            error = L"RAM preload failed. Using on-demand loading.";
            return false;
        }
    }

    bool VideoFrameSource::load(const float depth, std::wstring &error) {
        if (!std::isfinite(depth)) {
            error = L"Preview depth must be finite.";
            return false;
        }
        const float maximumDepth = static_cast<float>(frameCount);
        sampledDepth = std::min(depth, maximumDepth);
        if (sampledDepth >= maximumDepth) {
            sampledDepth = std::nextafter(maximumDepth, -std::numeric_limits<float>::infinity());
        }
        const uint32_t pairIndex = sampledDepth < 1.0f ? 0 : static_cast<uint32_t>(std::floor(sampledDepth));
        if (pairIndex == loadedPair) {
            return true;
        }
        auto nextNormal = getFrame(static_cast<uint32_t>(pairIndex + 1), error);
        if (!nextNormal) {
            return false;
        }
        auto nextZoomed =
            pairIndex > 0 ? getFrame(static_cast<uint32_t>(pairIndex), error) : std::make_shared<Frame>();
        if (!nextZoomed) {
            return false;
        }
        if (pairIndex == 0 && staticImages) {
            nextZoomed->image =
                cv::Mat::zeros(static_cast<int>(previewHeight ? previewHeight : height),
                               static_cast<int>(previewWidth ? previewWidth : width), CV_16UC4);
        }
        normalFrame = std::move(nextNormal);
        zoomedFrame = std::move(nextZoomed);
        loadedPair = pairIndex;
        return true;
    }
}
