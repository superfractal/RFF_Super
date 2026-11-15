//
// Created by Opus 5.5 on 2026-11-07
// Modified by Opus 5.5 on 2026-11-09, 2026-11-10, 2026-11-11, 2026-11-12, 2026-11-13, 2026-11-14, 2026-11-15
//

#include "GalleryWindow.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <condition_variable>
#include <cwctype>
#include <deque>
#include <filesystem>
#include <format>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <oleacc.h>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "CallbackFile.hpp"
#include "IOUtilities.h"
#include "NativeDialogs.hpp"
#include "RenderScene.hpp"
#include "SettingsMenu.hpp"
#include "SettingsTheme.hpp"
#include "ThumbnailBitmap.hpp"
#include "UiDpi.hpp"
#include "UiLanguage.hpp"
#include "Utilities.h"
#include "../constants/Constants.hpp"
#include "../io/GalleryIO.hpp"
#include "../io/PreferencesIO.h"
#include "../io/ThumbnailIO.hpp"

namespace merutilm::rff2 {
    namespace {
        using Kind = GalleryIO::Kind;
        constexpr size_t KIND_COUNT = static_cast<size_t>(Kind::COUNT);
        constexpr auto WINDOW_CLASS = L"RFF2Gallery";
        constexpr auto GRID_CLASS = L"RFF2GalleryGrid";
        constexpr auto FOLDERS_CLASS = L"RFF2GalleryFolders";
        constexpr UINT WM_THUMBNAIL_READY = WM_APP + 0x31;
        // Ends the naming of a new folder once its box has finished with the key or click that ended it.
        constexpr UINT WM_RENAME_DONE = WM_APP + 0x32;
        constexpr int SEARCH_ID = 10;
        // Width the worker makes thumbnails at; the cards and the details panel scale it to fit.
        constexpr uint32_t THUMBNAIL_WIDTH = 256;
        // Bitmaps kept at most before those scrolled out of sight longest are let go.
        constexpr size_t MAX_BITMAPS = 400;
        // Fluent Emoji U+1F4C1 file folder (Flat), (c) Microsoft Corporation, MIT License, rasterized to PNG; see NOTICE.
        constexpr unsigned char FOLDER_PNG[] = {
#include "GalleryFolderPng.inc"
        };
        // Folder pictures kept at most, one per size and well color, before the oldest is let go.
        constexpr size_t MAX_FOLDER_BITMAPS = 4;

        enum class Filter : uint8_t { ALL, SETTINGS, MAPS, IMAGES, APPEARANCE, RECENT, COUNT };
        constexpr std::array<const wchar_t *, static_cast<size_t>(Filter::COUNT)> FILTER_LABELS = {
            L"All", L"Settings", L"Maps", L"Images", L"Appearance", L"Recent Files"};
        constexpr std::array<const wchar_t *, KIND_COUNT> KIND_LABELS = {
            L"Settings", L"Location", L"Map", L"Image", L"Appearance"};
        constexpr std::array<const wchar_t *, KIND_COUNT> OPEN_LABELS = {
            L"Open Settings", L"Open Location", L"Open Map", L"Open Image", L"Open Appearance"};
        // Which kind a double-click opens: the one that brings back the most, in the order the cards list the kinds.
        constexpr std::array<Kind, KIND_COUNT> OPEN_ORDER = {Kind::SETTINGS, Kind::LOCATION, Kind::MAP, Kind::IMAGE,
                                                             Kind::APPEARANCE};

        enum ButtonId : int {
            BUTTON_SORT = 2, BUTTON_SETTINGS = 3, BUTTON_FILTER = 10, BUTTON_OPEN = 20,
            BUTTON_SAVE_WORK = 40, BUTTON_OTHER_SAVES = 42, BUTTON_OPEN_FOLDER = 43, BUTTON_CRUMB = 100
        };
        constexpr int NAME_ID = 11;
        // Waits after a save for the image and thumbnail written once the view has finished calculating, then reads the folder again.
        constexpr UINT_PTR REFRESH_TIMER = 7;

        // The saves the Other Saves... button offers, each captioned with what it is for.
        struct OtherSave {
            CallbackFile::SaveKind kind;
            const wchar_t *label;
        };
        constexpr std::array<OtherSave, 6> OTHER_SAVES = {{
            {CallbackFile::SaveKind::SETTINGS, L"Settings only - to continue working later (.rfc)"},
            {CallbackFile::SaveKind::LOCATION, L"Location only - the place, zoom and iteration limit (.rfl)"},
            {CallbackFile::SaveKind::MAP, L"Map only - change colors later, make videos (.rfmz)"},
            {CallbackFile::SaveKind::PLAIN_MAP, L"Map only, uncompressed (.rfm)"},
            {CallbackFile::SaveKind::IMAGE, L"Image only - the picture as you see it (.png)"},
            {CallbackFile::SaveKind::APPEARANCE, L"Appearance only - use the look at another place (.rfsp)"},
        }};

