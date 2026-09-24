//
// Created by Merutilm on 2025-07-18.
// Modified by GPT-6 on 2026-09-23
//

#pragma once

#include <memory>

#include "../core/vkh_base.hpp"

namespace merutilm::vkh {
    struct RepoAbstract {
        virtual ~RepoAbstract() = default;
    };

    using Repo = std::unique_ptr<RepoAbstract>;
    using RepoPtr = RepoAbstract *;
    using RepoRef = RepoAbstract &;
}
