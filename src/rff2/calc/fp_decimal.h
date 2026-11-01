//
// Created by Merutilm on 2025-05-05.
// Modified by Opus 5.5 on 2026-10-25
//

#pragma once
#include "gmp.h"
#include "fp_decimal_calculator.h"

namespace merutilm::rff2 {
    class fp_decimal {
        int exp2;
        mpz_t value;

    public:

        fp_decimal(const std::string &value, int exp10);

        explicit fp_decimal(const fp_decimal_calculator &calc);

        ~fp_decimal();

        fp_decimal(const fp_decimal& other);

        fp_decimal& operator=(const fp_decimal& other);

        fp_decimal(fp_decimal&& other) noexcept;

        fp_decimal& operator=(fp_decimal&& other) noexcept;

        std::string to_string() const;

        bool is_zero() const;

        fp_decimal_calculator edit() const;

    };
}