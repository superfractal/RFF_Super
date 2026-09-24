//
// Created by Merutilm on 2025-07-16.
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include <functional>
#include <string>

#include "../core/factory.hpp"
#include "../core/vkh_base.hpp"
#include "../impl/ShaderModule.hpp"
#include "../struct/StringHasher.hpp"
#include "Repository.hpp"

namespace merutilm::vkh {
    struct GlobalShaderModuleRepo final
        : Repository<std::string, const std::string &, ShaderModule, ShaderModuleRef,
                     StringHasher, std::equal_to<>> {
        using Repository::Repository;

        ShaderModuleRef pick(const std::string &filename) override {
            auto it = repository.find(filename);
            if (it == repository.end()) {
                it = repository.try_emplace(
                    filename, factory::create<ShaderModule>(core, filename)).first;
            }
            return *it->second;
        }
    };
}
