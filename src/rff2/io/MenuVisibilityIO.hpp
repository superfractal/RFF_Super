//
// Created by Opus 5.5 on 2026-09-29
// Modified by Opus 5.5 on 2026-10-04
//

#pragma once
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "../ui/Utilities.h"

namespace merutilm::rff2 {
    // Reads and writes menu-visibility.json, where a menu caption set to false is left off the menu bar.
    struct MenuVisibilityIO {
        using Json = nlohmann::ordered_json;

        enum class Status { MISSING, LOADED, UNREADABLE };

        static std::filesystem::path path() {
            return Utilities::getConfigFile(L"menu-visibility.json");
        }

        static Status load(Json &out) {
            out = Json::object();
            std::error_code error;
            const auto file = path();
            if (!std::filesystem::exists(file, error)) {
                return Status::MISSING;
            }
            std::ifstream in(file, std::ios::binary);
            if (!in) {
                return Status::UNREADABLE;
            }
            Json parsed = Json::parse(in, nullptr, false, true);
            if (parsed.is_discarded() || !parsed.is_object()) {
                return Status::UNREADABLE;
            }
            out = std::move(parsed);
            return Status::LOADED;
        }

        // A caption set to false hides its item; one the file does not reach falls back to shownByDefault.
        static bool visible(const Json &root, const std::vector<std::string> &captions, const bool shownByDefault) {
            const Json *node = &root;
            for (const auto &caption : captions) {
                if (!node->is_object()) {
                    return shownByDefault;
                }
                const auto found = node->find(caption);
                if (found == node->end()) {
                    return shownByDefault;
                }
                if (found->is_boolean() && !found->template get<bool>()) {
                    return false;
                }
                node = &*found;
            }
            return true;
        }

        static void saveTemplate(const Json &tree) {
            std::ofstream out(path(), std::ios::binary | std::ios::trunc);
            if (out) {
                out << tree.dump(2) << '\n';
            }
        }
    };
} // namespace merutilm::rff2
