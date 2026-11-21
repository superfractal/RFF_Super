//
// Created by Opus 5.5 on 2026-11-15
//

#pragma once
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <optional>

#include "../../vulkan_helper/core/logger.hpp"
#include "../ui/IOUtilities.h"

// Reads the version-led .rfm, .rfl and .rfsm files of upstream RFF-2.0; see documentation/notes/rff2-file-format.md.
namespace merutilm::rff2::RFF2FileVersion {
    // The newest version upstream RFF-2.0 had written as of 2026-11-15.
    constexpr uint32_t LATEST = 5;

    // From this version on, an integer is stored as its byte count followed by that many bytes.
    constexpr uint32_t SIZED_INTEGERS = 5;

    // Leading values at or above these can only be an unversioned file: a map's height, or a normal float's bits.
    constexpr uint32_t MAP_UNVERSIONED_FROM = 1u << 16;
    constexpr uint32_t FLOAT_UNVERSIONED_FROM = 1u << 23;

    // 0 for RFF_Super's own unversioned layout (stream rewound), the version otherwise; nullopt for a newer or unreadable file.
    inline std::optional<uint32_t> read(std::ifstream &in, const uint32_t unversionedFrom) {
        uint32_t lead = 0;
        IOUtilities::readAndDecode(in, &lead);
        if (!in) {
            return std::nullopt;
        }
        if (lead >= 1 && lead <= LATEST) {
            return lead;
        }
        if (lead != 0 && lead < unversionedFrom) {
            vkh::logger::w_log(L"WARNING : File written by a newer RFF-2.0 version, cannot read it");
            return std::nullopt;
        }
        in.seekg(0);
        return in ? std::optional<uint32_t>(0) : std::nullopt;
    }

    // Versioned files keep the zoom as a double; RFF_Super holds it as a float.
    inline bool readLogZoom(std::ifstream &in, float &logZoom) {
        double value = 0;
        IOUtilities::readAndDecode(in, &value);
        if (!in || !std::isfinite(value) || std::abs(value) > std::numeric_limits<float>::max()) {
            return false;
        }
        logZoom = static_cast<float>(value);
        return true;
    }

    // A period or iteration count; a value wider than 64 bits saturates rather than wrapping.
    inline bool readInteger(std::ifstream &in, const uint32_t version, uint64_t &value) {
        if (version < SIZED_INTEGERS) {
            IOUtilities::readAndDecode(in, &value);
            return static_cast<bool>(in);
        }
        uint64_t size = 0;
        IOUtilities::readAndDecode(in, &size);
        constexpr uint64_t MAX_INTEGER_BYTES = 64;
        if (!in || size == 0 || !IOUtilities::validateReadCount(in, size, 1, MAX_INTEGER_BYTES)) {
            return false;
        }
        char bytes[MAX_INTEGER_BYTES] = {};
        IOUtilities::readAndDecode(in, size, bytes);
        if (!in) {
            return false;
        }
        value = 0;
        for (uint64_t i = 0; i < size; ++i) {
            const auto byte = static_cast<uint64_t>(static_cast<unsigned char>(bytes[i]));
            if (i < sizeof(uint64_t)) {
                value |= byte << (8 * i);
            } else if (byte != 0) {
                value = std::numeric_limits<uint64_t>::max();
                return true;
            }
        }
        return true;
    }
}
