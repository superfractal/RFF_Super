//
// Created by Merutilm on 2025-06-08.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21, 2026-08-23, 2026-09-01
// Modified by Opus 5 on 2026-08-14
//

#include "IOUtilities.h"

#include <atomic>
#include <filesystem>
#include <opencv2/imgcodecs.hpp>

#include "Utilities.h"
#include "../constants/Constants.hpp"
#include "../data/ApproxTableCache.h"


namespace merutilm::rff2 {
    namespace {
        // How many of the dialogs below are on screen. A count, not a flag: the video callbacks run
        // on a worker and can have a dialog up while the main thread opens one of its own.
        std::atomic<int> openModalDialogs{0};
        std::atomic<uint64_t> temporaryFileCounter{0};

        struct ModalDialogScope {
            ModalDialogScope() { ++openModalDialogs; }
            ~ModalDialogScope() { --openModalDialogs; }
            ModalDialogScope(const ModalDialogScope &) = delete;
            ModalDialogScope &operator=(const ModalDialogScope &) = delete;
        };

        // The app's main window, handed to every dialog as its owner. Windows disables an owner for
        // the dialog's lifetime, so the app can no longer be closed while the user is still picking a
        // file - which used to tear the scene down underneath the callback waiting on the dialog.
        HWND dialogOwner() {
            return FindWindowW(Constants::Win32::CLASS_MASTER_WINDOW, nullptr);
        }

        // The close may have arrived anyway (a disabled window still receives WM_CLOSE from the
        // taskbar). Dropping the result leaves the caller on its "cancelled" path instead of
        // reporting a failure and then working on a half-destroyed scene.
        bool ownerLost(const HWND owner) {
            return owner != nullptr && !IsWindow(owner);
        }
    }

    bool IOUtilities::isModalDialogOpen() {
        return openModalDialogs.load(std::memory_order_relaxed) > 0;
    }
    std::unique_ptr<std::filesystem::path> IOUtilities::ioFileDialog(const std::wstring_view title,
                                                                     const std::wstring_view desc,
                                                                     const char type,
                                                                     const std::wstring_view extension) {
        OPENFILENAMEW fn;
        ZeroMemory(&fn, sizeof(fn));

        auto display = std::format(L"{}(*.{})", desc, extension);
        auto pattern = std::vector<wchar_t>();
        pattern.insert(pattern.end(), display.begin(), display.end());
        pattern.push_back(L'\0');

        const auto filter = std::format(L"*.{}", extension);
        pattern.insert(pattern.end(), filter.begin(), filter.end());
        pattern.push_back(L'\0');
        pattern.push_back(L'\0');
        wchar_t fileNameBuffer[MAX_PATH];
        fileNameBuffer[0] = L'\0';
        const HWND owner = dialogOwner();
        fn.lStructSize = sizeof(OPENFILENAME);
        fn.hwndOwner = owner;
        fn.lpstrFile = fileNameBuffer;
        fn.lpstrFilter = pattern.data();
        fn.nMaxFile = MAX_PATH;
        fn.lpstrFile[0] = '\0';
        fn.lpstrTitle = title.data();
        fn.Flags = OFN_PATHMUSTEXIST;
        const std::wstring end = std::format(L".{}", extension.data());
        const ModalDialogScope modal;

        switch (type) {
            case OPEN_FILE: {
                fn.Flags |= OFN_FILEMUSTEXIST;
                if (GetOpenFileNameW(&fn) && !ownerLost(owner)) {
                    std::wstring result = fn.lpstrFile;
                    if (!Utilities::endsWith(result, end)) {
                        result.append(end);
                    }
                    return std::make_unique<std::filesystem::path>(result);
                }
                break;
            }
            case SAVE_FILE: {
                fn.Flags |= OFN_OVERWRITEPROMPT;
                if (GetSaveFileNameW(&fn) && !ownerLost(owner)) {
                    std::wstring result = fn.lpstrFile;
                    if (!Utilities::endsWith(result, end)) {
                        result.append(end);
                    }
                    return std::make_unique<std::filesystem::path>(result);
                }
                break;
            }
            default: break;
        }
        return nullptr;
    }

