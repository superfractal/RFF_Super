# MFR-HDR display experiment

Implemented from the user-supplied MFR-HDR 1.0 specification on 2026-09-16.

## Try it

Open Export > HDR. Set HDR Rendering to On and Tone Mapping to MFR Shoulder.
Leave MFR Mastering Peak at 4000 nits, then compare Exposure at -2, 0 and +2.
The same controls are available in the legacy HDR dialog. Tone Mapping also
offers MFR Log View, MFR Linear Clip and MFR False Color. Select an older curve
or switch HDR Rendering Off to leave the experiment. Existing defaults stay unchanged.
Settings and shader presets retain the mode and MFR mastering peak.

Use Studio lighting and/or Bloom for scene highlights above one. With Studio
Off, existing legacy surface shading is decoded to linear light before color,
fog and bloom; that conversion cannot recover highlights clipped earlier by
legacy shading. Existing fractal calculations, perturbation precision and MPA
remain in use. The reference HTML's distance-estimate emission generator is not
reproduced because its palette/functions were not supplied.

## Display

The scene buffer remains linear BT.709/sRGB in the existing float16 pipeline.
The final MFR SDR view converts it to linear BT.2020 before exposure and uses
Y = 0.2627 R + 0.6780 G + 0.0593 B and P = masteringPeakNits / 203.
Shoulder uses clamp((Y/(1+Y))/(P/(1+P)),0,1); Log View uses
clamp(log(1+Y)/log(1+P),0,1); Linear Clip uses clamp(Y,0,1).
The supplied inverse color matrix and uniform saturation compression towards
neutral gray put the output in the sRGB gamut before the existing sRGB OETF.
Exposure and mastering peak affect the final display pass only. Headroom is
ignored by MFR SDR views. The existing Dither control adds up to half an 8-bit
step; it is skipped for False Color. MFR views bypass the display-stage VHS,
monochrome and print quantization finish so the specified display transform
remains the last color operation.

False Color visualizes exposure-adjusted log2 luminance, with zero black and
these original RFF_Super legend anchors: -16 EV navy, -8 EV blue, 0 EV gray,
+8 EV yellow, +16 EV red. Intermediate values interpolate; extremes saturate.
This palette is not claimed to match the unspecified HTML gradient.

## Limits

This is the display portion of the specification. No MFRV, LL16, NU32, Radiance
HDR export, histogram, P99 statistics or saved-video recoloring is implemented.
It does not claim 48-stop storage or native HDR monitor output. Existing SDR
image/video exports use the MFR view; existing PQ/HLG exports retain their
separate transfer path and peak setting. Antialiasing uses the application's
existing pipeline rather than the reference HTML's exact four-point sampler.

## Verification on 2026-09-16

The production final fragment shader passes 36 MFR GPU cases (589824 channels,
maximum absolute SDR-component error 1.37e-6 against a double-precision reference)
and 216 existing SDR/PQ/HLG cases (3538944 channels, maximum error 1.10e-5).
Vulkan validation reports zero errors for both suites on an RTX 5060 Ti.
RFC/RFSP modes 0–7 round-trip, missing-tail defaults on reused destinations,
truncated-tail rejection and scene-linear selection are tested using production
objects. The release executable is bin/RFF_Super_MFR.exe; the running normal
executable is retained. The GUI has not been visually inspected in this session.
