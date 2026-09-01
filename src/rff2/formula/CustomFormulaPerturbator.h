//
// Created and modified by AI; earlier exact dates unavailable.
// Modified by GPT-5 on 2026-08-21.
//

#pragma once
#include "MandelbrotPerturbator.h"
#include "ExpressionParser.h"
#include "DummyReference.h"

namespace merutilm::rff2 {
    class CustomFormulaPerturbator : public MandelbrotPerturbator {
        ExpressionParser parser;
        std::unique_ptr<DummyReference> dummyReference;
        double dcMax;
        bool hasParseError = false;

    public:
        CustomFormulaPerturbator(ParallelRenderState& state, const FractalAttribute& calc, double dcMax);

        ~CustomFormulaPerturbator() override = default;

        const MandelbrotReference* getReference() const override;
        dex getDcMaxAsDoubleExp() const override;
        double iterate(const dex& dcr, const dex& dci) const override;
    };
}
