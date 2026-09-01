//
// Created by Opus 5 on 2026-08-18.
// Modified by GPT-5 on 2026-08-18, 2026-08-23
//

#pragma once
#include <vector>

#include "../attr/VidTimelineAttribute.h"
#include "../attr/VidTimelineTarget.h"

namespace merutilm::rff2 {
    // The depth <-> time mapping of one export.
    //
    // The timeline's axis is depth, so how long the video runs is not a setting but a result: the
    // time a stretch of depth takes is the integral of 1/speed over it. This builds that integral
    // once, up front, which is what lets the frame count, the progress ratio and the remaining time
    // be known before the first frame is drawn, and what keeps frame n's depth free of the drift a
    // running subtraction accumulates.
    //
    // With no timeline the mapping is one constant speed, and isUniform() says so: the export then
    // walks the depth axis exactly as it did before timelines existed, down to the last bit.
    class TimelineSchedule {
    public:
        // Samples of the integral per keyframe of depth. Finer than any speed curve a person edits.
        static constexpr float SAMPLES_PER_KEYFRAME = 256.0f;
        // Ceiling on the sample table, so a folder of a hundred thousand keyframes still builds one.
        static constexpr size_t MAX_SAMPLES = 1u << 20;

        // startDepth is the keyframe count the video begins at, endDepth the (negative) depth it
        // ends at, and fallbackSpeed the Zoom Speed used wherever the timeline says nothing.
        static TimelineSchedule create(const VidTimelineAttribute &timeline, float startDepth, float endDepth,
                                       float fallbackSpeed);

        // True when the mapping is one constant speed with no holds - the pre-timeline behaviour.
        [[nodiscard]] bool isUniform() const { return uniform; }

        [[nodiscard]] float getStartDepth() const { return startDepth; }

        [[nodiscard]] float getEndDepth() const { return endDepth; }

        [[nodiscard]] float getTotalSeconds() const { return totalSeconds; }

        // Output frames the export will write at this frame rate.
        [[nodiscard]] uint64_t totalFrames(float fps) const;

        // Keyframes per second at this depth, never below VidTimelineAttribute::MIN_SPEED.
        [[nodiscard]] float speedAt(float depth) const;

        // Seconds from the start of the video to the moment this depth is reached.
        [[nodiscard]] float timeAt(float depth) const;

        // The depth shown at this instant. Clamped to the axis outside the video's own length.
        [[nodiscard]] float depthAt(float sec) const;

        // The value a track holds at this depth, or fallback when the track carries no keys.
        static float evaluateTrack(const VidTimelineTrack &track, float depth, float fallback);

        static float evaluateTrack(const VidTimelineTrack &track, float depth, float fallback,
                                   float minValue, float maxValue);

        // The track driving this target, or nullptr when the timeline has none.
        static const VidTimelineTrack *findTrack(const VidTimelineAttribute &timeline, uint16_t targetId);

    private:
        struct HoldPoint {
            float depth;
            float seconds;
            // Seconds from the start of the video to the moment this hold begins.
            float startTime;
        };

        bool uniform = true;
        float startDepth = 0.0f;
        float endDepth = 0.0f;
        float depthStep = 0.0f;
        float uniformSpeed = 1.0f;
        float totalSeconds = 0.0f;
        // The speed track, copied so the schedule stays valid however the attribute is later edited.
        VidTimelineTrack speedTrack = {};
        bool hasSpeedTrack = false;
        // Cumulative time of the speed integral alone, sampled at even depth steps from startDepth.
        std::vector<float> times = {};
        // Held in playback order, holds of no length dropped.
        std::vector<HoldPoint> holds = {};

        [[nodiscard]] float integralAt(float depth) const;

        [[nodiscard]] float invertIntegral(float sec) const;
    };
}
