//
// Created by Merutilm on 2025-06-08.
// Modified by AI; earlier exact modification date unavailable.
// Modified by Opus 5 on 2026-08-14
// Modified by GPT-5 on 2026-08-21, 2026-08-23, 2026-09-01
// Modified by GPT-6 on 2026-09-14, 2026-09-21, 2026-09-22, 2026-09-23
// Modified by Opus 5.5 on 2026-11-07, 2026-11-10
//

#include "IOUtilities.h"
#include "NativeDialogs.hpp"

#include <atomic>
#include <filesystem>
#include <opencv2/imgcodecs.hpp>
#include <shobjidl.h>

#include "Utilities.h"
#include "../constants/Constants.hpp"
#include "../data/ApproxTableCache.h"


namespace merutilm::rff2 {
    namespace {
        // How many of the dialogs below are on screen. A count, not a flag: the video callbacks run
        // on a worker and can have a dialog up while the main thread opens one of its own.
        using ModalDialogScope = NativeDialogs::Session;
        std::atomic<uint64_t> temporaryFileCounter{0};

        // The app's main window, handed to every dialog as its owner. Windows disables an owner for
        // the dialog's lifetime, so the app can no longer be closed while the user is still picking a
        // file - which used to tear the scene down underneath the callback waiting on the dialog.
        HWND dialogOwner() {
            return NativeDialogs::mainWindow();
        }

        // The close may have arrived anyway (a disabled window still receives WM_CLOSE from the
        // taskbar). Dropping the result leaves the caller on its "cancelled" path instead of
        // reporting a failure and then working on a half-destroyed scene.
        bool ownerLost(const HWND owner) {
            return owner != nullptr && !IsWindow(owner);
        }

        BOOL showFileDialog(OPENFILENAMEW &dialog, const char type) {
            switch (type) {
            case IOUtilities::OPEN_FILE:
                dialog.Flags |= OFN_FILEMUSTEXIST;
                return GetOpenFileNameW(&dialog);
            case IOUtilities::SAVE_FILE:
                dialog.Flags |= OFN_OVERWRITEPROMPT;
                return GetSaveFileNameW(&dialog);
            default:
                return FALSE;
            }
        }

        bool endsWithExtension(const std::wstring_view name, const std::wstring_view extension) {
            return name.size() >= extension.size() &&
                   CompareStringOrdinal(name.data() + name.size() - extension.size(),
                                        static_cast<int>(extension.size()), extension.data(),
                                        static_cast<int>(extension.size()), TRUE) == CSTR_EQUAL;
        }

        // Releases a COM interface when it leaves scope.
        template<typename T>
        struct Released {
            T *pointer = nullptr;
            ~Released() {
                if (pointer != nullptr) {
                    pointer->Release();
                }
            }
            Released() = default;
            Released(const Released &) = delete;
            Released &operator=(const Released &) = delete;
        };

        constexpr DWORD NOTE_CONTROL = 100;
        constexpr DWORD CHECK_CONTROL = 101;
        // Keeps a save dialog's extension on the chosen type, and reads the checkbox as OK is pressed.
        class DialogEvents final : public IFileDialogEvents {
            LONG references = 1;
            const std::vector<std::wstring> *extensions;
            // The checkbox read as OK is pressed, which is when the dialog guarantees its state.
            bool *checked;

        public:
            DialogEvents(const std::vector<std::wstring> *typeExtensions, bool *checkState)
                : extensions(typeExtensions), checked(checkState) {}
            virtual ~DialogEvents() = default;

