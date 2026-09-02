//
// Created by Merutilm on 2025-09-06.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-07-09, 2026-08-21, 2026-08-23, 2026-08-27, 2026-09-01, 2026-09-02
// Modified by Fable 5 on 2026-07-06
// Modified by Opus 5 on 2026-08-09, 2026-08-10, 2026-08-11, 2026-08-12, 2026-08-14, 2026-08-18, 2026-08-19, 2026-08-21, 2026-08-25
//

#include "VideoWindow.hpp"

#include "IOUtilities.h"
#include "../io/RFFDynamicMapBinary.h"
#include "../io/RFFStaticMapBinary.h"
#include "../video/TimelineSchedule.hpp"
#include "opencv2/opencv.hpp"

#include <commctrl.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <format>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace merutilm::rff2 {

    namespace {
        std::atomic<uint64_t> ffmpegPipeCounter{0};
        constexpr UINT WM_VIDEO_FIRST_FRAME_READY = WM_APP + 1;

        void setWindowCloaked(const HWND window, const bool cloaked) {
            using DwmSetWindowAttributeFn = HRESULT (WINAPI *)(HWND, DWORD, LPCVOID, DWORD);
            static const auto dwmSetWindowAttribute = reinterpret_cast<DwmSetWindowAttributeFn>(
                GetProcAddress(LoadLibraryW(L"dwmapi.dll"), "DwmSetWindowAttribute"));
            if (dwmSetWindowAttribute == nullptr || window == nullptr) {
                return;
            }
            const BOOL value = cloaked;
            dwmSetWindowAttribute(window, 13, &value, sizeof(value));
        }

        // SMPTE ST 2084 and ARIB STD-B67 transfer functions, acknowledged in NOTICE.
        float decodeHdrCode(const float code, const VidHdrTransfer transfer) {
            const float v = std::clamp(code, 0.0f, 1.0f);
            if (transfer == VidHdrTransfer::PQ) {
                constexpr float m1 = 0.1593017578125f;
                constexpr float m2 = 78.84375f;
                constexpr float c1 = 0.8359375f;
                constexpr float c2 = 18.8515625f;
                constexpr float c3 = 18.6875f;
                const float p = std::pow(v, 1.0f / m2);
                const float y = std::max(p - c1, 0.0f) / std::max(c2 - c3 * p, 1e-6f);
                return std::pow(y, 1.0f / m1) * 10000.0f;
            }
            if (transfer == VidHdrTransfer::HLG) {
                constexpr float a = 0.17883277f;
                constexpr float b = 0.28466892f;
                constexpr float c = 0.55991073f;
                return v <= 0.5f ? v * v / 3.0f : (std::exp((v - c) / a) + b) / 12.0f;
            }
            return v;
        }

        float encodeHdrCode(const float linear, const VidHdrTransfer transfer) {
            const float v = std::max(linear, 0.0f);
            if (transfer == VidHdrTransfer::PQ) {
                constexpr float m1 = 0.1593017578125f;
                constexpr float m2 = 78.84375f;
                constexpr float c1 = 0.8359375f;
                constexpr float c2 = 18.8515625f;
                constexpr float c3 = 18.6875f;
                const float y = std::pow(std::clamp(v / 10000.0f, 0.0f, 1.0f), m1);
                return std::pow((c1 + c2 * y) / (1.0f + c3 * y), m2);
            }
            if (transfer == VidHdrTransfer::HLG) {
                constexpr float a = 0.17883277f;
                constexpr float b = 0.28466892f;
                constexpr float c = 0.55991073f;
                return v <= 1.0f / 12.0f ? std::sqrt(3.0f * v)
                                         : a * std::log(std::max(12.0f * v - b, 1e-6f)) + c;
            }
            return v;
        }

        void decodeHdrFrame(const cv::Mat &encoded, cv::Mat &linear, const VidHdrTransfer transfer) {
            linear.create(encoded.size(), CV_32FC4);
            for (int y = 0; y < encoded.rows; ++y) {
                const auto *src = encoded.ptr<cv::Vec4w>(y);
                auto *dst = linear.ptr<cv::Vec4f>(y);
                for (int x = 0; x < encoded.cols; ++x) {
                    for (int c = 0; c < 3; ++c) {
                        dst[x][c] = decodeHdrCode(static_cast<float>(src[x][c]) / 65535.0f, transfer);
                    }
                    dst[x][3] = static_cast<float>(src[x][3]) / 65535.0f;
                }
            }
        }

        void encodeHdrFrame(const cv::Mat &linear, cv::Mat &encoded, const VidHdrTransfer transfer) {
            encoded.create(linear.size(), CV_16UC4);
            for (int y = 0; y < linear.rows; ++y) {
                const auto *src = linear.ptr<cv::Vec4f>(y);
                auto *dst = encoded.ptr<cv::Vec4w>(y);
                for (int x = 0; x < linear.cols; ++x) {
                    for (int c = 0; c < 3; ++c) {
                        const float code = std::clamp(encodeHdrCode(src[x][c], transfer), 0.0f, 1.0f);
                        dst[x][c] = static_cast<uint16_t>(std::lround(code * 65535.0f));
                    }
                    dst[x][3] = static_cast<uint16_t>(std::lround(
                        std::clamp(src[x][3], 0.0f, 1.0f) * 65535.0f));
                }
            }
        }

        struct FFmpegPipe {
            HANDLE writeEnd = nullptr;
            PROCESS_INFORMATION pi{};
            HANDLE logHandle = nullptr;
            std::atomic<bool> *cancelRequested = nullptr;
            std::atomic<bool> *abortRequested = nullptr;
            bool opened = false;
            bool writeFailed = false;
            bool finished = false;
            bool succeeded = false;
            DWORD exitCode = STILL_ACTIVE;

            FFmpegPipe(const std::filesystem::path &output, const int width, const int height,
                       const float fps, const uint32_t bitrate, const bool lossless,
                       const VidHdrTransfer hdr, const float peakNits,
                       std::atomic<bool> *cancelRequested,
                       std::atomic<bool> *abortRequested) : cancelRequested(cancelRequested),
                                                           abortRequested(abortRequested) {
                // Resolve ffmpeg.exe next to our own executable; fall back to PATH.
                std::wstring ffmpeg = L"ffmpeg.exe";
                wchar_t modPath[MAX_PATH];
                if (const DWORD n = GetModuleFileNameW(nullptr, modPath, MAX_PATH); n > 0 && n < MAX_PATH) {
                    std::wstring p(modPath, n);
                    if (const size_t slash = p.find_last_of(L"\\/"); slash != std::wstring::npos) {
                        if (std::wstring cand = p.substr(0, slash) + L"\\ffmpeg.exe";
                            GetFileAttributesW(cand.c_str()) != INVALID_FILE_ATTRIBUTES) {
                            ffmpeg = std::move(cand);
                        }
                    }
                }

                // HDR frames arrive as 16 bits per channel with alpha, which is what carries the PQ or HLG code values.
                const bool isHdr = hdr != VidHdrTransfer::SDR;
                std::wstring cmd = L"\"" + ffmpeg + L"\""
                                   L" -y -hide_banner -loglevel error -f rawvideo -pixel_format " +
                                   (isHdr ? std::wstring(L"rgba64le") : std::wstring(L"bgr24")) +
                                   L" -video_size " + std::to_wstring(width) + L"x" + std::to_wstring(height) +
                                   L" -framerate " + std::format(L"{}", fps) +
                                   L" -i pipe:0 -an";
                if (isHdr) {
                    // The pixels are already encoded, so nothing here may convert them: the tags name what
                    // they carry, and the filter is asked for the matrix and the even size, nothing else.
                    const std::wstring trc = hdr == VidHdrTransfer::HLG ? L"arib-std-b67" : L"smpte2084";
                    const auto nits = static_cast<uint32_t>(std::lround(std::max(peakNits, 1.0f)));
                    cmd += L" -c:v libx265 -preset veryfast -pix_fmt yuv420p10le"
                           L" -vf scale=trunc(iw/2)*2:trunc(ih/2)*2"
                           L" -color_primaries bt2020 -colorspace bt2020nc -color_trc " + trc +
                           L" -x265-params \"colorprim=bt2020:colormatrix=bt2020nc:transfer=" + trc;
                    if (hdr == VidHdrTransfer::PQ) {
                        cmd += L":master-display=G(8500,39850)B(6550,2300)R(35400,14600)WP(15635,16450)L("
                                + std::to_wstring(nits * 10000u) + L",50):max-cll=" + std::to_wstring(nits)
                                + L"," + std::to_wstring(nits);
                    }
                    cmd += L"\"";
                    if (bitrate > 0) {
                        cmd += L" -b:v " + std::to_wstring(bitrate) + L"k";
                    }
                } else if (lossless) {
                    // libx264rgb takes the piped BGR24 with no RGB->YUV step, so -qp 0 is bit-exact, and 4:4:4 RGB drops the even-dimension rule the scale filter exists for.
                    cmd += L" -c:v libx264rgb -preset veryfast -threads 0 -qp 0 -pix_fmt gbrp";
                } else {
                    // libx264 + yuv420p requires even dimensions; force-even via scale.
                    cmd += L" -c:v libx264 -preset veryfast -threads 0 -pix_fmt yuv420p"
                           L" -vf scale=trunc(iw/2)*2:trunc(ih/2)*2";
                    if (bitrate > 0) {
                        cmd += L" -b:v " + std::to_wstring(bitrate) + L"k";
                    }
                }
                const std::filesystem::path outputDirectory = output.parent_path();
                const std::wstring outputFile = output.filename().wstring();
                if (outputFile.empty()) {
                    return;
                }
                cmd += L" \"" + outputFile + L"\"";

                SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
                std::filesystem::path logPath = output;
                logPath += L".log";
                if (const HANDLE staleLog = CreateFileW(logPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                                                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
                    staleLog != INVALID_HANDLE_VALUE) {
                    CloseHandle(staleLog);
                    DeleteFileW(logPath.c_str());
                }
                logHandle = CreateFileW(L"NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa,
                                        OPEN_EXISTING, 0, nullptr);
                if (logHandle == INVALID_HANDLE_VALUE) {
                    logHandle = nullptr;
                    return;
                }
                const std::wstring pipeName = std::format(L"\\\\.\\pipe\\rff-ffmpeg-{}-{}",
                                                          GetCurrentProcessId(),
                                                          ffmpegPipeCounter.fetch_add(
                                                              1, std::memory_order_relaxed));
                writeEnd = CreateNamedPipeW(pipeName.c_str(), PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED,
                                            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1,
                                            1u << 20, 1u << 20, 0, nullptr);
                if (writeEnd == INVALID_HANDLE_VALUE) {
                    writeEnd = nullptr;
                    return;
                }
                HANDLE readEnd = CreateFileW(pipeName.c_str(), GENERIC_READ, 0, &sa, OPEN_EXISTING,
                                             FILE_ATTRIBUTE_NORMAL, nullptr);
                if (readEnd == INVALID_HANDLE_VALUE) {
                    readEnd = nullptr;
                    return;
                }

                const HANDLE connectEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
                if (connectEvent == nullptr) {
                    CloseHandle(readEnd);
                    return;
                }
                OVERLAPPED connect{};
                connect.hEvent = connectEvent;
                const BOOL connectedImmediately = ConnectNamedPipe(writeEnd, &connect);
                const DWORD connectError = connectedImmediately ? ERROR_SUCCESS : GetLastError();
                bool connected = connectedImmediately || connectError == ERROR_PIPE_CONNECTED;
                if (!connected && connectError == ERROR_IO_PENDING) {
                    connected = WaitForSingleObject(connectEvent, INFINITE) == WAIT_OBJECT_0;
                }
                CloseHandle(connectEvent);
                if (!connected) {
                    CloseHandle(readEnd);
                    return;
                }

                STARTUPINFOEXW si{};
                si.StartupInfo.cb = sizeof(si);
                si.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
                si.StartupInfo.hStdInput = readEnd;
                si.StartupInfo.hStdOutput = logHandle;
                si.StartupInfo.hStdError = logHandle;

                SIZE_T attributeBytes = 0;
                InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeBytes);
                std::vector<std::byte> attributeStorage(attributeBytes);
                si.lpAttributeList = reinterpret_cast<PPROC_THREAD_ATTRIBUTE_LIST>(attributeStorage.data());
                if (!InitializeProcThreadAttributeList(si.lpAttributeList, 1, 0, &attributeBytes)) {
                    CloseHandle(readEnd);
                    return;
                }
                const std::array inheritedHandles = {readEnd, logHandle};
                if (!UpdateProcThreadAttribute(si.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                               const_cast<HANDLE *>(inheritedHandles.data()),
                                               sizeof(inheritedHandles), nullptr, nullptr)) {
                    DeleteProcThreadAttributeList(si.lpAttributeList);
                    CloseHandle(readEnd);
                    return;
                }

                std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
                cmdBuf.push_back(L'\0');

                if (CreateProcessW(nullptr, cmdBuf.data(), nullptr, nullptr, TRUE,
                                   CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT, nullptr,
                                   outputDirectory.empty() ? nullptr : outputDirectory.c_str(),
                                   &si.StartupInfo, &pi)) {
                    opened = true;
                }
                DeleteProcThreadAttributeList(si.lpAttributeList);
                // The child owns the read end now; the parent never reads it.
                CloseHandle(readEnd);
            }

            FFmpegPipe(const FFmpegPipe &) = delete;
            FFmpegPipe &operator=(const FFmpegPipe &) = delete;

            [[nodiscard]] bool isOpened() const { return opened; }

            [[nodiscard]] bool isCancellationRequested() const {
                return (cancelRequested != nullptr && cancelRequested->load(std::memory_order_relaxed)) ||
                       (abortRequested != nullptr && abortRequested->load(std::memory_order_relaxed));
            }

            bool write(const void *data, const size_t bytes) {
                if (!opened || writeFailed) return false;
                const auto *p = static_cast<const char *>(data);
                size_t left = bytes;
                while (left > 0) {
                    if (isCancellationRequested()) {
                        writeFailed = true;
                        return false;
                    }
                    DWORD written = 0;
                    const DWORD chunk = static_cast<DWORD>(std::min<size_t>(left, 1u << 20));
                    const HANDLE event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
                    if (event == nullptr) {
                        writeFailed = true;
                        return false;
                    }
                    OVERLAPPED overlapped{};
                    overlapped.hEvent = event;
                    const BOOL completed = WriteFile(writeEnd, p, chunk, &written, &overlapped);
                    if (!completed && GetLastError() == ERROR_IO_PENDING) {
                        const std::array waitHandles = {event, pi.hProcess};
                        while (true) {
                            const DWORD wait = WaitForMultipleObjects(static_cast<DWORD>(waitHandles.size()),
                                                                      waitHandles.data(), FALSE, 100);
                            if (wait == WAIT_OBJECT_0) {
                                if (!GetOverlappedResult(writeEnd, &overlapped, &written, FALSE)) {
                                    written = 0;
                                }
                                break;
                            }
                            if (wait == WAIT_OBJECT_0 + 1 || wait == WAIT_FAILED ||
                                isCancellationRequested()) {
                                CancelIoEx(writeEnd, &overlapped);
                                GetOverlappedResult(writeEnd, &overlapped, &written, TRUE);
                                written = 0;
                                break;
                            }
                        }
                    } else if (!completed) {
                        written = 0;
                    }
                    CloseHandle(event);
                    if (written == 0) {
                        writeFailed = true;
                        return false;
                    }
                    p += written;
                    left -= written;
                }
                return true;
            }

            // Signal EOF, let ffmpeg finalize the container, then reap the child.
            bool finish() {
                if (finished) {
                    return succeeded;
                }
                if (writeEnd) {
                    CloseHandle(writeEnd);
                    writeEnd = nullptr;
                }
                if (opened) {
                    DWORD waitResult = WAIT_FAILED;
                    while (true) {
                        waitResult = WaitForSingleObject(pi.hProcess, 100);
                        if (waitResult != WAIT_TIMEOUT) {
                            break;
                        }
                        if (isCancellationRequested()) {
                            TerminateProcess(pi.hProcess, ERROR_CANCELLED);
                            waitResult = WaitForSingleObject(pi.hProcess, 2000);
                            break;
                        }
                    }
                    if (waitResult == WAIT_OBJECT_0) {
                        GetExitCodeProcess(pi.hProcess, &exitCode);
                    }
                    succeeded = waitResult == WAIT_OBJECT_0 && exitCode == 0 && !writeFailed;
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                    opened = false;
                }
                if (logHandle && logHandle != INVALID_HANDLE_VALUE) {
                    CloseHandle(logHandle);
                    logHandle = nullptr;
                }
                finished = true;
                return succeeded;
            }

            [[nodiscard]] DWORD getExitCode() const { return exitCode; }

            ~FFmpegPipe() { finish(); }
        };

        // Draws the progress readout centered in rc, laying every digit and every space on one
        // fixed cell.
        //
        // The face is proportional, and DrawText re-centers whatever it is handed: a '1' arriving
        // where a '4' was, or a digit taking the place of one of the number's leading spaces, gives
        // the string a different width and the whole line moves. Several updates a second of that
        // is what the readout shaking is. Pinning the two character classes that change to a common
        // width leaves the string the same length in pixels for every reading it can show, so it is
        // laid out at the same x from the first frame of the export to the last.
        void drawBarText(const HDC hdc, const std::wstring &text, const RECT &rc) {
            if (text.empty()) {
                return;
            }
            const auto advance = [hdc](const wchar_t c) {
                SIZE size = {};
                GetTextExtentPoint32W(hdc, &c, 1, &size);
                return static_cast<int>(size.cx);
            };
            int digitCell = 0;
            for (wchar_t d = L'0'; d <= L'9'; ++d) {
                digitCell = std::max(digitCell, advance(d));
            }

            std::vector<INT> cells(text.size());
            int total = 0;
            for (size_t i = 0; i < text.size(); ++i) {
                const wchar_t c = text[i];
                cells[i] = c == L' ' || (c >= L'0' && c <= L'9') ? digitCell : advance(c);
                total += cells[i];
            }

            TEXTMETRICW tm = {};
            GetTextMetricsW(hdc, &tm);
            // ExtTextOut takes the top of the line, so center it by the font's own height rather
            // than by what this particular string happens to reach.
            const int x = rc.left + (rc.right - rc.left - total) / 2;
            const int y = rc.top + (rc.bottom - rc.top - tm.tmHeight) / 2;
            ExtTextOutW(hdc, x, y, 0, nullptr, text.c_str(), static_cast<UINT>(text.size()), cells.data());
        }
    }

    VideoWindow::VideoWindow(vkh::EngineRef engine, const int width,
                             const int height) : EngineHandler(engine), width(width), height(height) {
        VideoWindow::init();
    }

    VideoWindow::~VideoWindow() {
        VideoWindow::destroy();
    }


    void VideoWindow::setClientSize(const int width, const int height) const {
        RECT work;
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
        const int workW = work.right - work.left;
        const int workH = work.bottom - work.top;

        RECT frame = {0, 0, 0, 0};
        AdjustWindowRect(&frame, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, false);
        const int frameW = frame.right - frame.left;
        const int frameH = frame.bottom - frame.top + Constants::Win32::PROGRESS_BAR_HEIGHT;

        double scale = 1.0;
        scale = std::min(scale, static_cast<double>(workW - frameW) / width);
        scale = std::min(scale, static_cast<double>(workH - frameH) / height);

        const int clientW = std::max(1, static_cast<int>(width * scale));
        const int clientH = std::max(1, static_cast<int>(height * scale));

        const RECT rect = {0, 0, clientW, clientH};
        RECT adjusted = rect;
        AdjustWindowRect(&adjusted, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, false);

        const int windowWidth = adjusted.right - adjusted.left;
        const int windowHeight = adjusted.bottom - adjusted.top + Constants::Win32::PROGRESS_BAR_HEIGHT;
        const int x = work.left + (workW - windowWidth) / 2;
        const int y = work.top + (workH - windowHeight) / 2;

        SetWindowPos(videoWindow, nullptr, x, y, windowWidth, windowHeight, SWP_NOZORDER);
        SetWindowPos(renderWindow, nullptr, 0, 0, clientW, clientH, SWP_NOZORDER);
        SetWindowPos(bar, nullptr, 0, clientH, clientW, Constants::Win32::PROGRESS_BAR_HEIGHT, SWP_NOZORDER);
    }

    LRESULT VideoWindow::videoWindowProc(const HWND hwnd, const UINT message, const WPARAM wParam,
                                         const LPARAM lParam) {
        auto *windowPtr = reinterpret_cast<VideoWindow *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        if (!windowPtr) {
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }
        auto &window = *windowPtr;
        switch (message) {
            case WM_VIDEO_FIRST_FRAME_READY: {
                setWindowCloaked(hwnd, false);
                SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                SetForegroundWindow(hwnd);
                return 0;
            }
            case WM_CLOSE: {
                if (!window.allowClose.load()) {
                    window.closeRequested.store(true);
                    {
                        std::scoped_lock lock(window.barMutex);
                        window.barText = L"Writing video...";
                    }
                    InvalidateRect(window.bar, nullptr, FALSE);
                    return 0;
                }
                DestroyWindow(hwnd);
                return 0;
            }
            case WM_DESTROY: {
                PostQuitMessage(0);
                return 0;
            }
            default: break;
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    LRESULT VideoWindow::progressBarProc(const HWND window, const UINT message, const WPARAM wParam,
                                         const LPARAM lParam,
                                         [[maybe_unused]] const UINT_PTR uIdSubclass,
                                         const DWORD_PTR dwRefData) {
        auto &self = *reinterpret_cast<VideoWindow *>(dwRefData);

        // Nothing here leaves a pixel of the bar untouched, so an erase pass would only flash the
        // control's background through the readout.
        if (message == WM_ERASEBKGND) {
            return 1;
        }
        if (message != WM_PAINT) {
            return DefSubclassProc(window, message, wParam, lParam);
        }

        // This used to be done from the video window's own WM_PAINT, which called BeginPaint on the
        // bar instead of on itself. That left the video window's update region standing, so Windows
        // sent it WM_PAINT again the moment the queue drained, and the bar was redrawn in a tight
        // loop for the whole export - straight onto the screen, wiped to two flat fills before the
        // text went back over them. Catching one of those partial frames is the flicker. The bar
        // now paints on its own WM_PAINT, once per update, and composes off-screen first.
        RECT rc;
        GetClientRect(window, &rc);
        PAINTSTRUCT ps;
        const HDC hdc = BeginPaint(window, &ps);
        const int w = rc.right - rc.left;
        const int h = rc.bottom - rc.top;
        if (w > 0 && h > 0) {
            float pos;
            std::wstring text;
            {
                std::scoped_lock lock(self.barMutex);
                pos = self.barRatio;
                text = self.barText;
            }

            const HDC mem = CreateCompatibleDC(hdc);
            const HBITMAP bmp = CreateCompatibleBitmap(hdc, w, h);
            const auto oldBmp = SelectObject(mem, bmp);

            RECT prc = rc;
            prc.right = static_cast<int>(
                std::lerp(static_cast<float>(prc.left), static_cast<float>(prc.right), pos));

            const HBRUSH pBar = CreateSolidBrush(Constants::Win32::COLOR_PROGRESS_BACKGROUND_PROG);
            FillRect(mem, &prc, pBar);
            DeleteObject(pBar);

            RECT brc = rc;
            brc.left = prc.right;
            const HBRUSH bBar = CreateSolidBrush(Constants::Win32::COLOR_PROGRESS_BACKGROUND_BACK);
            FillRect(mem, &brc, bBar);
            DeleteObject(bBar);

            SetBkMode(mem, TRANSPARENT);
            // The bar paints its own text, so WM_SETFONT on the control would not reach it -
            // the face has to be selected into the DC here, or GDI draws the progress readout
            // in the stock "System" bitmap font.
            const auto previousFont = SelectObject(mem, Constants::Win32::sharedUiFont());

            // One string, drawn twice in two colors and clipped to either side of the bar's edge,
            // so the part of the readout standing on the filled section reads against it.
            const HRGN tempRgn = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
            IntersectClipRect(mem, prc.left, prc.top, prc.right, prc.bottom);

            SetTextColor(mem, Constants::Win32::COLOR_PROGRESS_TEXT_PROG);
            drawBarText(mem, text, rc);

            SelectClipRgn(mem, tempRgn);
            IntersectClipRect(mem, brc.left, brc.top, brc.right, brc.bottom);

            SetTextColor(mem, Constants::Win32::COLOR_PROGRESS_TEXT_BACK);
            drawBarText(mem, text, rc);

            SelectClipRgn(mem, nullptr);
            SelectObject(mem, previousFont);
            DeleteObject(tempRgn);

            BitBlt(hdc, 0, 0, w, h, mem, 0, 0, SRCCOPY);
            SelectObject(mem, oldBmp);
            DeleteObject(bmp);
            DeleteDC(mem);
        }
        EndPaint(window, &ps);
        return 0;
    }


    void VideoWindow::createVideo(vkh::EngineRef engine,
                                  const Attribute &attr,
                                  const std::filesystem::path &open,
                                  const std::filesystem::path &save) {
        int imgWidth = 0;
        int imgHeight = 0;
        HWND wnd = engine.getWindowContext(Constants::VulkanWindow::MAIN_WINDOW_ATTACHMENT_INDEX).getWindow().
                getWindowHandle();
        wnd = IsWindow(wnd) ? wnd : nullptr;

        if (engine.isValidWindowContext(Constants::VulkanWindow::VIDEO_WINDOW_ATTACHMENT_INDEX)) {
            MessageBoxW(wnd, L"Video processor already using", L"Error", MB_TOPMOST | MB_ICONERROR | MB_OK);
            return;
        }

        if (attr.video.data.isStatic) {
            const RFFStaticMapBinary targetMap = RFFStaticMapBinary::readByID(open, 1);
            if (!targetMap.hasData()) {
                MessageBoxW(wnd, L"Cannot create video. There is no samples in the directory", L"Export failed",
                            MB_TOPMOST | MB_ICONERROR | MB_OK);
                return;
            }

            imgWidth = static_cast<int>(targetMap.getWidth());
            imgHeight = static_cast<int>(targetMap.getHeight());
        } else {
            const RFFDynamicMapBinary targetMap = RFFDynamicMapBinary::readByID(open, 1);
            if (!targetMap.hasData()) {
                MessageBoxW(wnd, L"Cannot create video. There is no samples in the directory", L"Export failed",
                            MB_TOPMOST | MB_ICONERROR | MB_OK);
                return;
            }

            const Matrix<double> &targetMatrix = targetMap.getMatrix();

            imgWidth = targetMatrix.getWidth();
            imgHeight = targetMatrix.getHeight();
        }


        const auto cw = static_cast<uint32_t>(std::min(imgWidth, 1280));
        const auto ch = cw * imgHeight / imgWidth;
        
        // Use heap allocation to ensure proper lifetime management
        auto window = std::make_shared<VideoWindow>(engine, cw, ch);
        window->createScene(VkExtent2D{static_cast<uint32_t>(imgWidth), static_cast<uint32_t>(imgHeight)}, attr);
        auto &scene = *window->scene;
        std::atomic<bool> exitFlag{false};
        std::atomic<bool> encoderFailed{false};
        std::atomic<bool> pipeAbort{false};
        std::atomic<bool> firstFrameReadyPosted{false};
        std::mutex workerFailureMutex;
        std::wstring workerFailure;
        const auto failVideo = [&](const std::wstring_view message) noexcept {
            encoderFailed.store(true);
            pipeAbort.store(true);
            exitFlag.store(true);
            try {
                {
                    std::scoped_lock lock(workerFailureMutex);
                    if (workerFailure.empty()) {
                        workerFailure.assign(message);
                    }
                }
                {
                    std::scoped_lock lock(scene.getBufferCachedMutex());
                    while (!scene.getQueuedBuffers().empty()) {
                        scene.getQueuedBuffers().pop();
                    }
                }
            } catch (...) {
            }
            scene.getBufferCachedCondition().notify_all();
        };


        const auto &[defaultZoomIncrement, isStatic] = attr.video.data;
        const auto &[overZoom, showText, mps] = attr.video.animation;
        const float fps = attr.video.exportation.fps;
        const uint32_t bitrate = attr.video.exportation.bitrate;
        const uint32_t keyframeAA = attr.video.exportation.keyframeAA;
        const uint32_t colorAA = attr.video.exportation.colorAA;
        // HDR needs the float chain to carry anything above white, and its encoder is x265, not the
        // RGB x264 the lossless switch means, so the two cannot both be on.
        const VidHdrTransfer hdrTransfer = attr.shader.hdr.use
                                               ? attr.video.exportation.hdrTransfer
                                               : VidHdrTransfer::SDR;
        const bool hdrOut = hdrTransfer != VidHdrTransfer::SDR;
        const float hdrPeakNits = attr.video.exportation.hdrPeakNits;
        const bool lossless = attr.video.exportation.lossless && !hdrOut;
        const int keyframeGrid = isStatic ? 1 : std::clamp<int>(static_cast<int>(keyframeAA), 1, 8);
        // Temporal supersampling of the color animation. Disabled for static (PNG) sources,
        // which have no shader-driven color animation to average.
        const int motionSamples = isStatic ? 1 : std::clamp<int>(static_cast<int>(colorAA), 1, 8);
        constexpr float KEYFRAME_AA_WINDOW = 0.12f;

        // The RGBA2BGR pass emits this size, so keep the formula identical to CPCImageRGBA2BGR::applyOutputSize.
        const int ssaa = std::max(1, static_cast<int>(attr.render.ssaa));
        const int outW = std::max(1, imgWidth / ssaa);
        const int outH = std::max(1, imgHeight / ssaa);

        // Same count the render thread uses for maxNumber; recorded so the frame at which a
        // run died can be converted back into a zoom depth without guessing.
        const uint32_t keyframes = isStatic
                                       ? IOUtilities::fileNameCount(open, Constants::Extension::STATIC_MAP)
                                       : RFFDynamicMapBinary::keyframeCount(open);
        const float frameInterval = mps / fps;
        // The depth axis runs from the last keyframe down to -overZoom, and the time that takes is
        // the speed curve integrated over it - a plain division only while the speed is constant.
        // Built here, before either thread starts, so the frame count is known from the outset.
        const TimelineSchedule schedule = TimelineSchedule::create(attr.video.timeline,
                                                                   static_cast<float>(keyframes), -overZoom, mps);
        const uint64_t totalFrames = schedule.totalFrames(fps);

        // Lossless is RGB H.264, which the save dialog's mp4 cannot carry to a player that will decode it, so retarget the container.
        std::filesystem::path output = save;
        if (lossless) {
            output.replace_extension(Constants::Extension::VIDEO_LOSSLESS);
        }
        const std::filesystem::path temporaryOutput = IOUtilities::temporaryFilePath(output);

        // Set before the first frame is queued: it decides how wide the readback buffer is packed.
        scene.setHdrOutput(hdrTransfer, hdrPeakNits);

        FFmpegPipe writer(temporaryOutput, outW, outH, fps, bitrate, lossless, hdrTransfer, hdrPeakNits,
                          &window->closeRequested, &pipeAbort);

        if (!writer.isOpened()) {
            IOUtilities::discardTemporaryFile(temporaryOutput);
            MessageBoxW(wnd, L"Cannot open file!", L"Export failed", MB_TOPMOST | MB_ICONERROR | MB_OK);
            return;
        }

        // Keep a local copy of window shared_ptr for thread safety
        std::shared_ptr<VideoWindow> windowRef = window;
        std::jthread queueResolveThread([&, wnd, windowRef] {
            try {
                std::unique_ptr<VideoBufferCache> buffer = nullptr;

            // The zoom overlay is drawn at diffuse white rather than at the peak the format reaches,
            // which on an HDR display would be painful to look at next to the picture.
            // The two curves below repeat the encoders in vk_linear_interpolation.frag; NOTICE records their standards.
            const uint16_t textLevel = [](const VidHdrTransfer t) -> uint16_t {
                if (t == VidHdrTransfer::PQ) {
                    // ST 2084 at 203 nits, the reference white of an HDR10 master.
                    constexpr double m1 = 0.1593017578125;
                    constexpr double m2 = 78.84375;
                    constexpr double c1 = 0.8359375;
                    constexpr double c2 = 18.8515625;
                    constexpr double c3 = 18.6875;
                    const double y = std::pow(203.0 / 10000.0, m1);
                    return static_cast<uint16_t>(std::lround(
                        std::pow((c1 + c2 * y) / (1.0 + c3 * y), m2) * 65535.0));
                }
                if (t == VidHdrTransfer::HLG) {
                    // ARIB STD-B67 at the 0.26 scene level HLG calls diffuse white.
                    constexpr double a = 0.17883277;
                    constexpr double b = 0.28466892;
                    constexpr double c = 0.55991073;
                    return static_cast<uint16_t>(std::lround((a * std::log(12.0 * 0.26 - b) + c) * 65535.0));
                }
                return 65535;
            }(hdrTransfer);

            // Draw text + encode one finished output frame. Rescales only when the GPU path is off.
            const auto processAndWrite = [&](const cv::Mat &img, const float zoom) {
                cv::Mat out;
                if (img.cols != outW || img.rows != outH) {
                    cv::resize(img, out, cv::Size(outW, outH), 0, 0, cv::INTER_AREA);
                } else {
                    out = img;
                }
                if (showText) {
                    const int xg = std::max(1, outW / 72);
                    const int yg = std::max(1, outW / 192);
                    const int loc = std::max(1, outW / 40);
                    const float size = std::max(1.0f, static_cast<float>(outW) / 800);
                    const int off = std::max(1, loc / 15);
                    const int tkn = std::max(1, off / 2);
                    const std::string zoomStr = std::format("Zoom : {:6f}E{:d}",
                                                            std::pow(10, std::fmod(zoom, 1)),
                                                            static_cast<int>(zoom));
                    if (out.depth() == CV_8U) {
                        cv::putText(out, zoomStr, cv::Point(xg + off, loc + yg + off), cv::FONT_HERSHEY_PLAIN, size,
                                    cv::Scalar(0, 0, 0));
                        cv::putText(out, zoomStr, cv::Point(xg, loc + yg), cv::FONT_HERSHEY_PLAIN, size,
                                    cv::Scalar(255, 255, 255), tkn, cv::LINE_AA);
                    } else {
                        // An antialiased glyph is only laid into an 8-bit image, so the HDR frame takes the
                        // two passes as coverage masks and has them composited over it at full range instead.
                        const int bandH = std::min(out.rows, loc + yg + off + static_cast<int>(size * 8) + 4);
                        cv::Mat shadow = cv::Mat::zeros(bandH, out.cols, CV_8UC1);
                        cv::Mat glyph = cv::Mat::zeros(bandH, out.cols, CV_8UC1);
                        cv::putText(shadow, zoomStr, cv::Point(xg + off, loc + yg + off), cv::FONT_HERSHEY_PLAIN,
                                    size, cv::Scalar(255));
                        cv::putText(glyph, zoomStr, cv::Point(xg, loc + yg), cv::FONT_HERSHEY_PLAIN, size,
                                    cv::Scalar(255), tkn, cv::LINE_AA);
                        for (int y = 0; y < bandH; ++y) {
                            const auto *sp = shadow.ptr<uint8_t>(y);
                            const auto *gp = glyph.ptr<uint8_t>(y);
                            auto *dp = out.ptr<cv::Vec4w>(y);
                            for (int x = 0; x < out.cols; ++x) {
                                if (sp[x] == 0 && gp[x] == 0) {
                                    continue;
                                }
                                const float sa = static_cast<float>(sp[x]) / 255.0f;
                                const float ga = static_cast<float>(gp[x]) / 255.0f;
                                for (int c = 0; c < 3; ++c) {
                                    float v = static_cast<float>(dp[x][c]) * (1.0f - sa);
                                    v = v * (1.0f - ga) + static_cast<float>(textLevel) * ga;
                                    dp[x][c] = static_cast<uint16_t>(std::lround(std::clamp(v, 0.0f, 65535.0f)));
                                }
                            }
                        }
                    }
                }
                // out is a continuous BGR24 buffer, or rgba64le once the export runs in HDR.
                if (!writer.write(out.data, out.total() * out.elemSize())) {
                    failVideo(L"FFmpeg stopped accepting video frames.");
                }
            };

            cv::Mat accum;
            int accumHave = 0;
            int accumNeed = 0;
            float accumZoom = 0.0f;

            while (!exitFlag.load() || !scene.getQueuedBuffers().empty()) {
                //MUTEX LOCK SCOPE BEGIN
                {
                    std::mutex &mutex = scene.getBufferCachedMutex();
                    std::unique_lock lock(mutex);
                    scene.getBufferCachedCondition().wait(lock, [&scene, &exitFlag] {
                        return !scene.getQueuedBuffers().empty() || exitFlag.load();
                    });
                    if (exitFlag.load() && scene.getQueuedBuffers().empty()) {
                        buffer = nullptr;
                        break;
                    }
                    buffer = std::move(scene.getQueuedBuffers().front());
                    scene.getQueuedBuffers().pop();
                    scene.getBufferCachedCondition().notify_all();
                }
                //MUTEX LOCK SCOPE END
                const int n = std::max(1, buffer->subsampleCount);
                if (n <= 1 && accumHave == 0) {
                    // Ordinary single-sample frame: encode directly (no extra copy).
                    processAndWrite(buffer->image, buffer->zoom);
                } else {
                    if (accumHave == 0) {
                        accum = cv::Mat::zeros(buffer->image.size(), hdrOut ? CV_32FC4 : CV_32FC3);
                        accumNeed = n;
                        accumZoom = buffer->zoom;
                    }
                    if (hdrOut) {
                        cv::Mat linear;
                        decodeHdrFrame(buffer->image, linear, hdrTransfer);
                        cv::accumulate(linear, accum);
                    } else {
                        cv::accumulate(buffer->image, accum);
                    }
                    cv::Mat avg;
                    const bool complete = ++accumHave >= accumNeed;
                    if (complete) {
                        if (hdrOut) {
                            accum *= 1.0f / static_cast<float>(accumNeed);
                            encodeHdrFrame(accum, avg, hdrTransfer);
                        } else {
                            accum.convertTo(avg, CV_8UC3, 1.0 / static_cast<double>(accumNeed));
                        }
                    }
                    if (complete) {
                        processAndWrite(avg, accumZoom);
                        accumHave = 0;
                    }
                }
            }
            if (accumHave > 0) {   // flush any incomplete group (shouldn't happen normally)
                cv::Mat avg;
                if (hdrOut) {
                    accum *= 1.0f / static_cast<float>(accumHave);
                    encodeHdrFrame(accum, avg, hdrTransfer);
                } else {
                    accum.convertTo(avg, CV_8UC3, 1.0 / static_cast<double>(accumHave));
                }
                processAndWrite(avg, accumZoom);
            }
                engine.getCore().getLogicalDevice().waitDeviceIdle();
            } catch (const std::exception &error) {
                try {
                    vkh::logger::log_err_silent("Video encoder worker failed: {}", error.what());
                } catch (...) {
                }
                failVideo(L"The video encoder worker failed.");
            } catch (...) {
                try {
                    vkh::logger::log_err_silent("Video encoder worker failed with an unknown exception");
                } catch (...) {
                }
                failVideo(L"The video encoder worker failed.");
            }
        });

        std::jthread imageRenderThread([&, imgWidth, imgHeight, windowRef] {
            try {
                const auto renderFrames = [&] {
            const auto frameInterval = mps / fps;
            const uint32_t maxNumber = isStatic
                                           ? IOUtilities::fileNameCount(open, Constants::Extension::STATIC_MAP)
                                           : RFFDynamicMapBinary::keyframeCount(open);
            const float minNumber = -overZoom;
            auto currentFrame = static_cast<float>(maxNumber);
            float currentSec = 0;
            // Without a timeline the depth walk is one constant speed, the mapping the schedule
            // itself falls back to, so both branches below land on the same frames at the same
            // seconds and an export comes out the same however the timeline is switched.
            const bool uniform = schedule.isUniform();
            uint64_t frameIndex = 0;
            uint32_t pf1 = UINT32_MAX;
            const float startSec = Utilities::getCurrentTime();

            RFFDynamicMapBinary zoomedDynamic = RFFDynamicMapBinary::DEFAULT;
            RFFDynamicMapBinary normalDynamic = RFFDynamicMapBinary::DEFAULT;
            RFFStaticMapBinary zoomedStatic = RFFStaticMapBinary::DEFAULT;
            RFFStaticMapBinary normalStatic = RFFStaticMapBinary::DEFAULT;
            cv::Mat zoomedStaticImage = cv::Mat::zeros(imgHeight, imgWidth, CV_16UC4);
            cv::Mat normalStaticImage = cv::Mat::zeros(imgHeight, imgWidth, CV_16UC4);

            scene.setStatic(isStatic);

            const uint64_t scheduledFrames = schedule.totalFrames(fps);
            // The top of the depth axis has no keyframe above it to interpolate towards, so the
            // frame standing on it reads the last pair, as the Timeline Editor's preview does.
            const float topFrame = std::nextafter(static_cast<float>(maxNumber),
                                                  -std::numeric_limits<float>::infinity());
            while (true) {
                // Frame n stands at second n / fps and at the depth that second has reached, both
                // counted from 0, so the video opens on the second and the depth the export was
                // given rather than one frame past them. Neither one is stepped to, so neither
                // accumulates any drift, and the walk stops before it passes the end depth.
                currentSec = static_cast<float>(frameIndex) / fps;
                if (uniform) {
                    currentFrame = static_cast<float>(maxNumber) -
                                   static_cast<float>(frameIndex) * frameInterval;
                    if (currentFrame <= minNumber) {
                        break;
                    }
                } else {
                    if (frameIndex >= scheduledFrames) {
                        break;
                    }
                    currentFrame = schedule.depthAt(currentSec);
                }
                // The depth the keyframe pair is read and blended at; the shader tracks still run
                // on the depth itself, which is the axis their keys were placed on.
                const float sampledFrame = std::min(currentFrame, topFrame);
                ++frameIndex;
                bool requiredRefresh = false;


                if (sampledFrame < 1) {
                    if (0 != pf1) {
                        if (isStatic) {
                            zoomedStatic = RFFStaticMapBinary::DEFAULT;
                            normalStatic = RFFStaticMapBinary::readByID(open, 1);
                            zoomedStaticImage = cv::Mat::zeros(imgHeight, imgWidth, CV_16UC4);
                            normalStaticImage = RFFStaticMapBinary::loadImageByID(open, 1);
                        } else {
                            zoomedDynamic = RFFDynamicMapBinary::DEFAULT;
                            normalDynamic = RFFDynamicMapBinary::readByID(open, 1);
                        }
                        pf1 = 0;
                        requiredRefresh = true;
                    }
                } else {
                    if (const auto f1 = static_cast<uint32_t>(sampledFrame); f1 != pf1) {
                        const uint32_t f2 = f1 + 1;
                        if (isStatic) {
                            zoomedStatic = RFFStaticMapBinary::readByID(open, f1);
                            normalStatic = RFFStaticMapBinary::readByID(open, f2);
                            zoomedStaticImage = RFFStaticMapBinary::loadImageByID(open, f1);
                            normalStaticImage = RFFStaticMapBinary::loadImageByID(open, f2);
                        } else {
                            zoomedDynamic = RFFDynamicMapBinary::readByID(open, f1);
                            normalDynamic = RFFDynamicMapBinary::readByID(open, f2);
                        }
                        const bool dimensionsMatch = isStatic
                            ? zoomedStatic.hasData() && normalStatic.hasData() &&
                              zoomedStatic.getWidth() == static_cast<uint32_t>(imgWidth) &&
                              zoomedStatic.getHeight() == static_cast<uint32_t>(imgHeight) &&
                              normalStatic.getWidth() == static_cast<uint32_t>(imgWidth) &&
                              normalStatic.getHeight() == static_cast<uint32_t>(imgHeight) &&
                              zoomedStaticImage.cols == imgWidth && zoomedStaticImage.rows == imgHeight &&
                              normalStaticImage.cols == imgWidth && normalStaticImage.rows == imgHeight
                            : zoomedDynamic.hasData() && normalDynamic.hasData() &&
                              zoomedDynamic.getMatrix().getWidth() == imgWidth &&
                              zoomedDynamic.getMatrix().getHeight() == imgHeight &&
                              normalDynamic.getMatrix().getWidth() == imgWidth &&
                              normalDynamic.getMatrix().getHeight() == imgHeight;
                        if (!dimensionsMatch) {
                            failVideo(L"All video keyframes must have the same dimensions.");
                            break;
                        }
                        pf1 = f1;
                        requiredRefresh = true;
                    }
                }

                if (windowRef->closeRequested.load() || encoderFailed.load()) {
                    break;
                }

                scene.setCurrentFrame(sampledFrame);
                if (requiredRefresh) {
                    if (isStatic) {
                        scene.setMap(&normalStatic, &zoomedStatic);
                        scene.applyCurrentStaticImage(normalStaticImage, zoomedStaticImage);
                    } else {
                        scene.setMap(&normalDynamic, &zoomedDynamic);
                        scene.applyCurrentDynamicMap(normalDynamic, zoomedDynamic, sampledFrame);
                        // Use the larger of the two keyframes' maxIteration: using normal's alone makes
                        // max_value jump at the keyframe switch, flipping mid-range pixels between
                        // "clamped/interior" and "real height" within a single frame (slope AO/shade pop).
                        const uint64_t normalMax = normalDynamic.getMaxIteration();
                        const uint64_t zoomedMax = zoomedDynamic.getMaxIteration();
                        scene.setMaxIterationDynamic(static_cast<double>(std::max(normalMax, zoomedMax)),
                                                     static_cast<double>(normalMax),
                                                     static_cast<double>(zoomedMax));
                    }
                }


                const float frac = currentFrame - std::floor(currentFrame);
                const float distToInt = std::min(frac, 1.0f - frac);
                // How much depth this frame covers - under a speed curve, the local amount.
                const float depthPerFrame = uniform ? frameInterval : schedule.speedAt(currentFrame) / fps;
                const float aaWindow = std::max(KEYFRAME_AA_WINDOW, depthPerFrame * 0.75f);
                const bool keyframeAAFrame = keyframeGrid >= 2 && currentFrame > 0.0f &&
                                             distToInt < aaWindow;

                // Render and queue one sub-sample. jx/jy jitter the spatial samples that
                // remove the keyframe-boundary brightness pop; timeFrac in [-0.5,0.5) shifts
                // the color-animation time inside this frame's 1/fps slice so the flow is
                // averaged over the frame (fixes Psychedelic / Color-Animation-Speed judder).
                const auto renderSub = [&](const float jx, const float jy, const float timeFrac, const int total) {
                    const float sampleSec = currentSec + timeFrac / fps;
                    scene.setSampleJitter(jx, jy);
                    if (!isStatic) {
                        scene.applyTimelineShader(currentFrame, sampleSec);
                    }
                    scene.setTime(sampleSec);
                    scene.renderOnce();
                    scene.queueImage(total);
                    scene.getBufferCachedCondition().notify_all();
                };

                if (keyframeAAFrame) {
                    // Spatial grid for the boundary; spread the color-animation time across the
                    // same samples so judder is averaged out at transitions too (no extra cost).
                    const int samples = keyframeGrid * keyframeGrid;
                    int idx = 0;
                    for (int sy = 0; sy < keyframeGrid; ++sy) {
                        for (int sx = 0; sx < keyframeGrid; ++sx) {
                            const float jx = (static_cast<float>(sx) + 0.5f) / static_cast<float>(keyframeGrid) - 0.5f;
                            const float jy = (static_cast<float>(sy) + 0.5f) / static_cast<float>(keyframeGrid) - 0.5f;
                            const float timeFrac = motionSamples > 1
                                                       ? (static_cast<float>(idx) + 0.5f) / static_cast<float>(samples) - 0.5f
                                                       : 0.0f;
                            renderSub(jx, jy, timeFrac, samples);
                            ++idx;
                        }
                    }
                    scene.setSampleJitter(0.0f, 0.0f);
                } else if (motionSamples > 1) {
                    // Pure temporal supersampling: average the color flow over the frame.
                    for (int i = 0; i < motionSamples; ++i) {
                        const float timeFrac = (static_cast<float>(i) + 0.5f) / static_cast<float>(motionSamples) - 0.5f;
                        renderSub(0.0f, 0.0f, timeFrac, motionSamples);
                    }
                    scene.setSampleJitter(0.0f, 0.0f);
                } else {
                    renderSub(0.0f, 0.0f, 0.0f, 1);
                }
                if (!firstFrameReadyPosted.exchange(true)) {
                    PostMessageW(windowRef->videoWindow, WM_VIDEO_FIRST_FRAME_READY, 0, 0);
                }

                // Depth no longer advances at a constant rate, so how far along the export is
                // comes from the time the schedule says has passed, not from the depth left to go.
                const float totalSec = schedule.getTotalSeconds();
                const float progressRatio = uniform || totalSec <= 0.0f
                                                ? (static_cast<float>(maxNumber) - currentFrame) / (
                                                      static_cast<float>(maxNumber) + overZoom)
                                                : currentSec / totalSec;
                const float spentSec = Utilities::getCurrentTime() - startSec;
                const float remainedSec = progressRatio > 0.0f
                                              ? (1 - progressRatio) / progressRatio * spentSec
                                              : 0.0f;
                const auto remainedTime = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::duration<float>(remainedSec));
                auto hms = std::chrono::hh_mm_ss(remainedTime);

                if (!windowRef->closeRequested.load()) {
                    {
                        std::scoped_lock lock(windowRef->barMutex);
                        windowRef->barRatio = std::clamp(progressRatio, 0.0f, 1.0f);
                        // Three decimals, in a field wide enough for "100.000". The spec used to be
                        // {:2f} - a width of 2, which a percentage passes on the first frame, and
                        // no precision at all, so it printed the default six decimals. The readout
                        // is centered, so every digit that came and went shifted the whole line
                        // sideways and the text jittered for the length of the export. Held to a
                        // constant character count it stays put.
                        windowRef->barText = std::format(L"Processing... {:7.3f}% [{:%H:%M:%S}]",
                                                         progressRatio * 100, hms);
                    }
                    InvalidateRect(windowRef->bar, nullptr, FALSE);
                }
            }

                };
                try {
                    renderFrames();
                } catch (const std::exception &error) {
                    try {
                        vkh::logger::log_err_silent("Video render worker failed: {}", error.what());
                    } catch (...) {
                    }
                    failVideo(L"The video render worker failed.");
                } catch (...) {
                    try {
                        vkh::logger::log_err_silent("Video render worker failed with an unknown exception");
                    } catch (...) {
                    }
                    failVideo(L"The video render worker failed.");
                }

            {
                std::scoped_lock lock(windowRef->barMutex);
                windowRef->barText = L"Writing video...";
            }
            InvalidateRect(windowRef->bar, nullptr, FALSE);
            exitFlag.store(true);
            scene.getBufferCachedCondition().notify_all();
            if (queueResolveThread.joinable()) queueResolveThread.join();
            const bool cancelled = windowRef->closeRequested.load();
            bool encoded = writer.finish() && !encoderFailed.load() && !cancelled;
            if (encoded && !IOUtilities::commitTemporaryFile(temporaryOutput, output)) {
                failVideo(L"The completed video could not replace the selected output file.");
                encoded = false;
            }
            if (!encoded) {
                IOUtilities::discardTemporaryFile(temporaryOutput);
            }

            windowRef->allowClose.store(true);
            if (IsWindow(windowRef->videoWindow)) {
                SendMessageW(windowRef->videoWindow, WM_CLOSE, 0, 0);
            }
            if (cancelled) {
                MessageBoxW(IsWindow(wnd) ? wnd : nullptr,
                            L"Render cancelled. The previous output file was left unchanged.", L"Cancelled",
                            MB_OK | MB_ICONWARNING | MB_TOPMOST);
            } else if (encoded) {
                MessageBoxW(IsWindow(wnd) ? wnd : nullptr, L"Render Finished!", L"Done",
                            MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
            } else {
                std::wstring message;
                {
                    std::scoped_lock lock(workerFailureMutex);
                    message = workerFailure;
                }
                if (message.empty()) {
                    message = std::format(L"FFmpeg could not finish the video (exit code {}).",
                                          writer.getExitCode());
                }
                MessageBoxW(IsWindow(wnd) ? wnd : nullptr, message.c_str(), L"Export failed",
                            MB_OK | MB_ICONERROR | MB_TOPMOST);
            }
            } catch (const std::exception &error) {
                try {
                    vkh::logger::log_err_silent("Video worker finalization failed: {}", error.what());
                } catch (...) {
                }
                failVideo(L"Video finalization failed.");
                windowRef->allowClose.store(true);
                if (IsWindow(windowRef->videoWindow)) {
                    PostMessageW(windowRef->videoWindow, WM_CLOSE, 0, 0);
                }
            } catch (...) {
                try {
                    vkh::logger::log_err_silent("Video worker finalization failed with an unknown exception");
                } catch (...) {
                }
                failVideo(L"Video finalization failed.");
                windowRef->allowClose.store(true);
                if (IsWindow(windowRef->videoWindow)) {
                    PostMessageW(windowRef->videoWindow, WM_CLOSE, 0, 0);
                }
            }
        });
        
        // Run message loop - this will block until WM_QUIT is posted
        window->messageLoop();
        
        // Wait for threads to complete before destroying window
        if (imageRenderThread.joinable()) {
            imageRenderThread.join();
        }
        
        // Window will be destroyed when shared_ptr goes out of scope
        // after all threads have finished
    }

    void VideoWindow::messageLoop() {
        MSG msg;

        while (GetMessage(&msg, nullptr, 0, 0) != 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    void VideoWindow::init() {
        videoWindow = CreateWindowExW(0,
                                      Constants::Win32::CLASS_VIDEO_WINDOW,
                                      L"Preview video",
                                      WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT, nullptr, nullptr,
                                      nullptr, nullptr);

        renderWindow = CreateWindowExW(0, Constants::Win32::CLASS_VIDEO_RENDER_WINDOW, nullptr,
                                       WS_CHILD | WS_VISIBLE | WS_BORDER | WS_CLIPSIBLINGS, CW_USEDEFAULT,
                                       CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT, videoWindow, nullptr, nullptr,
                                       nullptr);

        bar = CreateWindowExW(0, WC_STATICW, nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_CLIPSIBLINGS,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT, videoWindow, nullptr, nullptr, nullptr);
        SetWindowSubclass(bar, progressBarProc, 1, reinterpret_cast<DWORD_PTR>(this));

        SetWindowLongPtr(videoWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

        setClientSize(width, height);
        // A cloaked visible window can receive Vulkan presents without exposing its empty surface before the first one completes.
        setWindowCloaked(videoWindow, true);
        ShowWindow(videoWindow, SW_SHOWNA);
        UpdateWindow(videoWindow);
    }

    void VideoWindow::createScene(const VkExtent2D &videoExtent, const Attribute &targetAttribute) {
        const auto wc = engine.
                attachWindowContext(renderWindow, Constants::VulkanWindow::VIDEO_WINDOW_ATTACHMENT_INDEX);
        scene = std::make_unique<VideoRenderScene>(engine, *wc, videoExtent, targetAttribute);
    }

    void VideoWindow::destroy() {
        engine.getCore().getLogicalDevice().waitDeviceIdle();
        scene = nullptr;
        engine.detachWindowContext(Constants::VulkanWindow::VIDEO_WINDOW_ATTACHMENT_INDEX);
        if (IsWindow(videoWindow)) {
            DestroyWindow(videoWindow);
        }
    }
}
