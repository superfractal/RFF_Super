// Modified by GPT-6 on 2026-09-26
#pragma once
#include "AudioExport.hpp"
#include <windows.h>
#include <mmsystem.h>
#include <array>
#include <atomic>
#include <fstream>
#include <thread>

namespace merutilm::rff2 {
    class AudioPreview {
        HANDLE process = nullptr, input = nullptr, stopEvent = nullptr;
        std::thread worker;
        std::atomic<bool> failed = false;
        std::filesystem::path filterPath, logPath;
        inline static std::atomic<unsigned> counter = 0;

        void playPcm(ULONGLONG started) {
            const HANDLE done = CreateEventW(nullptr, FALSE, FALSE, nullptr);
            HWAVEOUT device = nullptr;
            WAVEFORMATEX format{WAVE_FORMAT_PCM, 2, 48000, 192000, 4, 16, 0};
            if (!done || waveOutOpen(&device, WAVE_MAPPER, &format, DWORD_PTR(done), 0, CALLBACK_EVENT) != MMSYSERR_NOERROR) {
                failed = true;
                if (done) CloseHandle(done);
                return;
            }
            std::array<std::array<char, 4096>, 3> data{};
            std::array<WAVEHDR, 3> headers{};
            size_t prepared = 0;
            for (size_t i = 0; i < headers.size(); ++i) {
                headers[i].lpData = data[i].data();
                headers[i].dwBufferLength = DWORD(data[i].size());
                if (waveOutPrepareHeader(device, &headers[i], sizeof(WAVEHDR)) != MMSYSERR_NOERROR) { failed = true; break; }
                ++prepared;
            }
            uint64_t decoded = 0;
            size_t slot = 0;
            bool startedPlayback = false;
            const std::array wait{stopEvent, done};
            while (prepared == headers.size() && WaitForSingleObject(stopEvent, 0) != WAIT_OBJECT_0) {
                auto &header = headers[slot];
                while ((header.dwFlags & WHDR_INQUEUE) && !(header.dwFlags & WHDR_DONE)) {
                    const DWORD result = WaitForMultipleObjects(2, wait.data(), FALSE, 1000);
                    if (result == WAIT_OBJECT_0 || result == WAIT_FAILED) break;
                }
                if (WaitForSingleObject(stopEvent, 0) == WAIT_OBJECT_0) break;
                DWORD size = 0;
                if (!ReadFile(input, data[slot].data(), DWORD(data[slot].size()), &size, nullptr) || size == 0) break;
                size -= size % 4;
                if (!size) continue;
                const uint64_t target = (GetTickCount64() - started) * 192;
                if ((!startedPlayback || decoded + size + 48000 < target) && decoded + size < target) {
                    decoded += size;
                    continue;
                }
                decoded += size;
                header.dwBufferLength = size;
                ResetEvent(done);
                if (waveOutWrite(device, &header, sizeof(header)) != MMSYSERR_NOERROR) { failed = true; break; }
                startedPlayback = true;
                slot = (slot + 1) % headers.size();
            }
            if (!failed && WaitForSingleObject(stopEvent, 0) != WAIT_OBJECT_0) {
                for (auto &header : headers) {
                    while ((header.dwFlags & WHDR_INQUEUE) && !(header.dwFlags & WHDR_DONE)) {
                        if (WaitForMultipleObjects(2, wait.data(), FALSE, 1000) != WAIT_OBJECT_0 + 1) break;
                    }
                }
            }
            waveOutReset(device);
            for (size_t i = 0; i < prepared; ++i) waveOutUnprepareHeader(device, &headers[i], sizeof(WAVEHDR));
            waveOutClose(device);
            CloseHandle(done);
            if (WaitForSingleObject(stopEvent, 0) != WAIT_OBJECT_0) {
                DWORD code = 0;
                if (WaitForSingleObject(process, 1000) == WAIT_OBJECT_0 && GetExitCodeProcess(process, &code) && code != 0) failed = true;
            }
        }

