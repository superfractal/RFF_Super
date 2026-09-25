// Modified by GPT-6 on 2026-09-25, 2026-09-26
#pragma once
#include "../attr/VidAudioAttribute.h"
#include <filesystem>
#include <format>
#include <stdexcept>

namespace merutilm::rff2 {
    struct AudioExport {
        std::vector<std::filesystem::path> inputs;
        std::string filters;

        static AudioExport prepare(const VidAudioAttribute &audio, int64_t offset = 0) {
            AudioExport result;
            if (!audio.exportEnabled || audio.clips.empty()) return result;
            if (!audio.valid()) throw std::runtime_error("Invalid audio timeline settings");
            const auto seconds = [](int64_t ticks) {
                return std::format("{}.{:06}", ticks / VidAudioAttribute::ticksPerSecond,
                                   ticks % VidAudioAttribute::ticksPerSecond);
            };
            for (const auto &clip : audio.clips) {
                if (clip.muted || clip.start + clip.duration() <= offset) continue;
                auto source = std::filesystem::absolute(std::filesystem::path(std::u8string(clip.path.begin(), clip.path.end())));
                if (!std::filesystem::is_regular_file(source)) {
                    throw std::runtime_error("Audio source is missing: " + clip.path);
                }
                const auto index = result.inputs.size();
                result.inputs.push_back(std::move(source));
                result.filters += std::format(
                    "[{}:a:0]atrim=start={}:end={},asetpts=PTS-STARTPTS,aresample=48000,"
                    "aformat=sample_fmts=fltp:channel_layouts=stereo,volume={}",
                    index + 1, seconds(clip.in), seconds(clip.out),
                    static_cast<double>(audio.gain) * clip.gain);
                if (clip.fadeIn > 0) {
                    result.filters += ",afade=t=in:st=0:d=" + seconds(clip.fadeIn);
                }
                if (clip.fadeOut > 0) {
                    result.filters += ",afade=t=out:st=" + seconds(clip.duration() - clip.fadeOut) +
                                      ":d=" + seconds(clip.fadeOut);
                }
                const auto skipped = std::max<int64_t>(0, offset - clip.start);
                if (skipped) result.filters += ",atrim=start=" + seconds(skipped) + ",asetpts=PTS-STARTPTS";
                const auto delaySamples = (std::max<int64_t>(0, clip.start - offset) * 48000 + 500000) / VidAudioAttribute::ticksPerSecond;
                result.filters += std::format(",adelay={}S:all=1[a{}];\n", delaySamples, index);
            }
            if (!result.inputs.empty()) {
                for (size_t i = 0; i < result.inputs.size(); ++i) {
                    result.filters += std::format("[a{}]", i);
                }
                result.filters += std::format(
                    "amix=inputs={}:duration=longest:dropout_transition=0:normalize=0,apad,asetpts=N/SR/TB[audio]\n",
                    result.inputs.size());
            }
            return result;
        }
    };
}
