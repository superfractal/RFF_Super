//
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21.
// Modified by Opus 5 on 2026-08-14, 2026-08-15, 2026-08-23, 2026-09-01
//

#include <chrono>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <string>

#ifndef NDEBUG
#include <fstream>
#endif

#include "Application.hpp"
#include "../io/PreferencesIO.h"
#include "Utilities.h"
#include "SettingsWindow.hpp"
#include "VideoWindow.hpp"
#include "../../vulkan_helper/util/GraphicsContextWindowProc.hpp"

void registerClasses() {
    using namespace merutilm::rff2;
    using namespace Constants::Win32;
    using namespace merutilm::vkh;
    WNDCLASSEXW wClass = {};
    wClass.cbSize = sizeof(WNDCLASSEXW);
    wClass.hInstance = GetModuleHandleW(nullptr);

    WNDCLASSEXW masterWindowClass = wClass;
    masterWindowClass.lpszClassName = CLASS_MASTER_WINDOW;
    masterWindowClass.lpfnWndProc = GraphicsContextWindowProc::WinProc;
    // Without a background brush the client area shows whatever the compositor last left there
    // until the first frame is presented, which is the flash of stale pixels as the window opens.
    masterWindowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    masterWindowClass.hIcon = static_cast<HICON>(LoadImageW(
    GetModuleHandleW(nullptr),
    MAKEINTRESOURCEW(1),
    IMAGE_ICON,
    32, 32,
    LR_DEFAULTCOLOR));
    if(!RegisterClassExW(&masterWindowClass)) throw exception_init("Failed to register class : Master Window");

    WNDCLASSEXW videoWindowClass = wClass;
    videoWindowClass.lpszClassName = CLASS_VIDEO_WINDOW;
    videoWindowClass.lpfnWndProc = VideoWindow::videoWindowProc;
    videoWindowClass.hIcon = masterWindowClass.hIcon;
    if(!RegisterClassExW(&videoWindowClass)) throw exception_init("Failed to register class : Video Window");

    WNDCLASSEXW settingsWindowClass = wClass;
    settingsWindowClass.lpszClassName = CLASS_SETTINGS_WINDOW;
    settingsWindowClass.lpfnWndProc = SettingsWindow::settingsWindowProc;
    settingsWindowClass.hbrBackground = CreateSolidBrush(COLOR_LABEL_BACKGROUND);
    if(!RegisterClassExW(&settingsWindowClass)) throw exception_init("Failed to register class : Settings Window");

    WNDCLASSEXW videoRenderWindowClass = wClass;
    videoRenderWindowClass.lpszClassName = CLASS_VIDEO_RENDER_WINDOW;
    videoRenderWindowClass.lpfnWndProc = DefWindowProcW;
    if (!RegisterClassExW(&videoRenderWindowClass)) throw exception_init("Failed to register class : Video Window");

    WNDCLASSEXW vkRenderSceneClass = wClass;
    vkRenderSceneClass.lpszClassName = CLASS_VK_RENDER_SCENE;
    vkRenderSceneClass.lpfnWndProc = RenderScene::renderSceneProc;
    // The canvas opens on the color the fractal's interior is drawn in, so the first presented frame
    // replaces black with black instead of the window lighting up on its way in.
    vkRenderSceneClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    if(!RegisterClassExW(&vkRenderSceneClass)) throw exception_init("Failed to register class : Video Scene");

    WNDCLASSEXW boxZoomOverlayClass = wClass;
    boxZoomOverlayClass.lpszClassName = CLASS_BOX_ZOOM_OVERLAY;
    boxZoomOverlayClass.lpfnWndProc = RenderScene::boxZoomOverlayProc;
    // No erase brush and nothing painted here: the overlay is handed its whole picture, position
    // and size at once through UpdateLayeredWindow, so it never sits on screen mid-repaint.
    boxZoomOverlayClass.hbrBackground = nullptr;
    if(!RegisterClassExW(&boxZoomOverlayClass)) throw exception_init("Failed to register class : Box Zoom Overlay");

}

#ifndef NDEBUG

void counter(const std::filesystem::path &path, uint32_t *lines) {
    if (std::filesystem::is_directory(path)) {
        for (std::filesystem::directory_iterator it(path); it != std::filesystem::directory_iterator(); ++it) {
            auto child = it->path();
            counter(child, lines);
        }
    }else if (path.string().ends_with(".cpp") || path.string().ends_with(".hpp")){

        std::ifstream ifs(path);
        std::string v;
        while (std::getline(ifs, v)) {
            ++*lines;
        }
    }
}

