//
// Created and modified by AI; earlier exact dates unavailable.
// Modified by GPT-5 on 2026-08-21.
// Modified by Opus 5 on 2026-08-16
//

#include "ExpressionParser.h"
#include <stack>
#include <cctype>
#include <map>
#include <algorithm>
#include <iostream>
#include <numbers>
#include <cmath>

namespace merutilm::rff2 {

    namespace {
        // How many values an instruction takes off the stack. The PUSH_* forms take none, the
        // arithmetic operators take two, and every function and the unary minus take one.
        int operandCount(const OpCode op) {
            switch (op) {
                case OpCode::PUSH_Z:
                case OpCode::PUSH_C:
                case OpCode::PUSH_CONST: return 0;
                case OpCode::ADD:
                case OpCode::SUB:
                case OpCode::MUL:
                case OpCode::DIV:
                case OpCode::POW: return 2;
                default: return 1;
            }
        }

        // std::stod stops at the first character it cannot use and reports success on what it did
        // read, so "1..2" came back as 1 rather than as the malformed literal it is.
        double toNumber(const std::string& text) {
            size_t used = 0;
            const double value = std::stod(text, &used);
            if (used != text.size()) {
                throw std::runtime_error("Invalid number: " + text);
            }
            return value;
        }
    }

    bool ExpressionParser::parse(const std::string& expression) {
        instructions.clear();
        parseError = false;
        errorMessage.clear();

        try {
            std::vector<std::string> tokens;
            const size_t len = expression.length();
            for (size_t i = 0; i < len; ++i) {
                const char c = expression[i];

                if (c == ';') break;       // only the first statement is used
                if (c == '=') {            // assignment "z = rhs": drop the lhs target,
                                           // otherwise the leading z implicit-multiplies the rhs
                    // Anything other than z as the target was dropped just as silently, so
                    // "c = z^2+c" ran as "z = z^2+c" and iterated something the user never wrote.
                    if (tokens.size() != 1 || tokens[0] != "z") {
                        throw std::runtime_error("Only \"z = ...\" can be assigned");
                    }
                    tokens.clear();
                    continue;
                }
                if (std::isspace(static_cast<unsigned char>(c))) continue;

                if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
                    // Number literal: digits/'.', optional e[+-]?digits, optional trailing 'i'.
                    size_t j = i;
                    while (j < len && (std::isdigit(static_cast<unsigned char>(expression[j])) || expression[j] == '.')) ++j;
                    if (j < len && (expression[j] == 'e' || expression[j] == 'E')) {
                        size_t k = j + 1;
                        if (k < len && (expression[k] == '+' || expression[k] == '-')) ++k;
                        if (k < len && std::isdigit(static_cast<unsigned char>(expression[k]))) {
                            j = k;
                            while (j < len && std::isdigit(static_cast<unsigned char>(expression[j]))) ++j;
                        }
                    }
                    // Imaginary suffix, but only when it is not the start of an identifier (e.g. "2in").
                    if (j < len && expression[j] == 'i' &&
                        !(j + 1 < len && std::isalpha(static_cast<unsigned char>(expression[j + 1])))) {
                        ++j;
                    }
                    tokens.push_back(expression.substr(i, j - i));
                    i = j - 1;
                } else if (std::isalpha(static_cast<unsigned char>(c))) {
                    // Identifier: starts with a letter, then letters/digits (so "log10",
                    // "atan2" become single tokens that fail loudly instead of silently
                    // misparsing into log(10) / atan(2)).
                    size_t j = i;
                    while (j < len && std::isalnum(static_cast<unsigned char>(expression[j]))) ++j;
                    tokens.push_back(expression.substr(i, j - i));
                    i = j - 1;
                } else {
                    tokens.push_back(std::string(1, c));
                }
            }

            // Treat "**" (Python-style power) as a single "^" so z**2 works.
            {
                std::vector<std::string> collapsed;
                collapsed.reserve(tokens.size());
                for (size_t i = 0; i < tokens.size(); ++i) {
                    if (tokens[i] == "*" && i + 1 < tokens.size() && tokens[i + 1] == "*") {
                        collapsed.push_back("^");
                        ++i;
                    } else {
                        collapsed.push_back(tokens[i]);
                    }
                }
                tokens.swap(collapsed);
            }

            auto isNumTok = [](const std::string& t) {
                return !t.empty() && (std::isdigit(static_cast<unsigned char>(t[0])) || t[0] == '.');
            };
            auto isIdentTok = [](const std::string& t) {
                return !t.empty() && std::isalpha(static_cast<unsigned char>(t[0]));
            };
            std::vector<std::string> merged;
            merged.reserve(tokens.size() * 2);
            for (size_t i = 0; i < tokens.size(); ++i) {
                if (i > 0) {
                    const std::string& prev = tokens[i - 1];
                    const std::string& cur = tokens[i];
                    const bool prevEndsValue = prev == ")" || isNumTok(prev) || (isIdentTok(prev) && !isFunction(prev));
                    const bool curStartsValue = cur == "(" || isNumTok(cur) || isIdentTok(cur);
                    if (prevEndsValue && curStartsValue) {
                        merged.push_back("*");
                    }
                }
                merged.push_back(tokens[i]);
            }

            std::stack<std::string> operators;
            bool expectUnary = true;

            auto emitValue = [&](const std::string& tok) {
                // Number with imaginary suffix (1i, 2i, 3.5i, 1e3i)
                if (tok.size() > 1 && tok.back() == 'i' &&
                    (std::isdigit(static_cast<unsigned char>(tok[0])) || tok[0] == '.')) {
                    instructions.push_back({OpCode::PUSH_CONST,
                                            std::complex<double>(0, toNumber(tok.substr(0, tok.size() - 1)))});
                } else if (std::isdigit(static_cast<unsigned char>(tok[0])) || tok[0] == '.') {
                    instructions.push_back({OpCode::PUSH_CONST, std::complex<double>(toNumber(tok), 0)});
                } else if (tok == "z") {
                    instructions.push_back({OpCode::PUSH_Z, {}});
                } else if (tok == "c") {
                    instructions.push_back({OpCode::PUSH_C, {}});
                } else if (tok == "i") {
                    instructions.push_back({OpCode::PUSH_CONST, std::complex<double>(0, 1)});
                } else if (tok == "pi") {
                    instructions.push_back({OpCode::PUSH_CONST, std::complex<double>(std::numbers::pi, 0)});
                } else if (tok == "tau") {
                    instructions.push_back({OpCode::PUSH_CONST, std::complex<double>(2.0 * std::numbers::pi, 0)});
                } else if (tok == "phi") {
                    instructions.push_back({OpCode::PUSH_CONST, std::complex<double>(std::numbers::phi, 0)});
                } else if (tok == "e") {
                    instructions.push_back({OpCode::PUSH_CONST, std::complex<double>(std::numbers::e, 0)});
                } else {
                    throw std::runtime_error("Unknown token: " + tok);
                }
            };

            for (const std::string& tok : merged) {
                if (tok == "(") {
                    operators.push("(");
                    expectUnary = true;
                } else if (tok == ")") {
                    while (!operators.empty() && operators.top() != "(") {
                        instructions.push_back({getOpCode(operators.top()), {}});
                        operators.pop();
                    }
                    if (operators.empty()) throw std::runtime_error("Mismatched parentheses");
                    operators.pop(); // discard "("
                    if (!operators.empty() && isFunction(operators.top())) {
                        instructions.push_back({getOpCode(operators.top()), {}});
                        operators.pop();
                    }
                    expectUnary = false;
                } else if (isFunction(tok)) {
                    operators.push(tok);
                    expectUnary = true;
                } else if (isOperator(tok)) {
                    if (tok == "-" && expectUnary) {
                        operators.push("_");
                    } else {
                        // "^" is right-associative: don't pop an equal-precedence "^".
                        const bool rightAssoc = (tok == "^");
                        while (!operators.empty() && operators.top() != "(" &&
                               (rightAssoc
                                    ? getPrecedence(operators.top()) > getPrecedence(tok)
                                    : getPrecedence(operators.top()) >= getPrecedence(tok))) {
                            instructions.push_back({getOpCode(operators.top()), {}});
                            operators.pop();
                        }
                        operators.push(tok);
                    }
                    expectUnary = true;
                } else {
                    emitValue(tok);
                    expectUnary = false;
                }
            }

            while (!operators.empty()) {
                if (operators.top() == "(" || operators.top() == ")") {
                    throw std::runtime_error("Mismatched parentheses");
                }
                instructions.push_back({getOpCode(operators.top()), {}});
                operators.pop();
            }

            // Nothing above counts arguments, so "z+", "sin()" and "z++c" all came through as
            // accepted formulas. Their programs then ran out of operands in evaluate(), which
            // answers 0 for that - and a formula that returns 0 for every z leaves the whole
            // frame at the interior color, with no error anywhere to say why.
            int depth = 0;
            for (const Instruction& instr : instructions) {
                const int operands = operandCount(instr.op);
                if (depth < operands) {
                    throw std::runtime_error("An operator or function is missing an argument");
                }
                depth += 1 - operands;
            }
            if (instructions.empty() || depth != 1) {
                throw std::runtime_error("The formula must work out to a single value");
            }

            return true;
        } catch (const std::exception& e) {
            parseError = true;
            errorMessage = e.what();
            instructions.clear();
            return false;
        }
    }

    std::complex<double> ExpressionParser::evaluate(std::complex<double> z, std::complex<double> c) const {
        if (parseError || instructions.empty()) {
            return std::complex<double>(0, 0);
        }
        
        std::vector<std::complex<double>> stack;
        stack.reserve(16);

        for (const auto& instr : instructions) {
            switch (instr.op) {
                case OpCode::PUSH_Z: stack.push_back(z); break;
                case OpCode::PUSH_C: stack.push_back(c); break;
                case OpCode::PUSH_CONST: stack.push_back(instr.value); break;
                case OpCode::ADD: {
                    if (stack.size() < 2) return std::complex<double>(0);
                    auto b = stack.back(); stack.pop_back();
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(a + b);
                    break;
                }
                case OpCode::SUB: {
                    if (stack.size() < 2) return std::complex<double>(0);
                    auto b = stack.back(); stack.pop_back();
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(a - b);
                    break;
                }
                case OpCode::MUL: {
                    if (stack.size() < 2) return std::complex<double>(0);
                    auto b = stack.back(); stack.pop_back();
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(a * b);
                    break;
                }
                case OpCode::DIV: {
                    if (stack.size() < 2) return std::complex<double>(0);
                    auto b = stack.back(); stack.pop_back();
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(a / b);
                    break;
                }
                case OpCode::POW: {
                    if (stack.size() < 2) return std::complex<double>(0);
                    auto b = stack.back(); stack.pop_back();
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(std::pow(a, b));
                    break;
                }
                case OpCode::NEG: {
                    if (stack.empty()) return std::complex<double>(0);
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(-a);
                    break;
                }
                case OpCode::SIN: {
                    if (stack.empty()) return std::complex<double>(0);
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(std::sin(a));
                    break;
                }
                case OpCode::COS: {
                    if (stack.empty()) return std::complex<double>(0);
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(std::cos(a));
                    break;
                }
                case OpCode::TAN: {
                    if (stack.empty()) return std::complex<double>(0);
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(std::tan(a));
                    break;
                }
                case OpCode::SINH: {
                    if (stack.empty()) return std::complex<double>(0);
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(std::sinh(a));
                    break;
                }
                case OpCode::COSH: {
                    if (stack.empty()) return std::complex<double>(0);
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(std::cosh(a));
                    break;
                }
                case OpCode::TANH: {
                    if (stack.empty()) return std::complex<double>(0);
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(std::tanh(a));
                    break;
                }
                case OpCode::EXP: {
                    if (stack.empty()) return std::complex<double>(0);
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(std::exp(a));
                    break;
                }
                case OpCode::LOG: {
                    if (stack.empty()) return std::complex<double>(0);
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(std::log(a));
                    break;
                }
                case OpCode::ABS: {
                    if (stack.empty()) return std::complex<double>(0);
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(std::abs(a));
                    break;
                }
                case OpCode::SQRT: {
                    if (stack.empty()) return std::complex<double>(0);
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(std::sqrt(a));
                    break;
                }
                case OpCode::REAL: {
                    if (stack.empty()) return std::complex<double>(0);
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(a.real());
                    break;
                }
                case OpCode::IMAG: {
                    if (stack.empty()) return std::complex<double>(0);
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(a.imag());
                    break;
                }
                case OpCode::CONJ: {
                    if (stack.empty()) return std::complex<double>(0);
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(std::conj(a));
                    break;
                }
                case OpCode::ARG: {
                    if (stack.empty()) return std::complex<double>(0);
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(std::arg(a));
                    break;
                }
                // New functions
                case OpCode::RABS: {
                    if (stack.empty()) return std::complex<double>(0);
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(std::complex<double>(std::abs(a.real()), a.imag()));
                    break;
                }
                case OpCode::IABS: {
                    if (stack.empty()) return std::complex<double>(0);
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(std::complex<double>(a.real(), std::abs(a.imag())));
                    break;
                }
                case OpCode::RIABS: {
                    if (stack.empty()) return std::complex<double>(0);
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(std::complex<double>(std::abs(a.real()), std::abs(a.imag())));
                    break;
                }
                case OpCode::NORM: {
                    if (stack.empty()) return std::complex<double>(0);
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(std::norm(a));
                    break;
                }
                case OpCode::FLOOR: {
                    if (stack.empty()) return std::complex<double>(0);
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(std::complex<double>(std::floor(a.real()), std::floor(a.imag())));
                    break;
                }
                case OpCode::CEIL: {
                    if (stack.empty()) return std::complex<double>(0);
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(std::complex<double>(std::ceil(a.real()), std::ceil(a.imag())));
                    break;
                }
                case OpCode::ROUND: {
                    if (stack.empty()) return std::complex<double>(0);
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(std::complex<double>(std::round(a.real()), std::round(a.imag())));
                    break;
                }
                case OpCode::SIGN: {
                    if (stack.empty()) return std::complex<double>(0);
                    auto a = stack.back(); stack.pop_back();
                    auto signR = (a.real() > 0) - (a.real() < 0);
                    auto signI = (a.imag() > 0) - (a.imag() < 0);
                    stack.push_back(std::complex<double>(signR, signI));
                    break;
                }
                case OpCode::ATAN: {
                    if (stack.empty()) return std::complex<double>(0);
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(std::atan(a));
                    break;
                }
                case OpCode::ASIN: {
                    if (stack.empty()) return std::complex<double>(0);
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(std::asin(a));
                    break;
                }
                case OpCode::ACOS: {
                    if (stack.empty()) return std::complex<double>(0);
                    auto a = stack.back(); stack.pop_back();
                    stack.push_back(std::acos(a));
                    break;
                }
           }
        }
        return stack.empty() ? std::complex<double>(0) : stack.back();
    }

    int ExpressionParser::getPrecedence(const std::string& op) {
        if (op == "+" || op == "-") return 1;
        if (op == "*" || op == "/") return 2;
        if (op == "_") return 3;   // unary minus: looser than ^, so -z^2 == -(z^2)
        if (op == "^") return 4;
        // Function application ties with ^ rather than beating it, so an unparenthesized "sin z^2"
        // reads as sin(z^2), the way the notation does. It still beats every other operator, so
        // "sin z + c" is (sin z) + c, and "sin(z)^2" still squares the sine.
        if (isFunction(op)) return 4;
        return 0;
    }

    bool ExpressionParser::isOperator(const std::string& token) {
        return token == "+" || token == "-" || token == "*" || token == "/" || token == "^";
    }

    bool ExpressionParser::isFunction(const std::string& token) {
         return token == "sin" || token == "cos" || token == "tan" ||
                token == "sinh" || token == "cosh" || token == "tanh" ||
                token == "exp" || token == "log" || token == "ln" || token == "abs" ||
                token == "sqrt" || token == "real" || token == "imag" || 
                token == "conj" || token == "arg" ||
                token == "rabs" || token == "iabs" || token == "riabs" ||
                token == "norm" || token == "floor" || token == "ceil" ||
                token == "round" || token == "sign" ||
                token == "atan" || token == "asin" || token == "acos" ||
                // Aliases
                token == "re" || token == "im";
    }

    OpCode ExpressionParser::getOpCode(const std::string& token) {
        if (token == "+") return OpCode::ADD;
        if (token == "-") return OpCode::SUB;
        if (token == "*") return OpCode::MUL;
        if (token == "/") return OpCode::DIV;
        if (token == "^") return OpCode::POW;
        if (token == "_") return OpCode::NEG;
        if (token == "sin") return OpCode::SIN;
        if (token == "cos") return OpCode::COS;
        if (token == "tan") return OpCode::TAN;
        if (token == "sinh") return OpCode::SINH;
        if (token == "cosh") return OpCode::COSH;
        if (token == "tanh") return OpCode::TANH;
        if (token == "exp") return OpCode::EXP;
        if (token == "log" || token == "ln") return OpCode::LOG;
        if (token == "abs") return OpCode::ABS;
        if (token == "sqrt") return OpCode::SQRT;
        if (token == "real" || token == "re") return OpCode::REAL;
        if (token == "imag" || token == "im") return OpCode::IMAG;
        if (token == "conj") return OpCode::CONJ;
        if (token == "arg") return OpCode::ARG;
        if (token == "rabs") return OpCode::RABS;
        if (token == "iabs") return OpCode::IABS;
        if (token == "riabs") return OpCode::RIABS;
        if (token == "norm") return OpCode::NORM;
        if (token == "floor") return OpCode::FLOOR;
        if (token == "ceil") return OpCode::CEIL;
        if (token == "round") return OpCode::ROUND;
        if (token == "sign") return OpCode::SIGN;
        if (token == "atan") return OpCode::ATAN;
        if (token == "asin") return OpCode::ASIN;
        if (token == "acos") return OpCode::ACOS;
        throw std::runtime_error("Unknown operator: " + token);
    }

    std::string ExpressionParser::getSupportedSyntax() {
        return
            "Variables: z, c, i, pi, tau, phi, e\n"
            "Operators: + - * / ^ or ** (power, right-assoc)\n"
            "Basic: abs, sqrt, exp, log/ln\n"
            "Trig: sin, cos, tan, asin, acos, atan\n"
            "Hyper: sinh, cosh, tanh\n"
            "Complex: real(re), imag(im), conj, arg, norm\n"
            "Fractal: rabs, iabs, riabs (Burning Ship)\n"
            "Util: floor, ceil, round, sign\n"
            "Imaginary: 1i, 2i, etc. (number followed by i)\n"
            "Implicit *: 2z, 2sin(z), 2(z+c), (z+1)(z-1)\n"
            "Examples:\n"
            "  z^2+c (Mandelbrot)\n"
            "  riabs(z)^2+c (Burning Ship)\n"
            "  iabs(z^2+1)+c";
    }
}
