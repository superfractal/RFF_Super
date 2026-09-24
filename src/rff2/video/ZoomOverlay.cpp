//
// Modified by GPT-6 on 2026-09-18, 2026-09-20, 2026-09-21, 2026-09-23
//

#include "ZoomOverlay.hpp"
#include <gdiplus.h>
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>
#include <format>
#include <stdexcept>
#include <array>
#include <sstream>

namespace merutilm::rff2 {
    namespace {
        enum OverlayLayer {
            ShadowLayer,
            OutlineLayer,
            TextLayer,
            LayerCount
        };

        struct GraphicsRuntime {
            ULONG_PTR token = 0;
            GraphicsRuntime() {
                Gdiplus::GdiplusStartupInput input;
                if (Gdiplus::GdiplusStartup(&token, &input, nullptr) != Gdiplus::Ok) {
                    throw std::runtime_error("Cannot initialize zoom overlay fonts");
                }
            }
            ~GraphicsRuntime() {
                Gdiplus::GdiplusShutdown(token);
            }
        };
        std::wstring utf8ToWide(const std::string &s) {
            const int n =
                MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), int(s.size()), nullptr, 0);
            if (!n) {
                throw std::runtime_error("Invalid overlay font name");
            }
            std::wstring out(n, 0);
            MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), int(s.size()), out.data(), n);
            return out;
        }
        // IEC 61966-2-1, SMPTE ST 2084 and ARIB STD-B67; shared project provenance is recorded in NOTICE.
        double decode(double v, VidHdrTransfer t) {
            if (t == VidHdrTransfer::PQ) {
                const double p = std::pow(v, 1.0 / 78.84375);
                return 10000 * std::pow(std::max(p - .8359375, 0.0) / (18.8515625 - 18.6875 * p),
                                        1.0 / .1593017578125);
            }
            if (t == VidHdrTransfer::HLG) {
                return v <= .5 ? v * v / 3 : (std::exp((v - .55991073) / .17883277) + .28466892) / 12;
            }
            return v <= .04045 ? v / 12.92 : std::pow((v + .055) / 1.055, 2.4);
        }
        // Inverses of the same project transfer functions; see NOTICE.
        double encode(double v, VidHdrTransfer t) {
            if (t == VidHdrTransfer::PQ) {
                const double p = std::pow(std::clamp(v / 10000, 0.0, 1.0), .1593017578125);
                return std::pow((.8359375 + 18.8515625 * p) / (1 + 18.6875 * p), 78.84375);
            }
            if (t == VidHdrTransfer::HLG) {
                v = std::clamp(v, 0.0, 1.0);
                return v <= 1.0 / 12 ? std::sqrt(3 * v)
                                     : .17883277 * std::log(12 * v - .28466892) + .55991073;
            }
            v = std::clamp(v, 0.0, 1.0);
            return v <= .0031308 ? 12.92 * v : 1.055 * std::pow(v, 1.0 / 2.4) - .055;
        }
        std::array<double, 3> linearOverlayColor(glm::vec4 c, VidHdrTransfer t) {
            std::array<double, 3> v{decode(c.r, VidHdrTransfer::SDR), decode(c.g, VidHdrTransfer::SDR),
                                    decode(c.b, VidHdrTransfer::SDR)};
            if (t == VidHdrTransfer::SDR) {
                return v;
            }
            // Reuses vk_linear_interpolation.frag's BT.709 to BT.2020 matrix; see NOTICE.
            const double w = t == VidHdrTransfer::PQ ? 203.0 : .26;
            return {w * (.6274039 * v[0] + .3292830 * v[1] + .0433131 * v[2]),
                    w * (.0690970 * v[0] + .9195406 * v[1] + .0113624 * v[2]),
                    w * (.0163916 * v[0] + .0880132 * v[1] + .8955952 * v[2])};
        }
        void drawLegacyOverlay(cv::Mat &out, double zoom, VidHdrTransfer hdrTransfer, uint32_t decimalPlaces);
    } // namespace

    struct ZoomOverlay::Impl {
        GraphicsRuntime runtime;
        std::string key;
        std::array<cv::Mat, LayerCount> masks;
        cv::Rect area;
        cv::Rect fullBounds;
        std::wstring warning;

        void layout(int width, int height, const std::string &text, const VidZoomOverlayAttribute &settings) {
            std::ostringstream cacheKey;
            cacheKey.precision(9);
            cacheKey << width << ',' << height << ',' << text << ',' << settings.family << ','
                     << settings.style << ',' << settings.anchor << ',' << settings.x << ',' << settings.y
                     << ',' << settings.size << ',' << settings.outline << ',' << settings.outlineWidth << ','
                     << settings.shadow << ',' << settings.shadowX << ',' << settings.shadowY;
            const std::string requestedKey = cacheKey.str();
            if (key == requestedKey) {
                return;
            }
            key.clear();
            warning.clear();
            const auto name = utf8ToWide(settings.family);
            Gdiplus::FontFamily requested(name.c_str());
            const Gdiplus::FontFamily *family = &requested;
            if (requested.GetLastStatus() != Gdiplus::Ok) {
                family = Gdiplus::FontFamily::GenericSansSerif();
                warning = L"Requested font unavailable; using system sans serif.";
            }
            int style = ((settings.style & 1) ? Gdiplus::FontStyleBold : 0) |
                        ((settings.style & 2) ? Gdiplus::FontStyleItalic : 0);
            if (!family->IsStyleAvailable(style)) {
                style = Gdiplus::FontStyleRegular;
                warning = L"Requested font style unavailable; using Regular.";
            }
            const float fontSize = std::max(1.f, settings.size * height);
            const auto label = utf8ToWide(text);
            Gdiplus::GraphicsPath path;
            Gdiplus::StringFormat format(Gdiplus::StringFormat::GenericTypographic());
            format.SetFormatFlags(format.GetFormatFlags() | Gdiplus::StringFormatFlagsNoWrap);
            if (path.AddString(label.c_str(), int(label.size()), family, style, fontSize,
                               Gdiplus::PointF(0, 0), &format) != Gdiplus::Ok) {
                throw std::runtime_error("Cannot render the selected overlay font");
            }
            Gdiplus::RectF ink;
            path.GetBounds(&ink);
            const float lineHeight =
                fontSize * family->GetLineSpacing(style) / std::max<UINT16>(1, family->GetEmHeight(style));
            const float left = settings.x * width - float(settings.anchor % 3) * .5f * ink.Width - ink.X;
            const float top = settings.y * height - float(settings.anchor / 3) * .5f * lineHeight;
            Gdiplus::Matrix position;
            position.Translate(left, top);
            path.Transform(&position);
            path.GetBounds(&ink);
            const float border = settings.outline ? fontSize * settings.outlineWidth : 0;
            const float shadowOffsetX = settings.shadow ? fontSize * settings.shadowX : 0;
            const float shadowOffsetY = settings.shadow ? fontSize * settings.shadowY : 0;
            const int x = int(std::floor(ink.X - border + std::min(0.f, shadowOffsetX))) - 2;
            const int y = int(std::floor(ink.Y - border + std::min(0.f, shadowOffsetY))) - 2;
            const int right = int(std::ceil(ink.GetRight() + border + std::max(0.f, shadowOffsetX))) + 2;
            const int bottom = int(std::ceil(ink.GetBottom() + border + std::max(0.f, shadowOffsetY))) + 2;
            fullBounds = {x, y, right - x, bottom - y};
            area = fullBounds & cv::Rect(0, 0, width, height);
            if (area != fullBounds) {
                if (!warning.empty()) {
                    warning += L" ";
                }
                warning += L"Overlay extends beyond the video frame.";
            }
            for (auto &mask : masks) {
                mask.release();
            }
            if (area.empty()) {
                key = requestedKey;
                return;
            }
            for (int layer = ShadowLayer; layer < LayerCount; ++layer) {
                if ((layer == ShadowLayer && !settings.shadow) ||
                    (layer == OutlineLayer && border == 0)) {
                    continue;
                }
                cv::Mat pixels(area.height, area.width, CV_8UC4, cv::Scalar(0, 0, 0, 0));
                Gdiplus::Bitmap bitmap(area.width, area.height, int(pixels.step), PixelFormat32bppARGB,
                                       pixels.data);
                Gdiplus::Graphics graphics(&bitmap);
                graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
                graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
                graphics.TranslateTransform(float(-area.x) + (layer == ShadowLayer ? shadowOffsetX : 0),
                                            float(-area.y) + (layer == ShadowLayer ? shadowOffsetY : 0));
                Gdiplus::SolidBrush white(Gdiplus::Color(255, 255, 255, 255));
                if (layer == OutlineLayer) {
                    Gdiplus::Pen pen(Gdiplus::Color(255, 255, 255, 255), border * 2);
                    pen.SetLineJoin(Gdiplus::LineJoinRound);
                    graphics.DrawPath(&pen, &path);
                }
                graphics.FillPath(&white, &path);
                graphics.Flush(Gdiplus::FlushIntentionSync);
                cv::extractChannel(pixels, masks[layer], 3);
            }
            key = requestedKey;
        }
    };

    ZoomOverlay::ZoomOverlay() : impl(std::make_unique<Impl>()) {}
    ZoomOverlay::~ZoomOverlay() = default;
    std::string ZoomOverlay::format(double logZoom, uint32_t decimalPlaces) {
        if (!std::isfinite(logZoom) || std::abs(logZoom) > 9e15) {
            throw std::runtime_error("Zoom value is unavailable or out of range");
        }
        if (decimalPlaces > 9) {
            throw std::runtime_error("Zoom decimal places must be from 0 to 9");
        }
        const double scale = std::pow(10.0, decimalPlaces);
        double exponent = std::floor(logZoom);
        double mantissa = std::round(std::pow(10.0, logZoom - exponent) * scale) / scale;
        if (mantissa >= 10) {
            mantissa = 1;
            exponent += 1;
        }
        return std::format("Zoom : {:.{}f}E{}", mantissa, decimalPlaces, static_cast<int64_t>(exponent));
    }
    std::wstring ZoomOverlay::status() const {
        return impl->warning;
    }
    cv::Rect ZoomOverlay::bounds() const {
        return impl->fullBounds;
    }

    void ZoomOverlay::apply(cv::Mat &image, double zoom, const VidZoomOverlayAttribute &settings,
                            VidHdrTransfer transfer) {
        if (!settings.visible) {
            impl->warning.clear();
            return;
        }
        if (!settings.custom) {
            drawLegacyOverlay(image, zoom, transfer, settings.decimalPlaces);
            impl->warning.clear();
            return;
        }
        if (image.type() != CV_8UC3 && image.type() != CV_8UC4 && image.type() != CV_16UC4) {
            throw std::runtime_error("Unsupported zoom overlay image format");
        }
        impl->layout(image.cols, image.rows, format(zoom, settings.decimalPlaces), settings);
        const std::array<glm::vec4, LayerCount> colors{
            settings.shadowColor, settings.outlineColor, settings.color};
        const bool hdr = image.depth() == CV_16U;
        const double channelMaximum = hdr ? 65535.0 : 255.0;
        for (int layer = ShadowLayer; layer < LayerCount; ++layer) {
            const auto &mask = impl->masks[layer];
            if (mask.empty() || colors[layer].a == 0) {
                continue;
            }
            const auto color = linearOverlayColor(colors[layer], transfer);
            for (int y = 0; y < mask.rows; ++y) {
                const auto *coverage = mask.ptr<uint8_t>(y);
                for (int x = 0; x < mask.cols; ++x) {
                    if (!coverage[x]) {
                        continue;
                    }
                    const double alpha = coverage[x] / 255.0 * colors[layer].a;
                    const int imageX = x + impl->area.x;
                    const int imageY = y + impl->area.y;
                    for (int component = 0; component < 3; ++component) {
                        const int channel = hdr ? component : 2 - component;
                        double value = hdr ? image.ptr<uint16_t>(imageY)[imageX * 4 + channel]
                                           : image.ptr<uint8_t>(imageY)[imageX * image.channels() + channel];
                        value = encode(decode(value / channelMaximum, transfer) * (1 - alpha) +
                                           color[component] * alpha,
                                       transfer);
                        const auto result = std::lround(std::clamp(value, 0.0, 1.0) * channelMaximum);
                        if (hdr) {
                            image.ptr<uint16_t>(imageY)[imageX * 4 + channel] = uint16_t(result);
                        } else {
                            image.ptr<uint8_t>(imageY)[imageX * image.channels() + channel] = uint8_t(result);
                        }
                    }
                }
            }
        }
    }

    void ZoomOverlay::paint(HDC dc, RECT image, double zoom, const VidZoomOverlayAttribute &s) {
        const int w = image.right - image.left;
        const int h = image.bottom - image.top;
        if (w <= 0 || h <= 0 || !s.visible) {
            return;
        }
        BITMAPINFO info{};
        info.bmiHeader = {sizeof(BITMAPINFOHEADER), w, -h, 1, 32, BI_RGB, 0, 0, 0, 0, 0};
        void *bits = nullptr;
        const HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
        const HDC memory = CreateCompatibleDC(dc);
        if (!bitmap || !memory) {
            if (bitmap) {
                DeleteObject(bitmap);
            }
            if (memory) {
                DeleteDC(memory);
            }
            return;
        }
        const auto previous = SelectObject(memory, bitmap);
        BitBlt(memory, 0, 0, w, h, dc, image.left, image.top, SRCCOPY);
        GdiFlush();
        try {
            cv::Mat pixels(h, w, CV_8UC4, bits);
            apply(pixels, zoom, s);
            BitBlt(dc, image.left, image.top, w, h, memory, 0, 0, SRCCOPY);
        } catch (const std::exception &) {
            impl->warning = L"Cannot render zoom overlay; check the font and zoom value.";
        }
        SelectObject(memory, previous);
        DeleteDC(memory);
        DeleteObject(bitmap);
    }
} // namespace merutilm::rff2

