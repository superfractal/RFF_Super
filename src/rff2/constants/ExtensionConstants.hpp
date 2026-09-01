//
// Created by Merutilm on 2025-08-09.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21.
// Modified by Opus 5 on 2026-08-12, 2026-08-14, 2026-08-18, 2026-08-21, 2026-09-01
//

#pragma once
namespace merutilm::rff2::Constants::Extension {
    constexpr auto DYNAMIC_MAP = L"rfm";
    constexpr auto COMPRESSED_MAP = L"rfmz";
    constexpr auto STATIC_MAP = L"rfsm";
    constexpr auto LOCATION = L"rfl";
    constexpr auto IMAGE = L"png";
    constexpr auto VIDEO = L"mp4";
    // Lossless export is RGB H.264, which mp4 players handle poorly; matroska carries it cleanly.
    constexpr auto VIDEO_LOSSLESS = L"mkv";
    constexpr auto KFR = L"kfr";
    constexpr auto SHADER_PRESET = L"rfsp";
    constexpr auto CONFIG = L"rfc";
    constexpr auto TIMELINE = L"rfvt";
    constexpr auto PREFERENCES = L"rfp";
    constexpr auto DESC_DYNAMIC_MAP = L"RFF dynamic map binary";
    constexpr auto DESC_COMPRESSED_MAP = L"RFF compressed map binary";
    constexpr auto DESC_STATIC_MAP = L"RFF static map binary";
    constexpr auto DESC_LOCATION = L"RFF location binary";
    constexpr auto DESC_IMAGE = L"Image file";
    constexpr auto DESC_VIDEO = L"Video file";
    constexpr auto DESC_KFR = L"Kalles Fraktaler file";
    constexpr auto DESC_SHADER_PRESET = L"RFF shader preset binary";
    constexpr auto DESC_CONFIG = L"RFF config binary";
    constexpr auto DESC_TIMELINE = L"RFF video timeline binary";
}