        std::wstring lower(std::wstring text) {
            std::ranges::transform(text, text.begin(), [](const wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
            return text;
        }

        int64_t lastWrite(const std::filesystem::path &file) {
            WIN32_FILE_ATTRIBUTE_DATA data{};
            if (!GetFileAttributesExW(file.c_str(), GetFileExInfoStandard, &data)) {
                return 0;
            }
            return static_cast<int64_t>(uint64_t(data.ftLastWriteTime.dwHighDateTime) << 32 |
                                        data.ftLastWriteTime.dwLowDateTime);
        }

        std::wstring shortDate(const int64_t written) {
            if (written == 0) {
                return {};
            }
            FILETIME time{static_cast<DWORD>(written & 0xFFFFFFFF), static_cast<DWORD>(uint64_t(written) >> 32)};
            SYSTEMTIME utc{};
            SYSTEMTIME local{};
            if (!FileTimeToSystemTime(&time, &utc) || !SystemTimeToTzSpecificLocalTime(nullptr, &utc, &local)) {
                return {};
            }
            return std::format(L"{:04}-{:02}-{:02} {:02}:{:02}", local.wYear, local.wMonth, local.wDay, local.wHour,
                               local.wMinute);
        }

        // A folder without the trailing separator a dialog or a typed path can leave, a drive root kept as it is.
        std::filesystem::path trimmedFolder(const std::filesystem::path &path) {
            if (!path.has_filename() && path.has_relative_path()) {
                return path.parent_path();
            }
            return path;
        }

        // Compares folders the way Windows does, ignoring case and a trailing separator.
        std::wstring folderKey(const std::filesystem::path &path) {
            return lower(trimmedFolder(path.lexically_normal()).wstring());
        }

        [[nodiscard]] bool isDriveRoot(const std::filesystem::path &path) {
            return !trimmedFolder(path).has_relative_path();
        }

        // The last part of a folder's path, or the drive itself for a drive root.
        std::wstring folderName(const std::filesystem::path &path) {
            const std::filesystem::path folder = trimmedFolder(path);
            if (isDriveRoot(folder)) {
                return folder.root_name().empty() ? folder.wstring() : folder.root_name().wstring();
            }
            return folder.filename().wstring();
        }

        // One item of a folder as a single directory read gives it, so no file needs asking about again.
        struct Listed {
            std::wstring name;
            DWORD attributes = 0;
            int64_t written = 0;
        };

        std::vector<Listed> listFolder(const std::filesystem::path &folder) {
            std::vector<Listed> items;
            if (folder.empty()) {
                return items;
            }
            WIN32_FIND_DATAW data{};
            const HANDLE find = FindFirstFileExW((folder / L"*").c_str(), FindExInfoBasic, &data, FindExSearchNameMatch,
                                                 nullptr, FIND_FIRST_EX_LARGE_FETCH);
            if (find == INVALID_HANDLE_VALUE) {
                return items;
            }
            do {
                const std::wstring_view name = data.cFileName;
                if (name == L"." || name == L"..") {
                    continue;
                }
                items.push_back({std::wstring(name), data.dwFileAttributes,
                                 static_cast<int64_t>(uint64_t(data.ftLastWriteTime.dwHighDateTime) << 32 |
                                                      data.ftLastWriteTime.dwLowDateTime)});
            } while (FindNextFileW(find, &data));
            FindClose(find);
            return items;
        }

        // Renames a file or folder, waiting up to a second while the thumbnail reader still has it open.
        [[nodiscard]] bool moveWaiting(const std::filesystem::path &from, const std::filesystem::path &to) {
            for (int attempt = 0;; ++attempt) {
                if (MoveFileW(from.c_str(), to.c_str())) {
                    return true;
                }
                if (GetLastError() != ERROR_SHARING_VIOLATION || attempt >= 20) {
                    return false;
                }
                Sleep(50);
            }
        }

        [[nodiscard]] bool visibleFolder(const Listed &item) {
            return (item.attributes & FILE_ATTRIBUTE_DIRECTORY) != 0 &&
                   (item.attributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) == 0;
        }

        // The visible folders directly inside one, ordered by name with numbers counted as numbers.
        std::vector<std::filesystem::path> subfolders(const std::filesystem::path &parent) {
            std::vector<std::filesystem::path> found;
            for (const Listed &item : listFolder(parent)) {
                if (visibleFolder(item)) {
                    found.push_back(parent / item.name);
                }
            }
            std::ranges::sort(found, [](const std::filesystem::path &a, const std::filesystem::path &b) {
                const std::wstring x = a.filename().wstring();
                const std::wstring y = b.filename().wstring();
                return CompareStringEx(LOCALE_NAME_USER_DEFAULT, NORM_IGNORECASE | SORT_DIGITSASNUMBERS, x.c_str(),
                                       static_cast<int>(x.size()), y.c_str(), static_cast<int>(y.size()), nullptr, nullptr,
                                       0) == CSTR_LESS_THAN;
            });
            return found;
        }

        // The drives a folder can be on: fixed, removable, network and RAM disks, leaving out disc drives.
        std::vector<std::filesystem::path> drives() {
            std::vector<std::filesystem::path> found;
            const DWORD mask = GetLogicalDrives();
            for (int i = 0; i < 26; ++i) {
                if ((mask & (1u << i)) == 0) {
                    continue;
                }
                const std::wstring root{static_cast<wchar_t>(L'A' + i), L':', L'\\'};
                const UINT type = GetDriveTypeW(root.c_str());
                if (type == DRIVE_FIXED || type == DRIVE_REMOVABLE || type == DRIVE_REMOTE || type == DRIVE_RAMDISK) {
                    found.emplace_back(root);
                }
            }
            return found;
        }

        // The Gallery's frames and details panel follow the unslop design method by Damir Sokolovsky (CC BY 4.0); see NOTICE.
        // GDI+ started once for the smooth frames below; drawing is skipped if it cannot start.
        [[nodiscard]] bool smoothDrawing() {
            struct Runtime {
                ULONG_PTR token = 0;
                Runtime() {
                    Gdiplus::GdiplusStartupInput input;
                    if (Gdiplus::GdiplusStartup(&token, &input, nullptr) != Gdiplus::Ok) {
                        token = 0;
                    }
                }
                ~Runtime() {
                    if (token != 0) {
                        Gdiplus::GdiplusShutdown(token);
                    }
                }
            };
            static Runtime runtime;
            return runtime.token != 0;
        }

        Gdiplus::Color plusColor(const COLORREF color) {
            return {255, GetRValue(color), GetGValue(color), GetBValue(color)};
        }

        // A rounded rectangle from left/top to right/bottom (exclusive), its corners cut at the radius.
        void addRounded(Gdiplus::GraphicsPath &path, const float left, const float top, const float right, const float bottom,
                        float radius) {
            radius = std::min({radius, (right - left) / 2, (bottom - top) / 2});
            if (radius <= 0) {
                path.AddRectangle(Gdiplus::RectF(left, top, right - left, bottom - top));
                return;
            }
            const float d = radius * 2;
            path.AddArc(left, top, d, d, 180, 90);
            path.AddArc(right - d, top, d, d, 270, 90);
            path.AddArc(right - d, bottom - d, d, d, 0, 90);
            path.AddArc(left, bottom - d, d, d, 90, 90);
            path.CloseFigure();
        }

        // A smooth rounded frame filling rect, its line of the given width drawn inside rect so the frame never grows.
        void roundFrame(const HDC dc, const RECT &rect, const float radius, const std::optional<COLORREF> fill,
                        const std::optional<COLORREF> line, const float width = 1.0f) {
            if (!smoothDrawing() || rect.right <= rect.left || rect.bottom <= rect.top) {
                return;
            }
            Gdiplus::Graphics graphics(dc);
            graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
            const auto l = static_cast<float>(rect.left);
            const auto t = static_cast<float>(rect.top);
            const auto r = static_cast<float>(rect.right);
            const auto b = static_cast<float>(rect.bottom);
            if (fill) {
                Gdiplus::GraphicsPath path;
                addRounded(path, l, t, r, b, radius);
                Gdiplus::SolidBrush brush(plusColor(*fill));
                graphics.FillPath(&brush, &path);
            }
            if (line) {
                const float half = width / 2;
                Gdiplus::GraphicsPath path;
                addRounded(path, l + half, t + half, r - half, b - half, radius - half);
                Gdiplus::Pen pen(plusColor(*line), width);
                graphics.DrawPath(&pen, &path);
            }
        }

        // Paints back over what lies outside the rounded corners of rect, rounding a picture drawn square inside it.
        void roundCorners(const HDC dc, const RECT &rect, const float radius, const COLORREF back, const bool bottomToo = true) {
            if (!smoothDrawing() || rect.right <= rect.left || rect.bottom <= rect.top) {
                return;
            }
            Gdiplus::Graphics graphics(dc);
            graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
            const auto l = static_cast<float>(rect.left);
            const auto t = static_cast<float>(rect.top);
            const auto r = static_cast<float>(rect.right);
            const auto b = static_cast<float>(rect.bottom);
            // A rounded shape taller than rect leaves the bottom corners square when only the top ones are wanted.
            const float shapeBottom = bottomToo ? b : b + radius * 2;
            Gdiplus::GraphicsPath path(Gdiplus::FillModeAlternate);
            path.AddRectangle(Gdiplus::RectF(l, t, r - l, b - t));
            addRounded(path, l, t, r, shapeBottom, radius);
            graphics.SetClip(Gdiplus::RectF(l, t, r - l, b - t));
            Gdiplus::SolidBrush brush(plusColor(back));
            graphics.FillPath(&brush, &path);
        }

        // A popup menu drawn in the window's dark colors by the main menu's painter in dark mode, and left to Windows otherwise.
        class ThemedPopup {
            // MSAAMENUINFO comes first so screen readers still read the caption of an owner-drawn row.
            struct Row {
                MSAAMENUINFO accessibility{};
                SettingsMenu::MenuEntry paint{};
                std::wstring caption;
                bool separator = false;
            };
            SettingsMenu::MenuMetrics metrics;
            std::deque<Row> rows;
            HBRUSH back = nullptr;
            HMENU root = nullptr;
            bool dark = false;

            void dress(const HMENU menu) const {
                if (dark) {
                    MENUINFO info{sizeof(info)};
                    info.fMask = MIM_BACKGROUND;
                    info.hbrBack = back;
                    SetMenuInfo(menu, &info);
                }
            }

            void append(const HMENU menu, const UINT flags, const UINT_PTR id, const std::wstring &caption, const bool separator) {
                if (!dark) {
                    AppendMenuW(menu, flags, id, separator ? nullptr : caption.c_str());
                    return;
                }
                // Escaped so an & in a name is drawn rather than taken for a shortcut underline.
                std::wstring escaped;
                for (const wchar_t c : caption) {
                    escaped += c;
                    if (c == L'&') escaped += c;
                }
                const int length = WideCharToMultiByte(CP_UTF8, 0, escaped.data(), static_cast<int>(escaped.size()), nullptr, 0, nullptr, nullptr);
                std::string text(static_cast<size_t>(std::max(0, length)), '\0');
                WideCharToMultiByte(CP_UTF8, 0, escaped.data(), static_cast<int>(escaped.size()), text.data(), length, nullptr, nullptr);
                Row &row = rows.emplace_back();
                row.caption = caption;
                row.separator = separator;
                row.paint = {menu, static_cast<UINT>(GetMenuItemCount(menu)), std::move(text), (flags & MF_POPUP) != 0, false};
                row.paint.metrics = &metrics;
                row.accessibility = {MSAA_MENU_SIG, static_cast<DWORD>(row.caption.size()), row.caption.data()};
                AppendMenuW(menu, flags | MF_OWNERDRAW, id, reinterpret_cast<LPCWSTR>(&row));
            }

        public:
            ThemedPopup(const HWND owner, const UINT dpi) {
                dark = darkSettingsMode() && !highContrastSettingsMode();
                metrics.owner = owner;
                dark = dark && metrics.refresh(dpi);
                if (dark) {
                    back = CreateSolidBrush(settingsTheme().background);
                }
                root = menu();
            }

            ThemedPopup(const ThemedPopup &) = delete;
            ThemedPopup &operator=(const ThemedPopup &) = delete;

            ~ThemedPopup() {
                // Destroying the top menu takes the submenus hanging off it along.
                DestroyMenu(root);
                if (back != nullptr) {
                    DeleteObject(back);
                }
            }

            [[nodiscard]] HMENU top() const {
                return root;
            }

            // A new submenu, to be hung off a row with sub().
            [[nodiscard]] HMENU menu() const {
                const HMENU made = CreatePopupMenu();
                dress(made);
                return made;
            }

            void item(const HMENU menu, const UINT id, const std::wstring &caption, const bool enabled = true, const bool checked = false) {
                append(menu, MF_STRING | (enabled ? 0 : MF_GRAYED) | (checked ? MF_CHECKED : 0), id, caption, false);
            }

            void sub(const HMENU menu, const HMENU child, const std::wstring &caption) {
                append(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(child), caption, false);
            }

            void separator(const HMENU menu) {
                append(menu, MF_SEPARATOR, 0, {}, true);
            }

            [[nodiscard]] UINT track(const UINT flags, const POINT at, const HWND owner) const {
                return static_cast<UINT>(TrackPopupMenu(root, flags | TPM_RETURNCMD | TPM_NONOTIFY, at.x, at.y, 0, owner, nullptr));
            }

            // The owner window's WM_MEASUREITEM and WM_DRAWITEM for these rows; false for anything else.
            static bool measure(const HWND owner, MEASUREITEMSTRUCT *item) {
                if (item == nullptr || item->CtlType != ODT_MENU || item->itemData == 0) {
                    return false;
                }
                const auto *row = reinterpret_cast<const Row *>(item->itemData);
                if (row->accessibility.dwMSAASignature != MSAA_MENU_SIG) {
                    return false;
                }
                if (row->separator) {
                    item->itemWidth = 0;
                    item->itemHeight = UiDpi::pixels(9, row->paint.metrics->dpi);
                    return true;
                }
                MEASUREITEMSTRUCT request = *item;
                request.itemData = reinterpret_cast<ULONG_PTR>(&row->paint);
                if (!SettingsMenu::measureMenuItem(owner, &request)) {
                    return false;
                }
                item->itemWidth = request.itemWidth;
                item->itemHeight = request.itemHeight;
                return true;
            }

            static bool draw(const DRAWITEMSTRUCT *item) {
                if (item == nullptr || item->CtlType != ODT_MENU || item->itemData == 0) {
                    return false;
                }
                const auto *row = reinterpret_cast<const Row *>(item->itemData);
                if (row->accessibility.dwMSAASignature != MSAA_MENU_SIG) {
                    return false;
                }
                if (row->separator) {
                    const HBRUSH fill = CreateSolidBrush(settingsTheme().background);
                    FillRect(item->hDC, &item->rcItem, fill);
                    DeleteObject(fill);
                    RECT line = item->rcItem;
                    line.left += row->paint.metrics->gutter();
                    line.right -= row->paint.metrics->rightPadding();
                    line.top = (line.top + line.bottom) / 2;
                    line.bottom = line.top + 1;
                    const HBRUSH rule = CreateSolidBrush(settingsTheme().sectionFrame);
                    FillRect(item->hDC, &line, rule);
                    DeleteObject(rule);
                    return true;
                }
                DRAWITEMSTRUCT request = *item;
                request.itemData = reinterpret_cast<ULONG_PTR>(&row->paint);
                return SettingsMenu::drawMenuItem(&request);
            }
        };


        struct Entry {
            std::wstring name;
            std::filesystem::path folder;
            std::array<std::filesystem::path, KIND_COUNT> files{};
            std::array<int64_t, KIND_COUNT> written{};
            uint8_t kinds = 0;
            int64_t newest = 0;
            // A folder inside the one shown, opened by a double-click instead of a file; its path is the source.
            bool isFolder = false;
            // What the thumbnail is looked up from: the .rfth beside it first, then the picture or map itself.
            std::filesystem::path source;
            // When that .rfth was written, so a thumbnail saved while the window is open replaces the old one.
            int64_t pictureWritten = 0;
            // The extension of every file of the work with its kind, both forms of a map included.
            std::vector<std::pair<Kind, std::wstring>> extensions;

            [[nodiscard]] bool has(const Kind kind) const {
                return (kinds & (1u << static_cast<unsigned>(kind))) != 0;
            }

            [[nodiscard]] std::wstring key() const {
                return lower(source.wstring()) + std::format(L"|{}|{}", newest, pictureWritten);
            }
        };

        enum class ThumbState : uint8_t { UNKNOWN, LOADING, READY, NONE };

        struct Thumb {
            ThumbState state = ThumbState::UNKNOWN;
            HBITMAP bitmap = nullptr;
            uint64_t seen = 0;
        };

        struct FolderBitmap {
            int side = 0;
            COLORREF back = 0;
            HBITMAP bitmap = nullptr;
        };

        // Makes thumbnails off the window's thread, newest request first, so what is on screen comes before what was scrolled past.
        class ThumbnailWorker {
            std::mutex mutex;
            std::condition_variable wake;
            std::vector<std::pair<std::wstring, std::filesystem::path>> queue;
            std::vector<std::pair<std::wstring, cv::Mat>> done;
            bool stopping = false;
            HWND notify = nullptr;
            std::thread thread;

            void run() {
                // Trimmed here, off the window's thread, before any picture is asked for.
                try {
                    ThumbnailIO::trimCache();
                } catch (...) {
                }
                for (;;) {
                    std::pair<std::wstring, std::filesystem::path> job;
                    {
                        std::unique_lock lock(mutex);
                        wake.wait(lock, [this] { return stopping || !queue.empty(); });
                        if (stopping) {
                            return;
                        }
                        job = std::move(queue.back());
                        queue.pop_back();
                    }
                    cv::Mat picture;
                    try {
                        picture = ThumbnailIO::lookup(job.second, THUMBNAIL_WIDTH, true);
                    } catch (...) {
                        picture = cv::Mat();
                    }
                    {
                        std::scoped_lock lock(mutex);
                        if (stopping) {
                            return;
                        }
                        done.emplace_back(std::move(job.first), std::move(picture));
                    }
                    PostMessageW(notify, WM_THUMBNAIL_READY, 0, 0);
                }
            }

        public:
            ~ThumbnailWorker() {
                stop();
            }

            void start(const HWND window) {
                notify = window;
                thread = std::thread([this] { run(); });
            }

            void stop() {
                {
                    std::scoped_lock lock(mutex);
                    stopping = true;
                    queue.clear();
                }
                wake.notify_all();
                if (thread.joinable()) {
                    thread.join();
                }
            }

            void enqueue(std::wstring key, std::filesystem::path source) {
                {
                    std::scoped_lock lock(mutex);
                    queue.emplace_back(std::move(key), std::move(source));
                }
                wake.notify_one();
            }

            // The keys still waiting, which go back to unknown so they are asked for again when seen.
            std::vector<std::wstring> clearQueue() {
                std::scoped_lock lock(mutex);
                std::vector<std::wstring> keys;
                for (auto &job : queue) {
                    keys.push_back(std::move(job.first));
                }
                queue.clear();
                return keys;
            }

            std::vector<std::pair<std::wstring, cv::Mat>> takeDone() {
                std::scoped_lock lock(mutex);
                return std::exchange(done, {});
            }
        };

        struct Button {
            RECT rect{};
            int id = 0;
            std::wstring label;
            bool primary = false;
            bool selected = false;
        };

        // One row of a folder list on the left.
        struct FolderItem {
            std::filesystem::path path;
            std::wstring name;
            std::wstring parent;
        };

        // One of the two scrolled folder lists on the left: the folders beside the one shown, and the recent ones.
        struct FolderPane {
            HWND hwnd = nullptr;
            std::vector<FolderItem> items;
            int scroll = 0;
            int hot = -1;
            // One line per row, the name only; otherwise the name with its parent folder below it.
            bool compact = false;
            RECT title{};
        };

        class Gallery {
        public:
            SettingsMenu &menu;
            RenderScene &scene;
            HWND window = nullptr;
            HWND grid = nullptr;
            HWND search = nullptr;
            UINT dpi = 96;
            HFONT font = nullptr;
            HFONT smallFont = nullptr;
            HFONT titleFont = nullptr;
            HBRUSH fieldBrush = nullptr;
            Filter filter = Filter::ALL;
            uint8_t sort = 0;
            std::filesystem::path folder;
            std::vector<Entry> entries;
            std::vector<size_t> shown;
            int selected = -1;
            int scroll = 0;
            uint64_t paintCount = 0;
            std::unordered_map<std::wstring, Thumb> thumbs;
            // The decoded folder picture (BGRA), and the sizes of it already drawn over the card well.
            cv::Mat folderPicture;
            std::vector<FolderBitmap> folderBitmaps;
            ThumbnailWorker worker;
            std::vector<Button> buttons;
            int hotButton = -1;
            int pressedButton = -1;
            std::vector<std::pair<std::wstring, std::wstring>> details;
            // The kinds of file the chosen work has, shown under its name without a label so it never runs out of room.
            std::wstring detailKinds;
            RECT gridRect{};
            RECT detailsRect{};
            RECT previewRect{};
            RECT searchFrame{};
            RECT filterFrame{};
            RECT folderText{};
            HWND nameEdit = nullptr;
            // The box over a card's name while it is being renamed, the card's source, whether it is a folder, and its name before.
            HWND renameEdit = nullptr;
            std::filesystem::path renaming;
            bool renamingFolder = false;
            std::wstring renamingName;
            // The folders shown before and after this one, walked by the mouse's back and forward buttons.
            std::vector<std::filesystem::path> backFolders;
            std::vector<std::filesystem::path> forwardFolders;
            bool walkingHistory = false;
            // Gallery (Save) when set: the save bar along the bottom and no Open buttons. Gallery (Open) otherwise.
            bool saving = false;
            RECT nameLabel{};
            RECT nameFrame{};
            // The Recent Folders pane on the left: its frame, its title, and the scrolled list inside it.
            FolderPane nearby;
            FolderPane recentPane;
            // The recent files the Recent Folders list was last made from.
            std::vector<std::filesystem::path> recentListedFrom;
            // The part of the window that had the keyboard when another window was chosen, given it back on return.
            HWND lastFocus = nullptr;
            std::wstring nearbyTitle;
            std::wstring nearbyFor;
            RECT paneRect{};
            int paneMiddle = 0;
            // Dragging the Recent Folders title moves the line between the two lists; where the upper list starts and their room.
            bool draggingSplit = false;
            int splitGrab = 0;
            int splitTop = 0;
            int splitSpace = 0;
            // The folder's path along the top as one button per part, each going to that folder.
            std::vector<std::filesystem::path> crumbPaths;
            std::vector<RECT> crumbSeparators;
            RECT crumbEllipsis{};

            Gallery(SettingsMenu &settingsMenu, RenderScene &renderScene) : menu(settingsMenu), scene(renderScene) {}

            Gallery(const Gallery &) = delete;
            Gallery &operator=(const Gallery &) = delete;

            ~Gallery() {
                worker.stop();
                for (auto &[key, thumb] : thumbs) {
                    if (thumb.bitmap != nullptr) {
                        DeleteObject(thumb.bitmap);
                    }
                }
                for (const auto &kept : folderBitmaps) {
                    if (kept.bitmap != nullptr) {
                        DeleteObject(kept.bitmap);
                    }
                }
                for (const HGDIOBJ object : {static_cast<HGDIOBJ>(font), static_cast<HGDIOBJ>(smallFont),
                                             static_cast<HGDIOBJ>(titleFont), static_cast<HGDIOBJ>(fieldBrush)}) {
                    if (object != nullptr) {
                        DeleteObject(object);
                    }
                }
            }

            [[nodiscard]] int px(const int value) const {
                return UiDpi::pixels(value, dpi);
            }

            // Card geometry inside the grid, which stretches the cards to fill the row.
            struct CardLayout {
                int columns = 1;
                int width = 0;
                int thumbHeight = 0;
                int height = 0;
                int gap = 0;
            };

            [[nodiscard]] CardLayout cardLayout() const {
                RECT client{};
                GetClientRect(grid, &client);
                CardLayout layout;
                layout.gap = px(12);
                const int minimum = px(176);
                const int available = std::max<int>(minimum, client.right - layout.gap);
                layout.columns = std::max(1, available / (minimum + layout.gap));
                layout.width = std::max(minimum, (static_cast<int>(client.right) - layout.gap) / layout.columns - layout.gap);
                layout.thumbHeight = layout.width * 10 / 16;
                layout.height = layout.thumbHeight + px(64);
                return layout;
            }

            // Where a card's name is written, under its picture; the box naming a new folder covers the same place.
            [[nodiscard]] RECT nameRect(const RECT &card, const CardLayout &layout) const {
                const int below = card.top + layout.thumbHeight;
                return {card.left + px(10), below + px(12), card.right - px(10), below + px(32)};
            }

            [[nodiscard]] RECT cardRect(const int index, const CardLayout &layout) const {
                const int column = index % layout.columns;
                const int row = index / layout.columns;
                const int left = layout.gap + column * (layout.width + layout.gap);
                const int top = layout.gap + row * (layout.height + layout.gap) - scroll;
                return {left, top, left + layout.width, top + layout.height};
            }

            [[nodiscard]] int contentHeight(const CardLayout &layout) const {
                const int rows = (static_cast<int>(shown.size()) + layout.columns - 1) / layout.columns;
                return layout.gap + rows * (layout.height + layout.gap);
            }

            void createFonts() {
                for (const HFONT old : {font, smallFont, titleFont}) {
                    if (old != nullptr) {
                        DeleteObject(old);
                    }
                }
                const auto make = [this](const int size, const int weight) {
                    return CreateFontW(-px(size), 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                       CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, UiLanguage::fontFace());
                };
                font = make(14, FW_NORMAL);
                smallFont = make(12, FW_NORMAL);
                // Regular weight: Yu Gothic UI's semibold blurs at this size, so the title stands out by size alone.
                titleFont = make(16, FW_NORMAL);
                // Both fields still hold the font just deleted until they are given the new one.
                for (const HWND field : {search, nameEdit}) {
                    if (field != nullptr) {
                        SendMessageW(field, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
                    }
                }
                // So does the box naming a new folder, which then moves to the name at its new size.
                if (renameEdit != nullptr) {
                    SendMessageW(renameEdit, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
                    placeRename();
                }
            }

            void applyTheme() {
                if (fieldBrush != nullptr) {
                    DeleteObject(fieldBrush);
                }
                fieldBrush = CreateSolidBrush(settingsTheme().textFieldBackground);
                applyDarkWindowFrame(window);
                applyDarkThemeClass(grid, false);
                applyDarkThemeClass(nearby.hwnd, false);
                applyDarkThemeClass(recentPane.hwnd, false);
                applyDarkThemeClass(search, true);
                applyDarkThemeClass(nameEdit, true);
                InvalidateRect(window, nullptr, TRUE);
                InvalidateRect(grid, nullptr, TRUE);
                InvalidateRect(nearby.hwnd, nullptr, TRUE);
                InvalidateRect(recentPane.hwnd, nullptr, TRUE);
                InvalidateRect(search, nullptr, TRUE);
                InvalidateRect(nameEdit, nullptr, TRUE);
            }

            // The kind a double-click or Enter opens for this entry: the leftmost listed on its card, whatever the filter.
            [[nodiscard]] static std::optional<Kind> primaryKind(const Entry &entry) {
                for (const Kind kind : OPEN_ORDER) {
                    if (entry.has(kind)) {
                        return kind;
                    }
                }
                return std::nullopt;
            }

            // The kinds a work has, or with Show File Extensions on its files' extensions, the thumbnail's last.
            [[nodiscard]] static std::wstring filesText(const Entry &entry) {
                std::wstring text;
                const auto add = [&text](const std::wstring &part) {
                    if (!text.empty()) text += L" · ";
                    text += part;
                };
                if (!GalleryIO::state().showExtensions) {
                    for (size_t k = 0; k < KIND_COUNT; ++k) {
                        if (entry.has(static_cast<Kind>(k))) {
                            add(UiLanguage::text(KIND_LABELS[k]));
                        }
                    }
                    return text;
                }
                for (size_t k = 0; k < KIND_COUNT; ++k) {
                    for (const auto &[kind, extension] : entry.extensions) {
                        if (static_cast<size_t>(kind) == k) {
                            add(extension);
                        }
                    }
                }
                if (entry.pictureWritten != 0) {
                    add(std::format(L".{}", Constants::Extension::THUMBNAIL));
                }
                return text;
            }

            [[nodiscard]] const Entry *selectedEntry() const {
                if (selected < 0 || selected >= static_cast<int>(shown.size())) {
                    return nullptr;
                }
                return &entries[shown[static_cast<size_t>(selected)]];
            }

            static void addFile(Entry &entry, const std::filesystem::path &file, const Kind kind, const int64_t written) {
                std::wstring extension = lower(file.extension().wstring());
                if (std::ranges::find(entry.extensions, std::pair{kind, extension}) == entry.extensions.end()) {
                    entry.extensions.emplace_back(kind, std::move(extension));
                }
                // A folder can hold a map in both forms; the newer one stands for the work.
                if (entry.has(kind) && entry.written[static_cast<size_t>(kind)] >= written) {
                    return;
                }
                entry.files[static_cast<size_t>(kind)] = file;
                entry.written[static_cast<size_t>(kind)] = written;
                entry.kinds |= static_cast<uint8_t>(1u << static_cast<unsigned>(kind));
                entry.newest = std::max(entry.newest, written);
            }

            static void chooseSource(Entry &entry) {
                if (entry.has(Kind::IMAGE)) {
                    entry.source = entry.files[static_cast<size_t>(Kind::IMAGE)];
                } else if (entry.has(Kind::MAP)) {
                    entry.source = entry.files[static_cast<size_t>(Kind::MAP)];
                } else {
                    for (const auto &file : entry.files) {
                        if (!file.empty()) {
                            entry.source = file;
                            break;
                        }
                    }
                }
            }

            // Reads the folder, or the recent list, again; the selection stays on the same work when it is still there.
            void rescan() {
                rebuildFolders();
                std::wstring keep;
                if (const Entry *current = selectedEntry()) {
                    keep = lower(current->source.wstring());
                }
                entries.clear();
                if (filter == Filter::RECENT) {
                    for (const auto &file : GalleryIO::state().recent) {
                        std::error_code error;
                        const auto kind = GalleryIO::kindOf(file);
                        if (!kind || !std::filesystem::is_regular_file(file, error)) {
                            continue;
                        }
                        Entry entry;
                        entry.name = file.filename().wstring();
                        entry.folder = file.parent_path();
                        addFile(entry, file, *kind, lastWrite(file));
                        chooseSource(entry);
                        entry.pictureWritten = lastWrite(ThumbnailIO::pathFor(entry.source));
                        entries.push_back(std::move(entry));
                    }
                } else {
                    // One read of the folder gives every name and date, the thumbnails' included.
                    std::unordered_map<std::wstring, size_t> groups;
                    std::unordered_map<std::wstring, int64_t> thumbnails;
                    const std::wstring thumbnailExtension = std::format(L".{}", Constants::Extension::THUMBNAIL);
                    for (const Listed &item : listFolder(folder)) {
                        if (item.attributes & FILE_ATTRIBUTE_DIRECTORY) {
                            if (visibleFolder(item)) {
                                Entry entry;
                                entry.name = item.name;
                                entry.folder = folder;
                                entry.isFolder = true;
                                entry.source = folder / item.name;
                                entry.newest = item.written;
                                entries.push_back(std::move(entry));
                            }
                            continue;
                        }
                        // A save still in progress writes under this prefix.
                        if (item.name.starts_with(L".rff-")) {
                            continue;
                        }
                        const std::filesystem::path file = folder / item.name;
                        if (lower(file.extension().wstring()) == thumbnailExtension) {
                            thumbnails[lower(file.stem().wstring())] = item.written;
                            continue;
                        }
                        const auto kind = GalleryIO::kindOf(file);
                        if (!kind) {
                            continue;
                        }
                        const std::wstring stem = file.stem().wstring();
                        const auto [slot, inserted] = groups.try_emplace(lower(stem), entries.size());
                        if (inserted) {
                            Entry entry;
                            entry.name = stem;
                            entry.folder = folder;
                            entries.push_back(std::move(entry));
                        }
                        addFile(entries[slot->second], file, *kind, item.written);
                    }
                    for (auto &entry : entries) {
                        if (!entry.isFolder) {
                            chooseSource(entry);
                            const auto found = thumbnails.find(lower(entry.source.stem().wstring()));
                            entry.pictureWritten = found != thumbnails.end() ? found->second : 0;
                        }
                    }
                }
                refilter(keep);
            }

            [[nodiscard]] bool passes(const Entry &entry, const std::wstring &needle) const {
                // Folders stay under every kind, so the way to another folder never disappears.
                if (entry.isFolder) {
                    return needle.empty() || lower(entry.name).find(needle) != std::wstring::npos;
                }
                switch (filter) {
                case Filter::SETTINGS:
                    if (!entry.has(Kind::SETTINGS) && !entry.has(Kind::LOCATION)) return false;
                    break;
                case Filter::MAPS:
                    if (!entry.has(Kind::MAP)) return false;
                    break;
                case Filter::IMAGES:
                    if (!entry.has(Kind::IMAGE)) return false;
                    break;
                case Filter::APPEARANCE:
                    if (!entry.has(Kind::APPEARANCE)) return false;
                    break;
                default:
                    break;
                }
                return needle.empty() || lower(entry.name).find(needle) != std::wstring::npos;
            }

            void refilter(const std::wstring &keep) {
                std::wstring needle;
                if (search != nullptr) {
                    needle.resize(static_cast<size_t>(GetWindowTextLengthW(search)) + 1);
                    GetWindowTextW(search, needle.data(), static_cast<int>(needle.size()));
                    needle.resize(wcslen(needle.c_str()));
                    needle = lower(needle);
                }
                shown.clear();
                for (size_t i = 0; i < entries.size(); ++i) {
                    if (passes(entries[i], needle)) {
                        shown.push_back(i);
                    }
                }
                // Recent Files keeps the order the files were used in unless names are asked for.
                if (sort == 1) {
                    std::ranges::stable_sort(shown, [this](const size_t a, const size_t b) {
                        const auto &x = entries[a].name;
                        const auto &y = entries[b].name;
                        return CompareStringEx(LOCALE_NAME_USER_DEFAULT, NORM_IGNORECASE | SORT_DIGITSASNUMBERS, x.c_str(),
                                               static_cast<int>(x.size()), y.c_str(), static_cast<int>(y.size()), nullptr,
                                               nullptr, 0) == CSTR_LESS_THAN;
                    });
                } else if (filter != Filter::RECENT) {
                    std::ranges::stable_sort(shown, [this](const size_t a, const size_t b) {
                        return entries[a].newest > entries[b].newest;
                    });
                }
                // Folders come first, by name with numbers in order, whichever order the files are in.
                const auto folderEnd = std::ranges::stable_partition(shown, [this](const size_t i) { return entries[i].isFolder; }).begin();
                std::stable_sort(shown.begin(), folderEnd, [this](const size_t a, const size_t b) {
                    const auto &x = entries[a].name;
                    const auto &y = entries[b].name;
                    return CompareStringEx(LOCALE_NAME_USER_DEFAULT, NORM_IGNORECASE | SORT_DIGITSASNUMBERS, x.c_str(),
                                           static_cast<int>(x.size()), y.c_str(), static_cast<int>(y.size()), nullptr,
                                           nullptr, 0) == CSTR_LESS_THAN;
                });
                selected = -1;
                for (size_t i = 0; i < shown.size() && !keep.empty(); ++i) {
                    if (lower(entries[shown[i]].source.wstring()) == keep) {
                        selected = static_cast<int>(i);
                        break;
                    }
                }
                // A folder just opened starts on its first file, not on the folders above it.
                if (selected < 0 && !shown.empty()) {
                    selected = static_cast<int>(folderEnd - shown.begin()) < static_cast<int>(shown.size())
                                   ? static_cast<int>(folderEnd - shown.begin())
                                   : 0;
                    scroll = 0;
                }
                updateScroll();
                ensureVisible();
                selectionChanged();
                InvalidateRect(grid, nullptr, FALSE);
                // The name box follows its card when the cards are reordered or narrowed, and hides while it is filtered out.
                placeRename();
            }

            void updateScroll() {
                if (grid == nullptr) {
                    return;
                }
                RECT client{};
                GetClientRect(grid, &client);
                const CardLayout layout = cardLayout();
                const int height = contentHeight(layout);
                scroll = std::clamp(scroll, 0, std::max(0, height - static_cast<int>(client.bottom)));
                SCROLLINFO info{sizeof(info)};
                info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
                info.nMin = 0;
                info.nMax = std::max(0, height - 1);
                info.nPage = static_cast<UINT>(std::max<LONG>(1, client.bottom));
                info.nPos = scroll;
                SetScrollInfo(grid, SB_VERT, &info, TRUE);
            }

            void scrollTo(const int position) {
                scroll = position;
                updateScroll();
                InvalidateRect(grid, nullptr, FALSE);
                placeRename();
            }

            void ensureVisible() {
                if (grid == nullptr || selected < 0) {
                    return;
                }
                RECT client{};
                GetClientRect(grid, &client);
                const CardLayout layout = cardLayout();
                const RECT card = cardRect(selected, layout);
                if (card.top < layout.gap) {
                    scrollTo(scroll + card.top - layout.gap);
                } else if (card.bottom > client.bottom - layout.gap) {
                    scrollTo(scroll + card.bottom - client.bottom + layout.gap);
                }
            }

            void selectionChanged() {
                details.clear();
                detailKinds.clear();
                const Entry *entry = selectedEntry();
                if (entry != nullptr && entry->isFolder) {
                    details.emplace_back(L"Folder", entry->source.wstring());
                    details.emplace_back(L"Modified", shortDate(entry->newest));
                } else if (entry != nullptr) {
                    if (const auto kind = primaryKind(*entry)) {
                        details = GalleryIO::describe(entry->files[static_cast<size_t>(*kind)]);
                    }
                    // The kinds of this work replace the kind of the one file described.
                    std::erase_if(details, [](const auto &line) { return line.first == L"Kind"; });
                    detailKinds = filesText(*entry);
                    if (filter == Filter::RECENT) {
                        details.insert(details.begin(), {L"Folder", entry->folder.wstring()});
                    }
                }
                layout();
                InvalidateRect(window, nullptr, FALSE);
            }

            void select(const int index) {
                if (shown.empty()) {
                    return;
                }
                const int clamped = std::clamp(index, 0, static_cast<int>(shown.size()) - 1);
                if (clamped == selected) {
                    ensureVisible();
                    return;
                }
                selected = clamped;
                // Choosing a card offers its name for saving over it, the way a save dialog takes a clicked file.
                if (const Entry &entry = entries[shown[static_cast<size_t>(selected)]]; nameEdit != nullptr && saving && !entry.isFolder) {
                    SetWindowTextW(nameEdit, entry.source.stem().c_str());
                }
                ensureVisible();
                InvalidateRect(grid, nullptr, FALSE);
                selectionChanged();
            }

            void openKind(const Kind kind) {
                const Entry *entry = selectedEntry();
                if (entry == nullptr || !entry->has(kind)) {
                    return;
                }
                const std::filesystem::path file = entry->files[static_cast<size_t>(kind)];
                if (IOUtilities::isModalDialogOpen()) {
                    return;
                }
                if (scene.isLongJobBusy()) {
                    NativeDialogs::message(window, L"RFF_Super is busy with a video or keyframe job.\n\nOpen the file after it ends.",
                                           L"Gallery", MB_OK | MB_ICONINFORMATION);
                    return;
                }
                std::error_code error;
                if (!std::filesystem::is_regular_file(file, error)) {
                    NativeDialogs::message(window, L"This file is no longer there.", L"Gallery", MB_OK | MB_ICONWARNING);
                    rescan();
                    return;
                }
                const bool opened = CallbackFile::openFile(menu, scene, file);
                if (!IsWindow(window)) {
                    return;
                }
                if (opened && GalleryIO::state().closeAfterOpen) {
                    // Posted, so the window goes once this call has returned rather than from under it.
                    PostMessageW(window, WM_CLOSE, 0, 0);
                    return;
                }
                if (filter == Filter::RECENT) {
                    rescan();
                }
            }

            void openPrimary() {
                if (const Entry *entry = selectedEntry(); entry != nullptr && entry->isFolder) {
                    const std::filesystem::path inside = entry->source;
                    showFolder(inside);
                    return;
                }
                if (saving) {
                    // A double-click or Enter saves over the work, after asking; a single click only chooses it.
                    if (const Entry *entry = selectedEntry()) {
                        const Entry chosen = *entry;
                        saveOver(chosen);
                    }
                    return;
                }
                if (const Entry *entry = selectedEntry()) {
                    if (const auto kind = primaryKind(*entry)) {
                        openKind(*kind);
                    }
                }
            }

            // The mouse's back (or forward) button: the folder shown before (or after) this one.
            void walkHistory(const bool forward) {
                auto &from = forward ? forwardFolders : backFolders;
                auto &to = forward ? backFolders : forwardFolders;
                if (from.empty() || IOUtilities::isModalDialogOpen()) {
                    return;
                }
                const std::filesystem::path target = from.back();
                from.pop_back();
                to.push_back(folder);
                walkingHistory = true;
                showFolder(target);
                walkingHistory = false;
            }

            // A double-click on a work in Gallery (Save): after asking, the view is saved over each kind of file it has.
            void saveOver(const Entry &entry) {
                if (IOUtilities::isModalDialogOpen()) {
                    return;
                }
                if (scene.isLongJobBusy()) {
                    NativeDialogs::message(window, L"RFF_Super is busy with a video or keyframe job.\n\nSave after it ends.",
                                           L"Gallery", MB_OK | MB_ICONINFORMATION);
                    return;
                }
                std::filesystem::path place;
                std::wstring listed;
                for (const auto &file : entry.files) {
                    if (!file.empty()) {
                        place = file.parent_path();
                        listed.append(file.filename().wstring()).append(L"\n");
                    }
                }
                if (place.empty()) {
                    return;
                }
                const std::wstring text = UiLanguage::text(L"Save the current view over these files?") + L"\n\n" + listed;
                if (NativeDialogs::message(window, text.c_str(), L"Confirm replacement",
                                           MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES) {
                    return;
                }
                const std::wstring name = entry.source.stem().wstring();
                const auto mapFile = entry.files[static_cast<size_t>(Kind::MAP)];
                const bool plainMap = entry.has(Kind::MAP) && lower(mapFile.extension().wstring()) == L".rfm";
                using SaveKind = CallbackFile::SaveKind;
                std::vector<std::pair<SaveKind, bool>> saves;
                // Settings and an image together are one work, saved as Save Work saves it, the map with them when they had one.
                const bool work = entry.has(Kind::SETTINGS) && entry.has(Kind::IMAGE);
                if (work) {
                    saves.emplace_back(SaveKind::WORK, entry.has(Kind::MAP) && !plainMap);
                } else {
                    if (entry.has(Kind::SETTINGS)) saves.emplace_back(SaveKind::SETTINGS, false);
                    if (entry.has(Kind::IMAGE)) saves.emplace_back(SaveKind::IMAGE, false);
                }
                if (entry.has(Kind::MAP) && (!work || plainMap)) {
                    saves.emplace_back(plainMap ? SaveKind::PLAIN_MAP : SaveKind::MAP, false);
                }
                if (entry.has(Kind::LOCATION)) saves.emplace_back(SaveKind::LOCATION, false);
                if (entry.has(Kind::APPEARANCE)) saves.emplace_back(SaveKind::APPEARANCE, false);
                bool any = false;
                for (const auto &[kind, withMap] : saves) {
                    any = CallbackFile::saveInto(scene, kind, place, name, withMap, false) || any;
                    if (!IsWindow(window)) {
                        return;
                    }
                }
                if (any) {
                    rescan();
                    SetTimer(window, REFRESH_TIMER, 1000, nullptr);
                }
            }

            // Shows the files of another folder, leaving Recent Files for All when it was showing.
            void showFolder(const std::filesystem::path &next) {
                // A name being typed is kept before the folder changes, as clicking elsewhere keeps it.
                endRename(true);
                if (filter != Filter::RECENT && folderKey(next) == folderKey(folder)) {
                    return;
                }
                std::error_code error;
                if (!std::filesystem::is_directory(next, error)) {
                    NativeDialogs::message(window, L"This folder is no longer there. Choose another one.", L"Gallery",
                                           MB_OK | MB_ICONWARNING);
                    // Recent Folders is checked again, so the folder that is gone leaves the list.
                    recentListedFrom.clear();
                    rescan();
                    return;
                }
                if (!walkingHistory && folderKey(next) != folderKey(folder)) {
                    backFolders.push_back(folder);
                    forwardFolders.clear();
                }
                folder = trimmedFolder(next);
                GalleryIO::state().folder = folder;
                for (const auto &key : worker.clearQueue()) {
                    thumbs.erase(key);
                }
                if (filter == Filter::RECENT) {
                    filter = Filter::ALL;
                    GalleryIO::state().filter = static_cast<uint8_t>(filter);
                }
                GalleryIO::save();
                selected = -1;
                scroll = 0;
                rescan();
                layout();
                InvalidateRect(window, nullptr, FALSE);
            }

            // Fills both lists on the left: the folders beside the one shown, and the folders of the files used most recently.
            void rebuildFolders() {
                const auto make = [](const std::filesystem::path &path) {
                    FolderItem item;
                    item.path = path;
                    item.name = folderName(path);
                    item.parent = isDriveRoot(path) ? UiLanguage::text(L"Drive") : path.parent_path().wstring();
                    return item;
                };

                // Checked only when the recent files change, since a folder on a drive that is gone can take seconds to answer.
                if (GalleryIO::state().recent != recentListedFrom) {
                    recentListedFrom = GalleryIO::state().recent;
                    std::vector<FolderItem> recentItems;
                    std::unordered_set<std::wstring> seen;
                    for (const auto &file : recentListedFrom) {
                        const std::filesystem::path path = trimmedFolder(file.parent_path());
                        std::error_code error;
                        if (path.empty() || !seen.insert(folderKey(path)).second || !std::filesystem::is_directory(path, error)) {
                            continue;
                        }
                        recentItems.push_back(make(path));
                    }
                    setPaneItems(recentPane, std::move(recentItems));
                }

                // A drive root has no folder beside it, so the drives stand in for them.
                std::vector<FolderItem> nearbyItems;
                const std::filesystem::path current = trimmedFolder(folder);
                const bool root = isDriveRoot(current);
                std::vector<std::filesystem::path> paths;
                if (!folder.empty()) {
                    paths = root ? drives() : subfolders(current.parent_path());
                }
                const std::wstring currentKey = folderKey(current);
                if (!folder.empty() && std::ranges::none_of(paths, [&](const auto &p) { return folderKey(p) == currentKey; })) {
                    paths.push_back(current);
                }
                for (const auto &path : paths) {
                    nearbyItems.push_back(make(path));
                }
                if (root) {
                    nearbyTitle = UiLanguage::text(L"Drives");
                } else {
                    const std::wstring parentName = folderName(current.parent_path());
                    nearbyTitle = std::vformat(UiLanguage::text(L"Folders in {}"), std::make_wformat_args(parentName));
                }
                setPaneItems(nearby, std::move(nearbyItems));
                // Moving to another folder brings it into view in the list beside it.
                if (nearbyFor != currentKey && nearby.hwnd != nullptr) {
                    nearbyFor = currentKey;
                    for (int i = 0; i < static_cast<int>(nearby.items.size()); ++i) {
                        if (folderKey(nearby.items[static_cast<size_t>(i)].path) == currentKey) {
                            RECT client{};
                            GetClientRect(nearby.hwnd, &client);
                            scrollPaneTo(nearby, i * paneStep(nearby) - (static_cast<int>(client.bottom) - paneRowHeight(nearby)) / 2);
                            break;
                        }
                    }
                }
                if (window != nullptr) {
                    InvalidateRect(window, &paneRect, FALSE);
                }
            }

            void setPaneItems(FolderPane &pane, std::vector<FolderItem> items) {
                pane.items = std::move(items);
                if (pane.hot >= static_cast<int>(pane.items.size())) {
                    pane.hot = -1;
                }
                updatePaneScroll(pane);
                if (pane.hwnd != nullptr) {
                    InvalidateRect(pane.hwnd, nullptr, FALSE);
                }
            }

            // The strip that moves the line between the two folder lists: the line itself and the Recent Folders title below it.
            [[nodiscard]] bool inSplitter(const POINT point) const {
                return point.x >= paneRect.left && point.x < paneRect.right && point.y >= paneMiddle - px(4) &&
                       point.y < recentPane.title.bottom;
            }

            void dragSplit(const int y) {
                if (splitSpace <= 0) {
                    return;
                }
                const int nearbyHeight = y - splitGrab - px(4) - splitTop;
                GalleryIO::state().folderSplit = std::clamp(static_cast<float>(nearbyHeight) / static_cast<float>(splitSpace), 0.0f, 1.0f);
                layout();
                InvalidateRect(window, &paneRect, FALSE);
            }

            [[nodiscard]] FolderPane *paneOf(const HWND hwnd) {
                return hwnd == nearby.hwnd ? &nearby : hwnd == recentPane.hwnd ? &recentPane : nullptr;
            }

            [[nodiscard]] int paneRowHeight(const FolderPane &pane) const {
                return px(pane.compact ? 32 : 44);
            }

            [[nodiscard]] int paneStep(const FolderPane &pane) const {
                return paneRowHeight(pane) + px(2);
            }

            void updatePaneScroll(FolderPane &pane) {
                if (pane.hwnd == nullptr) {
                    return;
                }
                RECT client{};
                GetClientRect(pane.hwnd, &client);
                const int height = static_cast<int>(pane.items.size()) * paneStep(pane);
                pane.scroll = std::clamp(pane.scroll, 0, std::max(0, height - static_cast<int>(client.bottom)));
                SCROLLINFO info{sizeof(info)};
                info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
                info.nMin = 0;
                info.nMax = std::max(0, height - 1);
                info.nPage = static_cast<UINT>(std::max<LONG>(1, client.bottom));
                info.nPos = pane.scroll;
                SetScrollInfo(pane.hwnd, SB_VERT, &info, TRUE);
            }

            void scrollPaneTo(FolderPane &pane, const int position) {
                pane.scroll = position;
                updatePaneScroll(pane);
                InvalidateRect(pane.hwnd, nullptr, FALSE);
            }

            [[nodiscard]] RECT paneRow(const FolderPane &pane, const int index) const {
                RECT client{};
                GetClientRect(pane.hwnd, &client);
                const int top = index * paneStep(pane) - pane.scroll;
                return {0, top, client.right, top + paneRowHeight(pane)};
            }

            [[nodiscard]] int hitPane(const FolderPane &pane, const POINT point) const {
                for (int i = 0; i < static_cast<int>(pane.items.size()); ++i) {
                    const RECT row = paneRow(pane, i);
                    if (PtInRect(&row, point)) {
                        return i;
                    }
                }
                return -1;
            }


            void setFilter(const Filter next) {
                if (next == filter) {
                    return;
                }
                endRename(true);
                const bool reread = (next == Filter::RECENT) != (filter == Filter::RECENT);
                filter = next;
                GalleryIO::state().filter = static_cast<uint8_t>(filter);
                if (reread) {
                    for (const auto &key : worker.clearQueue()) {
                        thumbs.erase(key);
                    }
                    selected = -1;
                    scroll = 0;
                    rescan();
                } else {
                    std::wstring keep;
                    if (const Entry *current = selectedEntry()) {
                        keep = lower(current->source.wstring());
                    }
                    refilter(keep);
                }
                layout();
                InvalidateRect(window, nullptr, FALSE);
            }

            // Lays the folder's path out as buttons from the left, dropping its first parts behind "..." when the row is too narrow.
            void layoutCrumbs(const auto &textWidth, const int left, const int top, const int rowHeight, const int right) {
                crumbPaths.clear();
                crumbSeparators.clear();
                crumbEllipsis = {};
                if (filter == Filter::RECENT || folder.empty()) {
                    return;
                }
                std::vector<std::filesystem::path> chain;
                for (std::filesystem::path walk = trimmedFolder(folder);; walk = walk.parent_path()) {
                    chain.push_back(walk);
                    if (isDriveRoot(walk) || walk.parent_path() == walk) {
                        break;
                    }
                }
                std::ranges::reverse(chain);
                const int separatorWidth = px(16);
                std::vector<int> widths;
                for (const auto &part : chain) {
                    widths.push_back(textWidth(folderName(part)) + px(16));
                }
                const int ellipsisWidth = textWidth(L"...") + px(8);
                size_t first = 0;
                const auto total = [&](const size_t from) {
                    int sum = from > 0 ? ellipsisWidth + separatorWidth : 0;
                    for (size_t i = from; i < widths.size(); ++i) {
                        sum += widths[i] + (i + 1 < widths.size() ? separatorWidth : 0);
                    }
                    return sum;
                };
                while (first + 1 < chain.size() && total(first) > right - left) {
                    ++first;
                }
                int x = left;
                if (first > 0) {
                    crumbEllipsis = {x, top, x + ellipsisWidth, top + rowHeight};
                    x += ellipsisWidth;
                    crumbSeparators.push_back({x, top, x + separatorWidth, top + rowHeight});
                    x += separatorWidth;
                }
                for (size_t i = first; i < chain.size(); ++i) {
                    const int width = std::min(widths[i], std::max<int>(px(48), right - x));
                    buttons.push_back(Button{{x, top, x + width, top + rowHeight},
                                             BUTTON_CRUMB + static_cast<int>(crumbPaths.size()), folderName(chain[i])});
                    crumbPaths.push_back(chain[i]);
                    x += width;
                    if (i + 1 < chain.size()) {
                        crumbSeparators.push_back({x, top, x + separatorWidth, top + rowHeight});
                        x += separatorWidth;
                    }
                }
            }

            // Places everything outside the grid, the open buttons of the selected work among it.
            void layout() {
                if (window == nullptr) {
                    return;
                }
                RECT client{};
                GetClientRect(window, &client);
                const int pad = px(12);
                const int rowHeight = px(32);
                buttons.clear();

                const HDC dc = GetDC(window);
                const auto previous = SelectObject(dc, font);
                const auto textWidth = [dc](const std::wstring &text) {
                    SIZE size{};
                    GetTextExtentPoint32W(dc, text.c_str(), static_cast<int>(text.size()), &size);
                    return static_cast<int>(size.cx);
                };

                int x = pad;
                int y = pad;
                int rowRight = client.right - pad;
                {
                    const std::wstring settingsLabel = UiLanguage::text(L"Settings") + L" ▾";
                    const int settingsWidth = textWidth(settingsLabel) + px(28);
                    buttons.push_back({{rowRight - settingsWidth, y, rowRight, y + rowHeight}, BUTTON_SETTINGS, settingsLabel});
                    rowRight -= settingsWidth + pad;
                }
                folderText = {x, y, rowRight, y + rowHeight};
                layoutCrumbs(textWidth, x, y, rowHeight, rowRight);

                y += rowHeight + px(8);
                // The filters sit side by side inside one frame, each a segment of it.
                const int segmentInset = px(2);
                x += segmentInset;
                for (size_t i = 0; i < FILTER_LABELS.size(); ++i) {
                    const std::wstring label = UiLanguage::text(FILTER_LABELS[i]);
                    const int width = textWidth(label) + px(24);
                    buttons.push_back({{x, y + segmentInset, x + width, y + rowHeight - segmentInset},
                                       BUTTON_FILTER + static_cast<int>(i), label, false, static_cast<size_t>(filter) == i});
                    x += width;
                }
                filterFrame = {pad, y, x + segmentInset, y + rowHeight};
                const std::wstring sortLabel = UiLanguage::text(sort == 1 ? L"By Name" : filter == Filter::RECENT ? L"Recently Used" : L"Newest First");
                const int sortWidth = std::max(textWidth(UiLanguage::text(L"Newest First")),
                                               std::max(textWidth(UiLanguage::text(L"Recently Used")),
                                                        textWidth(UiLanguage::text(L"By Name")))) + px(28);
                const int searchWidth = px(200);
                int right = client.right - pad;
                searchFrame = {right - searchWidth, y, right, y + rowHeight};
                right = searchFrame.left - px(8);
                buttons.push_back({{right - sortWidth, y, right, y + rowHeight}, BUTTON_SORT, sortLabel});
                if (search != nullptr) {
                    const int inset = px(6);
                    const int editHeight = px(20);
                    SetWindowPos(search, nullptr, searchFrame.left + inset + px(2),
                                 searchFrame.top + (rowHeight - editHeight) / 2, searchWidth - 2 * inset - px(2),
                                 editHeight, SWP_NOZORDER | SWP_NOACTIVATE);
                }

                // The save bar along the bottom, saving into the folder shown above.
                const int barTop = saving ? client.bottom - pad - rowHeight : client.bottom - pad + px(12);
                if (saving) {
                    // Starts at the grid's left edge, under the folder lists' column.
                    int bx = pad + px(224) + pad;
                    const std::wstring label = UiLanguage::text(L"Name");
                    nameLabel = {bx, barTop, bx + textWidth(label) + px(4), barTop + rowHeight};
                    bx = nameLabel.right + px(8);
                    nameFrame = {bx, barTop, bx + px(240), barTop + rowHeight};
                    bx = nameFrame.right + px(8);
                    const std::wstring workLabel = UiLanguage::text(L"Save Work");
                    const int workWidth = textWidth(workLabel) + px(36);
                    buttons.push_back({{bx, barTop, bx + workWidth, barTop + rowHeight}, BUTTON_SAVE_WORK, workLabel, true});
                    bx += workWidth + px(8);
                    const std::wstring otherSaves = UiLanguage::text(L"Other Saves...");
                    const int otherSavesWidth = textWidth(otherSaves) + px(28);
                    buttons.push_back({{bx, barTop, bx + otherSavesWidth, barTop + rowHeight}, BUTTON_OTHER_SAVES, otherSaves});
                    if (nameEdit != nullptr) {
                        const int inset = px(8);
                        const int editHeight = px(20);
                        SetWindowPos(nameEdit, nullptr, nameFrame.left + inset, nameFrame.top + (rowHeight - editHeight) / 2,
                                     nameFrame.right - nameFrame.left - 2 * inset, editHeight, SWP_NOZORDER | SWP_NOACTIVATE);
                    }
                }
                if (nameEdit != nullptr) {
                    ShowWindow(nameEdit, saving ? SW_SHOWNA : SW_HIDE);
                }

                y += rowHeight + px(12);
                const int bodyBottom = barTop - px(12);
                // As wide as a thumbnail is made, plus the panel's padding, so the preview shows it unscaled at 100%.
                const int detailsWidth = px(static_cast<int>(THUMBNAIL_WIDTH)) + 2 * px(16);
                detailsRect = {client.right - pad - detailsWidth, y, client.right - pad, bodyBottom};

                // The folders beside the one shown above Recent Folders, split as last dragged.
                paneRect = {pad, y, pad + px(224), bodyBottom};
                const int titleHeight = px(28);
                const int listLeft = paneRect.left + px(4);
                const int listWidth = std::max<int>(1, paneRect.right - paneRect.left - px(8));
                int paneY = paneRect.top + px(4);
                nearby.title = {paneRect.left + px(16), paneY, paneRect.right - px(16), paneY + titleHeight};
                paneY += titleHeight;
                const int listsBottom = paneRect.bottom - px(8);
                const int space = std::max(0, listsBottom - paneY - titleHeight - px(8));
                const int minimumList = px(60);
                const int nearbyHeight =
                    space <= 2 * minimumList
                        ? space / 2
                        : std::clamp(static_cast<int>(std::lround(space * GalleryIO::state().folderSplit)), minimumList, space - minimumList);
                splitTop = paneY;
                splitSpace = space;
                const RECT nearbyRect{listLeft, paneY, listLeft + listWidth, paneY + nearbyHeight};
                paneY += nearbyHeight + px(4);
                paneMiddle = paneY;
                paneY += px(4);
                recentPane.title = {paneRect.left + px(16), paneY, paneRect.right - px(16), paneY + titleHeight};
                paneY += titleHeight;
                const RECT recentRect{listLeft, paneY, listLeft + listWidth, std::max<int>(paneY + 1, listsBottom)};
                const std::array<std::pair<FolderPane *, RECT>, 2> lists{{{&nearby, nearbyRect}, {&recentPane, recentRect}}};
                for (const auto &[pane, rect] : lists) {
                    if (pane->hwnd != nullptr) {
                        SetWindowPos(pane->hwnd, nullptr, rect.left, rect.top, std::max<int>(1, rect.right - rect.left),
                                     std::max<int>(1, rect.bottom - rect.top), SWP_NOZORDER | SWP_NOACTIVATE);
                        updatePaneScroll(*pane);
                    }
                }

                gridRect = {paneRect.right + pad, y, detailsRect.left - pad, bodyBottom};
                if (grid != nullptr) {
                    SetWindowPos(grid, nullptr, gridRect.left, gridRect.top, std::max<int>(1, gridRect.right - gridRect.left),
                                 std::max<int>(1, gridRect.bottom - gridRect.top), SWP_NOZORDER | SWP_NOACTIVATE);
                }

                const int inner = px(16);
                const int previewWidth = detailsRect.right - detailsRect.left - 2 * inner;
                previewRect = {detailsRect.left + inner, detailsRect.top + inner, detailsRect.right - inner,
                               detailsRect.top + inner + previewWidth * 10 / 16};
                if (const Entry *entry = selectedEntry(); entry != nullptr && entry->isFolder) {
                    const int buttonHeight = px(32);
                    const int bottom = detailsRect.bottom - inner;
                    buttons.push_back({{detailsRect.left + inner, bottom - buttonHeight, detailsRect.right - inner, bottom},
                                       BUTTON_OPEN_FOLDER, UiLanguage::text(L"Open Folder"), true});
                } else if (entry != nullptr && !saving) {
                    const auto primary = primaryKind(*entry);
                    int bottom = detailsRect.bottom - inner;
                    const int buttonHeight = px(32);
                    // Laid out from the bottom up, the primary one last so it sits on top.
                    std::vector<Kind> order;
                    for (const Kind kind : OPEN_ORDER) {
                        if (entry->has(kind) && kind != primary) {
                            order.push_back(kind);
                        }
                    }
                    if (primary) {
                        order.push_back(*primary);
                    }
                    for (const Kind kind : order) {
                        buttons.push_back({{detailsRect.left + inner, bottom - buttonHeight, detailsRect.right - inner, bottom},
                                           BUTTON_OPEN + static_cast<int>(kind),
                                           UiLanguage::text(OPEN_LABELS[static_cast<size_t>(kind)]), kind == primary});
                        bottom -= buttonHeight + px(8);
                    }
                }
                SelectObject(dc, previous);
                ReleaseDC(window, dc);
                if (grid != nullptr) {
                    updateScroll();
                }
            }

            void drawButton(const HDC dc, const Button &button, const bool hot, const bool pressed) const {
                const auto &theme = settingsTheme();
                const auto drawLabel = [&](const COLORREF color) {
                    SetTextColor(dc, color);
                    RECT label = button.rect;
                    DrawTextW(dc, button.label.c_str(), -1, &label, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
                };
                // A part of the path is plain text, the folder shown in the text color and those above it quieter.
                if (button.id >= BUTTON_CRUMB) {
                    if (pressed || hot) {
                        roundFrame(dc, button.rect, static_cast<float>(px(6)), theme.buttonFacePressed, std::nullopt);
                    }
                    const bool shown = button.id == BUTTON_CRUMB + static_cast<int>(crumbPaths.size()) - 1;
                    drawLabel(shown || hot ? theme.text : theme.rangeText);
                    return;
                }
                // Settings and the sort order are plain text like the path, so the corner above the search field carries no frames.
                if (button.id == BUTTON_SETTINGS) {
                    if (pressed || hot) {
                        roundFrame(dc, button.rect, static_cast<float>(px(8)), theme.buttonFacePressed, std::nullopt);
                    }
                    drawLabel(theme.text);
                    return;
                }
                // The sort order keeps a frame so it can be found; the field's fill and a panel's hairline keep it quiet.
                if (button.id == BUTTON_SORT) {
                    roundFrame(dc, button.rect, static_cast<float>(px(8)), pressed || hot ? theme.buttonFacePressed : theme.textFieldBackground,
                               theme.sectionFrame);
                    drawLabel(theme.text);
                    return;
                }
                // A filter is a segment of the frame they share; the chosen one is raised a step, without the accent.
                if (button.id >= BUTTON_FILTER && button.id < BUTTON_FILTER + static_cast<int>(FILTER_LABELS.size())) {
                    if (button.selected) {
                        roundFrame(dc, button.rect, static_cast<float>(px(6)),
                                   darkSettingsMode() ? theme.buttonFacePressed : theme.buttonFace, theme.sectionFrame);
                    }
                    drawLabel(button.selected || hot || pressed ? theme.text : theme.rangeText);
                    return;
                }
                COLORREF face = theme.buttonFace;
                COLORREF border = theme.buttonBorder;
                COLORREF text = theme.text;
                if (button.primary) {
                    face = pressed ? theme.primaryButtonPressed : theme.primaryButton;
                    border = face;
                    text = theme.primaryButtonText;
                } else if (button.selected) {
                    // A toggle that is on is raised a step and outlined in the quiet text color, leaving the accent to the card.
                    face = theme.buttonFacePressed;
                    border = theme.rangeText;
                } else if (pressed || hot) {
                    face = theme.buttonFacePressed;
                }
                roundFrame(dc, button.rect, static_cast<float>(px(8)), face, border);
                drawLabel(text);
            }

            // The frame and titles behind the two folder lists, and the line between them.
            void paintPane(const HDC dc) const {
                const auto &theme = settingsTheme();
                roundFrame(dc, paneRect, static_cast<float>(px(8)), std::nullopt, theme.sectionFrame);
                const HPEN framePen = CreatePen(PS_SOLID, 1, theme.sectionFrame);
                const auto oldPen = SelectObject(dc, framePen);
                MoveToEx(dc, paneRect.left + 1, paneMiddle, nullptr);
                LineTo(dc, paneRect.right - 1, paneMiddle);
                SelectObject(dc, oldPen);
                DeleteObject(framePen);
                SelectObject(dc, smallFont);
                SetTextColor(dc, theme.rangeText);
                RECT nearbyTitleRect = nearby.title;
                DrawTextW(dc, nearbyTitle.c_str(), -1, &nearbyTitleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
                const std::wstring recentTitle = UiLanguage::text(L"Recent Folders");
                RECT recentTitleRect = recentPane.title;
                DrawTextW(dc, recentTitle.c_str(), -1, &recentTitleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
                SelectObject(dc, font);
            }

            // The > marks between the parts of the path, and the ... standing for the parts left out.
            void paintCrumbMarks(const HDC dc) const {
                const auto &theme = settingsTheme();
                if (!crumbSeparators.empty() && smoothDrawing()) {
                    Gdiplus::Graphics graphics(dc);
                    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
                    graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
                    Gdiplus::Pen pen(plusColor(theme.rangeText), static_cast<float>(std::max(1, px(1))));
                    pen.SetLineJoin(Gdiplus::LineJoinRound);
                    pen.SetStartCap(Gdiplus::LineCapRound);
                    pen.SetEndCap(Gdiplus::LineCapRound);
                    const auto half = static_cast<float>(px(4));
                    for (const RECT &mark : crumbSeparators) {
                        const float cx = static_cast<float>(mark.left + mark.right) / 2;
                        const float cy = static_cast<float>(mark.top + mark.bottom) / 2;
                        const Gdiplus::PointF chevron[3] = {{cx - half / 2, cy - half}, {cx + half / 2, cy}, {cx - half / 2, cy + half}};
                        graphics.DrawLines(&pen, chevron, 3);
                    }
                }
                if (crumbEllipsis.right > crumbEllipsis.left) {
                    SetTextColor(dc, theme.rangeText);
                    RECT text = crumbEllipsis;
                    DrawTextW(dc, L"...", -1, &text, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
                }
            }

            void paintFolders(const FolderPane &pane) {
                PAINTSTRUCT ps;
                const HDC target = BeginPaint(pane.hwnd, &ps);
                RECT client{};
                GetClientRect(pane.hwnd, &client);
                const HDC dc = CreateCompatibleDC(target);
                const HBITMAP buffer = CreateCompatibleBitmap(target, std::max<int>(1, client.right), std::max<int>(1, client.bottom));
                const auto oldBitmap = SelectObject(dc, buffer);
                const auto &theme = settingsTheme();
                const HBRUSH back = CreateSolidBrush(theme.background);
                FillRect(dc, &client, back);
                DeleteObject(back);
                SetBkMode(dc, TRANSPARENT);
                const auto oldFont = SelectObject(dc, font);

                const std::wstring current = filter == Filter::RECENT ? std::wstring() : folderKey(folder);
                for (int i = 0; i < static_cast<int>(pane.items.size()); ++i) {
                    const RECT row = paneRow(pane, i);
                    if (row.bottom < 0) {
                        continue;
                    }
                    if (row.top > client.bottom) {
                        break;
                    }
                    const FolderItem &item = pane.items[static_cast<size_t>(i)];
                    const bool isCurrent = !current.empty() && folderKey(item.path) == current;
                    // The folder shown is marked by its fill alone, so the pane carries no outline of its own.
                    if (isCurrent || i == pane.hot) {
                        // The folder shown is raised a full step without the accent; a row under the pointer half a step.
                        const COLORREF half = RGB((GetRValue(theme.background) + GetRValue(theme.buttonFacePressed)) / 2,
                                                  (GetGValue(theme.background) + GetGValue(theme.buttonFacePressed)) / 2,
                                                  (GetBValue(theme.background) + GetBValue(theme.buttonFacePressed)) / 2);
                        roundFrame(dc, row, static_cast<float>(px(6)), isCurrent ? theme.buttonFacePressed : half, std::nullopt);
                    }
                    SelectObject(dc, font);
                    SetTextColor(dc, theme.text);
                    if (pane.compact) {
                        RECT name{row.left + px(12), row.top, row.right - px(8), row.bottom};
                        DrawTextW(dc, item.name.c_str(), -1, &name, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
                        continue;
                    }
                    RECT name{row.left + px(12), row.top + px(4), row.right - px(8), row.top + px(24)};
                    DrawTextW(dc, item.name.c_str(), -1, &name, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
                    SelectObject(dc, smallFont);
                    SetTextColor(dc, theme.rangeText);
                    RECT parent{row.left + px(12), name.bottom, row.right - px(8), row.bottom - px(4)};
                    DrawTextW(dc, item.parent.c_str(), -1, &parent, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_PATH_ELLIPSIS | DT_NOPREFIX);
                }

                SelectObject(dc, oldFont);
                BitBlt(target, 0, 0, client.right, client.bottom, dc, 0, 0, SRCCOPY);
                SelectObject(dc, oldBitmap);
                DeleteObject(buffer);
                DeleteDC(dc);
                EndPaint(pane.hwnd, &ps);
            }

            void paintWindow() {
                PAINTSTRUCT ps;
                const HDC target = BeginPaint(window, &ps);
                RECT client{};
                GetClientRect(window, &client);
                const HDC dc = CreateCompatibleDC(target);
                const HBITMAP buffer = CreateCompatibleBitmap(target, std::max<int>(1, client.right), std::max<int>(1, client.bottom));
                const auto oldBitmap = SelectObject(dc, buffer);
                const auto &theme = settingsTheme();
                const HBRUSH back = CreateSolidBrush(theme.background);
                FillRect(dc, &client, back);
                DeleteObject(back);
                SetBkMode(dc, TRANSPARENT);
                const auto oldFont = SelectObject(dc, font);

                paintPane(dc);
                // The one frame the filters share, drawn first so the chosen segment sits on it.
                roundFrame(dc, filterFrame, static_cast<float>(px(8)), theme.controlDisabledFace, theme.sectionFrame);
                for (size_t i = 0; i < buttons.size(); ++i) {
                    drawButton(dc, buttons[i], static_cast<int>(i) == hotButton, static_cast<int>(i) == pressedButton);
                }
                paintCrumbMarks(dc);

                // Recent Files has no one folder, so the row above says what the list is instead of a path.
                if (filter == Filter::RECENT) {
                    SetTextColor(dc, theme.rangeText);
                    const std::wstring place = UiLanguage::text(L"Files you opened or saved recently");
                    RECT placeRect = folderText;
                    DrawTextW(dc, place.c_str(), -1, &placeRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
                }

                // A field shows its fill alone in dark mode; light mode keeps a hairline, as white on near white does not show.
                const std::optional<COLORREF> fieldEdge = darkSettingsMode() ? std::nullopt : std::optional(theme.sectionFrame);
                // The search field's rounded frame; the edit control inside it has no border of its own.
                {
                    roundFrame(dc, searchFrame, static_cast<float>(px(8)), theme.textFieldBackground,
                               GetFocus() == search ? std::optional(theme.previewBorderSelected) : fieldEdge);
                }

                paintDetails(dc);

                // The save bar's label and the name field's frame.
                if (saving) {
                SetTextColor(dc, theme.text);
                const std::wstring nameText = UiLanguage::text(L"Name");
                RECT labelRect = nameLabel;
                DrawTextW(dc, nameText.c_str(), -1, &labelRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                {
                    roundFrame(dc, nameFrame, static_cast<float>(px(8)), theme.textFieldBackground,
                               GetFocus() == nameEdit ? std::optional(theme.previewBorderSelected) : fieldEdge);
                }
                }

                SelectObject(dc, oldFont);
                BitBlt(target, 0, 0, client.right, client.bottom, dc, 0, 0, SRCCOPY);
                SelectObject(dc, oldBitmap);
                DeleteObject(buffer);
                DeleteDC(dc);
                EndPaint(window, &ps);
            }

            void paintDetails(const HDC dc) {
                const auto &theme = settingsTheme();
                roundFrame(dc, detailsRect, static_cast<float>(px(8)), std::nullopt, theme.sectionFrame);

                const Entry *entry = selectedEntry();
                if (entry == nullptr) {
                    SetTextColor(dc, theme.rangeText);
                    RECT text = detailsRect;
                    const std::wstring hint = UiLanguage::text(L"Select a file to see its details.");
                    DrawTextW(dc, hint.c_str(), -1, &text, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                    return;
                }
                paintThumbnail(dc, *entry, previewRect, false);
                roundCorners(dc, previewRect, static_cast<float>(px(4)), theme.background);

                const int inner = px(16);
                const int left = detailsRect.left + inner;
                const int right = detailsRect.right - inner;
                int y = previewRect.bottom + px(16);
                SelectObject(dc, titleFont);
                SetTextColor(dc, theme.text);
                RECT title{left, y, right, y + px(24)};
                DrawTextW(dc, entry->name.c_str(), -1, &title, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
                y += px(24);
                // The kinds under the name, as on the card, wrapping to a second line if all five do not fit.
                if (!detailKinds.empty()) {
                    SelectObject(dc, smallFont);
                    SetTextColor(dc, theme.rangeText);
                    RECT kinds{left, y, right, y};
                    DrawTextW(dc, detailKinds.c_str(), -1, &kinds, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX | DT_CALCRECT);
                    kinds.right = right;
                    DrawTextW(dc, detailKinds.c_str(), -1, &kinds, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);
                    y = kinds.bottom;
                }
                y += px(12);

                int lowestButton = detailsRect.bottom;
                for (const auto &button : buttons) {
                    if ((button.id >= BUTTON_OPEN && button.id < BUTTON_OPEN + static_cast<int>(KIND_COUNT)) ||
                        button.id == BUTTON_OPEN_FOLDER) {
                        lowestButton = std::min<int>(lowestButton, button.rect.top);
                    }
                }
                SelectObject(dc, smallFont);
                const int lineHeight = px(20);
                const auto textWidth = [dc](const std::wstring &text) {
                    SIZE size{};
                    GetTextExtentPoint32W(dc, text.c_str(), static_cast<int>(text.size()), &size);
                    return static_cast<int>(size.cx);
                };
                // Each label at the left edge and its value at the right, so the figures line up down the panel.
                for (const auto &[label, value] : details) {
                    const std::wstring shownLabel = UiLanguage::text(label);
                    const std::wstring shownValue = UiLanguage::text(value);
                    const int labelWidth = std::min(textWidth(shownLabel), right - left);
                    // A value too long to sit beside its label, such as a folder's path, takes the line below at full width.
                    const bool stacked = labelWidth + px(12) + textWidth(shownValue) > right - left;
                    const int rowHeight = stacked ? 2 * lineHeight : lineHeight;
                    if (y + rowHeight > lowestButton - px(8)) {
                        break;
                    }
                    SetTextColor(dc, theme.rangeText);
                    RECT labelRect{left, y, left + labelWidth, y + lineHeight};
                    DrawTextW(dc, shownLabel.c_str(), -1, &labelRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
                    SetTextColor(dc, theme.text);
                    RECT valueRect = stacked ? RECT{left, y + lineHeight, right, y + 2 * lineHeight}
                                             : RECT{labelRect.right + px(12), y, right, y + lineHeight};
                    DrawTextW(dc, shownValue.c_str(), -1, &valueRect,
                              (stacked ? DT_LEFT : DT_RIGHT) | DT_VCENTER | DT_SINGLELINE | DT_PATH_ELLIPSIS | DT_NOPREFIX);
                    y += rowHeight + px(4);
                }
                SelectObject(dc, font);
            }

            // Asks the worker for an entry's thumbnail the first time it is seen.
            Thumb &thumbFor(const Entry &entry) {
                Thumb &thumb = thumbs[entry.key()];
                thumb.seen = paintCount;
                if (thumb.state == ThumbState::UNKNOWN) {
                    thumb.state = ThumbState::LOADING;
                    worker.enqueue(entry.key(), entry.source);
                }
                return thumb;
            }

            // The folder picture side pixels square over the well color back; null if it cannot be decoded.
            HBITMAP folderBitmap(const int side, const COLORREF back) {
                for (const auto &kept : folderBitmaps) {
                    if (kept.side == side && kept.back == back) {
                        return kept.bitmap;
                    }
                }
                if (folderPicture.empty()) {
                    folderPicture = cv::imdecode(cv::_InputArray(FOLDER_PNG, static_cast<int>(sizeof(FOLDER_PNG))),
                                                 cv::IMREAD_UNCHANGED);
                    if (folderPicture.type() != CV_8UC4) {
                        folderPicture.release();
                        return nullptr;
                    }
                }
                // Blended over the well at full size first, so shrinking never pulls the transparent black into the edge.
                const int backB = GetBValue(back);
                const int backG = GetGValue(back);
                const int backR = GetRValue(back);
                cv::Mat flat(folderPicture.size(), CV_8UC3);
                for (int y = 0; y < flat.rows; ++y) {
                    const unsigned char *in = folderPicture.ptr<unsigned char>(y);
                    unsigned char *out = flat.ptr<unsigned char>(y);
                    for (int x = 0; x < flat.cols; ++x, in += 4, out += 3) {
                        const int alpha = in[3];
                        out[0] = static_cast<unsigned char>((in[0] * alpha + backB * (255 - alpha) + 127) / 255);
                        out[1] = static_cast<unsigned char>((in[1] * alpha + backG * (255 - alpha) + 127) / 255);
                        out[2] = static_cast<unsigned char>((in[2] * alpha + backR * (255 - alpha) + 127) / 255);
                    }
                }
                cv::Mat sized;
                cv::resize(flat, sized, cv::Size(side, side), 0, 0, side < flat.cols ? cv::INTER_AREA : cv::INTER_CUBIC);
                if (folderBitmaps.size() >= MAX_FOLDER_BITMAPS) {
                    if (folderBitmaps.front().bitmap != nullptr) {
                        DeleteObject(folderBitmaps.front().bitmap);
                    }
                    folderBitmaps.erase(folderBitmaps.begin());
                }
                folderBitmaps.push_back({side, back, ThumbnailBitmap::create(sized)});
                return folderBitmaps.back().bitmap;
            }

            // The folder picture in the middle of a card's picture box, its body at most 45% of the box tall.
            void paintFolderGlyph(const HDC dc, const RECT &box, const COLORREF back) {
                const int side = std::max(1, std::min<int>((box.bottom - box.top) * 54 / 100, (box.right - box.left) * 42 / 100));
                const int left = (box.left + box.right - side) / 2;
                const int top = (box.top + box.bottom - side) / 2;
                ThumbnailBitmap::draw(dc, folderBitmap(side, back), RECT{left, top, left + side, top + side});
            }

            void paintThumbnail(const HDC dc, const Entry &entry, const RECT &box, const bool card) {
                const auto &theme = settingsTheme();
                const HBRUSH well = CreateSolidBrush(theme.controlDisabledFace);
                FillRect(dc, &box, well);
                DeleteObject(well);
                if (entry.isFolder) {
                    paintFolderGlyph(dc, box, theme.controlDisabledFace);
                    return;
                }
                const Thumb &thumb = thumbFor(entry);
                if (thumb.state == ThumbState::READY && thumb.bitmap != nullptr) {
                    ThumbnailBitmap::draw(dc, thumb.bitmap, box);
                    return;
                }
                SelectObject(dc, card ? smallFont : font);
                SetTextColor(dc, theme.rangeText);
                RECT text = box;
                const std::wstring note = UiLanguage::text(thumb.state == ThumbState::NONE ? L"No thumbnail" : L"Loading...");
                DrawTextW(dc, note.c_str(), -1, &text, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }

            void paintGrid() {
                ++paintCount;
                PAINTSTRUCT ps;
                const HDC target = BeginPaint(grid, &ps);
                RECT client{};
                GetClientRect(grid, &client);
                const HDC dc = CreateCompatibleDC(target);
                const HBITMAP buffer = CreateCompatibleBitmap(target, std::max<int>(1, client.right), std::max<int>(1, client.bottom));
                const auto oldBitmap = SelectObject(dc, buffer);
                const auto &theme = settingsTheme();
                const HBRUSH back = CreateSolidBrush(theme.background);
                FillRect(dc, &client, back);
                DeleteObject(back);
                SetBkMode(dc, TRANSPARENT);
                const auto oldFont = SelectObject(dc, font);

                if (shown.empty()) {
                    SetTextColor(dc, theme.rangeText);
                    RECT text = client;
                    const wchar_t *empty = filter == Filter::RECENT ? L"Files you open or save appear here."
                                           : entries.empty()        ? L"No saved files in this folder."
                                                                    : L"Nothing matches.";
                    const std::wstring shownText = UiLanguage::text(empty);
                    DrawTextW(dc, shownText.c_str(), -1, &text, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                }

                const CardLayout layout = cardLayout();
                const bool focused = GetFocus() == grid;
                for (int i = 0; i < static_cast<int>(shown.size()); ++i) {
                    const RECT card = cardRect(i, layout);
                    if (card.bottom < 0) {
                        continue;
                    }
                    if (card.top > client.bottom) {
                        break;
                    }
                    const Entry &entry = entries[shown[static_cast<size_t>(i)]];
                    const bool isSelected = i == selected;
                    const float radius = static_cast<float>(px(8));
                    // A card is a step up from the window in dark mode and white with a hairline in light mode.
                    const COLORREF face = !darkSettingsMode() ? theme.buttonFace
                                          : isSelected            ? theme.buttonFacePressed
                                                                  : theme.controlDisabledFace;
                    roundFrame(dc, card, radius, face, std::nullopt);

                    // The picture runs to the card's top edge, its upper corners rounded with the card.
                    const RECT box{card.left, card.top, card.right, card.top + layout.thumbHeight};
                    paintThumbnail(dc, entry, box, true);
                    roundCorners(dc, box, radius, theme.background, false);
                    if (isSelected) {
                        roundFrame(dc, card, radius, std::nullopt,
                                   focused ? theme.previewBorderSelected : theme.radioSelectedBorder, static_cast<float>(px(2)));
                    } else {
                        roundFrame(dc, card, radius, std::nullopt, darkSettingsMode() ? theme.sliderTrackDisabled : theme.sectionFrame);
                    }

                    const int textLeft = card.left + px(10);
                    const int textRight = card.right - px(10);
                    SelectObject(dc, font);
                    SetTextColor(dc, theme.text);
                    const RECT name = nameRect(card, layout);
                    if (renameEdit != nullptr && folderKey(entry.source) == folderKey(renaming)) {
                        // The name being typed sits in a field like Search's, its edge in the focus color.
                        const RECT field{name.left - px(6), name.top - px(4), name.right + px(6), name.bottom + px(4)};
                        roundFrame(dc, field, static_cast<float>(px(6)), theme.textFieldBackground, theme.previewBorderSelected);
                    } else {
                        RECT text = name;
                        DrawTextW(dc, entry.name.c_str(), -1, &text, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
                    }
                    const std::wstring kinds = entry.isFolder ? UiLanguage::text(L"Folder") : filesText(entry);
                    SelectObject(dc, smallFont);
                    SetTextColor(dc, theme.rangeText);
                    // The date keeps its place at the right; the kinds before it are shortened first when the card is narrow.
                    const std::wstring date = shortDate(entry.newest);
                    SIZE dateSize{};
                    GetTextExtentPoint32W(dc, date.c_str(), static_cast<int>(date.size()), &dateSize);
                    const int metaTop = name.bottom + px(4);
                    RECT dateRect{std::max<int>(textLeft, textRight - dateSize.cx), metaTop, textRight, metaTop + px(18)};
                    DrawTextW(dc, date.c_str(), -1, &dateRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
                    RECT kindsRect{textLeft, metaTop, std::max<int>(textLeft, dateRect.left - px(8)), metaTop + px(18)};
                    DrawTextW(dc, kinds.c_str(), -1, &kindsRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
                }

                SelectObject(dc, oldFont);
                BitBlt(target, 0, 0, client.right, client.bottom, dc, 0, 0, SRCCOPY);
                SelectObject(dc, oldBitmap);
                DeleteObject(buffer);
                DeleteDC(dc);
                EndPaint(grid, &ps);
                trimBitmaps();
            }

            // Lets go of the bitmaps seen longest ago once too many are held; they are made again from the cache when needed.
            void trimBitmaps() {
                size_t held = 0;
                for (const auto &[key, thumb] : thumbs) {
                    held += thumb.bitmap != nullptr;
                }
                if (held <= MAX_BITMAPS) {
                    return;
                }
                std::vector<std::pair<uint64_t, std::wstring>> ages;
                for (const auto &[key, thumb] : thumbs) {
                    if (thumb.bitmap != nullptr) {
                        ages.emplace_back(thumb.seen, key);
                    }
                }
                std::ranges::sort(ages);
                for (size_t i = 0; i + MAX_BITMAPS * 3 / 4 < ages.size(); ++i) {
                    Thumb &thumb = thumbs[ages[i].second];
                    if (thumb.seen == paintCount) {
                        break;
                    }
                    DeleteObject(thumb.bitmap);
                    thumb.bitmap = nullptr;
                    thumb.state = ThumbState::UNKNOWN;
                }
            }

            void thumbnailsReady() {
                for (auto &[key, picture] : worker.takeDone()) {
                    const auto found = thumbs.find(key);
                    if (found == thumbs.end()) {
                        continue;
                    }
                    Thumb &thumb = found->second;
                    if (thumb.bitmap != nullptr) {
                        DeleteObject(thumb.bitmap);
                    }
                    thumb.bitmap = ThumbnailBitmap::create(picture);
                    thumb.state = thumb.bitmap != nullptr ? ThumbState::READY : ThumbState::NONE;
                }
                InvalidateRect(grid, nullptr, FALSE);
                InvalidateRect(window, &detailsRect, FALSE);
            }

            [[nodiscard]] int hitCard(const POINT point) const {
                const CardLayout layout = cardLayout();
                for (int i = 0; i < static_cast<int>(shown.size()); ++i) {
                    const RECT card = cardRect(i, layout);
                    if (PtInRect(&card, point)) {
                        return i;
                    }
                }
                return -1;
            }

            [[nodiscard]] int hitButton(const POINT point) const {
                for (size_t i = 0; i < buttons.size(); ++i) {
                    if (PtInRect(&buttons[i].rect, point)) {
                        return static_cast<int>(i);
                    }
                }
                return -1;
            }

            // The name a new save starts with: the open document's, or one made from the date and time.
            [[nodiscard]] std::wstring defaultName() const {
                if (const auto &document = scene.getConfigDocumentPath(); !document.empty()) {
                    return document.stem().wstring();
                }
                SYSTEMTIME now{};
                GetLocalTime(&now);
                return std::format(L"rff_{:04}{:02}{:02}_{:02}{:02}{:02}", now.wYear, now.wMonth, now.wDay, now.wHour,
                                   now.wMinute, now.wSecond);
            }

            // The name typed for saving, trimmed; empty when there is none or it cannot be a file name.
            [[nodiscard]] std::wstring typedName() const {
                std::wstring name(static_cast<size_t>(GetWindowTextLengthW(nameEdit)) + 1, L'\0');
                GetWindowTextW(nameEdit, name.data(), static_cast<int>(name.size()));
                name.resize(wcslen(name.c_str()));
                return checkedName(std::move(name), true);
            }

            // A name trimmed and checked against what Windows allows, empty after saying why when it cannot be one.
            // For a save, an extension typed with the name is left off, since each save adds its own.
            [[nodiscard]] std::wstring checkedName(std::wstring name, const bool forSave) const {
                const auto first = name.find_first_not_of(L" \t");
                if (first == std::wstring::npos) {
                    NativeDialogs::message(window, L"Type a name to save under.", L"Gallery", MB_OK | MB_ICONINFORMATION);
                    return {};
                }
                const auto last = name.find_last_not_of(L" \t.");
                if (last == std::wstring::npos || last < first) {
                    NativeDialogs::message(window, L"Type a name to save under.", L"Gallery", MB_OK | MB_ICONINFORMATION);
                    return {};
                }
                name = name.substr(first, last - first + 1);
                if (name.find_first_of(L"\\/:*?\"<>|") != std::wstring::npos) {
                    NativeDialogs::message(window, L"A name cannot contain any of these: \\ / : * ? \" < > |", L"Gallery",
                                           MB_OK | MB_ICONINFORMATION);
                    return {};
                }
                // An extension typed with the name is the one each save adds itself, so it is left off.
                const std::wstring typedExtension = lower(std::filesystem::path(name).extension().wstring());
                if (forSave && (GalleryIO::kindOf(name) ||
                                typedExtension == std::format(L".{}", Constants::Extension::THUMBNAIL))) {
                    name = std::filesystem::path(name).stem().wstring();
                    if (const auto end = name.find_last_not_of(L" \t."); end == std::wstring::npos) {
                        NativeDialogs::message(window, L"Type a name to save under.", L"Gallery", MB_OK | MB_ICONINFORMATION);
                        return {};
                    } else {
                        name.resize(end + 1);
                    }
                }
                // Windows keeps these for devices, with any extension after them.
                std::wstring device = name.substr(0, name.find(L'.'));
                device.erase(device.find_last_not_of(L' ') + 1);
                std::ranges::transform(device, device.begin(), [](const wchar_t c) { return static_cast<wchar_t>(std::towupper(c)); });
                static constexpr std::array<std::wstring_view, 4> DEVICES = {L"CON", L"PRN", L"AUX", L"NUL"};
                const bool numbered = device.size() == 4 && (device.starts_with(L"COM") || device.starts_with(L"LPT")) &&
                                      ((device[3] >= L'0' && device[3] <= L'9') || device[3] == L'¹' ||
                                       device[3] == L'²' || device[3] == L'³');
                if (numbered || std::ranges::find(DEVICES, device) != DEVICES.end()) {
                    NativeDialogs::message(window, L"Windows keeps this name for itself. Choose another one.", L"Gallery",
                                           MB_OK | MB_ICONINFORMATION);
                    return {};
                }
                return name;
            }

            void save(const CallbackFile::SaveKind kind) {
                if (IOUtilities::isModalDialogOpen()) {
                    return;
                }
                if (scene.isLongJobBusy()) {
                    NativeDialogs::message(window, L"RFF_Super is busy with a video or keyframe job.\n\nSave after it ends.",
                                           L"Gallery", MB_OK | MB_ICONINFORMATION);
                    return;
                }
                const std::wstring name = typedName();
                if (name.empty()) {
                    SetFocus(nameEdit);
                    return;
                }
                std::error_code error;
                if (!std::filesystem::is_directory(folder, error)) {
                    NativeDialogs::message(window, L"This folder is no longer there. Choose another one.", L"Gallery",
                                           MB_OK | MB_ICONWARNING);
                    return;
                }
                if (!CallbackFile::saveInto(scene, kind, folder, name, GalleryIO::state().saveMapWithWork)) {
                    return;
                }
                if (!IsWindow(window)) {
                    return;
                }
                // What was just saved is shown and chosen, in the folder it went to.
                if (filter == Filter::RECENT) {
                    setFilter(Filter::ALL);
                }
                SetWindowTextW(search, L"");
                rescan();
                for (size_t i = 0; i < shown.size(); ++i) {
                    if (!entries[shown[i]].isFolder && lower(entries[shown[i]].name) == lower(name)) {
                        selected = static_cast<int>(i);
                        ensureVisible();
                        selectionChanged();
                        InvalidateRect(grid, nullptr, FALSE);
                        break;
                    }
                }
                SetTimer(window, REFRESH_TIMER, 1000, nullptr);
            }

            void chooseOtherSave() {
                ThemedPopup popup(window, dpi);
                for (size_t i = 0; i < OTHER_SAVES.size(); ++i) {
                    popup.item(popup.top(), static_cast<UINT>(i + 1), UiLanguage::text(OTHER_SAVES[i].label));
                }
                RECT anchor{};
                for (const auto &button : buttons) {
                    if (button.id == BUTTON_OTHER_SAVES) {
                        anchor = button.rect;
                    }
                }
                POINT at{anchor.left, anchor.top};
                ClientToScreen(window, &at);
                const UINT chosen = popup.track(TPM_LEFTALIGN | TPM_BOTTOMALIGN, at, window);
                if (chosen >= 1 && chosen <= OTHER_SAVES.size()) {
                    save(OTHER_SAVES[chosen - 1].kind);
                }
            }

            // The Settings button's menu: Close After Opening and the thumbnail choices, dropped below the button.
            void chooseSetting() {
                enum : UINT { CLOSE_AFTER = 1, ATTACH_SAVES = 2, SHOW_EXTENSIONS = 3, SAVE_MAP = 4, WIDTH_FIRST = 10 };
                ThemedPopup popup(window, dpi);
                const HMENU top = popup.top();
                popup.item(top, CLOSE_AFTER, UiLanguage::text(L"Close After Opening"), !saving, GalleryIO::state().closeAfterOpen);
                popup.item(top, SHOW_EXTENSIONS, UiLanguage::text(L"Show File Extensions"), true, GalleryIO::state().showExtensions);
                popup.separator(top);
                // Only Save Work reads it, so Gallery (Open) shows it greyed out, as Close After Opening is in Gallery (Save).
                popup.item(top, SAVE_MAP, UiLanguage::text(L"Save Work Also Saves the Map"), saving, GalleryIO::state().saveMapWithWork);
                popup.item(top, ATTACH_SAVES, UiLanguage::text(L"Attach Thumbnails When Saving"), true, PreferencesIO::thumbnailsForSaves());
                const HMENU widthMenu = popup.menu();
                for (UINT i = 0; i <= PreferencesIO::MAX_THUMBNAIL_SIZE; ++i) {
                    popup.item(widthMenu, WIDTH_FIRST + i, UiLanguage::text(std::format(L"{} px", 128u << i)));
                }
                CheckMenuRadioItem(widthMenu, 0, PreferencesIO::MAX_THUMBNAIL_SIZE,
                                   std::min<UINT>(PreferencesIO::thumbnailSize(), PreferencesIO::MAX_THUMBNAIL_SIZE), MF_BYPOSITION);
                popup.sub(top, widthMenu, UiLanguage::text(L"Thumbnail Width"));
                RECT anchor{};
                for (const auto &button : buttons) {
                    if (button.id == BUTTON_SETTINGS) {
                        anchor = button.rect;
                    }
                }
                POINT at{anchor.right, anchor.bottom};
                ClientToScreen(window, &at);
                const UINT chosen = popup.track(TPM_RIGHTALIGN | TPM_TOPALIGN, at, window);
                if (chosen == CLOSE_AFTER) {
                    GalleryIO::state().closeAfterOpen = !GalleryIO::state().closeAfterOpen;
                    GalleryIO::save();
                } else if (chosen == SHOW_EXTENSIONS) {
                    GalleryIO::state().showExtensions = !GalleryIO::state().showExtensions;
                    GalleryIO::save();
                    selectionChanged();
                    InvalidateRect(grid, nullptr, FALSE);
                } else if (chosen == SAVE_MAP) {
                    GalleryIO::state().saveMapWithWork = !GalleryIO::state().saveMapWithWork;
                    GalleryIO::save();
                } else if (chosen == ATTACH_SAVES) {
                    PreferencesIO::thumbnailsForSaves() = !PreferencesIO::thumbnailsForSaves();
                    PreferencesIO::save();
                } else if (chosen >= WIDTH_FIRST && chosen <= WIDTH_FIRST + PreferencesIO::MAX_THUMBNAIL_SIZE) {
                    PreferencesIO::thumbnailSize() = static_cast<uint8_t>(chosen - WIDTH_FIRST);
                    PreferencesIO::save();
                }
            }

            // The cards' right-click menu, at the pointer or, from the keyboard, at the chosen card. A right-click on a
            // card chooses it first. Both are greyed out under Recent Files, which shows no one folder to change.
            void showGridMenu(POINT at) {
                endRename(true);
                int target = -1;
                if (at.x == -1 && at.y == -1) {
                    const CardLayout layout = cardLayout();
                    const RECT card = selected >= 0 ? cardRect(selected, layout) : RECT{layout.gap, layout.gap, layout.gap, layout.gap};
                    at = {card.left + px(8), card.top + px(8)};
                    ClientToScreen(grid, &at);
                    target = selected;
                } else {
                    POINT inside = at;
                    ScreenToClient(grid, &inside);
                    target = hitCard(inside);
                    if (target >= 0) {
                        select(target);
                    }
                }
                enum : UINT { NEW_FOLDER = 1, RENAME = 2 };
                const bool canMake = filter != Filter::RECENT && !folder.empty();
                const bool canChange = canMake && target >= 0 && target < static_cast<int>(shown.size());
                ThemedPopup popup(window, dpi);
                const HMENU top = popup.top();
                popup.item(top, NEW_FOLDER, UiLanguage::text(L"New Folder"), canMake);
                popup.item(top, RENAME, UiLanguage::text(L"Rename"), canChange);
                const UINT chosen = popup.track(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, at, window);
                if (chosen == NEW_FOLDER && canMake) {
                    makeFolder();
                } else if (chosen == RENAME && canChange) {
                    renameChosen();
                }
            }

            // Rename moves files, so it waits while a video or keyframe job may be reading them.
            [[nodiscard]] bool canChangeFiles() const {
                if (IOUtilities::isModalDialogOpen() || filter == Filter::RECENT || folder.empty()) {
                    return false;
                }
                if (scene.isLongJobBusy()) {
                    NativeDialogs::message(window, L"RFF_Super is busy with a video or keyframe job.\n\nTry again after it ends.",
                                           L"Gallery", MB_OK | MB_ICONINFORMATION);
                    return false;
                }
                // An image saved while the view calculates is written later under the name it was saved with.
                if (scene.getRequests().createImageRequested.load(std::memory_order_acquire)) {
                    NativeDialogs::message(window, L"An image is still waiting to be saved.\n\nTry again once the view has finished calculating.",
                                           L"Gallery", MB_OK | MB_ICONINFORMATION);
                    return false;
                }
                return true;
            }

            // The files one card stands for: those of the folder under its name that the Gallery lists, and its thumbnail.
            [[nodiscard]] static std::vector<std::filesystem::path> workFiles(const std::filesystem::path &place, const std::wstring &stem) {
                std::vector<std::filesystem::path> files;
                const std::wstring key = lower(stem);
                const std::wstring thumbnailExtension = std::format(L".{}", Constants::Extension::THUMBNAIL);
                for (const Listed &item : listFolder(place)) {
                    if ((item.attributes & FILE_ATTRIBUTE_DIRECTORY) != 0 || item.name.starts_with(L".rff-")) {
                        continue;
                    }
                    const std::filesystem::path file = place / item.name;
                    if (lower(file.stem().wstring()) == key &&
                        (GalleryIO::kindOf(file) || lower(file.extension().wstring()) == thumbnailExtension)) {
                        files.push_back(file);
                    }
                }
                return files;
            }

            // Where path is after from moved to to: to itself, a path inside it, or nothing when path was neither.
            [[nodiscard]] static std::optional<std::filesystem::path> movedPath(const std::filesystem::path &path, const std::filesystem::path &from,
                                                                                const std::filesystem::path &to) {
                const std::wstring key = folderKey(from);
                const std::wstring normal = trimmedFolder(path.lexically_normal()).wstring();
                const std::wstring pathKey = lower(normal);
                if (pathKey == key) {
                    return to;
                }
                if (pathKey.size() > key.size() + 1 && pathKey.starts_with(key) &&
                    (pathKey[key.size()] == L'\\' || pathKey[key.size()] == L'/')) {
                    return to / normal.substr(key.size() + 1);
                }
                return std::nullopt;
            }

            // Keeps the recent files and the back and forward folders on what was renamed.
            void followMove(const std::filesystem::path &from, const std::filesystem::path &to) {
                bool changed = false;
                for (auto &file : GalleryIO::state().recent) {
                    if (const auto moved = movedPath(file, from, to)) {
                        file = *moved;
                        changed = true;
                    }
                }
                if (changed) {
                    GalleryIO::save();
                }
                for (auto *history : {&backFolders, &forwardFolders}) {
                    for (auto &place : *history) {
                        if (const auto moved = movedPath(place, from, to)) {
                            place = *moved;
                        }
                    }
                }
            }

            // F2 or Rename: the chosen card's name is put up to be typed over.
            void renameChosen() {
                const Entry *entry = selectedEntry();
                if (entry == nullptr || !canChangeFiles()) {
                    return;
                }
                const Entry chosen = *entry;
                startRename(chosen);
            }

            // Where a card of this file or folder is among those shown, or -1.
            [[nodiscard]] int shownIndexOf(const std::filesystem::path &path) const {
                const std::wstring key = folderKey(path);
                for (size_t i = 0; i < shown.size(); ++i) {
                    if (folderKey(entries[shown[i]].source) == key) {
                        return static_cast<int>(i);
                    }
                }
                return -1;
            }

            // Makes a folder named New folder, numbered as Explorer numbers it when the name is taken, chooses its card
            // and puts its name up to be typed over.
            void makeFolder() {
                endRename(true);
                const std::wstring base = UiLanguage::text(L"New folder");
                std::filesystem::path made;
                for (int n = 1; n < 1000 && made.empty(); ++n) {
                    const std::filesystem::path next = folder / (n == 1 ? base : std::format(L"{} ({})", base, n));
                    if (CreateDirectoryW(next.c_str(), nullptr)) {
                        made = next;
                    } else if (GetLastError() != ERROR_ALREADY_EXISTS) {
                        break;
                    }
                }
                if (made.empty()) {
                    NativeDialogs::message(window, L"Cannot make a folder here.", L"Gallery", MB_OK | MB_ICONWARNING);
                    return;
                }
                rescan();
                // A search that hides the new folder is cleared, so its card is there to be named.
                if (shownIndexOf(made) < 0 && search != nullptr && GetWindowTextLengthW(search) > 0) {
                    SetWindowTextW(search, L"");
                }
                if (const int index = shownIndexOf(made); index >= 0) {
                    select(index);
                    const Entry chosen = entries[shown[static_cast<size_t>(index)]];
                    startRename(chosen);
                }
            }

            // Puts a box over a card's name for typing a new one. Enter or clicking elsewhere keeps what was typed;
            // Esc keeps the name it has.
            void startRename(const Entry &entry) {
                endRename(true);
                if (shownIndexOf(entry.source) < 0) {
                    return;
                }
                renaming = entry.source;
                renamingFolder = entry.isFolder;
                renamingName = entry.name;
                renameEdit = CreateWindowExW(0, L"EDIT", renamingName.c_str(), WS_CHILD | ES_AUTOHSCROLL, 0, 0, 1, 1, grid,
                                             nullptr, GetModuleHandleW(nullptr), nullptr);
                SendMessageW(renameEdit, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
                SendMessageW(renameEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, 0);
                applyDarkThemeClass(renameEdit, true);
                SetWindowSubclass(renameEdit, renameProc, 3, reinterpret_cast<DWORD_PTR>(this));
                placeRename();
                ShowWindow(renameEdit, SW_SHOW);
                SetFocus(renameEdit);
                SendMessageW(renameEdit, EM_SETSEL, 0, -1);
                InvalidateRect(grid, nullptr, FALSE);
            }

            // Keeps the name box over its card's name as the cards scroll or move to other columns.
            void placeRename() {
                if (renameEdit == nullptr) {
                    return;
                }
                const int index = shownIndexOf(renaming);
                if (index < 0) {
                    ShowWindow(renameEdit, SW_HIDE);
                    return;
                }
                const CardLayout layout = cardLayout();
                const RECT name = nameRect(cardRect(index, layout), layout);
                SetWindowPos(renameEdit, nullptr, name.left, name.top, name.right - name.left, name.bottom - name.top,
                             SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
            }

            // Takes the name box away and, when keep is set, gives the folder or the card's files the name typed. A name
            // left empty, or only spaces and dots, leaves the name as it was, as Explorer does.
            void endRename(const bool keep) {
                if (renameEdit == nullptr) {
                    return;
                }
                const HWND edit = std::exchange(renameEdit, nullptr);
                const std::filesystem::path from = std::exchange(renaming, {});
                const std::wstring oldName = std::exchange(renamingName, {});
                std::wstring typed(static_cast<size_t>(GetWindowTextLengthW(edit)) + 1, L'\0');
                GetWindowTextW(edit, typed.data(), static_cast<int>(typed.size()));
                typed.resize(wcslen(typed.c_str()));
                const bool hadFocus = GetFocus() == edit;
                DestroyWindow(edit);
                if (hadFocus) {
                    SetFocus(grid);
                }
                InvalidateRect(grid, nullptr, FALSE);
                if (!keep || typed.find_first_not_of(L" \t.") == std::wstring::npos) {
                    return;
                }
                // A card's name is its files' name without their extensions, so one typed with it is left off.
                const std::wstring name = checkedName(typed, !renamingFolder);
                if (name.empty() || name == oldName) {
                    return;
                }
                const Entry *current = selectedEntry();
                const bool stillChosen = current != nullptr && folderKey(current->source) == folderKey(from);
                std::filesystem::path to;
                if (renamingFolder) {
                    to = from.parent_path() / name;
                    if (!moveWaiting(from, to)) {
                        const DWORD error = GetLastError();
                        NativeDialogs::message(window,
                                               error == ERROR_ALREADY_EXISTS || error == ERROR_FILE_EXISTS
                                                   ? L"A folder or file with this name is already here."
                                                   : L"Cannot rename this folder.",
                                               L"Gallery", MB_OK | MB_ICONWARNING);
                        return;
                    }
                    followMove(from, to);
                } else {
                    to = renameWork(from.parent_path(), oldName, name, from);
                    if (to.empty()) {
                        return;
                    }
                }
                // Given its new path first, the card stays chosen through the read below without the cards scrolling to the top.
                if (stillChosen) {
                    entries[shown[static_cast<size_t>(selected)]].source = to;
                }
                rescan();
                if (const int index = shownIndexOf(to); stillChosen && index >= 0) {
                    select(index);
                    // Gallery (Save) offers the new name, which choosing the card already chosen would not put in Name.
                    if (saving && !renamingFolder && nameEdit != nullptr) {
                        SetWindowTextW(nameEdit, name.c_str());
                    }
                }
            }

            // Gives every file of a card the new name and answers where source went; all or none of them are renamed.
            std::filesystem::path renameWork(const std::filesystem::path &place, const std::wstring &oldName, const std::wstring &name,
                                             const std::filesystem::path &source) {
                const std::vector<std::filesystem::path> files = workFiles(place, oldName);
                if (files.empty()) {
                    NativeDialogs::message(window, L"This file is no longer there.", L"Gallery", MB_OK | MB_ICONWARNING);
                    rescan();
                    return {};
                }
                // Only the case changing leaves each file standing where its new name is.
                const bool sameFiles = lower(name) == lower(oldName);
                // Another card under the new name would take these files into it, so that name counts as taken too.
                if (!sameFiles && !workFiles(place, name).empty()) {
                    NativeDialogs::message(window, L"A folder or file with this name is already here.", L"Gallery", MB_OK | MB_ICONWARNING);
                    return {};
                }
                std::vector<std::pair<std::filesystem::path, std::filesystem::path>> moves;
                for (const auto &file : files) {
                    std::filesystem::path target = place / (name + file.extension().wstring());
                    if (!sameFiles && GetFileAttributesW(target.c_str()) != INVALID_FILE_ATTRIBUTES) {
                        NativeDialogs::message(window, L"A folder or file with this name is already here.", L"Gallery",
                                               MB_OK | MB_ICONWARNING);
                        return {};
                    }
                    moves.emplace_back(file, std::move(target));
                }
                for (size_t i = 0; i < moves.size(); ++i) {
                    if (!moveWaiting(moves[i].first, moves[i].second)) {
                        for (size_t back = i; back-- > 0;) {
                            static_cast<void>(moveWaiting(moves[back].second, moves[back].first));
                        }
                        NativeDialogs::message(window, L"Cannot rename these files.", L"Gallery", MB_OK | MB_ICONWARNING);
                        rescan();
                        return {};
                    }
                }
                std::filesystem::path moved;
                for (const auto &[file, target] : moves) {
                    followMove(file, target);
                    if (lower(file.wstring()) == lower(source.wstring())) {
                        moved = target;
                    }
                }
                return moved.empty() ? moves.front().second : moved;
            }

            static LRESULT CALLBACK renameProc(const HWND hwnd, const UINT message, const WPARAM wparam, const LPARAM lparam,
                                               UINT_PTR, const DWORD_PTR data) {
                auto *self = reinterpret_cast<Gallery *>(data);
                // The box is taken away once this message is done with, not in the middle of it.
                const auto finish = [&](const bool keep) {
                    PostMessageW(self->grid, WM_RENAME_DONE, keep, reinterpret_cast<LPARAM>(hwnd));
                };
                switch (message) {
                case WM_KEYDOWN:
                    if (wparam == VK_RETURN || wparam == VK_ESCAPE || wparam == VK_TAB) {
                        finish(wparam != VK_ESCAPE);
                        return 0;
                    }
                    break;
                case WM_CHAR:
                    if (wparam == L'\t' || wparam == L'\r' || wparam == 0x1B) {
                        return 0;
                    }
                    break;
                case WM_KILLFOCUS:
                    if (self->renameEdit == hwnd) {
                        finish(true);
                    }
                    break;
                case WM_NCDESTROY:
                    RemoveWindowSubclass(hwnd, renameProc, 3);
                    break;
                default:
                    break;
                }
                return DefSubclassProc(hwnd, message, wparam, lparam);
            }

            // Turns the window into Gallery (Save) or Gallery (Open).
            void setSaving(const bool save) {
                if (save == saving) {
                    return;
                }
                saving = save;
                SetWindowTextW(window, UiLanguage::text(saving ? L"Gallery (Save)" : L"Gallery (Open)").c_str());
                if (!saving && GetFocus() == nameEdit) {
                    SetFocus(grid);
                }
                layout();
                InvalidateRect(window, nullptr, FALSE);
            }

            void focusName() {
                SetFocus(nameEdit);
                SendMessageW(nameEdit, EM_SETSEL, 0, -1);
            }

            void press(const int id) {
                if (id == BUTTON_SORT) {
                    sort = sort == 1 ? 0 : 1;
                    GalleryIO::state().sort = sort;
                    std::wstring keep;
                    if (const Entry *current = selectedEntry()) {
                        keep = lower(current->source.wstring());
                    }
                    refilter(keep);
                    layout();
                    InvalidateRect(window, nullptr, FALSE);
                } else if (id == BUTTON_SETTINGS) {
                    chooseSetting();
                } else if (id == BUTTON_SAVE_WORK) {
                    save(CallbackFile::SaveKind::WORK);
                } else if (id == BUTTON_OTHER_SAVES) {
                    chooseOtherSave();
                } else if (id >= BUTTON_OPEN && id < BUTTON_OPEN + static_cast<int>(KIND_COUNT)) {
                    openKind(static_cast<Kind>(id - BUTTON_OPEN));
                } else if (id >= BUTTON_FILTER && id < BUTTON_FILTER + static_cast<int>(Filter::COUNT)) {
                    setFilter(static_cast<Filter>(id - BUTTON_FILTER));
                } else if (id == BUTTON_OPEN_FOLDER) {
                    openPrimary();
                } else if (id >= BUTTON_CRUMB && id < BUTTON_CRUMB + static_cast<int>(crumbPaths.size())) {
                    const std::filesystem::path target = crumbPaths[static_cast<size_t>(id - BUTTON_CRUMB)];
                    showFolder(target);
                }
            }

            [[nodiscard]] int visibleRows() const {
                RECT client{};
                GetClientRect(grid, &client);
                const CardLayout layout = cardLayout();
                return std::max(1, static_cast<int>(client.bottom) / (layout.height + layout.gap));
            }

            bool gridKey(const WPARAM key) {
                const CardLayout layout = cardLayout();
                switch (key) {
                case VK_LEFT: select(selected - 1); return true;
                case VK_RIGHT: select(selected + 1); return true;
                case VK_UP: select(selected - layout.columns); return true;
                case VK_DOWN: select(selected + layout.columns); return true;
                case VK_PRIOR: select(selected - layout.columns * visibleRows()); return true;
                case VK_NEXT: select(selected + layout.columns * visibleRows()); return true;
                case VK_HOME: select(0); return true;
                case VK_END: select(static_cast<int>(shown.size()) - 1); return true;
                case VK_RETURN: openPrimary(); return true;
                case VK_F2: renameChosen(); return true;
                default: return false;
                }
            }

            // Keys every part of the window answers: F5 reads the folder again, Ctrl+F goes to the search field, Tab between it and the grid.
            bool commonKey(const HWND from, const WPARAM key) {
                if (key == VK_F5) {
                    rescan();
                    return true;
                }
                if (key == 'F' && (GetKeyState(VK_CONTROL) & 0x8000)) {
                    SetFocus(search);
                    SendMessageW(search, EM_SETSEL, 0, -1);
                    return true;
                }
                if (key == VK_TAB) {
                    // Search, cards, then the name to save under, and round again.
                    SetFocus(from == search ? grid : from == grid && saving ? nameEdit : search);
                    return true;
                }
                return false;
            }

            static LRESULT CALLBACK nameProc(const HWND hwnd, const UINT message, const WPARAM wparam, const LPARAM lparam,
                                             UINT_PTR, const DWORD_PTR data) {
                auto *self = reinterpret_cast<Gallery *>(data);
                if (message == WM_KEYDOWN) {
                    if (self->commonKey(hwnd, wparam)) {
                        return 0;
                    }
                    if (wparam == VK_RETURN) {
                        self->save(CallbackFile::SaveKind::WORK);
                        return 0;
                    }
                }
                if (message == WM_CHAR && (wparam == L'\t' || wparam == L'\r' || wparam == 0x1B)) {
                    return 0;
                }
                if (message == WM_SETFOCUS || message == WM_KILLFOCUS) {
                    InvalidateRect(self->window, &self->nameFrame, FALSE);
                }
                return DefSubclassProc(hwnd, message, wparam, lparam);
            }

            static LRESULT CALLBACK searchProc(const HWND hwnd, const UINT message, const WPARAM wparam, const LPARAM lparam,
                                               UINT_PTR, const DWORD_PTR data) {
                auto *self = reinterpret_cast<Gallery *>(data);
                if (message == WM_KEYDOWN) {
                    if (self->commonKey(hwnd, wparam)) {
                        return 0;
                    }
                    if (wparam == VK_DOWN || wparam == VK_RETURN) {
                        SetFocus(self->grid);
                        return 0;
                    }
                    if (wparam == VK_ESCAPE && GetWindowTextLengthW(hwnd) > 0) {
                        SetWindowTextW(hwnd, L"");
                        return 0;
                    }
                }
                if (message == WM_CHAR && (wparam == L'\t' || wparam == L'\r' || wparam == 0x1B)) {
                    return 0;
                }
                if (message == WM_SETFOCUS || message == WM_KILLFOCUS) {
                    InvalidateRect(self->window, &self->searchFrame, FALSE);
                }
                return DefSubclassProc(hwnd, message, wparam, lparam);
            }

            static LRESULT CALLBACK gridProc(const HWND hwnd, const UINT message, const WPARAM wparam, const LPARAM lparam) {
                auto *self = reinterpret_cast<Gallery *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
                if (self == nullptr) {
                    return DefWindowProcW(hwnd, message, wparam, lparam);
                }
                switch (message) {
                case WM_ERASEBKGND:
                    return 1;
                case WM_PAINT:
                    self->paintGrid();
                    return 0;
                case WM_SIZE:
                    self->updateScroll();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    self->placeRename();
                    return 0;
                case WM_CTLCOLOREDIT: {
                    const auto dc = reinterpret_cast<HDC>(wparam);
                    SetTextColor(dc, settingsTheme().text);
                    SetBkColor(dc, settingsTheme().textFieldBackground);
                    return reinterpret_cast<LRESULT>(self->fieldBrush);
                }
                case WM_CONTEXTMENU:
                    self->showGridMenu({GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)});
                    return 0;
                case WM_RENAME_DONE:
                    if (reinterpret_cast<HWND>(lparam) == self->renameEdit) {
                        self->endRename(wparam != 0);
                    }
                    return 0;
                case WM_GETDLGCODE:
                    return DLGC_WANTARROWS | DLGC_WANTCHARS | DLGC_WANTTAB;
                case WM_SETFOCUS:
                case WM_KILLFOCUS:
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                case WM_LBUTTONDOWN: {
                    SetFocus(hwnd);
                    const int hit = self->hitCard({GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)});
                    if (hit >= 0) {
                        self->select(hit);
                    }
                    return 0;
                }
                case WM_LBUTTONDBLCLK: {
                    const int hit = self->hitCard({GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)});
                    if (hit >= 0) {
                        self->select(hit);
                        self->openPrimary();
                    }
                    return 0;
                }
                case WM_MOUSEWHEEL: {
                    // The wheel scrolls a folder list while the pointer is over it, though the cards keep the focus.
                    const HWND under = WindowFromPoint({GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)});
                    if (under != nullptr && self->paneOf(under) != nullptr) {
                        return SendMessageW(under, message, wparam, lparam);
                    }
                    const int delta = GET_WHEEL_DELTA_WPARAM(wparam);
                    const CardLayout layout = self->cardLayout();
                    self->scrollTo(self->scroll - delta * (layout.height + layout.gap) / (WHEEL_DELTA * 2));
                    return 0;
                }
                case WM_VSCROLL: {
                    SCROLLINFO info{sizeof(info)};
                    info.fMask = SIF_ALL;
                    GetScrollInfo(hwnd, SB_VERT, &info);
                    const CardLayout layout = self->cardLayout();
                    int position = self->scroll;
                    switch (LOWORD(wparam)) {
                    case SB_LINEUP: position -= (layout.height + layout.gap) / 3; break;
                    case SB_LINEDOWN: position += (layout.height + layout.gap) / 3; break;
                    case SB_PAGEUP: position -= static_cast<int>(info.nPage); break;
                    case SB_PAGEDOWN: position += static_cast<int>(info.nPage); break;
                    case SB_THUMBTRACK:
                    case SB_THUMBPOSITION: position = info.nTrackPos; break;
                    case SB_TOP: position = 0; break;
                    case SB_BOTTOM: position = info.nMax; break;
                    default: break;
                    }
                    self->scrollTo(position);
                    return 0;
                }
                case WM_KEYDOWN:
                    if (self->commonKey(hwnd, wparam) || self->gridKey(wparam)) {
                        return 0;
                    }
                    break;
                default:
                    break;
                }
                return DefWindowProcW(hwnd, message, wparam, lparam);
            }

            static LRESULT CALLBACK folderProc(const HWND hwnd, const UINT message, const WPARAM wparam, const LPARAM lparam) {
                auto *self = reinterpret_cast<Gallery *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
                FolderPane *pane = self != nullptr ? self->paneOf(hwnd) : nullptr;
                if (pane == nullptr) {
                    return DefWindowProcW(hwnd, message, wparam, lparam);
                }
                switch (message) {
                case WM_ERASEBKGND:
                    return 1;
                case WM_PAINT:
                    self->paintFolders(*pane);
                    return 0;
                case WM_SIZE:
                    self->updatePaneScroll(*pane);
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                case WM_MOUSEMOVE: {
                    const int hit = self->hitPane(*pane, {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)});
                    if (hit != pane->hot) {
                        pane->hot = hit;
                        InvalidateRect(hwnd, nullptr, FALSE);
                        TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, hwnd, 0};
                        TrackMouseEvent(&track);
                    }
                    return 0;
                }
                case WM_MOUSELEAVE:
                    if (pane->hot >= 0) {
                        pane->hot = -1;
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                    return 0;
                case WM_LBUTTONDOWN: {
                    const int hit = self->hitPane(*pane, {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)});
                    if (hit >= 0) {
                        const std::filesystem::path chosen = pane->items[static_cast<size_t>(hit)].path;
                        self->showFolder(chosen);
                    }
                    return 0;
                }
                case WM_MOUSEWHEEL: {
                    const int delta = GET_WHEEL_DELTA_WPARAM(wparam);
                    self->scrollPaneTo(*pane, pane->scroll - delta * self->paneStep(*pane) * 3 / WHEEL_DELTA);
                    return 0;
                }
                case WM_VSCROLL: {
                    SCROLLINFO info{sizeof(info)};
                    info.fMask = SIF_ALL;
                    GetScrollInfo(hwnd, SB_VERT, &info);
                    int position = pane->scroll;
                    switch (LOWORD(wparam)) {
                    case SB_LINEUP: position -= self->paneStep(*pane); break;
                    case SB_LINEDOWN: position += self->paneStep(*pane); break;
                    case SB_PAGEUP: position -= static_cast<int>(info.nPage); break;
                    case SB_PAGEDOWN: position += static_cast<int>(info.nPage); break;
                    case SB_THUMBTRACK:
                    case SB_THUMBPOSITION: position = info.nTrackPos; break;
                    case SB_TOP: position = 0; break;
                    case SB_BOTTOM: position = info.nMax; break;
                    default: break;
                    }
                    self->scrollPaneTo(*pane, position);
                    return 0;
                }
                default:
                    break;
                }
                return DefWindowProcW(hwnd, message, wparam, lparam);
            }

            void saveWindowState() const {
                WINDOWPLACEMENT placement{sizeof(placement)};
                if (GetWindowPlacement(window, &placement)) {
                    auto &state = GalleryIO::state();
                    state.window = placement.rcNormalPosition;
                    state.hasWindow = true;
                    state.maximized = placement.showCmd == SW_SHOWMAXIMIZED;
                }
                GalleryIO::save();
            }

            static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

            bool create(const HWND owner) {
                static const bool registered = [] {
                    WNDCLASSEXW klass{sizeof(klass)};
                    klass.lpfnWndProc = windowProc;
                    klass.hInstance = GetModuleHandleW(nullptr);
                    klass.hCursor = LoadCursor(nullptr, IDC_ARROW);
                    klass.hIcon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(1));
                    klass.lpszClassName = WINDOW_CLASS;
                    WNDCLASSEXW gridClass{sizeof(gridClass)};
                    gridClass.style = CS_DBLCLKS;
                    gridClass.lpfnWndProc = gridProc;
                    gridClass.hInstance = GetModuleHandleW(nullptr);
                    gridClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
                    gridClass.lpszClassName = GRID_CLASS;
                    WNDCLASSEXW folderClass{sizeof(folderClass)};
                    folderClass.lpfnWndProc = folderProc;
                    folderClass.hInstance = GetModuleHandleW(nullptr);
                    folderClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
                    folderClass.lpszClassName = FOLDERS_CLASS;
                    return RegisterClassExW(&klass) != 0 && RegisterClassExW(&gridClass) != 0 &&
                           RegisterClassExW(&folderClass) != 0;
                }();
                if (!registered) {
                    return false;
                }
                dpi = UiDpi::forWindow(owner);
                auto &state = GalleryIO::state();
                RECT placed{};
                bool usable = false;
                if (state.hasWindow) {
                    placed = state.window;
                    usable = MonitorFromRect(&placed, MONITOR_DEFAULTTONULL) != nullptr;
                }
                if (!usable) {
                    MONITORINFO monitor{sizeof(monitor)};
                    GetMonitorInfoW(MonitorFromWindow(owner, MONITOR_DEFAULTTONEAREST), &monitor);
                    const RECT work = monitor.rcWork;
                    const int width = std::min<int>(px(1280), work.right - work.left);
                    const int height = std::min<int>(px(720), work.bottom - work.top);
                    const int left = work.left + (work.right - work.left - width) / 2;
                    const int top = work.top + (work.bottom - work.top - height) / 2;
                    placed = {left, top, left + width, top + height};
                }
                const std::wstring title = UiLanguage::text(saving ? L"Gallery (Save)" : L"Gallery (Open)");
                window = CreateWindowExW(0, WINDOW_CLASS, title.c_str(), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, placed.left,
                                         placed.top, placed.right - placed.left, placed.bottom - placed.top, owner, nullptr,
                                         GetModuleHandleW(nullptr), this);
                if (window == nullptr) {
                    return false;
                }
                dpi = UiDpi::forWindow(window);
                createFonts();
                grid = CreateWindowExW(0, GRID_CLASS, L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | WS_CLIPCHILDREN, 0, 0, 1, 1, window,
                                       nullptr, GetModuleHandleW(nullptr), nullptr);
                SetWindowLongPtrW(grid, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
                nearby.compact = true;
                for (FolderPane *pane : {&nearby, &recentPane}) {
                    pane->hwnd = CreateWindowExW(0, FOLDERS_CLASS, L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL, 0, 0, 1, 1, window,
                                                 nullptr, GetModuleHandleW(nullptr), nullptr);
                    SetWindowLongPtrW(pane->hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
                }
                search =CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 1, 1,
                                         window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(SEARCH_ID)),
                                         GetModuleHandleW(nullptr), nullptr);
                SendMessageW(search, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
                const std::wstring cue = UiLanguage::text(L"Search by name");
                SendMessageW(search, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(cue.c_str()));
                SetWindowSubclass(search, searchProc, 1, reinterpret_cast<DWORD_PTR>(this));
                nameEdit = CreateWindowExW(0, L"EDIT", defaultName().c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                           0, 0, 1, 1, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(NAME_ID)),
                                           GetModuleHandleW(nullptr), nullptr);
                SendMessageW(nameEdit, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
                SetWindowSubclass(nameEdit, nameProc, 2, reinterpret_cast<DWORD_PTR>(this));
                worker.start(window);
                applyTheme();
                layout();
                rescan();
                ShowWindow(window, state.maximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);
                SetFocus(grid);
                return true;
            }
        };

        std::unique_ptr<Gallery> instance;

        LRESULT CALLBACK Gallery::windowProc(const HWND hwnd, const UINT message, const WPARAM wparam, const LPARAM lparam) {
            if (message == WM_NCCREATE) {
                const auto *create = reinterpret_cast<const CREATESTRUCTW *>(lparam);
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
            }
            auto *self = reinterpret_cast<Gallery *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (self == nullptr) {
                return DefWindowProcW(hwnd, message, wparam, lparam);
            }
            switch (message) {
            case WM_ERASEBKGND:
                return 1;
            case WM_PAINT:
                self->paintWindow();
                return 0;
            case WM_SIZE:
                self->layout();
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            case WM_GETMINMAXINFO: {
                auto *limits = reinterpret_cast<MINMAXINFO *>(lparam);
                limits->ptMinTrackSize = {self->px(980), self->px(520)};
                return 0;
            }
            case WM_DPICHANGED: {
                self->dpi = HIWORD(wparam);
                self->createFonts();
                const auto *suggested = reinterpret_cast<const RECT *>(lparam);
                SetWindowPos(hwnd, nullptr, suggested->left, suggested->top, suggested->right - suggested->left,
                             suggested->bottom - suggested->top, SWP_NOZORDER | SWP_NOACTIVATE);
                self->layout();
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            case WM_ACTIVATE:
                // Files saved meanwhile show up when the window is come back to.
                if (LOWORD(wparam) != WA_INACTIVE && self->grid != nullptr) {
                    self->rescan();
                } else if (LOWORD(wparam) == WA_INACTIVE) {
                    if (const HWND focus = GetFocus(); focus != nullptr && IsChild(hwnd, focus)) {
                        self->lastFocus = focus;
                    }
                }
                break;
            case WM_SETFOCUS:
                if (const HWND back = self->lastFocus;
                    back != nullptr && IsWindow(back) && IsChild(hwnd, back) && IsWindowVisible(back)) {
                    SetFocus(back);
                } else if (self->grid != nullptr) {
                    SetFocus(self->grid);
                }
                return 0;
            case WM_CTLCOLOREDIT: {
                const auto dc = reinterpret_cast<HDC>(wparam);
                SetTextColor(dc, settingsTheme().text);
                SetBkColor(dc, settingsTheme().textFieldBackground);
                return reinterpret_cast<LRESULT>(self->fieldBrush);
            }
            // The Gallery's own popup menus, drawn dark in dark mode.
            case WM_MEASUREITEM:
                if (ThemedPopup::measure(hwnd, reinterpret_cast<MEASUREITEMSTRUCT *>(lparam))) {
                    return TRUE;
                }
                break;
            case WM_DRAWITEM:
                if (ThemedPopup::draw(reinterpret_cast<const DRAWITEMSTRUCT *>(lparam))) {
                    return TRUE;
                }
                break;
            case WM_COMMAND:
                if (LOWORD(wparam) == SEARCH_ID && HIWORD(wparam) == EN_CHANGE) {
                    std::wstring keep;
                    if (const Entry *current = self->selectedEntry()) {
                        keep = lower(current->source.wstring());
                    }
                    self->refilter(keep);
                    return 0;
                }
                break;
            case WM_SETCURSOR:
                if (LOWORD(lparam) == HTCLIENT) {
                    POINT point{};
                    GetCursorPos(&point);
                    ScreenToClient(hwnd, &point);
                    if (self->draggingSplit || self->inSplitter(point)) {
                        SetCursor(LoadCursor(nullptr, IDC_SIZENS));
                        return TRUE;
                    }
                }
                break;
            case WM_CAPTURECHANGED:
                self->draggingSplit = false;
                break;
            case WM_MOUSEMOVE: {
                if (self->draggingSplit) {
                    self->dragSplit(GET_Y_LPARAM(lparam));
                    return 0;
                }
                const int hit = self->hitButton({GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)});
                if (hit != self->hotButton) {
                    self->hotButton = hit;
                    InvalidateRect(hwnd, nullptr, FALSE);
                    TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, hwnd, 0};
                    TrackMouseEvent(&track);
                }
                return 0;
            }
            case WM_MOUSELEAVE:
                if (self->hotButton >= 0) {
                    self->hotButton = -1;
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                return 0;
            case WM_LBUTTONDOWN:
                if (const POINT point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)}; self->inSplitter(point)) {
                    self->draggingSplit = true;
                    self->splitGrab = point.y - self->paneMiddle;
                    SetCapture(hwnd);
                    return 0;
                }
                self->pressedButton = self->hitButton({GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)});
                if (self->pressedButton >= 0) {
                    SetCapture(hwnd);
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                return 0;
            case WM_LBUTTONUP: {
                if (self->draggingSplit) {
                    ReleaseCapture();
                    GalleryIO::save();
                    return 0;
                }
                const int pressed = std::exchange(self->pressedButton, -1);
                if (pressed >= 0) {
                    ReleaseCapture();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    if (pressed == self->hitButton({GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)}) &&
                        pressed < static_cast<int>(self->buttons.size())) {
                        self->press(self->buttons[static_cast<size_t>(pressed)].id);
                    }
                }
                return 0;
            }
            case WM_KEYDOWN:
                if (self->commonKey(hwnd, wparam)) {
                    return 0;
                }
                break;
            case WM_MOUSEWHEEL: {
                // A wheel that reaches the window itself, as from the search field, goes to the list under the pointer.
                const HWND under = WindowFromPoint({GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)});
                if (under != nullptr && (self->paneOf(under) != nullptr || under == self->grid)) {
                    return SendMessageW(under, message, wparam, lparam);
                }
                break;
            }
            case WM_APPCOMMAND:
                // Sent up from whichever part of the window the mouse's back or forward button was pressed over.
                if (const int command = GET_APPCOMMAND_LPARAM(lparam);
                    command == APPCOMMAND_BROWSER_BACKWARD || command == APPCOMMAND_BROWSER_FORWARD) {
                    self->walkHistory(command == APPCOMMAND_BROWSER_FORWARD);
                    return TRUE;
                }
                break;
            case WM_THUMBNAIL_READY:
                self->thumbnailsReady();
                return 0;
            case WM_TIMER:
                if (wparam == REFRESH_TIMER) {
                    // Nothing to read while the image or thumbnail is still waiting for the view to finish.
                    if (self->scene.getRequests().createImageRequested.load(std::memory_order_acquire)) {
                        return 0;
                    }
                    KillTimer(hwnd, REFRESH_TIMER);
                    self->rescan();
                    return 0;
                }
                break;
            case WM_EXITSIZEMOVE:
                self->saveWindowState();
                return 0;
            case WM_CLOSE:
                // A name being typed is kept when the window closes, as clicking elsewhere keeps it.
                self->endRename(true);
                break;
            case WM_DESTROY:
                self->saveWindowState();
                self->worker.stop();
                return 0;
            case WM_NCDESTROY: {
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
                if (self->grid != nullptr) {
                    SetWindowLongPtrW(self->grid, GWLP_USERDATA, 0);
                }
                for (const HWND list : {self->nearby.hwnd, self->recentPane.hwnd}) {
                    if (list != nullptr) {
                        SetWindowLongPtrW(list, GWLP_USERDATA, 0);
                    }
                }
                const LRESULT result = DefWindowProcW(hwnd, message, wparam, lparam);
                if (instance.get() == self) {
                    instance.reset();
                }
                return result;
            }
            default:
                break;
            }
            return DefWindowProcW(hwnd, message, wparam, lparam);
        }
    }

    void GalleryWindow::open(SettingsMenu &menu, RenderScene &scene, const Mode mode) {
        if (instance != nullptr && IsWindow(instance->window)) {
            if (IsIconic(instance->window)) {
                ShowWindow(instance->window, SW_RESTORE);
            }
            SetForegroundWindow(instance->window);
            instance->setSaving(mode == Mode::SAVE);
            if (mode == Mode::SAVE) {
                // A new save starts from the open document's name, not from the name typed for the last one.
                SetWindowTextW(instance->nameEdit, instance->defaultName().c_str());
                instance->focusName();
            }
            return;
        }
        instance.reset();
        auto gallery = std::make_unique<Gallery>(menu, scene);
        const auto &state = GalleryIO::state();
        std::error_code error;
        gallery->folder = state.folder;
        if (gallery->folder.empty() || !std::filesystem::is_directory(gallery->folder, error)) {
            const auto &document = scene.getConfigDocumentPath();
            gallery->folder = !document.empty() ? document.parent_path() : Utilities::getDefaultPath();
        }
        gallery->filter = state.filter < static_cast<uint8_t>(Filter::COUNT) ? static_cast<Filter>(state.filter) : Filter::ALL;
        gallery->sort = state.sort == 1 ? 1 : 0;
        gallery->saving = mode == Mode::SAVE;
        Gallery *raw = gallery.get();
        instance = std::move(gallery);
        if (!raw->create(NativeDialogs::mainWindow())) {
            if (raw->window != nullptr) {
                DestroyWindow(raw->window);
            }
            instance.reset();
            return;
        }
        if (mode == Mode::SAVE) {
            raw->focusName();
        }
    }

    void GalleryWindow::applyTheme() {
        if (instance != nullptr && IsWindow(instance->window)) {
            instance->applyTheme();
        }
    }

    void GalleryWindow::close() {
        if (instance != nullptr && IsWindow(instance->window)) {
            DestroyWindow(instance->window);
        }
        instance.reset();
    }
}
