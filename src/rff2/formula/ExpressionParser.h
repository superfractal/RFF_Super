//
// Created by AI; exact creation date unavailable.
// Modified by GPT-5 on 2026-08-21
// Modified by GPT-6 on 2026-09-30, 2026-10-01
// Modified by Opus 5.5 on 2026-10-25, 2026-11-15
//

#pragma once

#include <complex>
#include <cstddef>
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
        // Burning Ship: Michelitsch and Roessler (1992), mathematical reference; see NOTICE; project license unchanged.
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

    // What stopped a parse, so an editor can point at the characters; what() keeps the English message.
    enum class ParseErrorKind {
        NONE,
        EMPTY,           // nothing to evaluate
        INVALID_NUMBER,  // token: the malformed number
        BAD_ASSIGNMENT,  // something other than "z = ..."
        UNKNOWN_NAME,    // token: a word that is no function or constant
        UNKNOWN_SYMBOL,  // token: a character the grammar does not use
        UNMATCHED_CLOSE, // a ")" with no "(" before it
        UNCLOSED_OPEN,   // a "(" that is never closed
        EMPTY_BRACKETS,  // "()"
        MISSING_AFTER,   // token: an operator or function with nothing after it
        MISSING_BETWEEN, // token, other: two operators in a row
        MISSING_BEFORE,  // token: an operator with nothing before it
        INCOMPLETE       // the formula does not work out to one value
    };

    struct ParseErrorDetail {
        ParseErrorKind kind = ParseErrorKind::NONE;
        size_t position = std::string::npos; // first character, or npos when no place applies
        size_t length = 0;
        std::string token, other;
    };

    struct Instruction {
        OpCode op;
        std::complex<double> value; // used for PUSH_CONST
    };

    class ExpressionParser {
        std::vector<Instruction> instructions;
        bool parseError = false;
        std::string errorMessage;
        ParseErrorDetail errorDetail;
        
    public:
        ExpressionParser() = default;
        
        // Parse expression, returns true on success
        bool parse(const std::string& expression);
        
        // Get error message from last parse
        [[nodiscard]] const std::string& getErrorMessage() const { return errorMessage; }

        // Where and why the last parse failed.
        [[nodiscard]] const ParseErrorDetail& getErrorDetail() const { return errorDetail; }
        
        // Evaluate (returns 0 if there was a parse error or empty instructions)
        [[nodiscard]] std::complex<double> evaluate(std::complex<double> z, std::complex<double> c) const;
        
    private:
        int getPrecedence(const std::string& op);
        bool isOperator(const std::string& token);
        bool isFunction(const std::string& token);
        OpCode getOpCode(const std::string& token);
    };
}