void countLines() {
    const std::filesystem::path path("../src");
    uint32_t lines = 0;
    counter(path, &lines);
    std::cout << "Lines : " << lines << std::endl;

}
#endif

// Catches a CPU-incompatible-build crash. The Release binary is compiled with
// -march=native, so an older CPU that lacks the required instruction set (AVX,
// FMA, ...) raises an illegal-instruction fault. Show a clear message instead
// of terminating silently.
static LONG WINAPI unsupportedCpuFilter(EXCEPTION_POINTERS *info) {
    if (info && info->ExceptionRecord &&
        info->ExceptionRecord->ExceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION) {
        MessageBoxW(nullptr,
            L"Your CPU does not support the instruction set (AVX / FMA, etc.) this program requires.\n\n"
            L"This build is optimized for the build machine's CPU (-march=native).\n"
            L"Run on a newer CPU, or rebuild without -march=native for better compatibility.",
            L"Unsupported CPU",
            MB_OK | MB_ICONERROR);
        return EXCEPTION_EXECUTE_HANDLER;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

// The text of whatever exception is being handled right now, or an empty string outside one.
static std::string activeExceptionText() {
    const std::exception_ptr active = std::current_exception();
    if (active == nullptr) {
        return {};
    }
    try {
        std::rethrow_exception(active);
    } catch (const std::exception &e) {
        return e.what();
    } catch (...) {
        return "a non-standard exception";
    }
}

// An exception that reaches no catch ends the process without a word, and one thrown inside a window
// procedure cannot cross back over the Win32 callback boundary to be caught at all. Both arrive here,
// where what happened is written down and said out loud instead of the program simply vanishing.
[[noreturn]] static void reportFatalError() {
    const std::string text = activeExceptionText();
    const std::string message = text.empty() ? "The program stopped on an unexpected error." : text;
    try {
        const std::filesystem::path path = merutilm::rff2::Utilities::getDefaultPath() / L"crash-report.log";
        std::filesystem::create_directories(path.parent_path());
        std::ofstream out(path, std::ios::app);
        if (out) {
            const auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
            out << std::format("{:%Y-%m-%d %H:%M:%S}", now) << "  " << message << "\n";
        }
    } catch (...) {
        // Reporting the failure must not fail in turn.
    }
    MessageBoxA(nullptr, message.c_str(), "RFF_Super stopped", MB_OK | MB_ICONERROR);
    std::_Exit(3);
}

// True when the failure points at a missing/incompatible Vulkan GPU or driver.
static bool isVulkanGpuFailure(const std::string &message) {
    return message.find("No suitable physical device") != std::string::npos ||
           message.find("Failed to create instance") != std::string::npos;
}

int main() {
    using namespace merutilm::rff2;
    using namespace merutilm::vkh;

    SetUnhandledExceptionFilter(unsupportedCpuFilter);
    std::set_terminate(reportFatalError);

    try {
        // Before anything is built: every window and panel is dressed as it is created.
        PreferencesIO::load();
        registerClasses();
#ifndef NDEBUG
        countLines();
#endif
        const auto app = Application();
        app.start();
        // start() returns only when the window was closed, which is not a crash - but a window
        // closed on a compute that never finished is still a run to offer back, so the scene is
        // asked before anything of it is torn down.
        app.endRecoverySession();
    } catch (const merutilm::vkh::exception &e) {
        if (isVulkanGpuFailure(e.what())) {
            MessageBoxW(nullptr,
                L"No Vulkan-capable GPU was found.\n\n"
                L"Possible causes:\n"
                L"- The GPU (or its driver) does not support Vulkan\n"
                L"- The graphics driver is outdated or not installed\n"
                L"- No supported GPU (a Vulkan-capable discrete GPU) is present\n\n"
                L"Install the latest GPU driver and try again.",
                L"Unsupported GPU",
                MB_OK | MB_ICONERROR);
        } else {
            MessageBoxA(nullptr, e.what(), "Error", MB_OK | MB_ICONERROR);
        }
        return 1;
    } catch (const std::exception &e) {
        MessageBoxA(nullptr, e.what(), "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    return 0;
}
