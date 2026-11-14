//
// Created by Opus 5.5 on 2026-11-07
// Modified by Opus 5.5 on 2026-11-09, 2026-11-10, 2026-11-13
//

#include "GalleryIO.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cwctype>
#include <format>
#include <fstream>
#include <nlohmann/json.hpp>

#include "ConfigIO.h"
#include "MapLimits.hpp"
#include "RFFDynamicMapBinary.h"
#include "RFFLocationBinary.h"
#include "../constants/Constants.hpp"
#include "../ui/IOUtilities.h"
#include "../ui/Utilities.h"

namespace merutilm::rff2 {
    namespace {
        using Json = nlohmann::json;

        std::filesystem::path stateFile() {
            return Utilities::getConfigFile(L"gallery.json");
        }

        std::string toUtf8(const std::filesystem::path &path) {
            const std::u8string text = path.u8string();
            return {text.begin(), text.end()};
        }

        std::filesystem::path fromUtf8(const std::string &text) {
            return std::filesystem::path(std::u8string(text.begin(), text.end()));
        }

        std::wstring lower(std::wstring text) {
            std::ranges::transform(text, text.begin(), [](const wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
            return text;
        }

        bool samePath(const std::filesystem::path &a, const std::filesystem::path &b) {
            return lower(a.lexically_normal().wstring()) == lower(b.lexically_normal().wstring());
        }

        GalleryIO::State loadState() {
            GalleryIO::State state;
            std::ifstream in(stateFile(), std::ios::binary);
            if (!in) {
                return state;
            }
            const Json root = Json::parse(in, nullptr, false, true);
            if (root.is_discarded() || !root.is_object()) {
                return state;
            }
            if (const auto folder = root.find("folder"); folder != root.end() && folder->is_string()) {
                state.folder = fromUtf8(folder->get<std::string>());
            }
            if (const auto window = root.find("window"); window != root.end() && window->is_array() &&
                window->size() == 4 && std::ranges::all_of(*window, [](const Json &v) { return v.is_number_integer(); })) {
                state.window = {(*window)[0].get<LONG>(), (*window)[1].get<LONG>(), (*window)[2].get<LONG>(),
                                (*window)[3].get<LONG>()};
                state.hasWindow = state.window.right > state.window.left && state.window.bottom > state.window.top;
            }
            if (const auto maximized = root.find("maximized"); maximized != root.end() && maximized->is_boolean()) {
                state.maximized = maximized->get<bool>();
            }
            if (const auto filter = root.find("filter"); filter != root.end() && filter->is_number_unsigned()) {
                state.filter = static_cast<uint8_t>(std::min<uint32_t>(filter->get<uint32_t>(), 255));
            }
            if (const auto sort = root.find("sort"); sort != root.end() && sort->is_number_unsigned()) {
                state.sort = static_cast<uint8_t>(std::min<uint32_t>(sort->get<uint32_t>(), 255));
            }
            if (const auto split = root.find("folderSplit"); split != root.end() && split->is_number()) {
                const double value = split->get<double>();
                if (std::isfinite(value)) {
                    state.folderSplit = static_cast<float>(std::clamp(value, 0.0, 1.0));
                }
            }
            if (const auto close = root.find("closeAfterOpen"); close != root.end() && close->is_boolean()) {
                state.closeAfterOpen = close->get<bool>();
            }
            if (const auto shown = root.find("showExtensions"); shown != root.end() && shown->is_boolean()) {
                state.showExtensions = shown->get<bool>();
            }
            if (const auto recent = root.find("recent"); recent != root.end() && recent->is_array()) {
                for (const auto &entry : *recent) {
                    if (entry.is_string() && state.recent.size() < GalleryIO::MAX_RECENT) {
                        state.recent.push_back(fromUtf8(entry.get<std::string>()));
                    }
                }
            }
            return state;
        }

        // A FILETIME as the local date and time, to the minute.
        std::wstring localTime(const FILETIME &time) {
            SYSTEMTIME utc{};
            SYSTEMTIME local{};
            if (!FileTimeToSystemTime(&time, &utc) || !SystemTimeToTzSpecificLocalTime(nullptr, &utc, &local)) {
                return {};
            }
            return std::format(L"{:04}-{:02}-{:02} {:02}:{:02}", local.wYear, local.wMonth, local.wDay, local.wHour,
                               local.wMinute);
        }

        std::wstring byteText(const uint64_t bytes) {
            if (bytes < 1024) {
                return std::format(L"{} B", bytes);
            }
            const double kib = static_cast<double>(bytes) / 1024.0;
            if (kib < 1024.0) {
                return std::format(L"{:.1f} KB", kib);
            }
            const double mib = kib / 1024.0;
            return mib < 1024.0 ? std::format(L"{:.1f} MB", mib) : std::format(L"{:.2f} GB", mib / 1024.0);
        }

        std::wstring grouped(const uint64_t value) {
            std::wstring digits = std::to_wstring(value);
            for (auto i = static_cast<ptrdiff_t>(digits.size()) - 3; i > 0; i -= 3) {
                digits.insert(static_cast<size_t>(i), 1, L',');
            }
            return digits;
        }

        // Width, height, zoom and iteration limit from either map form's header.
        bool readMapHeader(const std::filesystem::path &path, uint16_t &width, uint16_t &height, float &logZoom,
                           uint64_t &maxIteration) {
            std::ifstream in(path, std::ios::in | std::ios::binary);
            if (!in.is_open()) {
                return false;
            }
            if (RFFDynamicMapBinary::isCompressedFile(path)) {
                char magic[sizeof(RFFDynamicMapBinary::COMPRESSED_MAGIC)] = {};
                uint16_t version = 0;
                uint8_t mode = 0;
                IOUtilities::readAndDecode(in, sizeof(magic), magic);
                IOUtilities::readAndDecode(in, &version);
                IOUtilities::readAndDecode(in, &mode);
                if (!in || version != RFFDynamicMapBinary::COMPRESSED_VERSION) {
                    return false;
                }
            }
            uint64_t period = 0;
            IOUtilities::readAndDecode(in, &width);
            IOUtilities::readAndDecode(in, &height);
            IOUtilities::readAndDecode(in, &logZoom);
            IOUtilities::readAndDecode(in, &period);
            IOUtilities::readAndDecode(in, &maxIteration);
            return static_cast<bool>(in) && std::isfinite(logZoom) && MapLimits::valid(width, height);
        }

        // A PNG's size from its header chunk, without decoding the picture.
        bool readPngSize(const std::filesystem::path &path, uint32_t &width, uint32_t &height) {
            std::ifstream in(path, std::ios::in | std::ios::binary);
            unsigned char header[24] = {};
            in.read(reinterpret_cast<char *>(header), sizeof(header));
            static constexpr unsigned char SIGNATURE[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
            if (!in || std::memcmp(header, SIGNATURE, sizeof(SIGNATURE)) != 0 || std::memcmp(header + 12, "IHDR", 4) != 0) {
                return false;
            }
            const auto big = [&header](const int at) {
                return uint32_t(header[at]) << 24 | uint32_t(header[at + 1]) << 16 | uint32_t(header[at + 2]) << 8 |
                       uint32_t(header[at + 3]);
            };
            width = big(16);
            height = big(20);
            return width > 0 && height > 0;
        }

        std::wstring kindName(const GalleryIO::Kind kind) {
            switch (kind) {
            case GalleryIO::Kind::SETTINGS: return L"Settings";
            case GalleryIO::Kind::LOCATION: return L"Location";
            case GalleryIO::Kind::MAP: return L"Map";
            case GalleryIO::Kind::IMAGE: return L"Image";
            case GalleryIO::Kind::APPEARANCE: return L"Appearance";
            default: return {};
            }
        }
    }

    GalleryIO::State &GalleryIO::state() {
        static State live = loadState();
        return live;
    }

    void GalleryIO::save() {
        const State &live = state();
        Json root = Json::object();
        root["folder"] = toUtf8(live.folder);
        if (live.hasWindow) {
            root["window"] = Json::array({live.window.left, live.window.top, live.window.right, live.window.bottom});
        }
        root["maximized"] = live.maximized;
        root["filter"] = live.filter;
        root["sort"] = live.sort;
        root["folderSplit"] = live.folderSplit;
        root["closeAfterOpen"] = live.closeAfterOpen;
        root["showExtensions"] = live.showExtensions;
        Json recent = Json::array();
        for (const auto &file : live.recent) {
            recent.push_back(toUtf8(file));
        }
        root["recent"] = std::move(recent);
        const std::filesystem::path target = stateFile();
        const std::filesystem::path temporary = IOUtilities::temporaryFilePath(target);
        {
            std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
            if (!out) {
                return;
            }
            out << root.dump(2, ' ', false, Json::error_handler_t::replace) << '\n';
            out.close();
            if (out.fail()) {
                IOUtilities::discardTemporaryFile(temporary);
                return;
            }
        }
        if (!IOUtilities::commitTemporaryFile(temporary, target)) {
            IOUtilities::discardTemporaryFile(temporary);
        }
    }

    void GalleryIO::addRecent(const std::filesystem::path &file) {
        if (file.empty() || !kindOf(file)) {
            return;
        }
        std::error_code error;
        const std::filesystem::path absolute = std::filesystem::absolute(file, error);
        const std::filesystem::path entry = error ? file : absolute;
        auto &recent = state().recent;
        std::erase_if(recent, [&entry](const std::filesystem::path &known) { return samePath(known, entry); });
        recent.insert(recent.begin(), entry);
        if (recent.size() > MAX_RECENT) {
            recent.resize(MAX_RECENT);
        }
        save();
    }

    std::optional<GalleryIO::Kind> GalleryIO::kindOf(const std::filesystem::path &path) {
        const std::wstring ext = lower(path.extension().wstring());
        const auto is = [&ext](const wchar_t *kind) { return ext == std::format(L".{}", kind); };
        using namespace Constants::Extension;
        if (is(CONFIG)) return Kind::SETTINGS;
        if (is(LOCATION)) return Kind::LOCATION;
        if (is(COMPRESSED_MAP) || is(DYNAMIC_MAP)) return Kind::MAP;
        if (is(IMAGE)) return Kind::IMAGE;
        if (is(SHADER_PRESET)) return Kind::APPEARANCE;
        return std::nullopt;
    }

    std::wstring GalleryIO::zoomText(const double logZoom) {
        if (!std::isfinite(logZoom)) {
            return {};
        }
        double exponent = std::floor(logZoom);
        double mantissa = std::pow(10.0, logZoom - exponent);
        // Rounding the mantissa to two places can carry it to 10.
        if (mantissa >= 9.995) {
            mantissa = 1.0;
            exponent += 1.0;
        }
        return std::format(L"{:.2f}e{}", mantissa, static_cast<int64_t>(exponent));
    }

    std::vector<std::pair<std::wstring, std::wstring>> GalleryIO::describe(const std::filesystem::path &path) {
        std::vector<std::pair<std::wstring, std::wstring>> lines;
        const auto kind = kindOf(path);
        if (!kind) {
            return lines;
        }
        lines.emplace_back(L"Kind", kindName(*kind));
        WIN32_FILE_ATTRIBUTE_DATA data{};
        if (GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &data)) {
            lines.emplace_back(L"Saved", localTime(data.ftLastWriteTime));
            lines.emplace_back(L"File Size",
                               byteText(uint64_t(data.nFileSizeHigh) << 32 | uint64_t(data.nFileSizeLow)));
        }
        try {
            switch (*kind) {
            case Kind::SETTINGS: {
                Attribute settings{.fractal = FractalAttribute{.center = fp_complex("0", "0", 0)}};
                uint16_t width = 0;
                uint16_t height = 0;
                if (!ConfigIO::loadDetached(path, settings, &width, &height)) {
                    lines.emplace_back(L"Note", L"This file cannot be read by this version.");
                    break;
                }
                if (width > 0 && height > 0) {
                    lines.emplace_back(L"Picture Size", std::format(L"{} x {}", width, height));
                }
                lines.emplace_back(L"Zoom", zoomText(settings.fractal.logZoom));
                lines.emplace_back(L"Iteration Limit", grouped(settings.fractal.maxIteration));
                break;
            }
            case Kind::LOCATION: {
                const RFFLocationBinary location = RFFLocationBinary::read(path);
                if (location.hasData()) {
                    lines.emplace_back(L"Zoom", zoomText(location.getLogZoom()));
                    lines.emplace_back(L"Iteration Limit", grouped(location.getMaxIteration()));
                }
                break;
            }
            case Kind::MAP: {
                uint16_t width = 0;
                uint16_t height = 0;
                float logZoom = 0;
                uint64_t maxIteration = 0;
                if (readMapHeader(path, width, height, logZoom, maxIteration)) {
                    lines.emplace_back(L"Map Size", std::format(L"{} x {}", width, height));
                    lines.emplace_back(L"Zoom", zoomText(logZoom));
                    lines.emplace_back(L"Iteration Limit", grouped(maxIteration));
                }
                break;
            }
            case Kind::IMAGE: {
                uint32_t width = 0;
                uint32_t height = 0;
                if (readPngSize(path, width, height)) {
                    lines.emplace_back(L"Picture Size", std::format(L"{} x {}", width, height));
                }
                break;
            }
            default:
                break;
            }
        } catch (const std::exception &) {
            lines.emplace_back(L"Note", L"This file cannot be read by this version.");
        }
        return lines;
    }
}
