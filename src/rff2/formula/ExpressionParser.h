//
// Created by AI; exact creation date unavailable.
// Modified by GPT-5 on 2026-08-21.
//

#pragma once

#include <complex>
#include <vector>
#include <string>
#include <stdexcept>

namespace merutilm::rff2 {
    enum class OpCode {
        PUSH_Z, PUSH_C, PUSH_CONST,
        ADD, SUB, MUL, DIV, POW,
        SIN, COS, TAN, SINH, COSH, TANH,
        EXP, LOG, ABS, SQRT,
        REAL, IMAG, CONJ, ARG,
        NEG,
        // New functions for complex fractals
        RABS,  // real absolute: abs(real(z)) + i*imag(z)
        IABS,  // imag absolute: real(z) + i*abs(imag(z))
        RIABS, // both absolute: abs(real(z)) + i*abs(imag(z)) - Burning Ship style
        NORM,  // |z|^2 = real(z)^2 + imag(z)^2
        FLOOR, // floor of real and imag parts
        CEIL,  // ceil of real and imag parts
        ROUND, // round of real and imag parts
        SIGN,  // sign of real and imag parts
        ATAN,  // atan (complex)
        ASIN,  // asin (complex)
        ACOS   // acos (complex)
    };

    struct Instruction {
        OpCode op;
        std::complex<double> value; // used for PUSH_CONST
    };

    class ExpressionParser {
        std::vector<Instruction> instructions;
        bool parseError = false;
        std::string errorMessage;
        
    public:
        ExpressionParser() = default;
        
        // Parse expression, returns true on success
        bool parse(const std::string& expression);
        
        // Check if last parse had errors
        [[nodiscard]] bool hasError() const { return parseError; }
        
        // Get error message from last parse
        [[nodiscard]] const std::string& getErrorMessage() const { return errorMessage; }
        
        // Evaluate (returns 0 if there was a parse error or empty instructions)
        [[nodiscard]] std::complex<double> evaluate(std::complex<double> z, std::complex<double> c) const;
        
        // Get supported syntax as string for UI display
        static std::string getSupportedSyntax();

    private:
        int getPrecedence(const std::string& op);
        bool isOperator(const std::string& token);
        bool isFunction(const std::string& token);
        OpCode getOpCode(const std::string& token);
    };
}