            IFACEMETHODIMP QueryInterface(REFIID riid, void **object) override {
                if (object == nullptr) {
                    return E_POINTER;
                }
                if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IFileDialogEvents)) {
                    *object = static_cast<IFileDialogEvents *>(this);
                    AddRef();
                    return S_OK;
                }
                *object = nullptr;
                return E_NOINTERFACE;
            }
            IFACEMETHODIMP_(ULONG) AddRef() override {
                return static_cast<ULONG>(InterlockedIncrement(&references));
            }
            IFACEMETHODIMP_(ULONG) Release() override {
                const LONG left = InterlockedDecrement(&references);
                if (left == 0) {
                    delete this;
                }
                return static_cast<ULONG>(left);
            }
            IFACEMETHODIMP OnFileOk(IFileDialog *dialog) override {
                Released<IFileDialogCustomize> customize;
                if (checked != nullptr && SUCCEEDED(dialog->QueryInterface(IID_IFileDialogCustomize,
                                                                          reinterpret_cast<void **>(&customize.pointer)))) {
                    BOOL state = FALSE;
                    if (SUCCEEDED(customize.pointer->GetCheckButtonState(CHECK_CONTROL, &state))) {
                        *checked = state != FALSE;
                    }
                }
                return S_OK;
            }
            IFACEMETHODIMP OnFolderChanging(IFileDialog *, IShellItem *) override { return S_OK; }
            IFACEMETHODIMP OnFolderChange(IFileDialog *) override { return S_OK; }
            IFACEMETHODIMP OnSelectionChange(IFileDialog *) override { return S_OK; }
            IFACEMETHODIMP OnShareViolation(IFileDialog *, IShellItem *, FDE_SHAREVIOLATION_RESPONSE *) override {
                return S_OK;
            }
            IFACEMETHODIMP OnTypeChange(IFileDialog *dialog) override {
                UINT index = 0;
                if (extensions != nullptr && SUCCEEDED(dialog->GetFileTypeIndex(&index)) && index >= 1 &&
                    index <= extensions->size() && !(*extensions)[index - 1].empty()) {
                    dialog->SetDefaultExtension((*extensions)[index - 1].c_str());
                }
                return S_OK;
            }
            IFACEMETHODIMP OnOverwrite(IFileDialog *, IShellItem *, FDE_OVERWRITE_RESPONSE *) override { return S_OK; }
        };

        bool confirmCompletedSavePath(const HWND owner, const std::wstring &path, const char type) {
            if (type != IOUtilities::SAVE_FILE) {
                return true;
            }
            std::error_code error;
            if (!std::filesystem::exists(path, error)) {
                return !error;
            }
            const auto message = std::format(L"{} already exists. Replace it?", path);
            return NativeDialogs::message(owner, message.c_str(), L"Confirm replacement",
                                          MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDYES &&
                   !ownerLost(owner);
        }
    }

    bool IOUtilities::isModalDialogOpen() {
        return NativeDialogs::isOpen();
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
        fn.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
        const std::wstring end = std::format(L".{}", extension.data());
        const ModalDialogScope modal;

        const BOOL accepted = showFileDialog(fn, type);
        if (!accepted || ownerLost(owner)) {
            return nullptr;
        }
        std::wstring result = fn.lpstrFile;
        if (!endsWithExtension(result, end)) {
            result.append(end);
            if (!confirmCompletedSavePath(owner, result, type)) {
                return nullptr;
            }
        }
        return std::make_unique<std::filesystem::path>(result);
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
        fn.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
        const ModalDialogScope modal;

        if (!showFileDialog(fn, type) || ownerLost(owner)) {
            return nullptr;
        }
        if (type == OPEN_FILE) {
            return std::make_unique<std::filesystem::path>(std::wstring(fn.lpstrFile));
        }
        std::wstring result = fn.lpstrFile;
        const size_t idx = fn.nFilterIndex >= 1 && fn.nFilterIndex <= extByIndex.size()
                               ? fn.nFilterIndex - 1 : 0;
        if (const std::wstring &ext = extByIndex[idx]; !ext.empty()) {
            if (const std::wstring end = std::format(L".{}", ext); !endsWithExtension(result, end)) {
                result.append(end);
                if (!confirmCompletedSavePath(owner, result, type)) {
                    return nullptr;
                }
            }
        }
        return std::make_unique<std::filesystem::path>(result);
    }

    std::unique_ptr<std::filesystem::path> IOUtilities::fileDialog(FileDialogRequest &request) {
        // Torn down last, after every interface below has been released.
        struct ComScope {
            bool active;
            ~ComScope() {
                if (active) {
                    CoUninitialize();
                }
            }
        } com{SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))};
        const bool save = request.type == SAVE_FILE;
        Released<IFileDialog> dialog;
        if (FAILED(CoCreateInstance(save ? CLSID_FileSaveDialog : CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_IFileDialog, reinterpret_cast<void **>(&dialog.pointer)))) {
            return ioFileDialogMulti(request.title, request.type, request.filters);
        }

        // The same filter list ioFileDialogMulti offers, a combined entry first when opening.
        std::vector<std::wstring> names;
        std::vector<std::wstring> patterns;
        std::vector<std::wstring> extByIndex;
        if (!save && request.filters.size() > 1) {
            std::wstring combined;
            for (size_t i = 0; i < request.filters.size(); ++i) {
                if (i) combined += L";";
                combined += std::format(L"*.{}", request.filters[i].second);
            }
            names.push_back(std::format(L"{} ({})", UiLanguage::text(L"Supported"), combined));
            patterns.push_back(combined);
            extByIndex.emplace_back();
        }
        for (const auto &[desc, ext] : request.filters) {
            names.push_back(std::format(L"{} (*.{})", UiLanguage::text(desc), ext));
            patterns.push_back(std::format(L"*.{}", ext));
            extByIndex.push_back(ext);
        }
        std::vector<COMDLG_FILTERSPEC> types;
        for (size_t i = 0; i < names.size(); ++i) {
            types.push_back({names[i].c_str(), patterns[i].c_str()});
        }
        if (!types.empty()) {
            dialog.pointer->SetFileTypes(static_cast<UINT>(types.size()), types.data());
            dialog.pointer->SetFileTypeIndex(1);
        }
        const std::wstring title = UiLanguage::text(request.title);
        dialog.pointer->SetTitle(title.c_str());
        FILEOPENDIALOGOPTIONS options = 0;
        dialog.pointer->GetOptions(&options);
        options |= FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR;
        options |= save ? FOS_OVERWRITEPROMPT : FOS_FILEMUSTEXIST;
        dialog.pointer->SetOptions(options);
        if (save && !extByIndex.empty() && !extByIndex.front().empty()) {
            dialog.pointer->SetDefaultExtension(extByIndex.front().c_str());
        }

        Released<IFileDialogCustomize> customize;
        const bool customized = (!request.note.empty() || !request.checkLabel.empty()) &&
                                SUCCEEDED(dialog.pointer->QueryInterface(IID_IFileDialogCustomize,
                                                                         reinterpret_cast<void **>(&customize.pointer)));
        if (customized) {
            if (!request.note.empty()) {
                customize.pointer->AddText(NOTE_CONTROL, UiLanguage::text(request.note).c_str());
            }
            if (!request.checkLabel.empty()) {
                customize.pointer->AddCheckButton(CHECK_CONTROL, UiLanguage::text(request.checkLabel).c_str(),
                                                  request.checked ? TRUE : FALSE);
            }
        }

        DialogEvents *events = nullptr;
        DWORD cookie = 0;
        const bool hasCheck = customized && !request.checkLabel.empty();
        bool checkedAtOk = request.checked;
        if ((save && extByIndex.size() > 1) || hasCheck) {
            events = new DialogEvents(save ? &extByIndex : nullptr, hasCheck ? &checkedAtOk : nullptr);
            if (FAILED(dialog.pointer->Advise(events, &cookie))) {
                cookie = 0;
            }
        }

        const HWND owner = dialogOwner();
        HRESULT shown;
        {
            const ModalDialogScope modal;
            shown = dialog.pointer->Show(owner);
        }
        if (events != nullptr) {
            if (cookie != 0) {
                dialog.pointer->Unadvise(cookie);
            }
            events->Release();
        }
        if (hasCheck) {
            request.checked = checkedAtOk;
        }
        if (FAILED(shown) || ownerLost(owner)) {
            return nullptr;
        }
        Released<IShellItem> item;
        PWSTR name = nullptr;
        if (FAILED(dialog.pointer->GetResult(&item.pointer)) ||
            FAILED(item.pointer->GetDisplayName(SIGDN_FILESYSPATH, &name)) || name == nullptr) {
            return nullptr;
        }
        std::wstring result = name;
        CoTaskMemFree(name);
        if (save) {
            UINT index = 1;
            dialog.pointer->GetFileTypeIndex(&index);
            const size_t slot = index >= 1 && index <= extByIndex.size() ? index - 1 : 0;
            if (slot < extByIndex.size() && !extByIndex[slot].empty()) {
                if (const std::wstring end = std::format(L".{}", extByIndex[slot]); !endsWithExtension(result, end)) {
                    result.append(end);
                    if (!confirmCompletedSavePath(owner, result, request.type)) {
                        return nullptr;
                    }
                }
            }
        }
        return std::make_unique<std::filesystem::path>(result);
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
        const auto nextIndex = fileNameCount(dir, extension) + 1;
        return dir / fileNameFormat(nextIndex, extension);
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
        std::filesystem::path name = std::format(L".rff-{:x}-{:x}-{:x}", GetCurrentProcessId(),
            GetTickCount64(), temporaryFileCounter.fetch_add(1, std::memory_order_relaxed));
        name += target.extension();
        std::filesystem::path directory = target.parent_path();
        std::filesystem::path temporary = directory / name;
        while (temporary.native().size() >= MAX_PATH && directory.has_parent_path() &&
               directory != directory.parent_path()) {
            directory = directory.parent_path();
            temporary = directory / name;
        }
        return temporary;
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
