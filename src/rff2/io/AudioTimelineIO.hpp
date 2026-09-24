//
// Modified by GPT-6 on 2026-09-19, 2026-09-23
//

#pragma once

#include "../attr/VidAudioAttribute.h"

#include <filesystem>
#include <istream>
#include <ostream>

namespace merutilm::rff2 {
    struct AudioTimelineIO {
        static bool write(std::ostream &out, const VidAudioAttribute &audio);
        static bool read(std::istream &in, VidAudioAttribute &audio, bool required = false);

        static void resolvePaths(VidAudioAttribute &audio, const std::filesystem::path &document);
        static void relativePaths(VidAudioAttribute &audio, const std::filesystem::path &document);
    };
}
