//
// Created by Merutilm on 2025-08-13.
// Modified by Fable 5 on 2026-07-06
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include <chrono>
#include <ctime>
#include <format>
#include <iomanip>
#include <iostream>
#include <utility>

#include "vkh_base.hpp"

namespace merutilm::vkh {
    struct logger {
        logger() = delete;

        static std::tm get_tm() {
            const auto now = std::chrono::system_clock::now();
            const auto time = std::chrono::system_clock::to_time_t(now);
            std::tm localTime = {};
            localtime_s(&localTime, &time);
            return localTime;
        }

        // auto return type: std::_Put_time is an MSVC-STL internal name that does not exist in libc++.
        // Must be defined (not just declared) above any caller: GCC requires the auto-return-type
        // deduction to be textually complete before use, even for other members of the same class.
        static auto current_put_time(const std::tm *const localTime) {
            return std::put_time(localTime, "%Y/%m/%d, %H:%M:%S");
        }

        static auto w_current_put_time(const std::tm *const localTime) {
            return std::put_time(localTime, L"%Y/%m/%d, %H:%M:%S");
        }

        template <typename... Args>
        static void log_err_silent(std::format_string<Args...> message, Args &&... args) {
            const std::tm localTime = get_tm();
            std::cerr << current_put_time(&localTime) << " | "
                      << std::format(message, std::forward<Args>(args)...) << "\n"
                      << std::flush;
        }

        template <typename... Args>
        static void w_log_err_silent(std::wformat_string<Args...> message, Args &&... args) {
            const std::tm localTime = get_tm();
            std::wcerr << w_current_put_time(&localTime) << " | "
                       << std::format(message, std::forward<Args>(args)...) << "\n"
                       << std::flush;
        }

        template <typename... Args>
        static void log_err(std::format_string<Args...> message, Args &&... args) {
            log_err_silent(message, std::forward<Args>(args)...);
            auto formattedMessage = std::format(message, std::forward<Args>(args)...);
            MessageBox(nullptr, formattedMessage.data(), "Error", MB_ICONERROR | MB_OK);
        }

        template <typename... Args>
        static void w_log_err(std::wformat_string<Args...> message, Args &&... args) {
            w_log_err_silent(message, std::forward<Args>(args)...);
            auto formattedMessage = std::format(message, std::forward<Args>(args)...);
            MessageBoxW(nullptr, formattedMessage.data(), L"Error", MB_ICONERROR | MB_OK);
        }

        template <typename... Args>
        static void log(std::format_string<Args...> message, Args &&... args) {
            const std::tm localTime = get_tm();
            std::cout << current_put_time(&localTime) << " | "
                      << std::format(message, std::forward<Args>(args)...) << "\n"
                      << std::flush;
        }

        template <typename... Args>
        static void w_log(std::wformat_string<Args...> message, Args &&... args) {
            const std::tm localTime = get_tm();
            std::wcout << w_current_put_time(&localTime) << " | "
                       << std::format(message, std::forward<Args>(args)...) << "\n"
                       << std::flush;
        }
    };
}
