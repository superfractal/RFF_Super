//
// Created by Opus 5 on 2026-08-31.
//

#pragma once

namespace merutilm::rff2 {
    // How a pixel of the canvas is turned into a point of the complex plane. Every 360 mode places
    // the plane on a sphere by stereographic projection, which is conformal, so the fractal keeps
    // its shape wherever it lands; they differ only in how the canvas is pointed at that sphere.
    enum class FrtProjectionMethod {
        // The ordinary flat view.
        PLANAR = 0,
        // The whole sphere laid out flat, width one turn of longitude and height nadir to zenith.
        EQUIRECTANGULAR_360 = 1,
        // A camera standing inside the sphere, looking along a heading with a field of view, for exploring in 360.
        PERSPECTIVE_360 = 2
    };

    // The 360 projections are a debug-build feature, so a release build reads every projection as Planar
    // however it was set - including one carried in by a settings file a debug build wrote, which a release
    // build has no menu entry to turn off again.
    constexpr FrtProjectionMethod effectiveProjection(const FrtProjectionMethod method) {
#ifdef NDEBUG
        (void) method;
        return FrtProjectionMethod::PLANAR;
#else
        return method;
#endif
    }
}
