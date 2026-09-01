//
// Created by Merutilm on 2025-06-23.
// Modified by GPT-5 on 2026-08-23, 2026-09-01
// Modified by Opus 5 on 2026-08-31
//

#include "RFFStaticMapBinary.h"

#include "../../vulkan_helper/core/logger.hpp"
#include "../ui/IOUtilities.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/core/mat.hpp"

namespace merutilm::rff2 {


    namespace {
        // The same ceiling the dynamic map is read under. A keyframe header is three numbers with
        // nothing behind them to check against, so a file naming a four-billion-pixel frame passes
        // a bare "greater than zero" test and is carried on into an int cast and a window extent.
        constexpr uint64_t MAX_MAP_PIXELS = 100000000;
    }

    const RFFStaticMapBinary RFFStaticMapBinary::DEFAULT = RFFStaticMapBinary(0, 0, 0);

    RFFStaticMapBinary::RFFStaticMapBinary(const float logZoom, const uint32_t width, const uint32_t height) : RFFBinary(logZoom), width(width), height(height) {

    }

    bool RFFStaticMapBinary::hasData() const {
        return width > 0 && height > 0;
    }


    RFFStaticMapBinary RFFStaticMapBinary::read(const std::filesystem::path &path) {
        if (!std::filesystem::exists(path)) {
            return DEFAULT;
        }
        std::ifstream in(path, std::ios::in | std::ios::binary);

        if (!in.is_open()) {
            return DEFAULT;
        }

        float lz;
        IOUtilities::readAndDecode(in, &lz);
        uint32_t w;
        IOUtilities::readAndDecode(in, &w);
        uint32_t h;
        IOUtilities::readAndDecode(in, &h);
        if (!in || w == 0 || h == 0 || static_cast<uint64_t>(w) * h > MAX_MAP_PIXELS) {
            return DEFAULT;
        }
        return RFFStaticMapBinary(lz, w, h);
    }

    RFFStaticMapBinary RFFStaticMapBinary::readByID(const std::filesystem::path& dir, const uint32_t id) {
        return read(dir / IOUtilities::fileNameFormat(id, Constants::Extension::STATIC_MAP));
    }
    cv::Mat RFFStaticMapBinary::loadImageByID(const std::filesystem::path &dir, const uint32_t id) {
        return IOUtilities::readImage(dir / IOUtilities::fileNameFormat(id, Constants::Extension::IMAGE),
                                      cv::IMREAD_UNCHANGED);
    }


    void RFFStaticMapBinary::exportAsKeyframe(const std::filesystem::path &dir) const {
        exportFile(IOUtilities::generateFileName(dir, Constants::Extension::STATIC_MAP));
    }

    void RFFStaticMapBinary::exportFile(const std::filesystem::path &path) const {
        const std::filesystem::path temporary = IOUtilities::temporaryFilePath(path);
        if (std::ofstream out(temporary, std::ios::out | std::ios::binary | std::ios::trunc); out.is_open()) {
            IOUtilities::encodeAndWrite(out, getLogZoom());
            IOUtilities::encodeAndWrite(out, getWidth());
            IOUtilities::encodeAndWrite(out, getHeight());
            out.close();
            if (out.fail() || !IOUtilities::commitTemporaryFile(temporary, path)) {
                IOUtilities::discardTemporaryFile(temporary);
                vkh::logger::w_log(L"ERROR : Cannot save file");
            }
        } else {
            vkh::logger::log("ERROR : Cannot save file");
        }
    }

    uint32_t RFFStaticMapBinary::getWidth() const {
        return width;
    }
    uint32_t RFFStaticMapBinary::getHeight() const {
        return height;
    }


}
