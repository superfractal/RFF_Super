# Numeric input ranges

Updated by GPT-6 on 2026-09-24.

The shared policy lives in `src/rff2/attr/NumericSettingLimits.hpp`.
These are inclusive input limits, not guarantees about available memory or export duration.

| Setting | Minimum | Maximum |
| --- | ---: | ---: |
| Rendering / video FPS | 1 | 1000 |
| Extra Final Zoom-in | 0 | 8 |
| Estimated Keyframes (manual preview estimate) | 1 | 100000 |
| Color / slope gamma | 0.01 | 10 |
| Color exposure | -1 | 0.999 |
| Color hue, settings panels (turns) | -1 | 1 |
| Color hue, timeline (turns) | -1000 | 1000 |
| Color saturation | -1 | 10 |
| Color brightness | -1 | 1 |
| Color contrast | -1 | 0.999 |
| Slope brightness | 0 | 100 |
| Bloom intensity | 0 | 100 |
| Rim mask boost | 1 | 100 |
| Rim / focus blur (pixels at width 1280) | 0 | 256 |
| Log Zoom / relief zoom reference | 0 | 16777216 |
| HDR exposure (stops) | -20 | 20 |
| HDR headroom | 1 | 64 |
| Calculation threads | 1 | Logical processor count, at least 1 |
| Max Iteration | 1 | Existing uint64 maximum |
| Auto Iteration Multiplier | 1 | 65535 |

Relief zoom reference also accepts exactly -1 for an unbound reference;
negative fractions are invalid. Fractional FPS and Extra Final Zoom-in values
remain valid. Automatic iteration multiplication saturates at the uint64 maximum
instead of wrapping.

Workspace and legacy settings share the ranges. Timeline tracks use the same
appearance limits, except hue permits multiple turns. Legacy color exposure and
contrast display three decimal places as needed to preserve their 0.999 upper limit.
Unsigned legacy integer input rejects negative signs, overflow and trailing text.
Legacy floating input rejects non-decimal tokens before conversion, including
NaN and Infinity under fast-math builds.

Saved settings and shader presets are not rewritten or globally clamped by this
change. Numeric timeline tracks outside their supported range reject loading
instead of silently changing key values; the existing live timeline remains intact.
No serialization layout or version changes are made.

## Deferred and unchanged

Zoom Speed, Zoom Step per Keyframe, Color cycle, color animation speed and flow
amount, stripe speed/offset/intervals, and texture/pattern/warp periods retain
their previous numeric ranges. Their animation arithmetic is unchanged.
The shared legacy parser still rejects malformed/non-finite text in these fields.

Existing bounds for AA, SSAA, canvas dimensions, bitrate, HDR peak luminance,
MPA, compression, Surface and Effects remain unchanged. Clarity retains its
existing combined resolution/SSAA/GPU validation. No new arbitrary coordinate
precision or maximum-iteration cap is introduced.

## Validation

Scratch regression programs in `debug/ui-audit-20260924/` exercise actual form
validation and application, shared legacy parsing, timeline descriptor limits,
automatic iteration overflow, and timeline stream loading without partial mutation.
The float parser/range probe uses a separate Release translation unit built with
`-O3 -ffast-math`; its checking harness uses strict floating-point semantics.
Actual form tests pass both without optimization and with those Release flags.
Clang syntax checks cover the affected UI, model and timeline sources.
These checks do not constitute a full application build or a visual flicker test.