namespace merutilm::rff2 {
    namespace {
        void drawLegacyOverlay(cv::Mat &out, double zoom, VidHdrTransfer hdrTransfer,
                               uint32_t decimalPlaces) {
            const int imageWidth = out.cols;
            // The zoom overlay is drawn at diffuse white rather than at the peak the format reaches,
            // which on an HDR display would be painful to look at next to the picture.
            // The two curves below repeat the encoders in vk_linear_interpolation.frag; NOTICE records their standards.
            const uint16_t textLevel = [](const VidHdrTransfer t) -> uint16_t {
                if (t == VidHdrTransfer::PQ) {
                    // ST 2084 at 203 nits, the reference white of an HDR10 master.
                    constexpr double m1 = 0.1593017578125;
                    constexpr double m2 = 78.84375;
                    constexpr double c1 = 0.8359375;
                    constexpr double c2 = 18.8515625;
                    constexpr double c3 = 18.6875;
                    const double y = std::pow(203.0 / 10000.0, m1);
                    return static_cast<uint16_t>(
                        std::lround(std::pow((c1 + c2 * y) / (1.0 + c3 * y), m2) * 65535.0));
                }
                if (t == VidHdrTransfer::HLG) {
                    // ARIB STD-B67 at the 0.26 scene level HLG calls diffuse white.
                    constexpr double a = 0.17883277;
                    constexpr double b = 0.28466892;
                    constexpr double c = 0.55991073;
                    return static_cast<uint16_t>(std::lround((a * std::log(12.0 * 0.26 - b) + c) * 65535.0));
                }
                return 65535;
            }(hdrTransfer);

            const int leftMargin = std::max(1, imageWidth / 72);
            const int topMargin = std::max(1, imageWidth / 192);
            const int baselineOffset = std::max(1, imageWidth / 40);
            const float size = std::max(1.0f, static_cast<float>(imageWidth) / 800);
            const int shadowOffset = std::max(1, baselineOffset / 15);
            const int strokeWidth = std::max(1, shadowOffset / 2);
            const std::string label = ZoomOverlay::format(zoom, decimalPlaces);
            if (out.depth() == CV_8U) {
                cv::putText(out, label,
                            cv::Point(leftMargin + shadowOffset, baselineOffset + topMargin + shadowOffset),
                            cv::FONT_HERSHEY_PLAIN, size, cv::Scalar(0, 0, 0));
                cv::putText(out, label, cv::Point(leftMargin, baselineOffset + topMargin),
                            cv::FONT_HERSHEY_PLAIN, size, cv::Scalar(255, 255, 255), strokeWidth,
                            cv::LINE_AA);
            } else {
                // An antialiased glyph is only laid into an 8-bit image, so the HDR frame takes the
                // two passes as coverage masks and has them composited over it at full range instead.
                const int maskHeight = std::min(out.rows, baselineOffset + topMargin + shadowOffset +
                                                              static_cast<int>(size * 8) + 4);
                cv::Mat shadow = cv::Mat::zeros(maskHeight, out.cols, CV_8UC1);
                cv::Mat glyph = cv::Mat::zeros(maskHeight, out.cols, CV_8UC1);
                cv::putText(shadow, label,
                            cv::Point(leftMargin + shadowOffset, baselineOffset + topMargin + shadowOffset),
                            cv::FONT_HERSHEY_PLAIN, size, cv::Scalar(255));
                cv::putText(glyph, label, cv::Point(leftMargin, baselineOffset + topMargin),
                            cv::FONT_HERSHEY_PLAIN, size, cv::Scalar(255), strokeWidth, cv::LINE_AA);
                for (int y = 0; y < maskHeight; ++y) {
                    const auto *shadowRow = shadow.ptr<uint8_t>(y);
                    const auto *glyphRow = glyph.ptr<uint8_t>(y);
                    auto *imageRow = out.ptr<cv::Vec4w>(y);
                    for (int x = 0; x < out.cols; ++x) {
                        if (shadowRow[x] == 0 && glyphRow[x] == 0) {
                            continue;
                        }
                        const float shadowAlpha = static_cast<float>(shadowRow[x]) / 255.0f;
                        const float glyphAlpha = static_cast<float>(glyphRow[x]) / 255.0f;
                        for (int c = 0; c < 3; ++c) {
                            float v = static_cast<float>(imageRow[x][c]) * (1.0f - shadowAlpha);
                            v = v * (1.0f - glyphAlpha) + static_cast<float>(textLevel) * glyphAlpha;
                            imageRow[x][c] =
                                static_cast<uint16_t>(std::lround(std::clamp(v, 0.0f, 65535.0f)));
                        }
                    }
                }
            }
        }
    } // namespace
} // namespace merutilm::rff2
