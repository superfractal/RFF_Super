// Modified by GPT-6 on 2026-09-25
#pragma once
#include <windows.h>
#include <array>
#include <algorithm>
#include <cstdint>
#include <charconv>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace merutilm::rff2 {
    struct AudioSourceInfo {
        static std::optional<int64_t> duration(const std::filesystem::path &source) {
            struct Handle {
                HANDLE value = nullptr;
                ~Handle() { if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value); }
            } read, write, null, process, thread;
            std::wstring probe = L"ffprobe.exe";
            std::array<wchar_t, 32768> module{};
            const auto count = GetModuleFileNameW(nullptr, module.data(), DWORD(module.size()));
            if (count && count < module.size()) {
                auto local = std::filesystem::path(module.data()).parent_path() / L"ffprobe.exe";
                if (GetFileAttributesW(local.c_str()) != INVALID_FILE_ATTRIBUTES) probe = local.wstring();
            }
            auto command = L"\"" + probe + L"\" -v quiet -select_streams a:0 "
                           L"-show_entries stream=index,duration:format=duration -of default=noprint_wrappers=1 \"" +
                           std::filesystem::absolute(source).wstring() + L"\"";
            if (command.size() >= 32767) return {};
            SECURITY_ATTRIBUTES security{sizeof(security), nullptr, TRUE};
            if (!CreatePipe(&read.value, &write.value, &security, 8192) ||
                !SetHandleInformation(read.value, HANDLE_FLAG_INHERIT, 0)) return {};
            null.value = CreateFileW(L"NUL", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     &security, OPEN_EXISTING, 0, nullptr);
            if (null.value == INVALID_HANDLE_VALUE) return {};
            STARTUPINFOEXW startup{};
            startup.StartupInfo.cb = sizeof(startup);
            startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
            startup.StartupInfo.hStdInput = null.value;
            startup.StartupInfo.hStdError = null.value;
            startup.StartupInfo.hStdOutput = write.value;
            SIZE_T size = 0;
            InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
            std::vector<unsigned char> attributes(size);
            startup.lpAttributeList = reinterpret_cast<PPROC_THREAD_ATTRIBUTE_LIST>(attributes.data());
            if (!InitializeProcThreadAttributeList(startup.lpAttributeList, 1, 0, &size)) return {};
            std::array handles{write.value, null.value};
            PROCESS_INFORMATION info{};
            const bool started = UpdateProcThreadAttribute(startup.lpAttributeList, 0,
                PROC_THREAD_ATTRIBUTE_HANDLE_LIST, handles.data(), sizeof(handles), nullptr, nullptr) &&
                CreateProcessW(nullptr, command.data(), nullptr, nullptr, TRUE,
                    CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT, nullptr, nullptr, &startup.StartupInfo, &info);
            DeleteProcThreadAttributeList(startup.lpAttributeList);
            if (!started) return {};
            process.value = info.hProcess;
            thread.value = info.hThread;
            CloseHandle(write.value);
            write.value = nullptr;
            if (WaitForSingleObject(process.value, 5000) != WAIT_OBJECT_0) {
                TerminateProcess(process.value, ERROR_TIMEOUT);
                WaitForSingleObject(process.value, 1000);
                return {};
            }
            DWORD exitCode = 1;
            if (!GetExitCodeProcess(process.value, &exitCode) || exitCode) return {};
            std::array<char, 8192> output{};
            DWORD bytes = 0;
            if (!ReadFile(read.value, output.data(), DWORD(output.size()), &bytes, nullptr)) return {};
            const std::string text(output.data(), bytes);
            if (text.find("index=") == std::string::npos) return {};
            for (size_t at = text.find("duration="); at != std::string::npos; at = text.find("duration=", at + 9)) {
                const char *first = text.data() + at + 9;
                double seconds = 0;
                const auto parsed = std::from_chars(first, text.data() + text.size(), seconds);
                if (*first >= '0' && *first <= '9' && parsed.ec == std::errc{} && seconds > 0 && seconds <= 604800) {
                    return std::max<int64_t>(1, static_cast<int64_t>(seconds * 1000000 + 0.5));
                }
            }
            return {};
        }
    };
}
