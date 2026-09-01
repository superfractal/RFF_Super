//
// Created by Merutilm on 2025-06-08.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21, 2026-08-23, 2026-09-01
// Modified by Opus 5 on 2026-08-14
//

#pragma once
#include <algorithm>
#include <array>
#include <cstring>
#include <utility>
#include <filesystem>
#include <format>
#include <fstream>
#include <limits>
#include <string>
#include <vector>
#include <shlobj.h>
#include <opencv2/core/mat.hpp>

#include "Utilities.h"

namespace merutilm::rff2 {
    struct IOUtilities {
        IOUtilities() = delete;

        static constexpr char OPEN_FILE = 0;
        static constexpr char SAVE_FILE = 1;

        static std::unique_ptr<std::filesystem::path> ioFileDialog(std::wstring_view title, std::wstring_view desc,
                                                                   char type, std::wstring_view extension);

        // Like ioFileDialog but offers multiple file types ({description, extension}). On Open it
        // shows a combined "all supported" filter; on Save the chosen filter decides the extension.
        // The returned path keeps its extension, so callers branch on path.extension().
        static std::unique_ptr<std::filesystem::path> ioFileDialogMulti(std::wstring_view title, char type,
            const std::vector<std::pair<std::wstring, std::wstring>> &filters);

        static std::unique_ptr<std::filesystem::path> ioDirectoryDialog(std::wstring_view title);

        // True while one of the dialogs above is on screen. They own the main window for as long as
        // they are up, so it must not be destroyed under them - the caller that opened the dialog is
        // still on the stack, holding the scene the teardown would take away.
        static bool isModalDialogOpen();

        static std::wstring fileNameFormat(unsigned int n, std::wstring_view extension);

        static std::filesystem::path generateFileName(const std::filesystem::path &dir, std::wstring_view extension);

        static uint32_t fileNameCount(const std::filesystem::path &dir, std::wstring_view extension);

        static cv::Mat readImage(const std::filesystem::path &path, int flags);

        static bool writeImage(const std::filesystem::path &path, const cv::Mat &image);

        // Returns a unique path beside the target, so a completed write can replace it atomically.
        static std::filesystem::path temporaryFilePath(const std::filesystem::path &target);

        // Replaces the target only after its complete temporary file has been closed successfully.
        static bool commitTemporaryFile(const std::filesystem::path &temporary,
                                        const std::filesystem::path &target);

        static void discardTemporaryFile(const std::filesystem::path &temporary);

        static bool validateReadCount(std::ifstream &in, uint64_t count, uint64_t elementSize,
                                      uint64_t maxCount);

        template<typename T> requires std::is_arithmetic_v<T>
        static void encodeAndWrite(std::ofstream &out, const T &t);

        static void encodeAndWrite(std::ofstream &out, const char *t, uint64_t length);

        template<typename T> requires std::is_arithmetic_v<T>
        static void encodeAndWrite(std::ofstream &out, const std::vector<T> &t);

        template<typename T> requires std::is_arithmetic_v<T>
        static void readAndDecode(std::ifstream &in, T *t);

        static void readAndDecode(std::ifstream &in, uint64_t length, char *t);

        template<typename T> requires std::is_arithmetic_v<T>
        static void readAndDecode(std::ifstream &in, std::vector<T> *t);

        template<typename T> requires std::is_arithmetic_v<T>
        static std::array<char, sizeof(T)> toBinaryArray(const T &v);

        template<typename T> requires std::is_arithmetic_v<T>
        static void fromBinaryArray(const std::array<char, sizeof(T)> &arr, T *result);
    };


    template<typename T> requires std::is_arithmetic_v<T>
    void IOUtilities::encodeAndWrite(std::ofstream &out, const T &t) {
        const auto ot = toBinaryArray(t);
        out.write(ot.data(), ot.size());
    }

    inline bool IOUtilities::validateReadCount(std::ifstream &in, const uint64_t count,
                                               const uint64_t elementSize, const uint64_t maxCount) {
        const std::streampos current = in.tellg();
        if (in.fail() || current < 0 || count > maxCount ||
            (elementSize != 0 && count > std::numeric_limits<uint64_t>::max() / elementSize)) {
            in.setstate(std::ios::failbit);
            return false;
        }
        in.seekg(0, std::ios::end);
        const std::streampos end = in.tellg();
        in.seekg(current);
        const uint64_t bytes = count * elementSize;
        if (in.fail() || end < current || bytes > static_cast<uint64_t>(end - current)) {
            in.setstate(std::ios::failbit);
            return false;
        }
        return true;
    }

    inline void IOUtilities::encodeAndWrite(std::ofstream &out, const char *t, const uint64_t length) {
        out.write(t, length);
    }

    template<typename T> requires std::is_arithmetic_v<T>
    void IOUtilities::encodeAndWrite(std::ofstream &out, const std::vector<T> &t) {
        std::vector<char> ot;
        for (double et: t) {
            const auto oi = toBinaryArray(et);
            ot.insert(ot.end(), oi.begin(), oi.end());
        }
        out.write(ot.data(), ot.size());
    }


    template<typename T> requires std::is_arithmetic_v<T>
    void IOUtilities::readAndDecode(std::ifstream &in, T *t) {
        auto it = std::array<char, sizeof(T)>();
        in.read(it.data(), it.size());
        fromBinaryArray(it, t);
    }

    inline void IOUtilities::readAndDecode(std::ifstream &in, const uint64_t length, char *t) {
        in.read(t, length);
    }

    template<typename T> requires std::is_arithmetic_v<T>
    void IOUtilities::readAndDecode(std::ifstream &in, std::vector<T> *t) {
        auto it = std::vector<char>(t->size() * sizeof(T));
        in.read(it.data(), it.size());

        for (uint32_t i = 0; i < it.size(); i += sizeof(T)) {
            auto iSubArr = std::array<char, sizeof(T)>();
            std::memcpy(&iSubArr, &it[i], sizeof(T));
            fromBinaryArray(iSubArr, &(*t)[i / sizeof(T)]);
        }
    }

    template<typename T> requires std::is_arithmetic_v<T>
    std::array<char, sizeof(T)> IOUtilities::toBinaryArray(const T &v) {
        std::array<char, sizeof(T)> arr;
        std::memcpy(arr.data(), &v, sizeof(T));
        return arr;
    }

    template<typename T> requires std::is_arithmetic_v<T>
    void IOUtilities::fromBinaryArray(const std::array<char, sizeof(T)> &arr, T *result) {
        std::memcpy(result, arr.data(), sizeof(T));
    }
};
