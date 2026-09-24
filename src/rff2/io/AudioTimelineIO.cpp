//
// Modified by GPT-6 on 2026-09-19, 2026-09-21, 2026-09-23
//

#include "AudioTimelineIO.hpp"
#include <bit>
#include <sstream>
#include <type_traits>

namespace merutilm::rff2 {
    namespace {
        constexpr uint32_t magic = 0x31445541;
        constexpr uint32_t version = 1;
        constexpr uint32_t maximumBytes =
            VidAudioAttribute::maximumClips * (VidAudioAttribute::maximumPathBytes + 80) + 16;
        template <class T> void writeLittleEndian(std::ostream &out, T value) {
            using U = std::make_unsigned_t<T>;
            U bits = static_cast<U>(value);
            for (size_t i = 0; i < sizeof(T); ++i) {
                out.put(char((bits >> (i * 8)) & 255));
            }
        }
        template <class T> bool readLittleEndian(std::istream &in, T &value) {
            using U = std::make_unsigned_t<T>;
            U bits = 0;
            for (size_t i = 0; i < sizeof(T); ++i) {
                const auto byte = in.get();
                if (byte == std::char_traits<char>::eof()) {
                    return false;
                }
                bits |= U(static_cast<unsigned char>(byte)) << (i * 8);
            }
            value = std::bit_cast<T>(bits);
            return true;
        }
        void writeGain(std::ostream &out, float value) {
            writeLittleEndian(out, std::bit_cast<uint32_t>(value));
        }
        bool readGain(std::istream &in, float &value) {
            uint32_t bits = 0;
            if (!readLittleEndian(in, bits)) {
                return false;
            }
            value = std::bit_cast<float>(bits);
            return true;
        }
        bool readFlag(std::istream &in, bool &value) {
            uint32_t bits = 0;
            if (!readLittleEndian(in, bits) || bits > 1) {
                return false;
            }
            value = bits != 0;
            return true;
        }
        bool pathsConvertibleToWide(const VidAudioAttribute &audio) {
            try {
                for (const auto &clip : audio.clips) {
                    (void)std::filesystem::u8path(clip.path).wstring();
                }
            } catch (const std::filesystem::filesystem_error &) {
                return false;
            }
            return true;
        }
        std::string pathToUtf8(const std::filesystem::path &path) {
            const auto bytes = path.generic_u8string();
            return {reinterpret_cast<const char *>(bytes.data()), bytes.size()};
        }
    }

    bool AudioTimelineIO::write(std::ostream &out, const VidAudioAttribute &audio) {
        if (!audio.valid() || !pathsConvertibleToWide(audio)) {
            out.setstate(std::ios::failbit);
            return false;
        }
        std::ostringstream payload(std::ios::out | std::ios::binary);
        writeLittleEndian(payload, uint32_t(audio.exportEnabled));
        writeGain(payload, audio.gain);
        writeLittleEndian(payload, uint32_t(audio.clips.size()));
        for (const auto &clip : audio.clips) {
            writeLittleEndian(payload, clip.id);
            writeLittleEndian(payload, uint32_t(clip.path.size()));
            payload.write(clip.path.data(), std::streamsize(clip.path.size()));
            for (auto time :
                 {clip.sourceDuration, clip.start, clip.in, clip.out, clip.fadeIn, clip.fadeOut}) {
                writeLittleEndian(payload, time);
            }
            writeGain(payload, clip.gain);
            writeLittleEndian(payload, uint32_t(clip.muted));
        }
        const auto bytes = payload.str();
        writeLittleEndian(out, magic);
        writeLittleEndian(out, version);
        writeLittleEndian(out, uint32_t(bytes.size()));
        out.write(bytes.data(), std::streamsize(bytes.size()));
        return bool(out);
    }

    bool AudioTimelineIO::read(std::istream &in, VidAudioAttribute &audio, bool required) {
        const auto fail = [&] {
            in.setstate(std::ios::failbit);
            return false;
        };
        if (in.fail()) {
            return false;
        }
        if (in.rdbuf()->sgetc() == std::char_traits<char>::eof()) {
            if (required) {
                return fail();
            }
            audio = {};
            return true;
        }
        uint32_t marker = 0;
        uint32_t revision = 0;
        uint32_t length = 0;
        if (!readLittleEndian(in, marker) || marker != magic || !readLittleEndian(in, revision) ||
            revision != version || !readLittleEndian(in, length) || length < 12 || length > maximumBytes) {
            return fail();
        }
        std::string bytes(length, '\0');
        if (!in.read(bytes.data(), length)) {
            return fail();
        }
        std::istringstream payload(bytes, std::ios::in | std::ios::binary);
        VidAudioAttribute parsed;
        uint32_t count = 0;
        if (!readFlag(payload, parsed.exportEnabled) || !readGain(payload, parsed.gain) ||
            !readLittleEndian(payload, count) || count > VidAudioAttribute::maximumClips) {
            return fail();
        }
        for (uint32_t i = 0; i < count; ++i) {
            VidAudioClip clip;
            uint32_t pathLength = 0;
            if (!readLittleEndian(payload, clip.id) || !readLittleEndian(payload, pathLength) ||
                !pathLength || pathLength > VidAudioAttribute::maximumPathBytes) {
                return fail();
            }
            clip.path.resize(pathLength);
            if (!payload.read(clip.path.data(), pathLength)) {
                return fail();
            }
            for (auto *time :
                 {&clip.sourceDuration, &clip.start, &clip.in, &clip.out, &clip.fadeIn, &clip.fadeOut}) {
                if (!readLittleEndian(payload, *time)) {
                    return fail();
                }
            }
            if (!readGain(payload, clip.gain) || !readFlag(payload, clip.muted)) {
                return fail();
            }
            parsed.clips.push_back(std::move(clip));
        }
        if (payload.rdbuf()->sgetc() != std::char_traits<char>::eof() || !parsed.valid()) {
            return fail();
        }
        if (!pathsConvertibleToWide(parsed)) {
            return fail();
        }
        audio = std::move(parsed);
        return true;
    }

    void AudioTimelineIO::resolvePaths(VidAudioAttribute &audio, const std::filesystem::path &document) {
        const auto base = std::filesystem::absolute(document).parent_path();
        for (auto &clip : audio.clips) {
            const auto source = std::filesystem::u8path(clip.path);
            clip.path = pathToUtf8((source.is_absolute() ? source : base / source).lexically_normal());
        }
    }

    void AudioTimelineIO::relativePaths(VidAudioAttribute &audio, const std::filesystem::path &document) {
        const auto base = std::filesystem::absolute(document).parent_path();
        for (auto &clip : audio.clips) {
            const auto source =
                std::filesystem::absolute(std::filesystem::u8path(clip.path)).lexically_normal();
            const auto relative = source.lexically_relative(base);
            clip.path = pathToUtf8(relative.empty() ? source : relative);
        }
    }
}
