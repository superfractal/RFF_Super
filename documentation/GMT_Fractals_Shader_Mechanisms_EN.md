<!-- Modified by GPT-6 on 2026-10-07, 2026-10-08. -->
<!-- Modified by Opus 5.5 on 2026-11-01. -->

# GMT shader mechanisms and practical 2D adaptations

> **Final review entry point (2026-10-08):** [Improving RFF_Super with GMT as a reference](GMT_to_RFF_Review_EN.md). All 3,727 inventoried GMT files have completed inspection records and unchanged hashes. This companion preserves chronological evidence: earlier progress counts, pending review statements and provisional priorities describe their recording time, not current coverage. Later corrections and the final entry point govern recommendations. Source inspection does not establish runtime correctness or measured GPU benefit.

> **Follow-up:** [the deeper file-by-file audit](GMT_Fractals_Deep_Code_Audit_EN.md) adds concrete counterexamples for attenuation, palette wrapping, volumetric weighting, GGX regularization and bounded-VNDF PDF consistency. Its S06 strengthens the tentative sampler question below with an analytical endpoint case. Full-repository review remains in progress.

Date: 2026-10-07. Companion to [the feature review](GMT_Fractals_Feature_Review_EN.md) and [source inventory](GMT_Fractals_Source_Inventory_EN.md).

This appendix explains implementation mechanisms, rather than merely listing visual effects. Observations come from the downloaded source; GPU execution, visual comparisons and performance measurements were not performed. Costs mentioned in source comments are historical developer observations, not measurements reproduced here. Proposed adaptations target RFF_Super, preserving its arbitrary-precision coordinates, perturbation and period-derived MPA.

Difficulty and effectiveness are decimal 1–10 planning scores using the main report's rubric. They rate the incremental RFF adaptation described here, not the complete GMT subsystem. These scopes overlap the main report and must not be added together as independent projects.

## 1. Reflected footprint filtering, separate from material roughness

**Mechanism.** `envConeLod` converts an angular cone into the number of texels covered on an equirectangular environment image. With map dimensions W,H, cone angle a and normalized direction y-component y, the code computes approximately:

- `sinT = max(sqrt(max(0, 1-y*y)), 0.05)`;
- horizontal span `a*W/(2*pi*sinT)` and vertical span `a*H/pi`;
- footprint LOD `log2(max(horizontalSpan, verticalSpan, 1))`.

The longitude denominator accounts for distortion near the poles. The chosen mip is the maximum of footprint LOD and `roughness * maxMip`. Roughness and pixel minification are two reasons to widen the filter; summing them would introduce extra blur. Reflection code, when the `coneAA` gate is enabled, estimates angular spread from a neighboring normal: `2*length(nOff-n) + pixelSizeScale`. It sets `g_envConeAngle` before sampling and resets it afterward. This is an approximate footprint, not a complete anisotropic ray-differential solution.

**Why it helps.** A mirror can still cover many environment texels per screen pixel when its normal changes rapidly. Roughness alone misses that case and can produce unstable fine highlights during movement.

**2D adaptation.** Apply the same separation to environment lighting on RFF's existing relief normals. Derive the footprint from neighboring relief normals in output-pixel units; do not derive it from a float-converted deep-zoom center. No new fractal recurrence is needed. Retain a no-filter reference mode and measure temporal shimmer, highlight energy and sharpness independently. Test poles, longitude seams, grazing normals, flat surfaces and resized output.

**Difficulty 6.7 / effectiveness 7.8.** Mostly a 2.5D appearance improvement. GMT's implementation already exists behind a gate: the older comment saying footprint information is missing is stale, not proof of a missing feature.

Sources: [environment sampling](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/lighting/env.ts), [footprint producers and gate](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/reflections/index.ts).

## 2. Four-fetch cubic reconstruction and explicit texture LOD

**Mechanism.** A separable cubic B-spline ordinarily combines 4 by 4 texels. GMT groups each adjacent pair of nonnegative weights into a sum and an interpolation position. Hardware bilinear interpolation performs each pair's inner interpolation, reducing the two-dimensional reconstruction to four texture fetches. The nonnegative B-spline weights smooth without negative-lobe ringing; this is not a sharpening Catmull–Rom filter.

Environment-map taps explicitly request base LOD 0. The grouped sample positions jump when the base texel index changes even though the final weighted reconstruction is continuous. Inferring mip levels from derivatives of those intermediate positions can therefore create discontinuous LOD choices. `envImageSample` blends base reconstruction into mip 1 for LOD below 1 and uses explicit mip sampling above that.