    std::unique_ptr<std::filesystem::path> IOUtilities::ioFileDialogMulti(const std::wstring_view title,
            const char type, const std::vector<std::pair<std::wstring, std::wstring>> &filters) {
        OPENFILENAMEW fn;
        ZeroMemory(&fn, sizeof(fn));

        std::vector<wchar_t> pattern;
        auto addEntry = [&pattern](const std::wstring &display, const std::wstring &filterStr) {
            pattern.insert(pattern.end(), display.begin(), display.end());
            pattern.push_back(L'\0');
            pattern.insert(pattern.end(), filterStr.begin(), filterStr.end());
            pattern.push_back(L'\0');
        };

        // 1-based filter index -> extension to append on Save ("" = combined/any).
        std::vector<std::wstring> extByIndex;
        if (type == OPEN_FILE && filters.size() > 1) {
            std::wstring combined;
            for (size_t i = 0; i < filters.size(); ++i) {
                if (i) combined += L";";
                combined += std::format(L"*.{}", filters[i].second);
            }
            addEntry(std::format(L"Supported ({})", combined), combined);
            extByIndex.emplace_back(L"");
        }
        for (const auto &[desc, ext] : filters) {
            addEntry(std::format(L"{} (*.{})", desc, ext), std::format(L"*.{}", ext));
            extByIndex.push_back(ext);
        }
        pattern.push_back(L'\0');

        wchar_t fileNameBuffer[MAX_PATH];
        fileNameBuffer[0] = L'\0';
        const HWND owner = dialogOwner();
        fn.lStructSize = sizeof(OPENFILENAME);
        fn.hwndOwner = owner;
        fn.lpstrFile = fileNameBuffer;
        fn.lpstrFilter = pattern.data();
        fn.nMaxFile = MAX_PATH;
        fn.lpstrTitle = title.data();
        fn.Flags = OFN_PATHMUSTEXIST;
        const ModalDialogScope modal;

        switch (type) {
            case OPEN_FILE: {
                fn.Flags |= OFN_FILEMUSTEXIST;
                if (GetOpenFileNameW(&fn) && !ownerLost(owner)) {
                    return std::make_unique<std::filesystem::path>(std::wstring(fn.lpstrFile));
                }
                break;
            }
            case SAVE_FILE: {
                fn.Flags |= OFN_OVERWRITEPROMPT;
                if (GetSaveFileNameW(&fn) && !ownerLost(owner)) {
                    std::wstring result = fn.lpstrFile;
                    const size_t idx = fn.nFilterIndex >= 1 && fn.nFilterIndex <= extByIndex.size()
                                           ? fn.nFilterIndex - 1 : 0;
                    if (const std::wstring &ext = extByIndex[idx]; !ext.empty()) {
                        if (const std::wstring end = std::format(L".{}", ext); !Utilities::endsWith(result, end)) {
                            result.append(end);
                        }
                    }
                    return std::make_unique<std::filesystem::path>(result);
                }
                break;
            }
            default: break;
        }
        return nullptr;
    }

    std::unique_ptr<std::filesystem::path> IOUtilities::ioDirectoryDialog(const std::wstring_view title) {
        const HWND owner = dialogOwner();
        BROWSEINFOW bi = {};
        bi.hwndOwner = owner;
        bi.lpszTitle = title.data();
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        const ModalDialogScope modal;

        if (const LPITEMIDLIST item = SHBrowseForFolderW(&bi)) {
            wchar_t path[MAX_PATH];
            const BOOL resolved = SHGetPathFromIDListW(item, path);
            CoTaskMemFree(item);
            if (!resolved || ownerLost(owner)) {
                return nullptr;
            }
            return std::make_unique<std::filesystem::path>(path);
        }
        return nullptr;
    }

    std::wstring IOUtilities::fileNameFormat(const unsigned int n, const std::wstring_view extension) {
        return std::format(L"{:04d}.{}", n, extension);
    }

    std::filesystem::path IOUtilities::generateFileName(const std::filesystem::path &dir, const std::wstring_view extension) {
        unsigned int n = 0;
        std::filesystem::path p = dir;
        do {
            ++n;
            p = dir / fileNameFormat(n, extension);
        } while (std::filesystem::exists(p));
        return p;
    }

    uint32_t IOUtilities::fileNameCount(const std::filesystem::path &dir, const std::wstring_view extension) {
        unsigned int n = 0;
        std::filesystem::path p = dir;
        do {
            ++n;
            p = dir / fileNameFormat(n, extension);
        } while (std::filesystem::exists(p));
        return n - 1;
    }

    cv::Mat IOUtilities::readImage(const std::filesystem::path &path, const int flags) {
        std::ifstream in(path, std::ios::in | std::ios::binary | std::ios::ate);
        if (!in.is_open()) {
            return {};
        }
        const std::streamsize size = in.tellg();
        if (size <= 0 || static_cast<uint64_t>(size) > std::numeric_limits<size_t>::max()) {
            return {};
        }
        std::vector<unsigned char> encoded(static_cast<size_t>(size));
        in.seekg(0);
        in.read(reinterpret_cast<char *>(encoded.data()), size);
        if (!in) {
            return {};
        }
        try {
            return cv::imdecode(encoded, flags);
        } catch (const cv::Exception &) {
            return {};
        }
    }

    bool IOUtilities::writeImage(const std::filesystem::path &path, const cv::Mat &image) {
        try {
            std::vector<unsigned char> encoded;
            if (!cv::imencode(path.extension().string(), image, encoded)) {
                return false;
            }
            const std::filesystem::path temporary = temporaryFilePath(path);
            std::ofstream out(temporary, std::ios::out | std::ios::binary | std::ios::trunc);
            if (!out.is_open()) {
                return false;
            }
            out.write(reinterpret_cast<const char *>(encoded.data()),
                      static_cast<std::streamsize>(encoded.size()));
            out.close();
            if (out.fail() || !commitTemporaryFile(temporary, path)) {
                discardTemporaryFile(temporary);
                return false;
            }
            return true;
        } catch (const cv::Exception &) {
            return false;
        }
    }

    std::filesystem::path IOUtilities::temporaryFilePath(const std::filesystem::path &target) {
        std::filesystem::path name = target.stem();
        name += std::format(L".tmp-{}-{}-{}", GetCurrentProcessId(), GetTickCount64(),
                            temporaryFileCounter.fetch_add(1, std::memory_order_relaxed));
        name += target.extension();
        return target.parent_path() / name;
    }

    bool IOUtilities::commitTemporaryFile(const std::filesystem::path &temporary,
                                           const std::filesystem::path &target) {
        return MoveFileExW(temporary.c_str(), target.c_str(),
                           MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
    }

    void IOUtilities::discardTemporaryFile(const std::filesystem::path &temporary) {
        std::error_code error;
        std::filesystem::remove(temporary, error);
    }

}
