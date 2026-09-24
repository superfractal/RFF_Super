//
// Created by Merutilm on 2025-07-13.
// Modified by GPT-6 on 2026-09-23
//

#pragma once

#include "Handler.hpp"
#include "../impl/Core.hpp"

namespace merutilm::vkh {
    struct CoreHandler : public Handler {
        CoreRef core;

        explicit CoreHandler(CoreRef core) : core(core) {}

        ~CoreHandler() override = default;

        CoreHandler(const CoreHandler &) = delete;

        CoreHandler &operator=(const CoreHandler &) = delete;

        CoreHandler(CoreHandler &&) = delete;

        CoreHandler &operator=(CoreHandler &&) = delete;

    };
}