**2D adaptation.** Useful for imported texture appearance, magnified background images and post-process UV warps. Require linear-filterable formats and verify sampler wrap/clamp behavior. Do not smooth iteration counts before palette mapping indiscriminately: averaging different escape bands or interior/exterior classes changes their meaning. Reconstruct display colors or a specifically defined continuous appearance field instead.

**Difficulty 4.8 / effectiveness 6.8.** Verify continuity at texel boundaries, constant-color preservation, impulse response and HDR format fallbacks. Do not claim the four-fetch method improves every image: it deliberately softens.

Sources: [environment B-spline](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/lighting/env.ts), [post-process reconstruction](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/post_process.ts).

## 3. Solid-angle-aware environment averages

**Mechanism.** At very coarse equirectangular mips, an ordinary rectangular image average does not represent a spherical average: polar rows contain too much weight relative to their solid angle. The shader blends the last four mip levels toward `uEnvAvgColor`, documented as a sin(theta)-weighted average supplied on environment loading. If that value is unavailable, it falls back to restricting the roughness-driven mip range. This is a practical broad-lobe approximation, not full GGX preintegration.

`fogRadiance` deliberately samples below the average-blend window so directional sky variation remains visible. It then applies a luminance soft knee above 1, approaching 2, before mixing with a user tint. This prevents a bright HDR sun region from dominating an artistic fog contribution, at the cost of modifying radiometric energy.

**2D adaptation.** Useful for relief lighting with imported environments and for optional atmospheric appearance layers. Keep background visibility, reflected light intensity and artistic haze separately controllable. Test a constant environment, a polar bright patch and an equatorial patch of equal solid angle. Preserve HDR until the chosen display transform.

**Difficulty 5.8 / effectiveness 6.5.** Does not justify adding physical 3D fog to a planar Mandelbrot renderer.

Source: [environment and fog functions](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/lighting/env.ts).

## 4. Normal estimation: residual subtraction and meaningful step sizes

**Mechanism.** `GetFastNormal` uses four distance samples: the center and three axis offsets. Each component subtracts the center distance. Omitting that subtraction adds the same nonzero surface residual to every component, tilting the estimated normal. `GetNormal` instead uses four tetrahedral directions, accumulating direction times sampled distance and normalizing. Both guard very small vector magnitude.

For numerical DE formulas, GMT selects an escape-time-gradient normal instead of differentiating an already finite-differenced estimator again. Its material evaluation chooses epsilon from a floating-point scale floor and a visual footprint. The path-tracing build calls one normal-estimator function with a varying epsilon instead of carrying two expensive estimator bodies behind a runtime branch.

**2D adaptation.** For a relief height h, build a normal from defined height differences, for example normalize(-dh/dx,-dh/dy,1), with consistent height and pixel scales. Subtract the center for forward differences. Use masks around undefined/interior values; smoothing across those boundaries requires an explicit policy. Reuse existing RFF normals where suitable rather than recalculating fractal iterations per lighting tap. Any analytic derivative path needs its own precision assessment.

**Difficulty 5.6 / effectiveness 7.3.** Test a constant plane, known linear ramp, curved height field, interior boundary and resolution changes. A larger epsilon reduces noise by losing detail; it is not a free precision improvement.

Source: [normal and material evaluation](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/material_eval.ts).

## 5. Stable interaction noise versus progressive sampling noise

**Mechanism.** `getBlueNoise4` reads an RGBA noise texture and animates both its spatial offset and channel values using frame-dependent fractional offsets based on an R2 sequence. `getStableBlueNoise4` omits frame animation. The marcher uses a stable seed during navigation and animated seeds during accumulation; its step jitter lies in [1-jitter,1], favoring shorter steps rather than increasing overshoot.

This mechanism decorrelates sampling patterns. The source's descriptive claims about coverage do not establish that all channels and transformed samples are statistically independent.

**2D adaptation.** Reuse the stable-versus-progressive distinction for existing antialiasing, stochastic relief effects and export sampling. Key samples to absolute output-pixel coordinates, frame index, sample index and saved seed so tile scheduling does not alter the image. Never jitter the escape criterion or approximation acceptance rules to disguise numerical artifacts. Ray-step jitter itself has no direct role in RFF's complex iteration.

**Difficulty 5.5 / effectiveness 7.0.** Verify reproducibility, tile partition invariance and absence of navigation shimmer. Blue-noise assets and distinctive constants require provenance before adoption.

Sources: [noise helpers](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/blue_noise.ts), [step jitter use](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/trace.ts).

