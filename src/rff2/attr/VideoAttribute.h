//
// Created by Merutilm on 2025-05-04.
// Modified by Opus 5 on 2026-08-18
//

#pragma once
#include "VidAnimationAttribute.h"
#include "VidDataAttribute.h"
#include "VidExportAttribute.h"
#include "VidTimelineAttribute.h"


namespace merutilm::rff2 {
    struct VideoAttribute {
        VidDataAttribute data;
        VidAnimationAttribute animation;
        VidExportAttribute exportation;
        VidTimelineAttribute timeline;
    };
}