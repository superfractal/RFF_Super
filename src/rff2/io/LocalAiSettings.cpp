//
// Modified by GPT-6 on 2026-09-20, 2026-09-21, 2026-09-23, 2026-09-24
//

#include "LocalAiSettings.hpp"
#include "ShaderPresetIO.h"
#include "../video/TimelineParams.hpp"
#include "../ui/workspace/SurfaceColorRegistry.hpp"
#include "../attr/SurfaceStyleRecipe.hpp"
#include <windows.h>
#include <winhttp.h>
#include <fstream>
#include <iterator>
#include <set>
#include <chrono>
#include <thread>
#include <iphlpapi.h>

namespace merutilm::rff2 {
    namespace {
        using Json = LocalAiSettings::Json;
        std::filesystem::path resolveConfigPath(const std::filesystem::path &path) {
            if (path.is_absolute() || std::filesystem::exists(path)) {
                return std::filesystem::absolute(path);
            }

            std::wstring executable(32768, L'\0');
            const auto length = GetModuleFileNameW(nullptr, executable.data(), DWORD(executable.size()));
            if (length && length < executable.size()) {
                executable.resize(length);
                const auto folder = std::filesystem::path(executable).parent_path();
                for (const auto &base : {folder, folder.parent_path()}) {
                    if (std::filesystem::exists(base / path)) {
                        return base / path;
                    }
                }
            }

            return std::filesystem::absolute(path);
        }

        std::string toUtf8(std::wstring_view text) {
            if (text.empty()) {
                return {};
            }

            const int byteCount = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(),
                                                      int(text.size()), nullptr, 0, nullptr, nullptr);
            if (!byteCount) {
                throw std::runtime_error("Invalid Unicode text");
            }

