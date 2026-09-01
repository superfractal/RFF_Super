//
// Created by Merutilm on 2025-05-08.
// Modified by Opus 5 on 2026-08-14, 2026-08-23
// Modified by GPT-5 on 2026-08-18
//

#pragma once
#include <filesystem>

#include "RFFBinary.h"
#include "RFFMapCompression.h"
#include "../data/Matrix.h"
#include "opencv2/core/mat.hpp"

namespace merutilm::rff2 {
    class RFFDynamicMapBinary final : public RFFBinary{
        uint64_t period;
        uint64_t maxIteration;
        Matrix<double> iterations;

    public:
        static const RFFDynamicMapBinary DEFAULT;

        // Leading bytes of a compressed map, and the layout its reader expects. See the header
        // written by exportCompressedFile, and docs/compressed-map-format.md.
        static constexpr char COMPRESSED_MAGIC[4] = {'R', 'F', 'M', 'Z'};
        static constexpr uint16_t COMPRESSED_VERSION = 1;

        RFFDynamicMapBinary(float logZoom, uint64_t period, uint64_t maxIteration, Matrix<double> iterations);

        [[nodiscard]] bool hasData() const override;

        [[nodiscard]] static RFFDynamicMapBinary read(const std::filesystem::path &path);

        // The keyframe of that number, in whichever form the folder holds it.
        [[nodiscard]] static RFFDynamicMapBinary readByID(const std::filesystem::path &dir, uint32_t id);

        // Keyframes present in the folder, counted over both forms: a run started before compressed
        // maps existed, or interrupted and resumed, leaves a folder holding some of each.
        [[nodiscard]] static uint32_t keyframeCount(const std::filesystem::path &dir);

        [[nodiscard]] static bool readSizeByID(const std::filesystem::path &dir, uint32_t id,
                                               uint16_t &width, uint16_t &height);

        // The zoom a keyframe was rendered at, read from its header alone: the whole map is far too much to pay for it.
        [[nodiscard]] static bool readLogZoomByID(const std::filesystem::path &dir, uint32_t id, float &logZoom);

        // True when the file opens with the compressed map's magic, whatever it is named.
        [[nodiscard]] static bool isCompressedFile(const std::filesystem::path &path);

        [[nodiscard]] static RFFDynamicMapBinary readCompressed(const std::filesystem::path &path);

        // Reads either form, chosen by what the file holds rather than by its extension.
        [[nodiscard]] static RFFDynamicMapBinary readAny(const std::filesystem::path &path);

        // Writes the compressed form: a keyframe folder is where the size of a map is felt most.
        void exportAsKeyframe(const std::filesystem::path &dir) const override;

        // The same, in whichever form the caller asks for. Both are read back by readByID, so a
        // folder stays usable either way, and the number a keyframe takes does not depend on this.
        void exportAsKeyframe(const std::filesystem::path &dir, bool compressed) const;

        void exportFile(const std::filesystem::path &path) const override;

        // The same map, entropy-coded and lossless. False when the file could not be written or the
        // table could not be compressed; nothing usable is left behind in that case.
        bool exportCompressedFile(const std::filesystem::path &path) const;

        [[nodiscard]] uint64_t getPeriod() const;

        [[nodiscard]] uint64_t getMaxIteration() const;

        [[nodiscard]] const Matrix<double> &getMatrix() const;
    };

}
