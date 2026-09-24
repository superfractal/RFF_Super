//
// Created by Merutilm on 2025-05-31.
// Modified by GPT-6 on 2026-09-23
//

#include "CalculationPresets.h"


namespace merutilm::rff2 {
    std::string CalculationPresets::UltraFast::getName() const {
        return "Ultra Fast";
    }

    FrtMPAAttribute CalculationPresets::UltraFast::genMPA() const {
        return FrtMPAAttribute{
            .minSkipReference = 4,
            .maxMultiplierBetweenLevel = 2,
            .epsilonPower = -3,
            .mpaSelectionMethod = FrtMPASelectionMethod::HIGHEST,
            .mpaCompressionMethod = FrtMPACompressionMethod::NO_COMPRESSION
        };
    }

    FrtReferenceCompAttribute CalculationPresets::UltraFast::genReferenceCompression() const {
        return FrtReferenceCompAttribute{
            .compressCriteria = 0,
            .compressionThresholdPower = 0,
            .noCompressorNormalization = false
        };
    }

    std::string CalculationPresets::Fast::getName() const {
        return "Fast";
    }

    FrtMPAAttribute CalculationPresets::Fast::genMPA() const {
        return FrtMPAAttribute{
            .minSkipReference = 8,
            .maxMultiplierBetweenLevel = 2,
            .epsilonPower = -4,
            .mpaSelectionMethod = FrtMPASelectionMethod::HIGHEST,
            .mpaCompressionMethod = FrtMPACompressionMethod::NO_COMPRESSION
        };
    }

    FrtReferenceCompAttribute CalculationPresets::Fast::genReferenceCompression() const {
        return FrtReferenceCompAttribute{
            .compressCriteria = 1000000,
            .compressionThresholdPower = 7,
            .noCompressorNormalization = false
        };
    }

    std::string CalculationPresets::Normal::getName() const {
        return "Normal";
    }

    FrtMPAAttribute CalculationPresets::Normal::genMPA() const {
        return FrtMPAAttribute{
            .minSkipReference = 8,
            .maxMultiplierBetweenLevel = 2,
            .epsilonPower = -5,
            .mpaSelectionMethod = FrtMPASelectionMethod::HIGHEST,
            .mpaCompressionMethod = FrtMPACompressionMethod::LITTLE_COMPRESSION
        };
    }

    FrtReferenceCompAttribute CalculationPresets::Normal::genReferenceCompression() const {
        return FrtReferenceCompAttribute{
            .compressCriteria = 1000000,
            .compressionThresholdPower = 11,
            .noCompressorNormalization = false
        };
    }

    std::string CalculationPresets::Best::getName() const {
        return "Best";
    }

    FrtMPAAttribute CalculationPresets::Best::genMPA() const {
        return FrtMPAAttribute{
            .minSkipReference = 8,
            .maxMultiplierBetweenLevel = 2,
            .epsilonPower = -6,
            .mpaSelectionMethod = FrtMPASelectionMethod::HIGHEST,
            .mpaCompressionMethod = FrtMPACompressionMethod::LITTLE_COMPRESSION
        };
    }

    FrtReferenceCompAttribute CalculationPresets::Best::genReferenceCompression() const {
        return FrtReferenceCompAttribute{
            .compressCriteria = 1000000,
            .compressionThresholdPower = 15,
            .noCompressorNormalization = false
        };
    }

    std::string CalculationPresets::UltraBest::getName() const {
        return "Ultra Best";
    }

    FrtMPAAttribute CalculationPresets::UltraBest::genMPA() const {
        return FrtMPAAttribute{
            .minSkipReference = 8,
            .maxMultiplierBetweenLevel = 2,
            .epsilonPower = -7,
            .mpaSelectionMethod = FrtMPASelectionMethod::HIGHEST,
            .mpaCompressionMethod = FrtMPACompressionMethod::LITTLE_COMPRESSION
        };
    }

    FrtReferenceCompAttribute CalculationPresets::UltraBest::genReferenceCompression() const {
        return FrtReferenceCompAttribute{
            .compressCriteria = 1000000,
            .compressionThresholdPower = 19,
            .noCompressorNormalization = false
        };
    }

    std::string CalculationPresets::Stable::getName() const {
        return "Stable";
    }

    FrtMPAAttribute CalculationPresets::Stable::genMPA() const {
        return FrtMPAAttribute{
            .minSkipReference = 8,
            .maxMultiplierBetweenLevel = 2,
            .epsilonPower = -4,
            .mpaSelectionMethod = FrtMPASelectionMethod::HIGHEST,
            .mpaCompressionMethod = FrtMPACompressionMethod::STRONGEST
        };
    }

    FrtReferenceCompAttribute CalculationPresets::Stable::genReferenceCompression() const {
        return FrtReferenceCompAttribute{
            .compressCriteria = 1000000,
            .compressionThresholdPower = 6,
            .noCompressorNormalization = false
        };
    }

    std::string CalculationPresets::MoreStable::getName() const {
        return "More Stable";
    }

    FrtMPAAttribute CalculationPresets::MoreStable::genMPA() const {
        return FrtMPAAttribute{
            .minSkipReference = 8,
            .maxMultiplierBetweenLevel = 2,
            .epsilonPower = -4,
            .mpaSelectionMethod = FrtMPASelectionMethod::HIGHEST,
            .mpaCompressionMethod = FrtMPACompressionMethod::STRONGEST
        };
    }

    FrtReferenceCompAttribute CalculationPresets::MoreStable::genReferenceCompression() const {
        return FrtReferenceCompAttribute{
            .compressCriteria = 100000,
            .compressionThresholdPower = 6,
            .noCompressorNormalization = false
        };
    }

    std::string CalculationPresets::UltraStable::getName() const {
        return "Ultra Stable";
    }

    FrtMPAAttribute CalculationPresets::UltraStable::genMPA() const {
        return FrtMPAAttribute{
            .minSkipReference = 8,
            .maxMultiplierBetweenLevel = 2,
            .epsilonPower = -4,
            .mpaSelectionMethod = FrtMPASelectionMethod::HIGHEST,
            .mpaCompressionMethod = FrtMPACompressionMethod::STRONGEST
        };
    }

    FrtReferenceCompAttribute CalculationPresets::UltraStable::genReferenceCompression() const {
        return FrtReferenceCompAttribute{
            .compressCriteria = 10000,
            .compressionThresholdPower = 6,
            .noCompressorNormalization = true
        };
    }
}
