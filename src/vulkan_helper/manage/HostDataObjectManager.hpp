//
// Created by Merutilm on 2025-07-10.
// Modified by Opus 5 on 2026-08-26
//

#pragma once
#include "../core/vkh_core.hpp"

namespace merutilm::vkh {
    struct HostDataObjectManagerImpl final {
        std::vector<std::byte> data = {};
        std::vector<uint32_t> elements = {};
        std::vector<uint32_t> sizes = {};
        std::vector<uint32_t> paddingsPerElem = {};
        std::vector<uint32_t> offsets = {};

        explicit HostDataObjectManagerImpl() = default;

        ~HostDataObjectManagerImpl() = default;

        HostDataObjectManagerImpl(const HostDataObjectManagerImpl &) = delete;

        HostDataObjectManagerImpl &operator=(const HostDataObjectManagerImpl &) = delete;

        HostDataObjectManagerImpl(HostDataObjectManagerImpl &&) noexcept = delete;

        HostDataObjectManagerImpl &operator=(HostDataObjectManagerImpl &&) noexcept = delete;


        template<typename T> requires std::is_trivially_copyable_v<T>
        void reserve(const uint32_t targetExpected, const uint32_t padding = 0) {
            const size_t size = sizeof(T);
            offsets.push_back(static_cast<uint32_t>(data.size()));
            elements.push_back(1);
            safe_array::check_index_equal(targetExpected, static_cast<uint32_t>(sizes.size()),
                                              "Shader Object Value Reserve");
            data.resize(data.size() + size + padding);
            sizes.push_back(size);
            paddingsPerElem.push_back(padding);
        }

        template<typename T> requires std::is_trivially_copyable_v<T>
        void reserveArray(const uint32_t targetExpected, const uint32_t elementCount, const uint32_t paddingPerElem = 0) {
            offsets.push_back(static_cast<uint32_t>(data.size()));
            elements.push_back(elementCount);
            safe_array::check_index_equal(targetExpected, static_cast<uint32_t>(sizes.size()),
                                              "Shader Object Vector Reserve");
            data.resize(data.size() + (sizeof(T) + paddingPerElem) * elementCount);
            sizes.push_back(sizeof(T) * elementCount);
            paddingsPerElem.push_back(paddingPerElem);
        }


        template<typename T> requires std::is_trivially_copyable_v<T>
        void add(const uint32_t targetExpected, const T &t, const uint32_t padding = 0) {
            const auto raw = reinterpret_cast<const std::byte *>(&t);
            offsets.push_back(static_cast<uint32_t>(data.size()));
            elements.push_back(1);
            safe_array::check_index_equal(targetExpected, static_cast<uint32_t>(sizes.size()),
                                              "Shader Object Value Add");
            data.insert(data.end(), raw, raw + sizeof(T));
            data.resize(data.size() + padding);
            sizes.push_back(sizeof(T));
            paddingsPerElem.push_back(padding);
        }

        template<typename T> requires std::is_trivially_copyable_v<T>
        void addArray(const uint32_t targetExpected, const std::vector<T> &t, const uint32_t paddingPerElem = 0) {
            offsets.push_back(static_cast<uint32_t>(data.size()));
            elements.push_back(static_cast<uint32_t>(t.size()));
            safe_array::check_index_equal(targetExpected, static_cast<uint32_t>(sizes.size()),
                                              "Shader Object Vector Add");
            // The padding sits behind each element rather than behind the array: that is the layout
            // reserveArray lays out and the one every accessor steps through.
            for (const auto &element: t) {
                const auto raw = reinterpret_cast<const std::byte *>(&element);
                data.insert(data.end(), raw, raw + sizeof(T));
                data.resize(data.size() + paddingPerElem);
            }
            sizes.push_back(static_cast<uint32_t>(sizeof(T) * t.size()));
            paddingsPerElem.push_back(paddingPerElem);
        }
    };

    using HostDataObjectManager = std::unique_ptr<HostDataObjectManagerImpl>;
    using HostDataObjectManagerPtr = HostDataObjectManagerImpl *;
    using HostDataObjectManagerRef = HostDataObjectManagerImpl &;
}
