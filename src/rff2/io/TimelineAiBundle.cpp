//
// Modified by GPT-6 on 2026-09-23
//

#include "TimelineAiBundle.hpp"
#include "TimelineJsonIO.hpp"
#include "AudioTimelineIO.hpp"
#include "../ui/IOUtilities.h"
#include <format>
#include <fstream>
#include <windows.h>

namespace merutilm::rff2 {
    namespace {
        void writeText(const std::filesystem::path &path, const std::string &text) {
            const auto temporary = IOUtilities::temporaryFilePath(path);
            try {
                std::ofstream out(temporary, std::ios::binary);
                out.write(text.data(), static_cast<std::streamsize>(text.size()));
                out.close();
                if (!out || !IOUtilities::commitTemporaryFile(temporary, path))
                    throw std::runtime_error("Cannot save " + path.filename().string());
            } catch (...) {
                IOUtilities::discardTemporaryFile(temporary);
                throw;
            }
        }
    }

    TimelineAiBundle::Result TimelineAiBundle::save(
        const std::filesystem::path &parent, const VidTimelineAttribute &timeline,
        const std::string &prompt, int side, uint32_t frameCount,
        const std::function<cv::Mat(uint32_t)> &renderPage, const std::function<bool()> &cancelled,
        const std::function<void(uint32_t, uint32_t)> &progress) {
        Result result;
        nlohmann::json manifest;
        try {
            if ((side != 2 && side != 3) || !frameCount) throw std::runtime_error("No keyframes or invalid image grid");
            if (cancelled()) { result.cancelled = true; return result; }
            SYSTEMTIME now{};
            GetLocalTime(&now);
            const auto name = std::format(L"timeline-ai-{:04}{:02}{:02}-{:02}{:02}{:02}",
                now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond);
            for (uint32_t suffix = 0;; ++suffix) {
                const auto candidate = parent / (suffix ? name + L"-" + std::to_wstring(suffix) : name);
                if (std::filesystem::create_directory(candidate)) { result.directory = candidate; break; }
            }
            std::filesystem::create_directory(result.directory / L"backup");
            const uint32_t perPage = uint32_t(side * side), pages = (frameCount - 1) / perPage + 1;
            manifest = {{"format", "RFF_Super.timeline-images"}, {"version", 1}, {"status", "incomplete"},
                {"grid", side}, {"frameCount", frameCount}, {"pageCount", pages}, {"completedPages", 0},
                {"pages", nlohmann::json::array()}};
            writeText(result.directory / L"backup" / L"images.json", manifest.dump(2));
            auto portable = timeline;
            AudioTimelineIO::resolvePaths(portable.audio, std::filesystem::current_path() / L"timeline.json");
            const auto json = TimelineJsonIO::document(portable).dump(2);
            if (json.size() > TimelineJsonIO::maximumBytes) throw std::runtime_error("JSON exceeds 16 MiB");
            writeText(result.directory / L"backup" / L"timeline.json", json);
            writeText(result.directory / L"prompt.txt", prompt + "\nCurrent editable timeline JSON:\n" + json);
            progress(0, pages);
            for (uint32_t page = 0; page < pages; ++page) {
                if (cancelled()) { result.cancelled = true; break; }
                const auto image = renderPage(page);
                if (cancelled()) { result.cancelled = true; break; }
                if (image.empty()) throw std::runtime_error("A keyframe image page could not be rendered");
                const auto file = std::format("page_{:06}.png", page + 1);
                if (!IOUtilities::writeImage(result.directory / file, image))
                    throw std::runtime_error("Cannot save " + file);
                const uint32_t first = frameCount - page * perPage;
                manifest["pages"].push_back({{"file", file}, {"firstKey", first},
                    {"lastKey", first > perPage ? first - perPage + 1 : 1}});
                manifest["completedPages"] = ++result.completedPages;
                writeText(result.directory / L"backup" / L"images.json", manifest.dump(2));
                progress(result.completedPages, pages);
            }
            manifest["status"] = result.cancelled ? "cancelled" : "complete";
            writeText(result.directory / L"backup" / L"images.json", manifest.dump(2));
        } catch (const std::exception &e) {
            result.error = e.what();
            if (!result.directory.empty()) {
                try {
                    manifest["status"] = "failed";
                    manifest["error"] = result.error;
                    writeText(result.directory / L"backup" / L"images.json", manifest.dump(2));
                } catch (const std::exception &) {}
            }
        }
        return result;
    }
}