## 6. Progressive accumulation and channel-specific semantics

**Mechanism.** The main shader preserves history outside the scheduled region. Inside it, RGB is updated with `mix(history, sample, blendFactor)`. Arithmetic sample averaging requires the controller to provide the correct weight for each pixel's sample count; the shader expression alone does not establish equal sampling. This is why the main report separately probes pass-boundary band scheduling.

Depth is projected onto the clean, unjittered camera ray. Alpha output changes meaning with the export pass: projected depth for beauty/depth, or a per-sample binary coverage classification for the mask pass. The shown main shader mixes RGB history but writes alpha directly. Although the post-process comment describes accumulated coverage, that claim needs verification in the separate accumulation/export path; it cannot be established from this fragment alone.

**2D adaptation.** Define independent channels for continuous coloring values, coverage, normals and diagnostics, with an explicit reducer for each: average, minimum, last sample or another justified operation. Do not average categorical flags or reinterpret iteration count as physical depth. Reset affected histories after palette/material/camera changes according to the pipeline stage that owns each buffer.

**Difficulty 6.4 / effectiveness 8.0.** Verify sample-count invariance across band sizes, stale-history resets and mask edges. Distinguish finite-color handling from actual NaN detection: the shown `sanitizeColor` uses min/max clamping and should not be treated as a portable explicit nonfinite check.

Sources: [main output and accumulation](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/main.ts), [pass resolution](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/post_process.ts).

## 7. Shared materials and compile-time removal of expensive work

**Mechanism.** `getSurfaceMaterial` centralizes albedo, normal, emission and roughness for direct and path-traced lighting. Secondary color/noise layers are evaluated when needed by either visible blending or emission; opacity alone is not a sufficient dependency test. Noise-gradient taps are gated at their calculation site as well as their application site.

GMT also emits compile gates such as `USE_TEXTURE`. A disabled runtime branch can still burden a shader compiler or be predicated; excluding unused GLSL can reduce compiled work and source expansion. The trace generator contains a useful counterexample: replacing its inner `map()` with `mapDist()` was reportedly reverted because the compiler already removed unused outputs and the replacement introduced an extra evaluation. This is a documented historical result, not a universal optimization rule.

**2D adaptation.** Build a dependency graph from requested appearance outputs to source fields, and use bounded shader variants for genuinely expensive optional paths. Keep cheap continuously animated controls as uniforms where practical. Include all layout-affecting choices in pipeline/cache keys and ensure Vulkan descriptor layouts and push constants match the selected source. Compare compiled code and measured GPU time; fewer source statements need not mean faster rendering.

**Difficulty 6.8 / effectiveness 7.8.** Test disabled-feature equivalence, emission-only layers, cache invalidation and variant growth. Existing RFF material and shader infrastructure remains the starting point.

Sources: [material evaluation](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/material_eval.ts), [trace generation](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/trace.ts).

## 8. Color pipeline, data-pass bypass and final-output dithering

**Mechanism.** GMT's post pass applies UV effects, samples the render image, handles depth/mask outputs before beauty color processing, applies injected color effects, tone maps, optionally encodes linear RGB to sRGB, then adds static output noise. The dither is `(n1+n2-1)/255` from two shader hashes: nominal triangular noise at roughly one 8-bit step. It is applied after encoding, so it targets display quantization rather than lighting values. The hash inputs use fragment coordinates; deterministic within a fixed path does not imply cross-GPU bitwise reproducibility.

The ACES-named branch is a compact filmic fit and the AgX-named branch is a matrix/log/polynomial approximation. Their names must not imply full reference color-management pipelines. The None branch still clamps to [0,1], so it is not an unrestricted HDR-output mode.

**2D adaptation.** Preserve one explicit linear-light-to-output contract around existing RFF palette and lighting code. Keep numeric exports outside tone mapping and dither. For tiled exports, use global pixel coordinates for noise; provide a reproducibility/dither control and use an output-bit-depth-aware amplitude. Do not copy hash constants or tone-mapping coefficient sets without resolving attribution in NOTICE.

**Difficulty 5.2 / effectiveness 7.4.** Test dark ramps, saturated highlights, double encoding, numerical-channel round trips and seams. Dither reduces visible quantization structure; it does not recover lost precision.

Source: [post-process ordering and functions](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/post_process.ts).

## 9. UV-warp smoothing and downsampled previews

**Mechanism.** UV features can request a continuous smoothing amount. GMT blends the ordinary lookup toward cubic reconstruction and then toward a 3 by 3 [1,2,1]-weighted filter with continuously expanding tap spacing. This avoids snapping a sampling grid as the slider changes. For large render targets displayed on a smaller canvas, a separate box filter uses up to 8 by 8 taps.