            std::string result(byteCount, 0);
            WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), int(text.size()), result.data(),
                                byteCount, nullptr, nullptr);
            return result;
        }

        std::wstring toWide(const std::string &text) {
            const int characterCount =
                MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), nullptr, 0);
            if (!characterCount) {
                throw std::runtime_error("Invalid UTF-8 connection setting");
            }

            std::wstring result(characterCount, 0);
            MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), result.data(),
                                characterCount);
            return result;
        }

        struct Field {
            std::string key;
            std::string label;
            Json rule;
            std::function<Json(const ShaderAttribute &)> get;
            std::function<void(ShaderAttribute &, const Json &)> set;
        };

        double readNumber(const Json &value, double minimum, double maximum, bool requireInteger = false) {
            if (!value.is_number()) {
                throw std::runtime_error("expected a JSON number");
            }

            const double number = value.get<double>();
            if (!std::isfinite(number) || number < minimum || number > maximum ||
                (requireInteger && std::trunc(number) != number)) {
                throw std::runtime_error("expected " + std::string(requireInteger ? "integer" : "number") +
                                         " in [" + std::to_string(minimum) + ", " + std::to_string(maximum) +
                                         "]");
            }

            return number;
        }

        glm::vec4 readColor(const Json &value) {
            if (!value.is_array() || value.size() != 4) {
                throw std::runtime_error("expected [red,green,blue,alpha], four numbers in [0,1]");
            }

            glm::vec4 rgba;
            for (int channel = 0; channel < 4; ++channel) {
                rgba[channel] = float(readNumber(value[channel], 0, 1));
            }

            return rgba;
        }

        Json writeColor(const glm::vec4 &rgba) {
            return Json::array({rgba.r, rgba.g, rgba.b, rgba.a});
        }

        template <typename Accessor>
        void appendShaderField(std::vector<Field> &registeredFields,
                               std::set<const void *> &registeredAddresses,
                               const ShaderAttribute &referenceShader, std::string key, std::string label,
                               Accessor access, double minimum, double maximum, Json options = Json()) {
            if (!registeredAddresses.insert(&access(referenceShader)).second) {
                return;
            }

            using Value = std::remove_cvref_t<decltype(access(referenceShader))>;
            Json rule;
            if constexpr (std::is_same_v<Value, glm::vec4>) {
                rule = {{"type", "rgba"}};
            } else if constexpr (std::is_same_v<Value, bool>) {
                rule = {{"type", "boolean"}};
            } else {
                rule = {{"type", std::is_enum_v<Value> || std::is_integral_v<Value> ? "integer" : "number"},
                        {"minimum", minimum},
                        {"maximum", maximum}};
                if (!options.is_null()) {
                    rule["options"] = options;
                }
            }
            registeredFields.push_back(
                {key, label, rule,
                 [access](const auto &shader) -> Json {
                     if constexpr (std::is_same_v<Value, glm::vec4>) {
                         return writeColor(access(shader));
                     } else if constexpr (std::is_enum_v<Value>) {
                         return int(access(shader));
                     } else {
                         return access(shader);
                     }
                 },
                 [access, minimum, maximum, options](auto &shader, const Json &value) {
                     if constexpr (std::is_same_v<Value, glm::vec4>) {
                         access(shader) = readColor(value);
                     } else if constexpr (std::is_same_v<Value, bool>) {
                         if (!value.is_boolean()) {
                             throw std::runtime_error("expected boolean");
                         }

                         access(shader) = value.get<bool>();
                     } else {
                         const auto number = readNumber(value, minimum, maximum,
                                                        std::is_enum_v<Value> || std::is_integral_v<Value>);
                         if (!options.is_null() && !options.contains(std::to_string(int(number)))) {
                             throw std::runtime_error("invalid option");
                         }

                         access(shader) = Value(number);
                     }
                 }});
        }

        const std::vector<Field> &shaderFields() {
            static const auto list = [] {
                std::vector<Field> registeredFields;
                const ShaderAttribute referenceShader{};
                std::set<const void *> registeredAddresses;
                for (const auto &parameter : workspace::surfaceParameters()) {
                    registeredAddresses.insert(&parameter.value(referenceShader.slope));
                    if (workspace::paletteLineControl(parameter.id)) {
                        continue;
                    }

                    registeredFields.push_back({std::string(parameter.id),
                                                toUtf8(parameter.label),
                                                {{"type", parameter.isInteger() ? "integer" : "number"},
                                                 {"minimum", parameter.minimum},
                                                 {"maximum", parameter.maximum}},
                                                [parameter](const auto &shader) {
                                                    return Json(parameter.value(shader.slope));
                                                },
                                                [parameter](auto &shader, const Json &value) {
                                                    const float number = float(
                                                        readNumber(value, parameter.minimum,
                                                                   parameter.maximum, parameter.isInteger()));
                                                    if (!parameter.valid(number)) {
                                                        throw std::runtime_error("invalid surface value");
                                                    }

                                                    parameter.value(shader.slope) = number;
                                                    parameter.activate(shader.slope);
                                                }});
                }

                for (const auto &parameter : workspace::surfaceColors()) {
                    registeredAddresses.insert(&(referenceShader.slope.*parameter.member));
                    if (workspace::paletteLineControl(parameter.id)) {
                        continue;
                    }

                    registeredFields.push_back({std::string(parameter.id),
                                                toUtf8(parameter.label),
                                                {{"type", "rgba"}},
                                                [parameter](const auto &shader) {
                                                    return writeColor(shader.slope.*parameter.member);
                                                },
                                                [parameter](auto &shader, const Json &value) {
                                                    shader.slope.*parameter.member = readColor(value);
                                                    activateSurfaceColor(shader.slope,
                                                                         &(shader.slope.*parameter.member));
                                                }});
                }

                for (const auto &parameter : TimelineParams::all()) {
                    if (std::wstring_view(parameter.group).starts_with(L"Texture")) {
                        continue;
                    }

                    if (hasTimelineDirty(parameter.dirty, TimelineDirtyMask::CAMERA) || !parameter.address) {
                        continue;
                    }

                    const auto *address = parameter.address(referenceShader);
                    if (!registeredAddresses.insert(address).second) {
                        continue;
                    }

                    const auto key = "param." + std::to_string(parameter.id);
                    const auto label = toUtf8(parameter.group) + " / " + toUtf8(parameter.name);
                    if (parameter.kind == TimelineParamKind::COLOR) {
                        registeredFields.push_back({key,
                                                    label,
                                                    {{"type", "rgba"}},
                                                    [parameter](const auto &shader) {
                                                        return writeColor(parameter.getColor(shader));
                                                    },
                                                    [parameter](auto &shader, const Json &value) {
                                                        parameter.setColor(shader, readColor(value));
                                                    }});
                    } else if (parameter.kind == TimelineParamKind::BOOL) {
                        registeredFields.push_back({key,
                                                    label,
                                                    {{"type", "boolean"}},
                                                    [parameter](const auto &shader) {
                                                        return Json(parameter.getValue(shader) != 0);
                                                    },
                                                    [parameter](auto &shader, const Json &value) {
                                                        if (!value.is_boolean()) {
                                                            throw std::runtime_error(
                                                                "expected true or false");
                                                        }

                                                        parameter.setValue(shader, value.get<bool>() ? 1 : 0);
                                                    }});
                    } else {
                        registeredFields.push_back(
                            {key,
                             label,
                             {{"type", parameter.kind == TimelineParamKind::ENUM ? "integer" : "number"},
                              {"minimum", parameter.minValue},
                              {"maximum", parameter.maxValue}},
                             [parameter](const auto &shader) {
                                 return Json(parameter.getValue(shader));
                             },
                             [parameter](auto &shader, const Json &value) {
                                 parameter.setValue(
                                     shader, float(readNumber(value, parameter.minValue, parameter.maxValue,
                                                              parameter.kind == TimelineParamKind::ENUM)));
                             }});
                    }
                }

                registeredFields.push_back(
                    {"surface.style",
                     "Surface recipe: 0 Original, 1 Liquid Metal, 2 Cyber Sigilism, 3 Phonk, 4 Black Metal, "
                     "5 Deep Sea, 6 Ukiyo-e",
                     {{"type", "integer"}, {"minimum", 0}, {"maximum", 6}},
                     [](const auto &shader) {
                         return Json(int(shader.slope.surfaceStyle));
                     },
                     [](auto &shader, const Json &value) {
                         applySurfaceStyleRecipe(shader, ShdSurfaceStyle(int(readNumber(value, 0, 6, true))));
                     }});
                registeredFields.push_back({"surface.studio.enabled",
                                            "Enable physically based studio lighting",
                                            {{"type", "boolean"}},
                                            [](const auto &shader) {
                                                return Json(shader.slope.studio.use);
                                            },
                                            [](auto &shader, const Json &value) {
                                                if (!value.is_boolean()) {
                                                    throw std::runtime_error("expected boolean");
                                                }

                                                shader.slope.studio.use = value.get<bool>();
                                            }});
                registeredFields.push_back(
                    {"palette.colors",
                     "Cyclic palette stops, 2 to 64 RGBA colors",
                     {{"type", "palette"}},
                     [](const auto &shader) {
                         Json colors = Json::array();
                         for (const auto &rgba : shader.palette.colors) {
                             colors.push_back(writeColor(rgba));
                         }

                         return colors;
                     },
                     [](auto &shader, const Json &value) {
                         if (!value.is_array() || value.size() < 2 || value.size() > 64) {
                             throw std::runtime_error("palette requires 2 to 64 RGBA stops");
                         }

                         std::vector<glm::vec4> colors;
                         for (const auto &stop : value) {
                             auto rgba = readColor(stop);
                             if (rgba.a != 1) {
                                 throw std::runtime_error(
                                     "palette alpha must be 1 for lossless preset storage");
                             }

                             colors.push_back(rgba);
                         }

                         shader.palette.colors = std::move(colors);
                         shader.palette.recipePresetId = -1;
                         shader.palette.recipeSeed = 0;
                     }});
#include "LocalAiShaderFields.inc"
                return registeredFields;
            }();
            return list;
        }

        class ResponseStatistics {
            LocalAiSettings::Statistics statistics;
            size_t outputBytes = 0;
            std::chrono::steady_clock::time_point firstToken{};
            Json usage;
            const LocalAiSettings::OnStatistics &onStatistics;

          public:
            ResponseStatistics(const Json &connection, const Json &request,
                               const LocalAiSettings::OnStatistics &callback)
                : onStatistics(callback) {
                statistics.context = connection.value("runtime_context", int64_t(-1));
                if (request.contains("messages")) {
                    statistics.prompt = LocalAiSettings::estimatePrompt(request["messages"]);
                }
            }

            const Json &getUsage() const {
                return usage;
            }

            void update(const Json &chunk) {
                if (chunk.contains("choices") && !chunk["choices"].empty()) {
                    const auto &choice = chunk["choices"][0];
                    const auto delta = choice.value("delta", choice.value("message", Json::object()));
                    for (const auto *key : {"content", "reasoning_content", "reasoning"}) {
                        if (delta.contains(key) && delta[key].is_string()) {
                            outputBytes += delta[key].get_ref<const std::string &>().size();
                        }
                    }
                }

                if (outputBytes) {
                    if (firstToken == std::chrono::steady_clock::time_point{}) {
                        firstToken = std::chrono::steady_clock::now();
                    }

                    statistics.generated = int64_t((outputBytes + 3) / 4);
                    const double seconds =
                        std::chrono::duration<double>(std::chrono::steady_clock::now() - firstToken).count();
                    if (seconds >= .25) {
                        statistics.tokensPerSecond = double(statistics.generated) / seconds;
                    }
                }

                if (chunk.contains("usage") && chunk["usage"].is_object()) {
                    usage = chunk["usage"];
                    if (usage.contains("prompt_tokens") && usage["prompt_tokens"].is_number_integer()) {
                        statistics.prompt = usage["prompt_tokens"].get<int64_t>();
                    }

                    if (usage.contains("completion_tokens") &&
                        usage["completion_tokens"].is_number_integer()) {
                        statistics.generated = usage["completion_tokens"].get<int64_t>();
                        double seconds = 0;
                        if (firstToken != std::chrono::steady_clock::time_point{}) {
                            seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() -
                                                                    firstToken)
                                          .count();
                        }
                        if (seconds >= .25) {
                            statistics.tokensPerSecond =
                                double(std::max<int64_t>(0, statistics.generated - 1)) / seconds;
                        }

                        statistics.estimated = !usage.contains("prompt_tokens");
                    }
                }

                if (chunk.contains("timings") && chunk["timings"].is_object()) {
                    const auto &timings = chunk["timings"];
                    if (timings.contains("predicted_n") && timings["predicted_n"].is_number_integer()) {
                        statistics.generated = timings["predicted_n"].get<int64_t>();
                    }

                    if (timings.contains("predicted_per_second") &&
                        timings["predicted_per_second"].is_number()) {
                        statistics.tokensPerSecond = timings["predicted_per_second"].get<double>();
                    }
                }

                if (onStatistics) {
                    onStatistics(statistics);
                }
            }
        };

        struct InternetHandle {
            std::atomic<HINTERNET> handle{nullptr};
            void close() {
                if (const auto value = handle.exchange(nullptr)) {
                    WinHttpCloseHandle(value);
                }
            }

            ~InternetHandle() {
                close();
            }
        };

        void checkHttpResult(bool ok, const char *operation) {
            if (!ok) {
                throw std::runtime_error(std::string(operation) + " (Windows error " +
                                         std::to_string(GetLastError()) + ")");
            }
        }

    }

    LocalAiSettings::Json LocalAiSettings::catalog(const ShaderAttribute &shader) {
        Json result = Json::object();
        for (const auto &field : shaderFields()) {
            auto item = field.rule;
            item["description"] = field.label;
            auto current = field.get(shader);
            if (field.key == "palette.colors" && current.size() > 64) {
                item["currentStopCount"] = current.size();
            } else {
                item["current"] = std::move(current);
            }

            result[field.key] = std::move(item);
        }

        return result;
    }

    std::string LocalAiSettings::systemPrompt(const ShaderAttribute &shader,
                                              const std::filesystem::path &path) {
        Json rows = Json::array();
        const auto settings = catalog(shader);
        for (const auto &[key, item] : settings.items()) {
            rows.push_back(Json::array({key, item.at("description"), item.at("type"),
                                        item.value("minimum", Json()), item.value("maximum", Json()),
                                        item.value("current", Json()), item.value("options", Json())}));
        }

        std::ifstream input(resolveConfigPath(path), std::ios::binary);
        if (!input) {
            throw std::runtime_error("System prompt file not found. Place local-ai-system-prompt.md beside "
                                     "RFF_Super.exe or in its parent folder, then try again.");
        }

        std::string prompt((std::istreambuf_iterator<char>(input)), {});
        if (input.bad()) {
            throw std::runtime_error("Cannot read system prompt: " + path.string());
        }

        if (prompt.starts_with("\xEF\xBB\xBF")) {
            prompt.erase(0, 3);
        }

        toWide(prompt);
        constexpr std::string_view marker = "{{SETTINGS_CATALOG}}";
        const auto position = prompt.find(marker);
        if (position == std::string::npos ||
            prompt.find(marker, position + marker.size()) != std::string::npos) {
            throw std::runtime_error(
                "System prompt Markdown must contain {{SETTINGS_CATALOG}} exactly once.");
        }

        prompt.replace(position, marker.size(), rows.dump());
        return prompt;
    }

    ShaderAttribute LocalAiSettings::apply(const ShaderAttribute &original, const Json &patch) {
        if (!patch.is_object() || !patch.contains("summary") || !patch["summary"].is_string() ||
            !patch.contains("changes") || !patch["changes"].is_object()) {
            throw std::runtime_error("Expected {summary: string, changes: object}");
        }

        for (const auto &[key, value] : patch.items()) {
            if (key != "summary" && key != "changes") {
                throw std::runtime_error("Unknown response field: " + key);
            }
        }

        const auto &changes = patch["changes"];
        if (changes.empty() || changes.size() > shaderFields().size()) {
            throw std::runtime_error("changes must contain one or more catalog settings");
        }

        auto result = original;
        auto applySetting = [&](const std::string &key, const Json &value) {
            const auto field =
                std::find_if(shaderFields().begin(), shaderFields().end(), [&](const auto &candidate) {
                    return candidate.key == key;
                });
            if (field == shaderFields().end()) {
                throw std::runtime_error("Unknown or protected setting: " + key);
            }

            try {
                field->set(result, value);
            } catch (const std::exception &e) {
                throw std::runtime_error(key + ": " + e.what());
            }
        };
        // Apply the recipe before explicit field overrides, then restore the requested Studio enable state.
        if (changes.contains("surface.style")) {
            applySetting("surface.style", changes["surface.style"]);
        }

        for (const auto &[key, value] : changes.items()) {
            if (key != "surface.style" && key != "surface.studio.enabled") {
                applySetting(key, value);
            }
        }

        if (changes.contains("surface.studio.enabled")) {
            applySetting("surface.studio.enabled", changes["surface.studio.enabled"]);
        }

        if (changes.contains("palette.colors")) {
            result.palette.stops.clear();
        }

        if (changes.contains("palette.stops") || changes.contains("palette.stopEasing") ||
            changes.contains("palette.colorInterpolation")) {
            bakePaletteStops(result.palette);
        }

        if (!ShaderPresetIO::validate(result)) {
            throw std::runtime_error("Combined shader settings failed ShaderPresetIO validation; use "
                                     "conservative values and check enum/integer constraints");
        }

        return result;
    }

    int LocalAiSettings::errorLimit(const Json &connection) {
        const auto value = connection.value("max_errors", Json(3));
        if (!value.is_number_integer() || value < Json(1) || value > Json(100)) {
            throw std::runtime_error("max_errors must be an integer from 1 to 100");
        }

        return value.get<int>();
    }

    int LocalAiSettings::refinementLimit(const Json &connection) {
        const auto value = connection.value("max_refinements", Json(3));
        if (!value.is_number_integer() || value < Json(1) || value > Json(10)) {
            throw std::runtime_error("max_refinements must be an integer from 1 to 10");
        }

        return value.get<int>();
    }

    int64_t LocalAiSettings::estimatePrompt(const Json &messages) {
        size_t bytes = 0;
        bool image = false;
        for (const auto &message : messages) {
            bytes += 16;
            const auto &content = message.at("content");
            if (content.is_string()) {
                bytes += content.get_ref<const std::string &>().size();
            } else if (content.is_array()) {
                for (const auto &part : content) {
                    if (part.value("type", std::string()) == "text") {
                        bytes += part.at("text").get_ref<const std::string &>().size();
                    } else {
                        image = true;
                    }
                }
            }
        }

        return image ? -1 : int64_t((bytes + 3) / 4);
    }

    void LocalAiSettings::saveErrorLimit(int limit, const std::filesystem::path &path) {
        errorLimit(Json{{"max_errors", limit}});
        const auto resolved = resolveConfigPath(path);
        auto connection = readConnection(resolved);
        connection["max_errors"] = limit;
        auto temporary = resolved;
        temporary += L".tmp";
        {
            std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
            out << connection.dump(2) << '\n';
            out.close();
            if (!out) {
                throw std::runtime_error("Cannot save Local LLM error limit");
            }
        }

        if (!MoveFileExW(temporary.c_str(), resolved.c_str(),
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            throw std::runtime_error("Cannot replace Local LLM connection settings");
        }
    }
    LocalAiSettings::Json LocalAiSettings::readConnection(const std::filesystem::path &path) {
        const auto resolved = resolveConfigPath(path);
        std::ifstream input(resolved);
        if (!input) {
            throw std::runtime_error(
                "Connection settings not found: " + path.filename().string() +
                ". Place this file beside RFF_Super.exe or in its parent folder, then try again.");
        }

        auto connection = Json::parse(input);
        if (!connection.is_object() || !connection.contains("endpoint") ||
            !connection["endpoint"].is_string() || !connection.contains("model") ||
            !connection["model"].is_string()) {
            throw std::runtime_error("local-ai.json requires endpoint and model strings");
        }

        const auto timeoutSeconds = connection.value("timeout_seconds", Json(300));
        if (!timeoutSeconds.is_number_integer() || timeoutSeconds < Json(1) ||
            timeoutSeconds > Json(900)) {
            throw std::runtime_error("timeout_seconds must be an integer from 1 to 900");
        }

        if (connection.contains("request") && !connection["request"].is_object()) {
            throw std::runtime_error("request must be an object");
        }

        errorLimit(connection);
        refinementLimit(connection);
        if (connection.contains("vision") && !connection["vision"].is_boolean()) {
            throw std::runtime_error("vision must be true or false");
        }

        const auto provider = connection.value("provider", std::string("openai"));
        if (provider != "openai") {
            throw std::runtime_error("Unsupported provider. Configure an OpenAI-compatible local endpoint.");
        }

        return connection;
    }

    void LocalAiSettings::stopConfiguredServer(const std::filesystem::path &serverConfig,
                                               const std::filesystem::path &connectionConfig) {
        const auto serverPath = resolveConfigPath(serverConfig);
        std::ifstream input(serverPath);
        if (!input) {
            return;
        }

        const auto server = Json::parse(input);
        const auto connection = readConnection(connectionConfig);
        auto url = toWide(connection.at("endpoint").get<std::string>());
        URL_COMPONENTS parts{};
        parts.dwStructSize = sizeof(parts);
        parts.dwHostNameLength = DWORD(-1);
        if (!WinHttpCrackUrl(url.c_str(), 0, 0, &parts)) {
            return;
        }

        const std::wstring host(parts.lpszHostName, parts.dwHostNameLength);
        if ((host != L"127.0.0.1" && host != L"localhost") || parts.nPort != server.at("port").get<int>()) {
            return;
        }

        const auto expected = std::filesystem::weakly_canonical(serverPath.parent_path() /
                                                                server.at("executable").get<std::string>());
        if (_wcsicmp(expected.filename().c_str(), L"llama-server.exe") != 0) {
            return;
        }

        const auto library = LoadLibraryW(L"iphlpapi.dll");
        if (!library) {
            return;
        }

        const auto table =
            reinterpret_cast<decltype(&GetExtendedTcpTable)>(GetProcAddress(library, "GetExtendedTcpTable"));
        DWORD size = 0;
        std::vector<unsigned char> bytes;
        if (table &&
            table(nullptr, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_LISTENER, 0) ==
                ERROR_INSUFFICIENT_BUFFER &&
            size < 16 * 1024 * 1024) {
            bytes.resize(size);
            if (table(bytes.data(), &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_LISTENER, 0) == NO_ERROR) {
                const auto rows = reinterpret_cast<const MIB_TCPTABLE_OWNER_PID *>(bytes.data());
                for (DWORD i = 0; i < rows->dwNumEntries; ++i) {
                    const auto &row = rows->table[i];
                    const auto port = ((row.dwLocalPort & 255) << 8) | ((row.dwLocalPort >> 8) & 255);
                    if (port != parts.nPort) {
                        continue;
                    }

                    const auto process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE,
                                                     FALSE, row.dwOwningPid);
                    if (!process) {
                        continue;
                    }

                    std::wstring name(32768, 0);
                    DWORD length = DWORD(name.size());
                    if (QueryFullProcessImageNameW(process, 0, name.data(), &length)) {
                        name.resize(length);
                        if (_wcsicmp(name.c_str(), expected.c_str()) == 0) {
                            TerminateProcess(process, 0);
                        }
                    }

                    CloseHandle(process);
                }
            }
        }

        FreeLibrary(library);
    }

    LocalAiSettings::Json LocalAiSettings::serverInfo(const Json &connection,
                                                      const std::atomic_bool &cancelled) {
        Json result = {{"context", -1}, {"llama", false}};
        auto endpoint = connection.at("endpoint").get<std::string>();
        const auto suffix = endpoint.find("/v1/chat/completions");
        if (suffix == std::string::npos) {
            return result;
        }

        auto probe = connection;
        probe["timeout_seconds"] = 2;
        probe["metadata_request"] = true;
        probe["endpoint"] = endpoint.substr(0, suffix) + "/props";
        try {
            const auto props = post(probe, Json(), cancelled);
            if (props.contains("default_generation_settings")) {
                const auto &settings = props.at("default_generation_settings");
                if (settings.contains("n_ctx") && settings["n_ctx"].is_number_integer() &&
                    settings["n_ctx"].get<int64_t>() > 0) {
                    result["context"] = settings["n_ctx"];
                    result["llama"] = true;
                }
            }

        } catch (const std::exception &) {
        }

        return result;
    }

    LocalAiSettings::Json LocalAiSettings::post(const Json &connection, const Json &request,
                                                const std::atomic_bool &cancelled, const Progress &onToken,
                                                const Progress &onReasoning,
                                                const OnStatistics &onStatistics) {
        if (cancelled) {
            throw std::runtime_error("Cancelled");
        }

        auto url = toWide(connection.at("endpoint").get<std::string>());
        URL_COMPONENTS parts{};
        parts.dwStructSize = sizeof(parts);
        parts.dwHostNameLength = DWORD(-1);
        parts.dwUrlPathLength = DWORD(-1);
        parts.dwExtraInfoLength = DWORD(-1);
        checkHttpResult(WinHttpCrackUrl(url.c_str(), 0, 0, &parts), "Invalid endpoint");
        if (parts.nScheme != INTERNET_SCHEME_HTTP && parts.nScheme != INTERNET_SCHEME_HTTPS) {
            throw std::runtime_error("Endpoint must use HTTP or HTTPS");
        }

        const std::wstring host(parts.lpszHostName, parts.dwHostNameLength);
        auto path = std::wstring(parts.lpszUrlPath, parts.dwUrlPathLength) +
                    std::wstring(parts.lpszExtraInfo, parts.dwExtraInfoLength);
        InternetHandle session{WinHttpOpen(L"RFF_Super local AI", WINHTTP_ACCESS_TYPE_NO_PROXY,
                                           WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0)};
        checkHttpResult(session.handle, "Open HTTP session");
        const int receiveTimeoutMs = connection.value("timeout_seconds", 300) * 1000;
        const bool isMetadataRequest = connection.value("metadata_request", false) && request.is_null();
        const int resolveAndConnectTimeoutMs = isMetadataRequest ? 2000 : 10000;
        const int sendTimeoutMs = isMetadataRequest ? 2000 : 30000;
        checkHttpResult(WinHttpSetTimeouts(session.handle, resolveAndConnectTimeoutMs,
                                           resolveAndConnectTimeoutMs, sendTimeoutMs, receiveTimeoutMs),
                        "Set timeout");
        InternetHandle server{WinHttpConnect(session.handle, host.c_str(), parts.nPort, 0)};
        checkHttpResult(server.handle, "Connect");
        const wchar_t *method = isMetadataRequest ? L"GET" : L"POST";
        const DWORD requestFlags = parts.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
        InternetHandle message{WinHttpOpenRequest(
            server.handle, method, path.c_str(), nullptr, WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES, requestFlags)};
        checkHttpResult(message.handle, "Open request");
        std::jthread cancellation([&](std::stop_token stop) {
            while (!stop.stop_requested()) {
                if (cancelled) {
                    message.close();
                    return;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        });
        DWORD policy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
        checkHttpResult(
            WinHttpSetOption(message.handle, WINHTTP_OPTION_REDIRECT_POLICY, &policy, sizeof(policy)),
            "Disable redirects");
        std::wstring headers = L"Content-Type: application/json\r\n";
        auto body = isMetadataRequest ? std::string() : request.dump();
        checkHttpResult(WinHttpSendRequest(message.handle, headers.c_str(), DWORD(-1), body.data(),
                                           DWORD(body.size()), DWORD(body.size()), 0),
                        "Send request");
        checkHttpResult(WinHttpReceiveResponse(message.handle, nullptr), "Receive response");
        DWORD status = 0, size = sizeof(status);
        checkHttpResult(WinHttpQueryHeaders(message.handle,
                                            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr,
                                            &status, &size, nullptr),
                        "Read status");
        std::string response;
        std::string pendingLines;
        std::string eventData;
        std::string content;
        std::string finishReason;
        char buffer[8192];
        DWORD received = 0;
        bool streaming = false;
        bool streamComplete = false;
        constexpr size_t maxResponseBytes = 2 * 1024 * 1024;
        constexpr size_t maxReasoningBytes = 16 * 1024 * 1024;
        size_t reasoningBytes = 0;
        ResponseStatistics statistics(connection, request, onStatistics);
        auto dispatchEvent = [&] {
            if (eventData.empty()) {
                return;
            }

            if (eventData == "[DONE]") {
                streamComplete = true;
                eventData.clear();
                return;
            }

            const auto chunk = Json::parse(eventData);
            eventData.clear();
            if (chunk.contains("error")) {
                throw std::runtime_error("AI stream error: " + chunk["error"].dump());
            }

            statistics.update(chunk);
            if (!chunk.contains("choices") || chunk["choices"].empty()) {
                return;
            }

            const auto &choice = chunk["choices"][0];
            if (choice.contains("delta")) {
                const auto &delta = choice["delta"];
                for (const auto *key : {"reasoning_content", "reasoning"}) {
                    if (delta.contains(key) && delta[key].is_string()) {
                        const auto text = delta[key].get<std::string>();
                        if (text.size() > maxReasoningBytes - reasoningBytes) {
                            throw std::runtime_error("AI reasoning text exceeds 16 MiB");
                        }

                        reasoningBytes += text.size();
                        if (onReasoning && !text.empty()) {
                            onReasoning(text);
                        }

                        break;
                    }
                }

                if (delta.contains("content") && delta["content"].is_string()) {
                    auto token = delta["content"].get<std::string>();
                    if (token.size() > maxResponseBytes - content.size()) {
                        throw std::runtime_error("AI response text exceeds 2 MiB");
                    }

                    content += token;
                    if (onToken && !token.empty()) {
                        onToken(token);
                    }
                }
            }

            if (choice.contains("finish_reason") && choice["finish_reason"].is_string()) {
                finishReason = choice["finish_reason"].get<std::string>();
            }
        };
        do {
            if (cancelled) {
                throw std::runtime_error("Cancelled");
            }

            DWORD available = 0;
            checkHttpResult(WinHttpQueryDataAvailable(message.handle, &available), "Wait for response data");
            if (!available) {
                break;
            }

            checkHttpResult(WinHttpReadData(message.handle, buffer,
                                            std::min<DWORD>(available, sizeof(buffer)), &received),
                            "Read response");
            // Retain the raw body only until SSE is detected, or for a non-streaming response.
            if (!streaming) {
                response.append(buffer, received);
            }

            if (status >= 200 && status < 300 && !isMetadataRequest && request.value("stream", false)) {
                pendingLines.append(buffer, received);
                size_t end;
                while (!streamComplete && (end = pendingLines.find('\n')) != std::string::npos) {
                    if (end > maxResponseBytes) {
                        throw std::runtime_error("AI stream line exceeds 2 MiB");
                    }

                    auto line = pendingLines.substr(0, end);
                    pendingLines.erase(0, end + 1);
                    if (!line.empty() && line.back() == '\r') {
                        line.pop_back();
                    }

                    if (line.empty()) {
                        dispatchEvent();
                        continue;
                    }

                    if (line.starts_with("data:")) {
                        streaming = true;
                        response.clear();
                        auto data = line.substr(5);
                        if (data.starts_with(' ')) {
                            data.erase(0, 1);
                        }

                        if (data.size() + size_t(!eventData.empty()) > maxResponseBytes - eventData.size()) {
                            throw std::runtime_error("AI stream event exceeds 2 MiB");
                        }

                        if (!eventData.empty()) {
                            eventData += '\n';
                        }

                        eventData += data;
                    }
                }

                if (!streamComplete && pendingLines.size() > maxResponseBytes &&
                    (streaming || pendingLines.starts_with("data:"))) {
                    throw std::runtime_error("AI stream line exceeds 2 MiB");
                }
            }

            if (!streaming && response.size() > maxResponseBytes) {
                throw std::runtime_error("AI non-streaming response exceeds 2 MiB");
            }

        } while (received && !streamComplete);
        if (status < 200 || status >= 300) {
            throw std::runtime_error("HTTP " + std::to_string(status) + ": " + response.substr(0, 1500));
        }

        if (streaming) {
            if (!streamComplete || finishReason.empty()) {
                throw std::runtime_error("AI stream ended before completion. Settings were not changed.");
            }

            return Json{{"choices", Json::array({{{"message", {{"content", content}}},
                                                  {"finish_reason", finishReason}}})},
                        {"usage", statistics.getUsage()}};
        }

        auto parsed = Json::parse(response);
        if (!isMetadataRequest) {
            statistics.update(parsed);
        }

        return parsed;
    }

    double LocalAiSettings::zoomFactor(const std::string &text) {
        try {
            size_t used = 0;
            const auto factor = std::stod(text, &used);
            if (used == text.size() && std::isfinite(factor) && factor > 1 && factor <= 100) {
                return factor;
            }

        } catch (const std::exception &) {
        }

        throw std::runtime_error("Zoom factor must be greater than 1 and at most 100 (for example 2 or 10).");
    }

    float LocalAiSettings::retryLogZoom(float current, const std::string &decrement, float minimum) {
        size_t used = 0;
        double delta = 0;
        try {
            delta = std::stod(decrement, &used);
        } catch (const std::exception &) {
        }

        if (used != decrement.size() || !std::isfinite(delta) || delta <= 0 || delta > 10) {
            throw std::runtime_error("Log zoom decrease must be greater than 0 and at most 10.");
        }

        const float next = std::max(minimum, float(double(current) - delta));
        if (!std::isfinite(current) || !std::isfinite(minimum) || next >= current) {
            throw std::runtime_error("Cannot lower log zoom further. Locate Minibrot stopped.");
        }

        return next;
    }

    double LocalAiSettings::smallerExplorationZoom(double factor) {
        if (!std::isfinite(factor) || factor <= 1 || factor > 100) {
            throw std::runtime_error("Invalid exploration zoom factor");
        }

        const double lower = std::sqrt(factor);
        if (lower < 1.01) {
            throw std::runtime_error(
                "Exploration zoom is too small to reduce further; previous location retained.");
        }

        return lower;
    }

    LocalAiSettings::Json LocalAiSettings::analyzeIterations(const Matrix<double> &sample,
                                                             uint64_t maxIteration) {
        const int width = sample.getWidth();
        const int height = sample.getHeight();
        if (width < 3 || height < 3 || sample.getLength() > 192 * 192 || maxIteration == 0) {
            throw std::runtime_error("Invalid iteration sample for AI exploration");
        }

        const auto isValidSample = [&](int x, int y) {
            const auto iteration = sample(x, y);
            return std::isfinite(iteration) && iteration > 0;
        };
        const auto hasEscaped = [&](int x, int y) {
            return isValidSample(x, y) && sample(x, y) < double(maxIteration);
        };
        std::vector<unsigned char> features(size_t(width) * height, 0);
        int validCount = 0;
        int unescapedCount = 0;
        int featureCount = 0;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                if (isValidSample(x, y)) {
                    ++validCount;
                    if (!hasEscaped(x, y)) {
                        ++unescapedCount;
                    }
                }
            }
        }

        for (int y = 1; y < height - 1; ++y) {
            for (int x = 1; x < width - 1; ++x) {
                if (!isValidSample(x, y) || !isValidSample(x - 1, y) || !isValidSample(x + 1, y) ||
                    !isValidSample(x, y - 1) || !isValidSample(x, y + 1)) {
                    continue;
                }

                const bool centerEscaped = hasEscaped(x, y);
                bool hasFeature =
                    centerEscaped != hasEscaped(x - 1, y) || centerEscaped != hasEscaped(x + 1, y) ||
                    centerEscaped != hasEscaped(x, y - 1) || centerEscaped != hasEscaped(x, y + 1);
                if (centerEscaped && !hasFeature) {
                    const double centerIteration = sample(x, y);
                    const double leftDelta = sample(x - 1, y) - centerIteration;
                    const double rightDelta = sample(x + 1, y) - centerIteration;
                    const double topDelta = sample(x, y - 1) - centerIteration;
                    const double bottomDelta = sample(x, y + 1) - centerIteration;
                    const double curvature =
                        std::abs(leftDelta + rightDelta) + std::abs(topDelta + bottomDelta);
                    const double variation = std::abs(leftDelta) + std::abs(rightDelta) + std::abs(topDelta) +
                                             std::abs(bottomDelta);
                    hasFeature = curvature > 0.05 && curvature > 0.2 * variation;
                }

                if (hasFeature) {
                    features[size_t(y) * width + x] = 1;
                    ++featureCount;
                }
            }
        }

        struct Candidate {
            int x;
            int y;
            double score;
            Json data;
        };
        std::vector<Candidate> rankedCandidates;
        const int radiusX = std::max(2, width / 10);
        const int radiusY = std::max(2, height / 10);
        for (int centerY = radiusY; centerY < height - radiusY; centerY += radiusY) {
            for (int centerX = radiusX; centerX < width - radiusX; centerX += radiusX) {
                int featureSamples = 0;
                int validSamples = 0;
                int unescapedSamples = 0;
                int candidateX = -1;
                int candidateY = -1;
                double nearestDistance = 1e30;
                double minIteration = double(maxIteration);
                double maxEscapedIteration = 0;
                for (int y = std::max(1, centerY - radiusY); y < std::min(height - 1, centerY + radiusY + 1);
                     ++y) {
                    for (int x = std::max(1, centerX - radiusX);
                         x < std::min(width - 1, centerX + radiusX + 1); ++x) {
                        if (isValidSample(x, y)) {
                            ++validSamples;
                            if (!hasEscaped(x, y)) {
                                ++unescapedSamples;
                            } else {
                                minIteration = std::min(minIteration, sample(x, y));
                                maxEscapedIteration = std::max(maxEscapedIteration, sample(x, y));
                            }
                        }

                        if (features[size_t(y) * width + x]) {
                            ++featureSamples;
                            const double distance = std::pow(double(x - centerX) / radiusX, 2) +
                                                    std::pow(double(y - centerY) / radiusY, 2);
                            if (distance < nearestDistance) {
                                nearestDistance = distance;
                                candidateX = x;
                                candidateY = y;
                            }
                        }
                    }
                }

                if (featureSamples < 8 || validSamples == 0 || double(featureSamples) / validSamples < 0.02 ||
                    candidateX < 0) {
                    continue;
                }

                const double density = double(featureSamples) / validSamples;
                rankedCandidates.push_back(
                    {candidateX,
                     candidateY,
                     density,
                     {{"structured_fraction", density},
                      {"unescaped_fraction", double(unescapedSamples) / validSamples},
                      {"escaped_iteration_min", maxEscapedIteration > 0 ? minIteration : 0},
                      {"escaped_iteration_max", maxEscapedIteration}}});
            }
        }

        std::stable_sort(rankedCandidates.begin(), rankedCandidates.end(), [](const auto &a, const auto &b) {
            return a.score > b.score;
        });
        Json candidates = Json::array();
        for (const auto &candidate : rankedCandidates) {
            const double x = double(candidate.x) / (width - 1), y = double(candidate.y) / (height - 1);
            bool nearby = false;
            for (const auto &kept : candidates) {
                const double dx = x - kept["x"].get<double>(), dy = y - kept["y"].get<double>();
                if (dx * dx + dy * dy < 0.0225) {
                    nearby = true;
                    break;
                }
            }

            if (nearby) {
                continue;
            }

            auto data = candidate.data;
            data["id"] = candidates.size();
            data["x"] = x;
            data["y"] = y;
            data["crop"] = Json::array({std::max(0.0, x - 0.12), std::max(0.0, y - 0.12),
                                        std::min(1.0, x + 0.12), std::min(1.0, y + 0.12)});
            candidates.push_back(data);
            if (candidates.size() == 6) {
                break;
            }
        }

        return {{"sample_width", width},
                {"sample_height", height},
                {"iteration_limit", maxIteration},
                {"valid_fraction", double(validCount) / (width * height)},
                {"structured_fraction", validCount ? double(featureCount) / validCount : 0},
                {"unescaped_fraction", validCount ? double(unescapedCount) / validCount : 0},
                {"has_structure", !candidates.empty()},
                {"candidates", candidates},
                {"note", "Sampled numerical candidates, not a spiral classifier. Unescaped at the limit does "
                         "not prove set membership. Zero/nonfinite samples are unknown. Crop boxes use "
                         "full-image normalized coordinates."}};
    }

    LocalAiSettings::ZoomTarget LocalAiSettings::chooseZoomTarget(
        const std::string &instruction, const std::string &imageDataUrl, const Json &connection,
        const std::atomic_bool &cancelled, const Progress &progress, const Progress &onToken,
        const Progress &onReasoning, const OnStatistics &onStatistics, const Transport &transport,
        const Json &evidence, const std::vector<std::string> &crops) {
        if (instruction.empty() || instruction.size() > 16000) {
            throw std::runtime_error("Describe the desired exploration target (1-16000 UTF-8 bytes).");
        }

        if (!imageDataUrl.starts_with("data:image/png;base64,") || imageDataUrl.size() > 32 * 1024 * 1024) {
            throw std::runtime_error("Invalid exploration image.");
        }

        auto messages = Json::array(
            {{{"role", "system"},
              {"content",
               "You select the next zoom destination in a Mandelbrot image. Return one JSON object only: "
               "{\"x\":0.5,\"y\":0.5,\"stop\":false,\"summary\":\"brief Japanese explanation\"}. x and y are "
               "normalized image coordinates, 0 at left/top, 1 at right/bottom. Select a clearly visible "
               "interesting detail matching the user's goal, preferably a small minibrot or structured "
               "boundary. Avoid flat interiors and featureless backgrounds. The application centers exactly "
               "on this point and controls zoom strength separately. You cannot change settings, choose a "
               "zoom factor, or produce complex-plane coordinates. Set stop=true if no useful destination is "
               "visible. Do not claim a minibrot is mathematically confirmed."}},
             {{"role", "user"},
              {"content", Json::array({{{"type", "text"}, {"text", instruction}},
                                       {{"type", "image_url"}, {"image_url", {{"url", imageDataUrl}}}}})}}});
        const bool guided = evidence.contains("candidates");
        if (guided) {
            if (!evidence["candidates"].is_array() || evidence["candidates"].size() > 6 ||
                crops.size() != evidence["candidates"].size()) {
                throw std::runtime_error("Candidate images do not match numerical evidence");
            }

            messages[0]["content"] =
                "Select a Mandelbrot exploration target from the supplied numerical candidates and their "
                "image crops. Return JSON only: {\"candidate_id\":0,\"stop\":false,\"summary\":\"brief "
                "Japanese explanation\"}. Select only an available candidate ID; never invent coordinates. "
                "The first image is the full view, subsequent labeled images are crops. This is a multi-step "
                "SEARCH, not a test requiring the goal to be visible already. Prefer a candidate visibly "
                "matching the requested shape. If that shape is not yet resolved, choose the most promising "
                "available structured boundary, branching detail, or bulb junction for closer inspection and "
                "continue with stop=false. Absence of a visible spiral at the current scale is not by itself "
                "a reason to stop. Explain in Japanese whether the choice is an exploratory hypothesis or a "
                "visually observed match; never claim a spiral was found without visual evidence. Large "
                "iteration variation alone does not prove a spiral or minibrot. A still image cannot prove "
                "motion. Use stop=true and candidate_id=null only when no supplied candidate offers a useful "
                "further inspection, such as all candidates being featureless or unusable. The application "
                "limits the total number of zoom steps. The application controls zoom strength and validates "
                "the next render. Respect rejected candidates and explain uncertainty.";
            messages[1]["content"].push_back(
                {{"type", "text"}, {"text", "Iteration evidence: " + evidence.dump()}});
            for (size_t i = 0; i < crops.size(); ++i) {
                if (!crops[i].starts_with("data:image/png;base64,") || crops[i].size() > 32 * 1024 * 1024) {
                    throw std::runtime_error("Invalid candidate PNG");
                }

                messages[1]["content"].push_back(
                    {{"type", "text"},
                     {"text", "Candidate " + evidence["candidates"][i]["id"].dump() + " crop"}});
                messages[1]["content"].push_back({{"type", "image_url"}, {"image_url", {{"url", crops[i]}}}});
            }
        }

        const auto limit = errorLimit(connection);
        std::string error;
        for (int attempt = 1; attempt <= limit; ++attempt) {
            if (cancelled) {
                throw std::runtime_error("Cancelled");
            }

            progress("Choosing zoom destination (" + std::to_string(attempt) + "/" + std::to_string(limit) +
                     ")...");
            auto request = connection.value("request", Json::object());
            request["model"] = connection.at("model");
            request["messages"] = messages;
            request["stream"] = bool(onToken) || bool(onReasoning);
            request["response_format"] = {{"type", "json_object"}};
            if (request["stream"].get<bool>()) {
                request["stream_options"] = {{"include_usage", true}};
                if (connection.value("runtime_llama", false)) {
                    request["timings_per_token"] = true;
                }
            }

            if (onStatistics) {
                Statistics stats;
                stats.context = connection.value("runtime_context", int64_t(-1));
                stats.prompt = estimatePrompt(messages);
                stats.generated = 0;
                onStatistics(stats);
            }

            const auto response =
                transport ? transport(request)
                          : post(connection, request, cancelled, onToken, onReasoning, onStatistics);
            if (cancelled) {
                throw std::runtime_error("Cancelled");
            }

            std::string content;
            try {
                const auto &choice = response.at("choices").at(0);
                if (choice.value("finish_reason", std::string()) == "length") {
                    throw std::runtime_error("Truncated answer");
                }

                content = choice.at("message").at("content").get<std::string>();
                const auto objectStart = content.find('{');
                const auto objectEnd = content.rfind('}');
                if (objectStart == std::string::npos || objectEnd == std::string::npos ||
                    objectEnd < objectStart) {
                    throw std::runtime_error("Expected one JSON object");
                }

                const auto value = Json::parse(content.substr(objectStart, objectEnd - objectStart + 1));
                if (guided) {
                    if (!value.is_object() || value.size() != 3 || !value.contains("candidate_id") ||
                        !value.contains("stop") || !value["stop"].is_boolean() ||
                        !value.contains("summary") || !value["summary"].is_string()) {
                        throw std::runtime_error(
                            "Expected candidate_id, boolean stop and string summary only");
                    }

                    if (value["stop"].get<bool>()) {
                        if (!value["candidate_id"].is_null()) {
                            throw std::runtime_error("Use null candidate_id when stopping");
                        }

                        return {.stop = true, .summary = value["summary"].get<std::string>()};
                    }

                    if (!value["candidate_id"].is_number_integer()) {
                        throw std::runtime_error("candidate_id must be an available integer ID");
                    }

                    for (const auto &candidate : evidence["candidates"]) {
                        if (candidate["id"] == value["candidate_id"]) {
                            return {candidate["x"].get<double>(), candidate["y"].get<double>(), false,
                                    value["summary"].get<std::string>()};
                        }
                    }

                    throw std::runtime_error("Unknown or rejected candidate_id");
                }

                if (value.size() != 4 || !value.contains("x") || !value["x"].is_number() ||
                    !value.contains("y") || !value["y"].is_number() || !value.contains("stop") ||
                    !value["stop"].is_boolean() || !value.contains("summary") ||
                    !value["summary"].is_string()) {
                    throw std::runtime_error("Expected numeric x/y, boolean stop, string summary only");
                }

                const double x = value["x"].get<double>();
                const double y = value["y"].get<double>();
                if (!std::isfinite(x) || !std::isfinite(y) || x < 0 || x > 1 || y < 0 || y > 1) {
                    throw std::runtime_error("x and y must be finite values from 0 to 1");
                }

                return {x, y, value["stop"].get<bool>(), value["summary"].get<std::string>()};
            } catch (const std::exception &e) {
                error = e.what();
            }

            progress("Invalid destination: " + error);
            if (!content.empty()) {
                messages.push_back({{"role", "assistant"}, {"content", content.substr(0, 32000)}});
            }

            messages.push_back(
                {{"role", "user"},
                 {"content", "No movement occurred. Return a corrected JSON object. Error: " + error}});
        }

        throw std::runtime_error("No valid zoom destination: " + error);
    }

    LocalAiSettings::Result
    LocalAiSettings::generate(const ShaderAttribute &original, const std::string &instruction,
                              const Json &connection, const std::atomic_bool &cancelled,
                              const Progress &progress, const Transport &transport, const Progress &onToken,
                              const Progress &onReasoning, const OnStatistics &onStatistics,
                              const std::string &imageDataUrl, const std::string &history) {
        if (instruction.empty() || instruction.size() > 16000) {
            throw std::runtime_error("Instruction must contain 1 to 16000 UTF-8 bytes");
        }

        const bool isVisionRequest = !imageDataUrl.empty();
        auto messages = Json::array({{{"role", "system"}, {"content", systemPrompt(original)}},
                                     {{"role", "user"}, {"content", instruction}}});
        if (isVisionRequest) {
            if (!imageDataUrl.starts_with("data:image/png;base64,") ||
                imageDataUrl.size() > 32 * 1024 * 1024) {
                throw std::runtime_error("Invalid or oversized vision PNG");
            }

            messages[1]["content"] = Json::array(
                {{{"type", "text"},
                  {"text", "VISION MODE. Evaluate the attached CURRENT rendered image against this goal: " +
                               instruction + "\nPrior evaluations and applied changes (context only):\n" +
                               history +
                               "\nReturn summary, score (0-100 for the CURRENT image, not your proposed "
                               "result), satisfied (boolean), and changes. Explain visible strengths and "
                               "remaining problems in Japanese. Preserve fine detail. Propose a small "
                               "improvement using the current catalog. An empty changes object is allowed in "
                               "vision mode when no useful improvement remains. If satisfied, return empty "
                               "changes. Use a consistent score scale across rounds."}},
                 {{"type", "image_url"}, {"image_url", {{"url", imageDataUrl}}}}});
        }

        const int limit = errorLimit(connection);
        std::string lastError;
        for (int attempt = 1; attempt <= limit; ++attempt) {
            if (cancelled) {
                throw std::runtime_error("Cancelled");
            }

            progress("Generating (" + std::to_string(attempt) + "/" + std::to_string(limit) + ")...");
            Json request = connection.value("request", Json::object());
            request["model"] = connection.at("model");
            request["messages"] = messages;
            request["stream"] = bool(onToken) || bool(onReasoning);
            if (request["stream"].get<bool>() && onStatistics) {
                request["stream_options"] = {{"include_usage", true}};
                if (connection.value("runtime_llama", false)) {
                    request["timings_per_token"] = true;
                }
            }

            if (onStatistics) {
                Statistics stats;
                stats.context = connection.value("runtime_context", int64_t(-1));
                stats.prompt = estimatePrompt(messages);
                stats.generated = 0;
                onStatistics(stats);
            }

            if (!request.contains("response_format")) {
                request["response_format"] = {{"type", "json_object"}};
            }

            const auto response =
                transport ? transport(request)
                          : post(connection, request, cancelled, onToken, onReasoning, onStatistics);
            if (cancelled) {
                throw std::runtime_error("Cancelled");
            }

            std::string content;
            try {
                const auto &choice = response.at("choices").at(0);
                content = choice.at("message").at("content").get<std::string>();
                if (choice.value("finish_reason", std::string()) == "length") {
                    throw std::runtime_error("Output was truncated; return a shorter complete patch");
                }

                Json patch;
                try {
                    patch = Json::parse(content);
                } catch (const Json::parse_error &) {
                    const auto objectStart = content.find('{');
                    const auto objectEnd = content.rfind('}');
                    if (objectStart == std::string::npos || objectEnd == std::string::npos ||
                        objectEnd < objectStart) {
                        throw;
                    }

                    patch = Json::parse(content.substr(objectStart, objectEnd - objectStart + 1));
                }

                double score = -1;
                bool satisfied = false;
                if (isVisionRequest) {
                    if (!patch.is_object() || !patch.contains("score") || !patch["score"].is_number() ||
                        !patch.contains("satisfied") || !patch["satisfied"].is_boolean()) {
                        throw std::runtime_error(
                            "Vision result requires numeric score and boolean satisfied");
                    }

                    score = patch["score"].get<double>();
                    if (!std::isfinite(score) || score < 0 || score > 100) {
                        throw std::runtime_error("score must be between 0 and 100");
                    }

                    satisfied = patch["satisfied"].get<bool>();
                    patch.erase("score");
                    patch.erase("satisfied");
                    if (patch.size() != 2 || !patch.contains("summary") || !patch["summary"].is_string() ||
                        !patch.contains("changes") || !patch["changes"].is_object()) {
                        throw std::runtime_error(
                            "Vision result requires summary and changes only, alongside score and satisfied");
                    }

                    if (satisfied && !patch["changes"].empty()) {
                        throw std::runtime_error("Return empty changes when satisfied");
                    }
                }

                if (!isVisionRequest || !patch.at("changes").empty()) {
                    apply(original, patch);
                }

                return {patch, patch.at("summary").get<std::string>(), attempt, score, satisfied};
            } catch (const std::exception &e) {
                lastError = e.what();
            }

            progress("Validation error (" + std::to_string(attempt) + "/" + std::to_string(limit) +
                     "): " + lastError);
            if (!content.empty()) {
                messages.push_back({{"role", "assistant"}, {"content", content.substr(0, 32000)}});
            }

            messages.push_back(
                {{"role", "user"},
                 {"content",
                  "RFF_Super rejected the settings. Nothing was applied. Error: " + lastError +
                      "\nReturn a complete corrected JSON patch using only the catalog and valid ranges."}});
        }

        throw std::runtime_error("Stopped after " + std::to_string(limit) +
                                 " invalid responses. Settings were not changed. Last error: " + lastError);
    }
}
