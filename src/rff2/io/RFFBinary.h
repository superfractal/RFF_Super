//
// Created by Merutilm on 2025-06-23.
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include <filesystem>

namespace merutilm::rff2 {
    class RFFBinary {
    public:
        explicit RFFBinary(float logZoom);

        virtual ~RFFBinary() = default;

        virtual bool hasData() const = 0;

        virtual void exportAsKeyframe(const std::filesystem::path &dir) const = 0;

        virtual void exportFile(const std::filesystem::path &path) const = 0;

        float getLogZoom() const;

    private:
        float logZoom;
    };
}
