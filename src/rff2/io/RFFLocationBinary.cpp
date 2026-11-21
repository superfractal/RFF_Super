//
// Created by Merutilm on 2025-06-25.
// Modified by GPT-5 on 2026-08-23, 2026-08-31, 2026-09-01
// Modified by GPT-6 on 2026-09-23
// Modified by Opus 5.5 on 2026-11-15
//

#include "RFFLocationBinary.h"

#include <cmath>
#include <utility>

#include "RFF2FileVersion.hpp"
#include "../../vulkan_helper/core/logger.hpp"
#include "../calc/fp_decimal_calculator.h"
#include "../ui/IOUtilities.h"

namespace merutilm::rff2 {
    namespace {
        constexpr uint64_t MAX_COORDINATE_BYTES = 16ULL * 1024 * 1024;
    }
    inline const RFFLocationBinary RFFLocationBinary::DEFAULT = RFFLocationBinary(0, "", "", 0);

    RFFLocationBinary::RFFLocationBinary(const float logZoom, std::string real, std::string imag,
                             const uint64_t maxIteration) : RFFBinary(logZoom), real(std::move(real)), imag(std::move(imag)),
                                                      maxIteration(maxIteration) {
    }

    RFFLocationBinary RFFLocationBinary::read(const std::filesystem::path &path) {
        if (!std::filesystem::exists(path)) {
            return DEFAULT;
        }
        std::ifstream in(path, std::ios::in | std::ios::binary);

        if (!in.is_open()) {
            return DEFAULT;
        }
        const std::optional<uint32_t> version = RFF2FileVersion::read(in, RFF2FileVersion::FLOAT_UNVERSIONED_FROM);
        if (!version) {
            return DEFAULT;
        }
        float logZoom = 0;
        uint64_t maxIteration = 0;
        if (*version == 0) {
            IOUtilities::readAndDecode(in, &logZoom);
            IOUtilities::readAndDecode(in, &maxIteration);
        } else if (!RFF2FileVersion::readLogZoom(in, logZoom) ||
                   !RFF2FileVersion::readInteger(in, *version, maxIteration)) {
            return DEFAULT;
        }
        uint64_t realLength;
        IOUtilities::readAndDecode(in, &realLength);
        if (!IOUtilities::validateReadCount(in, realLength, sizeof(char), MAX_COORDINATE_BYTES)) {
            return DEFAULT;
        }
        std::string real(static_cast<size_t>(realLength), '\0');
        IOUtilities::readAndDecode(in, realLength, real.data());
        uint64_t imagLength;
        IOUtilities::readAndDecode(in, &imagLength);
        if (!IOUtilities::validateReadCount(in, imagLength, sizeof(char), MAX_COORDINATE_BYTES)) {
            return DEFAULT;
        }
        std::string imag(static_cast<size_t>(imagLength), '\0');
        IOUtilities::readAndDecode(in, imagLength, imag.data());
        if (!in || !std::isfinite(logZoom) || logZoom < 0.0f ||
            logZoom > static_cast<float>(MAX_COORDINATE_BYTES) ||
            !fp_decimal_calculator::isValidString(real) || !fp_decimal_calculator::isValidString(imag)) {
            return DEFAULT;
        }

        return RFFLocationBinary(logZoom, std::move(real), std::move(imag), maxIteration);
    }


    bool RFFLocationBinary::hasData() const {
        return !real.empty() && !imag.empty();
    }


    void RFFLocationBinary::exportAsKeyframe(const std::filesystem::path &dir) const {
        exportFile(IOUtilities::generateFileName(dir, Constants::Extension::LOCATION));
    }


    void RFFLocationBinary::exportFile(const std::filesystem::path &path) const {
        const std::filesystem::path temporary = IOUtilities::temporaryFilePath(path);
        if (std::ofstream out(temporary, std::ios::out | std::ios::binary | std::ios::trunc); out.is_open()) {
            IOUtilities::encodeAndWrite(out, getLogZoom());
            IOUtilities::encodeAndWrite(out, maxIteration);
            const uint64_t realLength = real.length();
            IOUtilities::encodeAndWrite(out, realLength);
            IOUtilities::encodeAndWrite(out, real.data(), realLength);
            const uint64_t imagLength = imag.length();
            IOUtilities::encodeAndWrite(out, imagLength);
            IOUtilities::encodeAndWrite(out, imag.data(), imagLength);
            out.close();
            if (out.fail() || !IOUtilities::commitTemporaryFile(temporary, path)) {
                IOUtilities::discardTemporaryFile(temporary);
                vkh::logger::w_log(L"ERROR : Cannot save file");
            }
        } else {
            vkh::logger::w_log(L"ERROR : Cannot save file");
        }
    }


    const std::string &RFFLocationBinary::getReal() const {
        return real;
    }

    const std::string &RFFLocationBinary::getImag() const {
        return imag;
    }

    uint64_t RFFLocationBinary::getMaxIteration() const {
        return maxIteration;
    }
}
