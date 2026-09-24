//
// Modified by GPT-6 on 2026-09-20, 2026-09-21, 2026-09-23
//

#pragma once
#include "../attr/ShaderAttribute.h"
#include "../data/Matrix.h"
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace merutilm::rff2 {
    struct LocalAiSettings {
        using Json = nlohmann::json;
        using Progress = std::function<void(const std::string &)>;
        using Transport = std::function<Json(const Json &)>;
        struct Statistics {
            int64_t context = -1;
            int64_t prompt = -1;
            int64_t generated = -1;
            double tokensPerSecond = -1;
            bool estimated = true;
        };

        using OnStatistics = std::function<void(const Statistics &)>;
        static Json serverInfo(const Json &connection, const std::atomic_bool &cancelled);
        static void stopConfiguredServer(const std::filesystem::path &serverConfig = "local-ai-server.json",
                                         const std::filesystem::path &connectionConfig = "local-ai.json");
        struct Result {
            Json patch;
            std::string summary;
            int attempts = 0;
            double score = -1;
            bool satisfied = false;
        };

        struct ZoomTarget {
            double x = 0.5;
            double y = 0.5;
            bool stop = false;
            std::string summary;
        };

        static Json analyzeIterations(const Matrix<double> &sample, uint64_t maxIteration);
        static double smallerExplorationZoom(double factor);
        static double zoomFactor(const std::string &text);
        static float retryLogZoom(float current, const std::string &decrement, float minimum);
        static ZoomTarget chooseZoomTarget(
            const std::string &instruction, const std::string &imageDataUrl,
            const Json &connection, const std::atomic_bool &cancelled, const Progress &progress,
            const Progress &onToken = {}, const Progress &onReasoning = {},
            const OnStatistics &onStatistics = {}, const Transport &transport = {},
            const Json &evidence = Json::object(), const std::vector<std::string> &crops = {});
        static int refinementLimit(const Json &connection);
        static int64_t estimatePrompt(const Json &messages);
        static Json catalog(const ShaderAttribute &shader);
        static std::string systemPrompt(const ShaderAttribute &shader,
                                        const std::filesystem::path &path = "local-ai-system-prompt.md");
        static ShaderAttribute apply(const ShaderAttribute &original, const Json &patch);
        static Json readConnection(const std::filesystem::path &path = "local-ai.json");
        static int errorLimit(const Json &connection);
        static void saveErrorLimit(int limit, const std::filesystem::path &path = "local-ai.json");
        static Json post(const Json &connection, const Json &request, const std::atomic_bool &cancelled,
                         const Progress &onToken = {}, const Progress &onReasoning = {},
                         const OnStatistics &onStatistics = {});
        static Result generate(const ShaderAttribute &original, const std::string &instruction,
                               const Json &connection, const std::atomic_bool &cancelled,
                               const Progress &progress, const Transport &transport = {},
                               const Progress &onToken = {}, const Progress &onReasoning = {},
                               const OnStatistics &onStatistics = {}, const std::string &imageDataUrl = {},
                               const std::string &history = {});
    };
}
