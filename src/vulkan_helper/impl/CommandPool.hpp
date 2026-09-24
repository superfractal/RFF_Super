//
// Created by Merutilm on 2025-07-09.
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include <memory>

#include "../handle/CoreHandler.hpp"

namespace merutilm::vkh {
    class CommandPoolImpl final : public CoreHandler {
        VkCommandPool commandPool = {};

    public:
        explicit CommandPoolImpl(CoreRef core);
        ~CommandPoolImpl() override;

        CommandPoolImpl(const CommandPoolImpl &) = delete;
        CommandPoolImpl &operator=(const CommandPoolImpl &) = delete;
        CommandPoolImpl(CommandPoolImpl &&) = delete;
        CommandPoolImpl &operator=(CommandPoolImpl &&) = delete;

        [[nodiscard]] VkCommandPool getCommandPoolHandle() const {
            return commandPool;
        }

    private:
        void init() override;
        void destroy() override;
    };

    using CommandPool = std::unique_ptr<CommandPoolImpl>;
    using CommandPoolPtr = CommandPoolImpl *;
    using CommandPoolRef = CommandPoolImpl &;
}