**2D adaptation.** Useful for Droste/log-polar previews and high-resolution export previews. Keep display-preview filtering separate from saved full-resolution output. For extreme reduction, a bounded 8 by 8 neighborhood is not a general area filter; use a mip chain or staged reduction with a known footprint. A warped bitmap cannot reveal new deep fractal detail. Tile halos must include the complete filter support and warp mapping.

**Difficulty 5.1 / effectiveness 6.9.** Verify animated filter-radius continuity, severe reduction, source wrap modes and tiled/untiled equivalence.

Source: [post-process sampling branches](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/post_process.ts).

## 10. Raymarch robustness: instructive, but not a 2D perturbation shortcut

**Mechanism.** GMT derives a hit threshold from a pixel footprint and a floating-point floor; perspective footprints grow with ray distance while orthographic footprints remain constant. The marcher clamps suspicious DE growth against the previous distance plus step, damps steps when distance collapses, subtracts a threshold-based safety amount, and optionally refines a bracket by bisection. Candidate recovery remembers the best distance/threshold ratio and its auxiliary fields.

These are policies around an approximate 3D distance estimator. Bisection refines an encountered threshold crossing; it does not prove that no earlier thin surface was skipped. Candidate recovery intentionally accepts a near miss under a tolerance. Refinement retains earlier auxiliary color fields, which is a deliberate approximation and deserves discontinuity testing.

**2D adaptation.** Transfer the engineering pattern—explicit error units, cached auxiliary values and a recoverable failure path—to diagnostics. Do not transfer the heuristic acceptance rules into Mandelbrot membership, deep coordinates or MPA. A future relief raymarcher would be a separate appearance feature with its own error budget.

**Difficulty 8.5 / effectiveness 3.5** for a new RFF relief-raymarch implementation; low direct value to its core iteration. Test thin geometry and discontinuous fields if pursued.

Source: [trace kernel generation](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/trace.ts).

## 11. Importance sampling, MIS and firefly control

**Mechanism.** GMT shares a visible-normal GGX sampler between reflection and path-tracing code. It constructs a local tangent frame, stretches the view by roughness squared, samples a bounded spherical cap and transforms the resulting microfacet normal back to world space. Path tracing also samples lights/environment directions, combines estimators with squared-PDF MIS weights, and applies Russian roulette with survivor compensation. Environment importance sampling uses a luminance CDF and a solid-angle conversion. A dedicated environment lookup uses the CDF's image resolution to reduce extreme mismatch between sampling probabilities and radiance.

Large radiance contributions receive a luminance soft knee after multiplication by throughput. This suppresses fireflies by introducing bias; it is not an unbiased replacement for better sampling. Correctness depends on the sampler, its PDF, lobe-selection probability and throughput weight describing the same distribution.

**Specific verification issue.** `vndf.ts` describes and implements a bounded-cap sampler, while `pdfVNDF` and nearby throughput comments in `pathtracer.ts` describe a conventional visible-normal density and use an approximate masking term. No bounded-cap normalization correction is evident in the inspected PDF expression. This is a concrete consistency question requiring derivation and numerical integration before reuse, not a proven rendered defect from this review. Do not copy the sampler and assume the old PDF remains correct.

**2D adaptation.** Potentially useful for stochastic environment lighting on relief surfaces, but full multi-bounce path tracing is a large, low-priority extension. Prefer improving existing deterministic material evaluation first. Validate a constant-environment energy test, roughness/grazing sweeps, PDF normalization, sampler histograms and long-run agreement with a trusted numerical reference.

**Difficulty 8.7 / effectiveness 5.6** for a limited relief-lighting sampler; a full 3D path tracer retains the main report's deferred rating.

Sources: [bounded sampler](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/vndf.ts), [PDFs, MIS and integrator](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/pathtracer.ts), [CDF-resolution radiance lookup](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/lighting/env.ts).

## 12. Implementation order and evidence limits

For RFF, start with field/channel semantics, normal-scale consistency and color/output contracts; then prototype reflection footprint filtering and image reconstruction. These improve existing 2D/2.5D paths without changing fractal arithmetic. Treat stochastic illumination and a new relief marcher as separate research scopes.

Before adopting distinctive source, resolve the original author/license and update NOTICE as required by the repository. Names cited in GMT comments identify leads, not completed license clearance. This appendix describes inspected implementations and proposed tests; none of its GPU acceptance checks has been run here.
