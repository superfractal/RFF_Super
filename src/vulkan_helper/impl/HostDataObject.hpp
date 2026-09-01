//
// Created by Merutilm on 2025-07-15.
// Modified by Opus 5 on 2026-08-26
//

#pragma once

#include "../core/vkh_base.hpp"

#include "../manage/HostDataObjectManager.hpp"

namespace merutilm::vkh {
    struct HostDataObjectImpl final {

        std::vector<std::byte> data;
        std::vector<uint32_t> elements;
        std::vector<uint32_t> sizes;
        std::vector<uint32_t> paddingsPerElem;
        std::vector<uint32_t> offsets;

        explicit HostDataObjectImpl(HostDataObjectManager &&uploadManager) : data(std::move(
                                                                                 uploadManager->data)),
                                                                             elements(
                                                                                 std::move(uploadManager->elements)),
                                                                             sizes(std::move(uploadManager->sizes)),
                                                                             paddingsPerElem(
                                                                                 std::move(
                                                                                     uploadManager->paddingsPerElem)),
                                                                             offsets(std::move(
                                                                                 uploadManager->offsets)) {
        }

        ~HostDataObjectImpl() = default;

        HostDataObjectImpl(const HostDataObjectImpl &) = delete;

        HostDataObjectImpl &operator=(const HostDataObjectImpl &) = delete;

        HostDataObjectImpl(HostDataObjectImpl &&) = delete;

        HostDataObjectImpl &operator=(HostDataObjectImpl &&) = delete;

        template<typename T> requires std::is_trivially_copyable_v<T>
        const T &get(const uint32_t target) const {
            safe_array::check_size_equal(sizes[target], sizeof(T), "Buffer Object get");
            auto view = std::span(data.begin() + offsets[target], data.begin() + offsets[target] + sizes[target]);
            return *reinterpret_cast<const T *>(view.data());
        }

        template<typename T> requires std::is_trivially_copyable_v<T>
        const T &get(const uint32_t target, const uint32_t index) const {
            // Against this target's own element count, not against how many targets there are, and
            // stepped by the stride the elements were reserved at rather than by the bare type.
            safe_array::check_size_equal(sizes[target], sizeof(T) * elements[target], "Buffer Object Vector get");
            safe_array::check_index(index, elements[target], "Buffer Object Vector get");
            const size_t offset = offsets[target] + elementStride<T>(target) * static_cast<size_t>(index);
            auto view = std::span(data.begin() + offset, data.begin() + offset + sizeof(T));
            return *reinterpret_cast<const T *>(view.data());
        }

        template<typename T> requires std::is_trivially_copyable_v<T>
        void set(const uint32_t target, const T &t) {
            safe_array::check_size_equal(sizes[target], sizeof(T), "Buffer Object set");
            const uint32_t offset = offsets[target];
            memcpy(&data[offset], &t, sizeof(T));
        }


        template<typename T> requires std::is_trivially_copyable_v<T>
        void set(const uint32_t target, const std::vector<T> &arr) {
            const uint32_t size = sizeof(T) * static_cast<uint32_t>(arr.size());
            safe_array::check_size_equal(sizes[target], size, "Buffer Object Vector set");
            const size_t stride = elementStride<T>(target);
            if (stride == sizeof(T)) {
                memcpy(&data[offsets[target]], arr.data(), size);
                return;
            }
            // Padded elements do not sit end to end, so they are written one stride apart rather
            // than as one block, which would lay the whole array over the first few elements.
            for (size_t i = 0; i < arr.size(); ++i) {
                memcpy(&data[offsets[target] + stride * i], &arr[i], sizeof(T));
            }
        }

        template<typename T> requires std::is_trivially_copyable_v<T>
        void set(const uint32_t target, const uint32_t arrIndex, const T &t) {
            safe_array::check_index(arrIndex, elements[target], "Buffer Object Vector set");
            const size_t offset = offsets[target] + elementStride<T>(target) * static_cast<size_t>(arrIndex);
            memcpy(&data[offset], &t, sizeof(T));
        }

        void reset(const uint32_t target) {
            // The padding belongs to the target as much as its values do; leaving it behind would
            // keep whatever an earlier, longer array left in the gaps.
            std::fill_n(data.begin() + offsets[target], reservedByte(target), static_cast<std::byte>(0));
        }

        template<typename T> requires std::is_trivially_copyable_v<T>
        void resizeArray(const uint32_t target, const uint32_t elementCount) {
            // Measured in whole strides, padding included: the offsets recomputed below count the
            // padding, so growing or shrinking by the bare type would leave every later target
            // pointing at the wrong place - the last of them past the end of the buffer.
            const size_t stride = elementStride<T>(target);
            if (elementCount < elements[target]) {
                data.erase(
                    data.begin() + offsets[target] + stride * elementCount,
                    data.begin() + offsets[target] + stride * elements[target]
                );
            }
            if (elementCount > elements[target]) {
                const auto fill = std::vector<std::byte>(stride * (elementCount - elements[target]));
                data.insert(data.begin() + offsets[target] + stride * elements[target],
                            fill.begin(), fill.end());
            }

            sizes[target] = sizeof(T) * elementCount;
            elements[target] = elementCount;
            uint32_t sizeSum = 0;

            for (uint32_t i = 0; i < static_cast<uint32_t>(sizes.size()); ++i) {
                offsets[i] = sizeSum;
                sizeSum += sizes[i] + paddingsPerElem[i] * elements[i];
            }
        }

        template<typename T> requires std::is_trivially_copyable_v<T>
        void resizeAndClear(const uint32_t target, const uint32_t elementCount) {
            resizeArray<T>(target, elementCount);
            reset(target);
        }

        [[nodiscard]] const std::vector<std::byte> &getData() const { return data; }

        [[nodiscard]] uint32_t getOffset(const uint32_t target) const { return offsets[target]; }

        [[nodiscard]] uint32_t getSizeByte(const uint32_t target) const { return sizes[target]; }

        [[nodiscard]] uint32_t getTotalSizeByte() const { return static_cast<uint32_t>(data.size()); }

        [[nodiscard]] uint32_t getObjectCount() const {
            return static_cast<uint32_t>(sizes.size());
        }

        [[nodiscard]] uint32_t getElementCount(const uint32_t target) const {
            return elements[target];
        }

    private:
        // How far apart two elements of this target sit, which is the type plus whatever padding
        // the target was reserved with.
        template<typename T>
        [[nodiscard]] size_t elementStride(const uint32_t target) const {
            return sizeof(T) + paddingsPerElem[target];
        }

        // Every byte the target owns in the buffer, its padding included.
        [[nodiscard]] size_t reservedByte(const uint32_t target) const {
            return static_cast<size_t>(sizes[target]) +
                   static_cast<size_t>(paddingsPerElem[target]) * elements[target];
        }
    };

    using HostDataObject = std::unique_ptr<HostDataObjectImpl>;
    using HostDataObjectPtr = HostDataObjectImpl *;
    using HostDataObjectRef = HostDataObjectImpl &;
}