    public:
        AudioPreview() = default;
        AudioPreview(const AudioPreview &) = delete;
        AudioPreview &operator=(const AudioPreview &) = delete;
        ~AudioPreview() { stop(); }
        const std::filesystem::path &diagnosticLog() const { return logPath; }
        bool takeFailure() { return failed.exchange(false); }
        void stop() {
            if (stopEvent) SetEvent(stopEvent);
            if (process) TerminateProcess(process, ERROR_CANCELLED);
            if (worker.joinable()) worker.join();
            if (process) { WaitForSingleObject(process, 1000); CloseHandle(process); process = nullptr; }
            if (input) { CloseHandle(input); input = nullptr; }
            if (stopEvent) { CloseHandle(stopEvent); stopEvent = nullptr; }
            if (!filterPath.empty()) { std::error_code error; std::filesystem::remove(filterPath, error); filterPath.clear(); }
        }
        bool start(const VidAudioAttribute &audio, double seconds, double total) {
            stop();
            failed = false;
            if (seconds >= total) return true;
            HANDLE output = nullptr, null = INVALID_HANDLE_VALUE, log = INVALID_HANDLE_VALUE;
            const auto closeLocals = [&] {
                if (output) { CloseHandle(output); output = nullptr; }
                if (null != INVALID_HANDLE_VALUE) { CloseHandle(null); null = INVALID_HANDLE_VALUE; }
                if (log != INVALID_HANDLE_VALUE) { CloseHandle(log); log = INVALID_HANDLE_VALUE; }
            };
            try {
                auto plan = AudioExport::prepare(audio, static_cast<int64_t>(std::llround(seconds * 1000000)));
                if (plan.inputs.empty()) return true;
                std::array<wchar_t, 32768> module{};
                std::wstring ffmpeg = L"ffmpeg.exe";
                const DWORD length = GetModuleFileNameW(nullptr, module.data(), DWORD(module.size()));
                if (length && length < module.size()) {
                    auto local = std::filesystem::path(module.data()).parent_path() / L"ffmpeg.exe";
                    if (GetFileAttributesW(local.c_str()) != INVALID_FILE_ATTRIBUTES) ffmpeg = local.wstring();
                }
                filterPath = std::filesystem::temp_directory_path() /
                    std::format(L"rff-audio-preview-{}-{}.txt", GetCurrentProcessId(), counter++);
                std::ofstream file(filterPath, std::ios::binary);
                file << plan.filters;
                file.close();
                if (!file) throw std::runtime_error("Cannot create preview audio filters");
                std::wstring command = L"\"" + ffmpeg + L"\" -nostdin -v error -f lavfi -i anullsrc=r=48000:cl=stereo";
                for (const auto &source : plan.inputs) command += L" -i \"" + source.wstring() + L"\"";
                command += L" -filter_complex_script \"" + filterPath.wstring() + L"\" -map [audio] -t " +
                    std::format(L"{:.6f}", total - seconds) + L" -f s16le -ac 2 -ar 48000 pipe:1";
                if (command.size() >= 32767) throw std::runtime_error("Audio command is too long");
                SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
                if (!CreatePipe(&input, &output, &sa, 65536) || !SetHandleInformation(input, HANDLE_FLAG_INHERIT, 0))
                    throw std::runtime_error("Cannot create preview audio pipe");
                null = CreateFileW(L"NUL", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, nullptr);
                if (logPath.empty()) { logPath = filterPath; logPath += L".log"; }
                log = CreateFileW(logPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
                stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
                if (null == INVALID_HANDLE_VALUE || log == INVALID_HANDLE_VALUE || !stopEvent) throw std::runtime_error("Cannot create preview handles");
                STARTUPINFOEXW startup{};
                startup.StartupInfo.cb = sizeof(startup);
                startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
                startup.StartupInfo.hStdInput = null;
                startup.StartupInfo.hStdError = log;
                startup.StartupInfo.hStdOutput = output;
                SIZE_T size = 0;
                InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
                std::vector<unsigned char> attributes(size);
                startup.lpAttributeList = reinterpret_cast<PPROC_THREAD_ATTRIBUTE_LIST>(attributes.data());
                if (!InitializeProcThreadAttributeList(startup.lpAttributeList, 1, 0, &size)) throw std::runtime_error("Cannot initialize preview process");
                std::array handles{output, null, log};
                PROCESS_INFORMATION info{};
                const ULONGLONG started = GetTickCount64();
                const bool launched = UpdateProcThreadAttribute(startup.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                    handles.data(), sizeof(handles), nullptr, nullptr) &&
                    CreateProcessW(nullptr, command.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT,
                                   nullptr, nullptr, &startup.StartupInfo, &info);
                DeleteProcThreadAttributeList(startup.lpAttributeList);
                if (!launched) throw std::runtime_error("Cannot launch FFmpeg preview");
                process = info.hProcess;
                CloseHandle(info.hThread);
                closeLocals();
                worker = std::thread([this, started] { playPcm(started); });
                return true;
            } catch (...) {
                closeLocals();
                stop();
                return false;
            }
        }
    };
}
