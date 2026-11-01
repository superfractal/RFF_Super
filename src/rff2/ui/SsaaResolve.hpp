//
// Created by Opus 5.5 on 2026-10-07.
// Modified by Opus 5.5 on 2026-10-08
//

#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "../attr/RndSsaaFilter.h"
#include "../attr/VidHdrTransfer.h"
#include "../video/HdrTransfer.hpp"

// Supersampling resolve on the CPU, the same filter shdsrc/ssaa_resolve.glsl applies on the GPU.
namespace merutilm::rff2::SsaaResolve {
    // Mitchell-Netravali cubic with B = C = 1/3 (Mitchell & Netravali, SIGGRAPH 1988; see NOTICE), zero from |x| = 2 on.
    inline double mitchell(double x) {
        x = std::abs(x);
        if (x < 1.0) {
            return (7.0 * x * x * x - 12.0 * x * x + 16.0 / 3.0) / 6.0;
        }
        if (x < 2.0) {
            return (-7.0 / 3.0 * x * x * x + 12.0 * x * x - 20.0 * x + 32.0 / 3.0) / 6.0;
        }
        return 0.0;
    }

    // IEC 61966-2-1 sRGB transfer; provenance: NOTICE.
    inline float srgbToLinear(const float c) {
        return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
    }

    inline float linearToSrgb(float l) {
        l = std::clamp(l, 0.0f, 1.0f);
        return l <= 0.0031308f ? l * 12.92f : 1.055f * std::pow(l, 1.0f / 2.4f) - 0.055f;
    }

    // A stored code value to linear light by the curve the image carries; SDR means sRGB, as in ssaa_decode.
    inline float decode(const float code, const VidHdrTransfer transfer) {
        return transfer == VidHdrTransfer::SDR ? srgbToLinear(code) : HdrTransfer::decode(code, transfer);
    }

    // Linear light back to a code value in [0, 1], as in ssaa_encode.
    inline float encode(const float linear, const VidHdrTransfer transfer) {
        return transfer == VidHdrTransfer::SDR ? linearToSrgb(linear)
                                               : std::clamp(HdrTransfer::encode(linear, transfer), 0.0f, 1.0f);
    }

    // For one axis, the source taps of every output position and their weights, normalized to sum to 1.
    struct AxisTaps {
        std::vector<int> first;
        std::vector<int> count;
        std::vector<float> weights;
        int stride = 0;
    };

    inline AxisTaps axisTaps(const int source, const int output) {
        const double ratio = static_cast<double>(source) / output;
        const double scale = std::max(ratio, 1.0);
        const double radius = 2.0 * scale;
        AxisTaps taps;
        taps.stride = static_cast<int>(std::ceil(2.0 * radius)) + 1;
        taps.first.resize(output);
        taps.count.resize(output);
        taps.weights.assign(static_cast<size_t>(output) * taps.stride, 0.0f);
        for (int o = 0; o < output; ++o) {
            const double center = (o + 0.5) * ratio;
            const int lo = std::max(static_cast<int>(std::ceil(center - radius - 0.5)), 0);
            const int hi = std::min(static_cast<int>(std::floor(center + radius - 0.5)), source - 1);
            const int n = std::clamp(hi - lo + 1, 0, taps.stride);
            double sum = 0.0;
            for (int i = 0; i < n; ++i) {
                sum += mitchell((lo + i + 0.5 - center) / scale);
            }
            for (int i = 0; i < n; ++i) {
                taps.weights[static_cast<size_t>(o) * taps.stride + i] =
                    sum > 0.0 ? static_cast<float>(mitchell((lo + i + 0.5 - center) / scale) / sum) : 0.0f;
            }
            taps.first[o] = lo;
            taps.count[o] = n;
        }
        return taps;
    }

