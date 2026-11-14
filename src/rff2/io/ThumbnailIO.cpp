//
// Created by Opus 5.5 on 2026-11-07
// Modified by Opus 5.5 on 2026-11-10, 2026-11-11
//

#include "ThumbnailIO.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <cwctype>
#include <format>
#include <fstream>
#include <vector>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "GalleryIO.hpp"
#include "../constants/Constants.hpp"
#include "../ui/IOUtilities.h"
#include "../ui/Utilities.h"

namespace merutilm::rff2 {
    namespace {
        cv::Mat toBgr8(const cv::Mat &picture) {
            cv::Mat eight;
            if (picture.depth() == CV_16U) {
                picture.convertTo(eight, CV_8U, 1.0 / 257.0);
            } else if (picture.depth() == CV_32F) {
                picture.convertTo(eight, CV_8U, 255.0);
            } else {
                eight = picture;
            }
            cv::Mat bgr;
            if (eight.channels() == 4) {
                cv::cvtColor(eight, bgr, cv::COLOR_BGRA2BGR);
            } else if (eight.channels() == 1) {
                cv::cvtColor(eight, bgr, cv::COLOR_GRAY2BGR);
            } else {
                bgr = eight;
            }
            return bgr;
        }

        cv::Mat shrink(const cv::Mat &picture, const uint32_t width) {
            if (picture.empty() || picture.cols <= static_cast<int>(width)) {
                return picture.clone();
            }
            const int height = std::max(1, static_cast<int>(std::lround(
                static_cast<double>(picture.rows) * width / picture.cols)));
            cv::Mat result;
            cv::resize(picture, result, cv::Size(static_cast<int>(width), height), 0, 0, cv::INTER_AREA);
            return result;
        }

        std::wstring lower(std::wstring text) {
            std::ranges::transform(text, text.begin(), [](const wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
            return text;
        }
    }

    std::filesystem::path ThumbnailIO::pathFor(const std::filesystem::path &file) {
        std::filesystem::path result = file;
        result.replace_extension(std::format(L".{}", Constants::Extension::THUMBNAIL));
        return result;
    }

    bool ThumbnailIO::write(const std::filesystem::path &target, const cv::Mat &picture, const uint32_t width) {
        if (picture.empty() || target.empty()) {
            return false;
        }
        try {
            const cv::Mat small = shrink(toBgr8(picture), width);
            std::vector<uchar> encoded;
            if (!cv::imencode(".png", small, encoded)) {
                return false;
            }
            const std::filesystem::path temporary = IOUtilities::temporaryFilePath(target);
            std::ofstream out(temporary, std::ios::out | std::ios::binary | std::ios::trunc);
            if (!out.is_open()) {
                return false;
            }
            out.write(reinterpret_cast<const char *>(encoded.data()), static_cast<std::streamsize>(encoded.size()));
            out.close();
            if (out.fail() || !IOUtilities::commitTemporaryFile(temporary, target)) {
                IOUtilities::discardTemporaryFile(temporary);
                return false;
            }
            return true;
        } catch (const cv::Exception &) {
            return false;
        }
    }

    cv::Mat ThumbnailIO::read(const std::filesystem::path &thumbnail) {
        std::error_code error;
        if (!std::filesystem::is_regular_file(thumbnail, error)) {
            return {};
        }
        try {
            const cv::Mat picture = IOUtilities::readImage(thumbnail, cv::IMREAD_COLOR);
            return picture.empty() ? cv::Mat() : toBgr8(picture);
        } catch (const cv::Exception &) {
            return {};
        }
    }

    cv::Mat ThumbnailIO::fromImage(const std::filesystem::path &image, const uint32_t width) {
        try {
            const cv::Mat picture = IOUtilities::readImage(image, cv::IMREAD_COLOR);
            return picture.empty() ? cv::Mat() : shrink(toBgr8(picture), width);
        } catch (const cv::Exception &) {
            return {};
        }
    }

    cv::Mat ThumbnailIO::lookup(const std::filesystem::path &file, const uint32_t width, const bool slow) {
        if (cv::Mat own = read(pathFor(file)); !own.empty()) {
            return own;
        }
        // Only a picture file is its own thumbnail. Anything else without a .rfth shows none, rather than a
        // picture in colors that are not the work's.
        if (GalleryIO::kindOf(file) != GalleryIO::Kind::IMAGE) {
            return {};
        }
        const std::filesystem::path cache = cachePathFor(file, width);
        if (cv::Mat made = read(cache); !made.empty()) {
            // Marked as used, so trimming the cache lets go of the pictures not looked at longest first.
            std::error_code touched;
            std::filesystem::last_write_time(cache, std::filesystem::file_time_type::clock::now(), touched);
            return made;
        }
        if (!slow) {
            return {};
        }
        cv::Mat made = fromImage(file, width);
        if (!made.empty()) {
            write(cache, made, width);
        }
        return made;
    }

    std::filesystem::path ThumbnailIO::cachePathFor(const std::filesystem::path &source, const uint32_t width) {
        std::error_code error;
        const auto absolute = std::filesystem::absolute(source, error);
        const uint64_t size = std::filesystem::file_size(source, error);
        const auto written = std::filesystem::last_write_time(source, error).time_since_epoch().count();
        std::wstring key = (error ? source : absolute).wstring();
        std::ranges::transform(key, key.begin(), [](const wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
        key += std::format(L"|{}|{}|{}", size, written, width);
        // FNV-1a over the key, enough to tell one file and version from another.
        uint64_t hash = 14695981039346656037ULL;
        for (const wchar_t c : key) {
            hash = (hash ^ static_cast<uint16_t>(c)) * 1099511628211ULL;
        }
        return Utilities::getConfigFile(std::filesystem::path(L"gallery-cache") / std::format(L"{:016x}.png", hash));
    }

    void ThumbnailIO::trimCache() {
        const std::filesystem::path folder = Utilities::getConfigFile(L"gallery-cache");
        std::vector<std::pair<std::filesystem::file_time_type, std::pair<std::filesystem::path, uint64_t>>> files;
        uint64_t total = 0;
        std::error_code error;
        for (std::filesystem::directory_iterator it(folder, error), end; !error && it != end; it.increment(error)) {
            std::error_code itemError;
            if (!it->is_regular_file(itemError) || lower(it->path().extension().wstring()) != L".png") {
                continue;
            }
            const uint64_t size = it->file_size(itemError);
            const auto written = it->last_write_time(itemError);
            if (itemError) {
                continue;
            }
            total += size;
            files.push_back({written, {it->path(), size}});
        }
        if (total <= MAX_CACHE_BYTES) {
            return;
        }
        // Trimmed to three quarters, so the next few pictures do not start it again.
        std::ranges::sort(files, [](const auto &a, const auto &b) { return a.first < b.first; });
        for (const auto &[written, file] : files) {
            if (total <= MAX_CACHE_BYTES / 4 * 3) {
                break;
            }
            std::error_code removed;
            if (std::filesystem::remove(file.first, removed)) {
                total -= file.second;
            }
        }
    }
}
