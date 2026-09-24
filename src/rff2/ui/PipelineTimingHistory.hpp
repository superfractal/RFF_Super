//
// Modified by GPT-6 on 2026-09-18, 2026-09-22
//

#pragma once
#include <bit>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <string>
#include <windows.h>

namespace merutilm::rff2 {
    class PipelineTimingHistory {
        std::filesystem::path path;
        std::map<std::string, double> timings;
        static bool valid(double seconds) {
            return (std::bit_cast<uint64_t>(seconds) & 0x7ff0000000000000ULL) != 0x7ff0000000000000ULL &&
                   seconds >= 2 && seconds <= 3600;
        }

      public:
        explicit PipelineTimingHistory(std::filesystem::path file) : path(std::move(file)) {
            std::error_code error;
            if (std::filesystem::file_size(path, error) > 65536 || error) {
                return;
            }
            std::ifstream input(path);
            std::string key;
            double seconds;
            while (timings.size() < 256 && input >> std::quoted(key) >> seconds) {
                if (key.size() <= 512 && valid(seconds)) {
                    timings[key] = seconds;
                }
            }
        }
        double estimate(const std::string &key) const {
            const auto found = timings.find(key);
            return found == timings.end() ? 0 : found->second;
        }
        void record(const std::string &key, double seconds) {
            // Cache hits must not replace the measured cost of compiling a shader.
            if (!valid(seconds) || key.size() > 512 || (timings.size() >= 256 && !timings.contains(key))) {
                return;
            }
            timings[key] = seconds;
            auto temporary = path;
            temporary += L"." + std::to_wstring(GetCurrentProcessId()) + L".tmp";
            std::ofstream output(temporary, std::ios::trunc);
            for (const auto &[name, duration] : timings) {
                output << std::quoted(name) << ' ' << duration << '\n';
            }
            output.close();
            if (output) {
                MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING);
            }
        }
    };
} // namespace merutilm::rff2