    // Resizes an 8- or 16-bit, 3- or 4-channel image stored with transfer (SDR = sRGB); alpha is filtered as stored.
    inline cv::Mat resize(const cv::Mat &image, const cv::Size size, const RndSsaaFilter filter,
                          const VidHdrTransfer transfer = VidHdrTransfer::SDR) {
        const bool shrinks = size.width < image.cols || size.height < image.rows;
        const int depth = image.depth();
        if (filter == RndSsaaFilter::BOX_GAMMA || !shrinks || (depth != CV_8U && depth != CV_16U) ||
            (image.channels() != 3 && image.channels() != 4)) {
            cv::Mat resized;
            cv::resize(image, resized, size, 0, 0, cv::INTER_AREA);
            return resized;
        }
        const int channels = image.channels();
        const float maxCode = depth == CV_8U ? 255.0f : 65535.0f;

        // The decode as a table over every code value, so each sample costs a lookup instead of a pow.
        std::vector<float> decodeTable(static_cast<size_t>(maxCode) + 1);
        for (size_t i = 0; i < decodeTable.size(); ++i) {
            decodeTable[i] = decode(static_cast<float>(i) / maxCode, transfer);
        }
        cv::Mat linear(image.size(), CV_MAKETYPE(CV_32F, channels));
        const auto toLinear = [&]<typename T>(T) {
            for (int y = 0; y < image.rows; ++y) {
                const T *src = image.ptr<T>(y);
                float *dst = linear.ptr<float>(y);
                for (int i = 0; i < image.cols * channels; ++i) {
                    const bool alpha = channels == 4 && i % 4 == 3;
                    dst[i] = alpha ? static_cast<float>(src[i]) / maxCode : decodeTable[src[i]];
                }
            }
        };
        if (depth == CV_8U) toLinear(uint8_t{});
        else toLinear(uint16_t{});

        // Separable: across the rows first, then down the columns of that result.
        const AxisTaps horizontal = axisTaps(image.cols, size.width);
        const AxisTaps vertical = axisTaps(image.rows, size.height);
        cv::Mat wide(image.rows, size.width, linear.type());
        cv::parallel_for_(cv::Range(0, image.rows), [&](const cv::Range &rows) {
            for (int y = rows.start; y < rows.end; ++y) {
                const float *src = linear.ptr<float>(y);
                float *dst = wide.ptr<float>(y);
                for (int o = 0; o < size.width; ++o) {
                    const float *w = &horizontal.weights[static_cast<size_t>(o) * horizontal.stride];
                    for (int c = 0; c < channels; ++c) {
                        float sum = 0.0f;
                        for (int i = 0; i < horizontal.count[o]; ++i) {
                            sum += w[i] * src[(horizontal.first[o] + i) * channels + c];
                        }
                        dst[o * channels + c] = sum;
                    }
                }
            }
        });
        cv::Mat result(size, image.type());
        cv::parallel_for_(cv::Range(0, size.height), [&](const cv::Range &rows) {
            std::vector<float> line(static_cast<size_t>(size.width) * channels);
            for (int o = rows.start; o < rows.end; ++o) {
                std::fill(line.begin(), line.end(), 0.0f);
                const float *w = &vertical.weights[static_cast<size_t>(o) * vertical.stride];
                for (int i = 0; i < vertical.count[o]; ++i) {
                    const float *src = wide.ptr<float>(vertical.first[o] + i);
                    for (size_t k = 0; k < line.size(); ++k) {
                        line[k] += w[i] * src[k];
                    }
                }
                for (size_t k = 0; k < line.size(); ++k) {
                    // The negative lobes can overshoot past either end, so linear light is clamped before encoding.
                    const bool alpha = channels == 4 && k % 4 == 3;
                    const float encoded = alpha ? std::clamp(line[k], 0.0f, 1.0f) : encode(line[k], transfer);
                    const float code = std::round(encoded * maxCode);
                    if (depth == CV_8U) result.ptr<uint8_t>(o)[k] = static_cast<uint8_t>(code);
                    else result.ptr<uint16_t>(o)[k] = static_cast<uint16_t>(code);
                }
            }
        });
        return result;
    }
}
