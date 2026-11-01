<!-- Modified by GPT-6 on 2026-10-07, 2026-10-08. -->
<!-- Modified by Opus 5.5 on 2026-11-01. -->

# GMT-fractals: file-by-file deep code audit — evidence record

> **Final review entry point (2026-10-08):** [Improving RFF_Super with GMT as a reference](GMT_to_RFF_Review_EN.md). All 3,727 inventoried GMT files have completed inspection records and unchanged hashes. This companion preserves chronological evidence: earlier progress counts, pending review statements and provisional priorities describe their recording time, not current coverage. Later corrections and the final entry point govern recommendations. Source inspection does not establish runtime correctness or measured GPU benefit.

Date: 2026-10-07. Source root: [gamazama/GMT-fractals at ad28b8b](https://github.com/gamazama/GMT-fractals/tree/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1).

## Scope and completion contract

The expanded request is to inspect **all code in detail**, including documents, and identify additions/improvements with explicit 3D-to-2D transfer analysis. The previous [feature review](GMT_Fractals_Feature_Review_EN.md), [registration inventory](GMT_Fractals_Source_Inventory_EN.md) and [shader mechanism appendix](GMT_Fractals_Shader_Mechanisms_EN.md) are useful selective reviews, but do not establish completion of this broader task. Their earlier completion language refers only to that earlier scope.

The initial full filesystem inventory contains **3,727 files**: 2,293 code files (595,578 physical lines), 656 Markdown documents (112,429 lines), 151 text data/configuration files and 627 binary assets. These counts include bundled reference shaders, test/debug programs and generated material; they are inventory counts, not claims of original authorship or live application use. No subtree is silently excluded from the inventory.

[The coverage ledger](gmt-audit/coverage.json) records a SHA-256 hash for every file. `pending` means not yet read in full in this expanded audit. `partial` identifies a bounded inspection. `read-in-full` means full text examined, **not** runtime correctness, all caller contracts validated or mathematical proof. Notes state what was actually examined. Prior searches and registration extraction do not automatically grant full-read status. A changed hash resets coverage on regeneration. Binary assets require role/provenance inventory rather than pretending they are code.

The source tree remains unmodified. The report and audit records live in the RFF workspace. All results below are source-derived unless a specific probe is identified. The dependency-free probes use CPU arithmetic with guards against the actual inspected source expressions; they are not GLSL execution, GPU measurements or screenshots. Numerical ratings use decimal 1–10 scores; difficulty increases with cost. GMT fixes and RFF adoption opportunities have different beneficiaries and are scored separately.

## Batch 1: shader arithmetic and cross-path contracts

### S01 — Linear light attenuation is lost in volumetric scattering

**Evidence: confirmed producer/consumer mismatch; CPU arithmetic reproduced.** `UniformManager.syncFrame` packs the quadratic coefficient in `uLightFalloff` and the linear coefficient in `uLightFalloffType`. For a linear light it writes `(0,k)`. `calculatePBRContribution` uses both coefficients in `1/(1+q*d*d+l*d)`. The volumetric body instead gates attenuation on `uLightFalloff > 0.001` and treats `uLightFalloffType` as a selector. Therefore a linear light bypasses attenuation entirely in the volume path.

For k=1 and distance 10, the surface coefficient is **0.090909**, while volume uses **1.0**: an elevenfold difference in this factor. This is not a claim of an elevenfold difference in final displayed brightness; phase, shadowing, density and tone mapping also contribute.

**Improvement.** Centralize the polynomial attenuation contract and use it in surface, path-tracing and volumetric paths. Test linear/quadratic modes, range-derived coefficients, zero falloff and directional lights. If preserving old appearance is required, provide an explicit migration/version choice rather than perpetuating an ambiguous uniform name.

**GMT fix:** difficulty **3.5**, effectiveness **7.7**. **2D transfer:** useful to RFF's existing relief lights if introducing additional haze/scatter paths; share attenuation semantics instead of copying a mismatched consumer. Adoption difficulty **4.2**, effectiveness **5.8**.

Sources: [UniformManager.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/engine/managers/UniformManager.ts) (`falArr[i] = 0`, `typArr[i] = k`), [pbr.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/lighting/pbr.ts) (`calculatePBRContribution`), [volumetric_scatter.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/lighting/volumetric_scatter.ts) (`_att`).

### S02 — The volumetric sum lacks a physical segment-length weight

**Evidence: source-level dimensional/integration issue.** Both density and emissive scattering add a contribution per accepted primary-march sample, multiplied by `_seg = 1/_gateP`. That factor compensates a sampling gate; it is not ray-segment length. The actual adaptive march advance is computed later in `traceScene`. The returned `accScatter` becomes `fogScatter`, and the feature's post injection adds it directly to color. No later step-length normalization appears along this traced chain.

Consequently, the expression is a weighted sum over march visits, rather than a spatial integral: subdividing a region into more visits increases its contribution even if the physical medium is unchanged. Furthermore, height-varying density uses `exp(-sigma_at_sample*d)` rather than an integrated optical depth, which is only the constant-density form. The feature describes a participating medium and single scattering, so these approximations should be explicit.

**Improvement.** Integrate over actual bounded ray segments, with transmittance accumulated consistently along the path, or use a separate volume pass with a defined sampling measure. Clip the final segment at the surface. Do not simply multiply by a stale DE value and assume it is the accepted damped/jittered step. Validate a homogeneous slab against its analytic integral, then vary step settings without changing geometry. Separately validate a nonuniform density profile.

**GMT fix:** difficulty **7.5**, effectiveness **7.2**. **2D transfer:** do not copy this volume accumulator into a screen-space haze effect. For artistic 2D haze, define a mask-to-opacity mapping explicitly; a physically integrated volume requires a separate 2.5D/3D model. Mask-based alternative difficulty **5.7**, effectiveness **5.2** (overlaps the original A18 scope).

Sources: [volumetric_scatter.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/lighting/volumetric_scatter.ts) (`_seg`, `_trans`, `accScatter`), [trace.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/trace.ts) (`mb3dStepW`, `fogScatter = accScatter`), [volumetric feature](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/volumetric/index.ts) (`VOLUMETRIC_SCATTER_POST`), [post.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/post.ts) (`applyPostProcessing`).

### S03 — The stochastic volume gate does not justify the stated unbiasedness

**Evidence: mathematical counterexample, independently of the missing segment weight.** At full jitter the gate is `fract(seed*7.43+d) < gateP`. Even if seed is uniform on [0,1), multiplying by the noninteger 7.43 and taking the fractional part does not preserve a uniform distribution. The first 0.43 of the period has eight preimages; the remainder has seven. Dividing accepted contributions by gateP therefore does not make their expected value one at every d.

A deterministic one-million-point uniform-seed quadrature with gateP=1/128 gives a compensated unit contribution of **1.076224 at d=0** and **0.942080 at d=0.5**, instead of 1. At zero jitter the gate is deterministic and the source intentionally exposes slicing, so comments promising the same converged image also need that qualification. This probe does not assert the actual blue-noise texture has exactly uniform samples; it shows that even ideal uniform input is insufficient.

**Improvement.** Use a deliberately specified random dimension with a uniform marginal, keep its mapping independent of geometry-driven sample-count changes where necessary, and test the estimator's expectation over seed/depth/quality. Preserve a separately named artistic slicing mode if desired.

**GMT fix:** difficulty **5.0**, effectiveness **6.6**. **2D transfer:** valuable validation practice for probabilistic sampling, particles and adaptive effects; do not equate an arbitrary `fract` expression with an unbiased random variable. A reusable tested sampling contract: difficulty **5.5**, effectiveness **7.0**.

Source: [volumetric_scatter.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/lighting/volumetric_scatter.ts) (`_volSeed`, `_gateP`).

### S04 — Negative palette coordinates differ between glow and surface

**Evidence: confirmed expression mismatch; CPU arithmetic reproduced.** Surface evaluation uses `fract(mod(t1Raw,1))` before bias. `getGlowColor` uses `fract(t1Raw)`, then replaces it with `1-fract(t1Raw)` whenever the raw value is negative. With raw coordinate -0.2 and bias 1, the surface samples **0.8** and glow samples **0.2**. A non-symmetric gradient therefore yields different colors even when the same mapping, scale, offset and twist are intended.

**Improvement.** Define a shared wrap/mirror policy and use it for surface, glow and volume palette lookup. If the reversal is intentional, expose it as a glow mapping mode. Test coordinates just below/above zero and negative integer boundaries, nonunit bias and animated negative offsets.

**GMT fix:** difficulty **3.0**, effectiveness **6.5**. **2D transfer:** directly relevant to palettes, glow and orbit-based appearance. A shared coordinate-transform contract prevents effects drifting from the base image. Difficulty **3.5**, effectiveness **6.7**, conditional on an analogous gap in RFF rather than a confirmed existing RFF bug.

Sources: [coloring.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/coloring.ts) (`getGlowColor`), [material_eval.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/material_eval.ts) (`t1Raw`, `t1`).

### S05 — The GGX denominator floor strongly changes near-mirror peaks

**Evidence: numerical contradiction of the source comment.** `GGX_EPSILON=1e-7` is added to `pi*denom*denom` in the GGX distribution. At NdotH=1 and roughness 0.05, alpha squared is roughness^4 and the ideal distribution is approximately **50,929.58**. The implemented denominator produces **62.42**, an approximately **815.87-fold suppression** of this NDF peak. The comment stating that this epsilon only acts at literal singularities and leaves the peak intact is false for this input.

This is a distribution-term comparison, not an image-wide energy or brightness measurement. Finite pixels, filtering and other BRDF terms affect observed highlights. A practical roughness floor may be appropriate, but should have a controlled, documented meaning.

**Improvement.** Separate singularity handling from intentional antialiasing/roughness regularization. Define a minimum roughness or footprint-aware lobe broadening and validate integrated energy and temporal stability. Avoid replacing the constant with an arbitrarily smaller one without testing floating-point limits and fireflies.

**GMT fix:** difficulty **5.5**, effectiveness **7.0**. **2D transfer:** highly relevant to glossy relief shading; inspect the corresponding RFF formulas before declaring a matching issue. Validation/controlled regularization difficulty **5.8**, effectiveness **7.2**.

Sources: [math.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/math.ts) (`GGX_EPSILON`), [pbr.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/lighting/pbr.ts) (`ggxA2`, `float D`).

### S06 — Bounded VNDF sampler and reported PDF disagree in a simple limiting case

**Evidence: analytical counterexample plus CPU evaluation; strengthens the earlier appendix's open question.** For view direction equal to the normal and roughness=1, the bounded-cap sampler has alpha=1, k=0 and cap z in [0,1]. Its generated half-vector is normalize(c+view). Reflecting the incident direction around this half-vector produces c, uniformly distributed on the upper hemisphere. The resulting direction density is therefore **1/(2*pi)**.

In the inspected `pdfVNDF`, this same configuration has D approximately 1/pi and G1V=1, giving **1/(4*pi)** (apart from the tiny epsilon). The reported density is approximately half the actual sampler density. Thus the mismatch is more than different comments or names. This endpoint calculation does not by itself quantify the final rendered bias across all materials, and full sampler/throughput/MIS validation remains outstanding.

**Improvement.** Derive the PDF and throughput weights for the actual bounded distribution and masking model, then update sampling, PDF evaluation and both sides of MIS together. Check normalization, sampled histograms, the roughness=1 normal-view limit and integrated radiance under a constant environment. Replacing only the PDF without inspecting throughput compensation is insufficient.

**GMT fix:** difficulty **7.8**, effectiveness **8.0** for path-tracing correctness. **2D transfer:** relevant only when adopting stochastic relief illumination; do not copy this sampler/PDF combination unchanged. Limited adoption retains difficulty **8.7**, effectiveness **5.6**.

Sources: [vndf.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/vndf.ts) (`getVNDFSamplerGLSL`, `k`, `bnd`), [pathtracer.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/pathtracer.ts) (`pdfVNDF`, `pdfBSDF`, `BOUNCE DIRECTION SELECTION`).

### S07 — Visible light-sphere selection is order-dependent

**Evidence: local control-flow issue; integration/visual reproduction pending.** `intersectLightSphere` iterates the light array and returns immediately for the first qualifying sphere/halo. It does not compare entry distances or accumulate multiple overlapping contributions. `compositeLightSpheres` depth-tests only that returned light against the fractal. A farther light placed first can therefore mask selection of a nearer light; if the farther selection fails the later geometry depth test, a nearer visible candidate is never examined by this function.

**Improvement.** Separate closest opaque-emitter intersection from translucent halo composition. Test reversing light-array order, overlapping halos and a far emitter behind geometry with a nearer emitter in front. Define whether translucent volumes should accumulate or intentionally follow layer order.

**GMT fix:** difficulty **5.0**, effectiveness **6.4**. **2D transfer:** relevant to future light sprites/overlays, not to the core fractal calculation. Adoption difficulty **4.5**, effectiveness **4.2**.

Source: [shared.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/lighting/shared.ts) (`intersectLightSphere`, `compositeLightSpheres`).

### S08 — Four-dimensional pre-bailout differs between geometry and color kernels

**Evidence: actual generator execution and an isolated arithmetic fixture.** With `is4D: true`, `DE_MASTER` emits a four-dimensional radius and post-iteration bailout. However, `map` still tests the three-dimensional `r2_check` before the formula, while `mapDist` tests `dot(z,z)` at that point. Their documented geometry equivalence therefore does not hold for all generated inputs.

For initial xyz=0, w=20, squared-radius bailout=100, eight iterations and a synthetic formula `z *= 0.5`, the full kernel executes eight iterations and finishes at radius **0.078125**. The geometry kernel exits before any iteration and retains radius **20**. The fixture's `getDist` returns radius, making the mismatch observable without an unrelated distance formula. This tests the generator contract; it is not a claim that a shipped preset reproduces this exact image. Native/imported formula reachability and `SKIP_PRE_BAILOUT` conditions still require tracing.

The numerical helper functions also hard-code xyz-only bailout/radius expressions despite the enclosing `is4D` option. Separately, numeric `map` uses `uNumDESmooth` while `mapDist` uses fixed 2.5; the refinement algorithm therefore needs validation when the user changes that setting. Neither issue should be hidden behind a blanket assertion that the two kernels always evaluate the same field.

**Improvement.** Share the dimensional bailout expression across full, geometry-only and numeric paths. State which differences in the secondary estimator are intentional, and use the primary field for root refinement if the algorithm assumes the primary crossing. Add generated-source checks plus numerical parity fixtures covering initial escape, w-only escape and nondefault numerical smoothing.

**GMT fix:** difficulty **6.5**, effectiveness **7.5**. **2D transfer:** formula compilation should preserve all iteration-affecting state across rendering, derivative, normal and export paths. Useful to a future restricted formula system, but not a reason to change RFF's established Mandelbrot recurrence. Difficulty **6.0**, effectiveness **6.8** for parity infrastructure, overlapping the broader formula proposal.

Source: [de.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/de.ts) (`DE_MASTER`, `rExpr`, `bail4`, `r2_check`, `centerCount`, `iterateLogRadius`, `mapDistGeomResult`).

### Additional implementation observations from this batch

- **Shadow scale normalization:** `GetSoftShadow` expresses start offsets, thresholds and floors in `surfEps` units, with signed depth growth along the shadow ray. This is a useful 2.5D design principle: tie bias to meaningful sampling scale instead of fixed world-unit constants. Its Lite path deliberately uses a much larger step floor than HQ, so quality/geometry tradeoffs remain. For RFF relief shadow scale consistency: difficulty **6.0**, effectiveness **6.8**; no existing RFF regression is claimed.
- **Directional shadow range:** `DIR_LIGHT_DIST=100` is commented as larger than `BOUNDING_RADIUS`, which is 400 in the same generated source. The comment is demonstrably wrong; whether distant blockers actually matter depends on scene bounds. A bound-derived exit distance is preferable to relying on this statement. GMT verification/fix: difficulty **4.5**, effectiveness **5.5**.
- **Texture color decoding:** `applyTextureProfile` labels `pow(color,2.2)` as sRGB-to-linear, whereas the display encoder uses the piecewise sRGB curve. These are not inverses, especially in shadows. Before replacing it, inspect texture-upload color-space handling and all callers to avoid double decoding. This is an identified approximation, not yet a proven complete-pipeline color bug. RFF transfer: make decode/encode ownership explicit, difficulty **5.2**, effectiveness **7.4** (overlaps the earlier color-pipeline scope).
- **Polygonal lens sampling:** the ray generator uses a circular sqrt-radius sample and multiplies radius by an angle-dependent hexagon boundary. That produces a polygonal footprint, but uniform angle with that radial scaling is not uniform-area sampling of the polygon. Treat it as an artistic aperture distribution unless corrected and normalized. For 2D, useful only for a deliberately synthetic depth-of-field effect; difficulty **5.8**, effectiveness **5.4**.
- **Mesh helper semantics:** `MESH_GLSL_HELPERS` fixes `getLength` to Euclidean and supplies no-op rotations, while the live math chunk supports metric selection and rotation matrices. This is a contract to trace through mesh assembly before claiming an export discrepancy; callers may transform the sample domain or reject unsupported combinations.

## Batch 2: complete path-tracer read and sampling/export follow-through

### S09 — Bounce selection, diffuse sampling and roulette distort their input distributions

**Evidence: source arithmetic with one million uniformly spaced input samples.** The integrator sets `randType=fract(blueNoise.a*1.618)` but compensates the selected lobe using `probSpec` or `1-probSpec`. At probSpec=0.5, the transformed uniform input selects specular with probability **0.618046**, not 0.5. This is the same noninteger-multiplier issue as S03, in a different and more central estimator.

`cosineSampleHemisphere` similarly uses `fract(seedVec.x*phi)` for squared disk radius. Its mean is **0.427051** under ideal uniform input instead of 0.5, so it is not the claimed cosine-weighted sampling law. Roulette uses `fract(blueNoise.r*1.618+0.7)`; with q=0.5, the measured ideal-input survival is **0.505563** rather than 0.5. The problem varies with q, and marginal correctness alone would not prove independence from the already sampled path state.

**Improvement.** Preserve uniform marginals with a justified rotation/scramble, allocate distinct sample dimensions, and derive weights from the actual distributions. Test lobe frequencies, radial CDFs and roulette expectation across thresholds. Do not label decimal constants such as 17.123 and 23.456 “mutually irrational”: those finite decimals are rational, and that comment is not evidence of decorrelation.

**GMT fix:** difficulty **6.8**, effectiveness **8.2**. **2D transfer:** directly useful when adding stochastic antialiasing, relief illumination or particle generation; use a tested sampling interface. Difficulty **5.5**, effectiveness **7.0**. The numeric probes falsify the ideal-input transforms, not measure the installed blue-noise texture's full joint distribution.

Source: [pathtracer.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/pathtracer.ts) (`cosineSampleHemisphere`, `randType`, `rrRand`, `probSpec`).

### S10 — Environment CDF selection is discrete, while MIS treats it as a continuous density

**Evidence: complete sampling/PDF source read.** `sampleEnvImportance` chooses a row and column by inverse CDF, then always constructs direction from `(i+0.5)/W,(j+0.5)/H`. There is no residual-CDF remapping or within-cell jitter. Consequently, the sampler has at most W*H output directions. Its reported PDF divides cell probability by approximate cell solid angle, and `pdfEnvImportance` assigns that density throughout the cell for arbitrary BSDF directions. Those are different measures: a discrete center-direction sampler is not the continuous distribution that the MIS formulas assume.

**Improvement.** Recover the residual position inside each CDF interval and sample continuously within the selected cell, using the appropriate Jacobian or exact solid-angle construction. Alternatively, explicitly define a quadrature approximation and do not present it as ordinary continuous MIS. Test normalization, within-cell coverage, moving narrow shadows and rotated environment maps. Also preserve zero-probability semantics: returning a uniform-sphere PDF for a zero-mass cell does not make the actual sampler uniform there.

**GMT fix:** difficulty **7.0**, effectiveness **7.8**. **2D transfer:** relevant to importance-sampled relief lighting and image-guided sampling. A 2D equivalent is choosing an image cell and falsely treating its center as a uniform sample within the cell. Adoption difficulty **6.5**, effectiveness **6.2**.

Source: [pathtracer.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/pathtracer.ts) (`sampleEnvImportance`, `pdfEnvImportance`, `binarySearchCDFRow`).

### S11 — Environment reduction can miss bright features and does not match the sampled mip

**Evidence: actual CPU CDF builder executed with a texture-storage stub.** `buildEnvCDF` samples one bilinear source position per destination cell. This is point reconstruction, not area reduction. An 8x4 source with a single RGB=100 texel at (0,0), reduced to 2x2 by the actual function, produces `avgColor=[0,0,0]` although the image has nonzero energy. The same mechanism can miss small bright environment features at larger resolutions.

The shader samples NEE radiance using `uEnvCDFMipBias`, whereas the CDF was built from those single base-image lookups. The comment that these are matching cell-average quantities is therefore not generally true. Furthermore, NEE uses mip-filtered radiance while the BSDF branch uses full-resolution radiance. Standard complementary MIS weights only reconstruct a common integrand; changing the radiance between estimators introduces another approximation even after their PDFs are repaired.

The all-black fallback comment is also inconsistent with the implementation: the builder returns `lumIntegral=1.0` and full-size uniform tables, and `MaterialController.rebuildEnvCDF` uploads them without switching to the 1x1 stub. This is a contract/documentation mismatch, not evidence that a black environment alone produces visible energy.

**Improvement.** Build the importance map and solid-angle average with a genuine reduction over source energy. Keep a nonzero sampling support policy where needed, and choose a consistent radiance integrand for both MIS branches. Evaluate the intended bias/variance tradeoff explicitly if filtered NEE remains. Test single-pixel emitters, subcell translations, all-black maps and saturated HDR values.

**GMT fix:** difficulty **6.7**, effectiveness **7.7**. **2D transfer:** strong relevance to image-to-palette extraction, thumbnail analysis and texture prefiltering: isolated important colors can disappear under sparse sampling. For robust image reduction, difficulty **5.3**, effectiveness **7.5**.

Sources: [env_cdf.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/reflections/env_cdf.ts) (`sampleLuminance`, `sampleRGB`, `buildEnvCDF`), [MaterialController.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/engine/MaterialController.ts) (`rebuildEnvCDF`), [pathtracer.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/pathtracer.ts) (`sampleEnvAtCDFMip`, `sampleMiss`).

### S12 — Path-tracing seeds use tile-local fragment coordinates

**Evidence: cross-path source mismatch; tiled GPU comparison pending.** The primary ray generator explicitly remaps image-tile UVs and uses full-output pixel coordinates for blue noise. The path tracer's bounce, area-light and environment seeds instead call noise/Sobol helpers with `gl_FragCoord.xy`. These coordinates refer to the current render surface. In a tile-sized render target, the same local pixel position in different image tiles therefore reuses the same spatial seed rather than the corresponding full-image seed.

This does not prove every tile seam is visible; it establishes that the current seed contract is not invariant to image-tile partitioning. The final dither also uses local fragment coordinates. Distinguish image tiles, which change render surfaces, from scissored GPU buckets in a single full-sized target.

**Improvement.** Pass one canonical absolute output-pixel coordinate to every stochastic stage and final dither. Retain sample index and purpose as separate dimensions. Compare a fixed-seed render with multiple image-tile sizes and the same per-pixel sample count.

**GMT fix:** difficulty **5.2**, effectiveness **7.0**. **2D transfer:** directly useful for deterministic still/video exports and future memory-bounded tiles. Difficulty **5.5**, effectiveness **7.3**.

Sources: [ray.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/ray.ts) (`uvFull`, `noisePixel`), [pathtracer.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/pathtracer.ts) (`gl_FragCoord`, `sobol2CPSeeded`), [post_process.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/post_process.ts) (final dither).

### S13 — Mask alpha is overwritten instead of temporally averaged

**Evidence: traced shader and ping-pong render step; GPU export reproduction pending.** The main shader averages RGB through `mix(safeHistory,col,uBlendFactor)` but writes the new `alphaOut` directly. `RenderPipeline.render` supplies the history texture and blend factor, renders once and swaps targets; the inspected bucket composite is a copy. The post-process mask branch assumes `tex.a` already contains fractional temporal coverage, but this recurrence does not produce it. For four coverage samples [1,0,1,0], stored alpha is 0 rather than 0.5. Spatial filtering may still soften an edge, but is not that promised temporal average.

There is a second classification contract to verify: mask coverage is inferred from projected depth rather than carried from the actual `hit` flag. `renderPixel` only replaces a missed distance with `MISS_DIST` when d<0.001. A budget-exhausted miss with a finite nearer d can therefore satisfy the depth-based coverage test. The final export/session lifecycle still needs examination before a complete user-flow reproduction is claimed.

**Improvement.** Carry a hit/coverage signal explicitly and average it only in coverage mode. Keep the depth channel's last-sample or other chosen policy separate, and reset history when channel meaning changes. Test alternating hit/miss samples, exhausted marches, light overlays and depth-of-field edges at several sample counts.

**GMT fix:** difficulty **5.5**, effectiveness **7.6**. **2D transfer:** directly relevant to alpha export, interior/exterior masks and compositing. Difficulty **5.4**, effectiveness **7.8**, overlapping the render-channel proposal rather than a separate full subsystem.

Sources: [main.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/main.ts) (`alphaOut`, `finalCol`, `renderPixel`), [RenderPipeline.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine/RenderPipeline.ts) (`render`), [BucketRunner.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine/export/BucketRunner.ts) (`compositeMaterial`), [post_process.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/post_process.ts) (alpha mask branch).

### Integrator questions retained for targeted follow-up

- Sphere NEE samples the whole emitter surface and uses an absolute light-normal cosine. Its shadow march tests fractal distance only; emitter self-occlusion, far-side samples, one/two-sided emission and selection PDFs need a coordinated derivation. Do not claim physical area-light correctness solely from the area-to-solid-angle conversion.
- `envVisibility` returns visible on step exhaustion and has a hard cap of 256, while the hard-shadow generator has 512. Those are deliberate approximate policies with possible light-leak tradeoffs; expose or diagnose exhaustion rather than equating it with reaching the sky.
- Mobile iteration stops at two bounces, while the last-bounce optimization compares against `maxBounces`; this can leave unused final tracing work when a larger user bounce count is configured. Confirm mobile configuration clamping before calling it a runtime regression.
- Primary Direct shading calls shared material evaluation with `highQuality=false`, which disables both noise bump and the second-layer bump. PT primary calls it with true. Confirm UI conditions and intended mode support before treating controls as broken.
- The specular bounce uses view-normal Fresnel in throughput, while direct/environment NEE evaluates half-vector Fresnel. Along with S06/S09 and the below-horizon fallback, this means the entire sampler/BRDF/weight combination needs consistency tests, not a single local PDF edit.

## Shader-priority follow-up: marching and environment filtering

The user prioritizes shader investigation. Formula-family expansion is deferred while generated shader contracts, producers, consumers and relevant design records are reconciled. The following findings concern implementation behavior, not reproduced image defects.

### S14 — Recovered hits retain volume accumulated beyond the recovered surface

`traceScene` stores a candidate depth and material tuple when a near miss has the best normalized distance. On recovery it rewinds `d` to `candidateD`, but returns the current `accScatter`, `accDensity` and `accColor`. These accumulators can include the later trajectory behind that candidate. Calling the injected finalizer at the candidate does not by itself restore the earlier accumulator state. This is explicitly acknowledged as an artistic consistency choice in the source, so treat it as a documented approximation to improve, not an undiscovered unconditional crash.

**Mechanism and improvement:** snapshot matching accumulator prefixes when updating a candidate, or resolve volume in a separate integration pass bounded by the final accepted depth. Snapshotting increases live registers; a second pass costs bandwidth and samples. Measure both, especially because the existing volumetric source reports substantial register pressure. The source's historical timing numbers are not measurements reproduced by this audit.

**GMT difficulty 5.8 / effectiveness 7.0. 2D adoption difficulty 5.7 / effectiveness 5.9:** useful for optional relief-depth haze, with integration bounds derived from the final height/depth field; ordinary Mandelbrot coloring needs no volume march.

### S15 — Refined geometry and color metadata refer to different positions

The refinement gate bisects the ray parameter with `mapDist`, assigns `d=dIn`, and retains `h.yzw` from the original coarse hit. The source asserts that the nudge is sub-pixel, but the implementation does not test that bound or bound variation of the trap/color function. A small displacement can still cross a sharp orbit-color transition. On a first-sample hit, `dPrev==d`, so the bracket has no width; it is not an independently established outside/inside interval in that case.

**Mechanism and improvement:** offer one full material evaluation at the final point for color-sensitive modes, or retain the cheaper approximation with a precise quality contract. Independently confirm that `map` and `mapDist` evaluate the same geometric field for every enabled DE mode; S08 already identifies counterexamples to the ADR's broad equivalence claim. More bisection iterations cannot repair a mismatched field. ADR-0084's update also withdraws the original claim that refinement cures the cited fragmented hybrid: it distinguishes an estimator-fidelity problem from march overshoot.

**GMT difficulty 5.0 / effectiveness 6.7. 2D adoption difficulty 4.5 / effectiveness 7.2:** compute antialiasing coverage, distance shading and orbit color from consistent samples. Do not assume a refined boundary allows arbitrary reuse of coarse color data. This complements existing render-channel proposals rather than requiring ray tracing in 2D.

### S16 — March history records the unjittered step

The convergence controller records `mb3dRLastStep=mb3dStepW`, then advances by `mb3dStepW*stepJitter`. On the next iteration the stored value feeds both the distance-growth clamp and the collapse ratio. Thus the variable described as the previous step width is larger than actual travel when jitter is below one. For a nominal step of 1 and jitter 0.85 it records 1 instead of 0.85, an overstatement of about 17.65%. This demonstrates a bookkeeping difference, not a measured 17.65% image error or a proof that every surface will be missed.

**Improvement:** decide explicitly whether the controller needs nominal or actual travel. If actual, store the product used to advance, then compare convergence and silhouettes at zero/default/high jitter with equal budgets. The nominal-step heuristic may be intentional, but its comments and derivation should then say so. **GMT difficulty 3.8 / effectiveness 6.2. 2D transfer:** applies only to adaptive field/relief traversal; do not transfer march-step damping into precision-sensitive perturbation equations.

### Environment filter mechanism worth transferring

`lighting/env.ts` reconstructs a separable cubic B-spline from four bilinear taps. Each tap explicitly requests LOD 0 because its constructed coordinates jump at texel boundaries; implicit derivatives of those coordinates would select unstable mip levels even though the weighted filter result remains continuous. Environment mip selection takes the maximum of a roughness term and an angular-footprint term, and blends the last mips toward a solid-angle-weighted average. This is a practical approximation, not a full BRDF convolution. Its correctness also depends on CDF/average construction (S11), texture color interpretation and caller cone state.

**2D opportunity:** use explicit, meaningful footprints for magnified palette/field lookup and minified texture effects. Never apply cubic smoothing indiscriminately across interior/exterior classifications, invalid values or sharp intended palette boundaries. Preserve RFF's high-precision orbit data; filter the display field or color samples. **Difficulty 5.2 / effectiveness 7.4**, provisional until compared with RFF's existing filtering. A 1D palette generally needs a simpler filter than this 2D environment routine. Independently implement the mathematics and verify original provenance before adapting distinctive code.

Sources: [trace.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/trace.ts), [environment shader](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/lighting/env.ts), [ADR-0084](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/docs/adr/0084-post-hit-surface-refinement.md), [ADR-0092](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/docs/adr/0092-faithful-marcher-unification.md).

[Trace probe results](gmt-audit/trace-probe-results.json) execute the actual TypeScript shader generator and check refinement gating and emitted statements. The numerical examples are isolated CPU illustrations; no GPU rendering, performance benchmark or visual confirmation is claimed.

### Current shader investigation order

1. **Sampling/PDF/weight agreement (S06, S09–S11):** highest priority for path-tracing correctness; more samples do not remove systematic estimator bias.
2. **Volume units and shared light contracts (S01–S03, S14):** make appearance independent of incidental march density and correct cross-path attenuation.
3. **Geometry/material/sample consistency (S08, S13, S15–S16):** align visibility, boundary coverage, color and depth before adding visual polish.
4. **Stable filtering and feature specialization:** quantify image quality, compilation cost and GPU time only after establishing the intended result.

This is an investigation priority based on source evidence, not a measured performance ranking. For RFF's 2D renderer, consistent render channels and footprint-aware display filtering are more directly applicable than a full 3D path tracer.

## Shader assembly and analysis-pass follow-up

### S17 — Geometry histograms do not reproduce all visible mapping inputs

**Confirmed source contracts:** the Histogram shader passes a constant up normal `(0,1,0)` to `getMappingValue`. Mapping mode 5 evaluates `0.5*dot(n,up)+0.5`, so every hit contributes 1 for that mode, regardless of its actual slope. A sideways normal would contribute 0.5 and a downward normal 0. The pass is therefore not a faithful Normal-mode distribution. This is independent of histogram binning or display behavior downstream.

The worker also overwrites its camera bases with `tan(fov/2)` and aspect scaling. Main rendering instead uses `orthoScale/2` when orthographic. With orthoScale=4 and FOV=45 degrees, main half-height is 2 while the histogram's is approximately 0.4142. If orthographic mode is preserved in its uniforms, histogram rays sample the wrong extent; if it is not preserved, projection itself is mismatched. Main camera modulations are also absent from this reconstruction. These are explicit producer differences; the complete UI impact still needs an end-to-end scene comparison.

Further differences: the generated histogram ray computes a stochastic seed, but trace receives literal 0; histogram tracing does not receive the refinement gate or Main's `isMobile/isLite` option. Some approximation is reasonable for a small analysis pass, but its contract must say which distribution is being approximated. Do not promise pixel-exact visible-image statistics from this path. The color histogram instead samples the pipeline output texture and computes luminance; its color space and pre/post-processing position require separate verification.

**Improvement:** share camera/sample configuration with the visible render; supply an actual normal for Normal mode or mark that analysis mode unsupported. Distinguish raw field statistics, visible-surface statistics and displayed-color statistics. For cost control, reuse an existing auxiliary render channel or explicitly downsample a field buffer rather than independently reconstructing an approximate scene.

**GMT difficulty 5.5 / effectiveness 7.3. 2D adoption difficulty 5.8 / effectiveness 8.0:** a field-aware histogram can guide palette ranges and contrast without recomputing expensive orbits. Keep masks, sample coverage, normalization and the displayed viewport aligned. RFF's high-precision field computation remains unchanged; this is an analysis/display facility, not a replacement perturbator.

Sources: [ShaderBuilder.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/engine/ShaderBuilder.ts) (Histogram branch), [WorkerHistogram.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/engine/worker/WorkerHistogram.ts), [MappingModes.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/coloring/MappingModes.ts), and the camera basis section of [UniformManager.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/engine/managers/UniformManager.ts).

### S18 — Builder emission mutates state and uniform conflicts are not rejected

`buildFragment()` appends the generated shading integrator to its persistent array each time it is called. A second build on the same shading-enabled builder emits two definitions. A dependency-isolated execution of the actual class confirms one then two definitions. **The inspected production factory constructs a fresh builder for each build**, so this is a latent API/reuse hazard, not evidence of a current user-visible recompilation failure.

`addUniform` silently overwrites injected entries keyed by name, including incompatible types or array sizes. The static uniform prefix is separate: injecting a name present there emits a duplicate declaration. This is already documented in the source. It remains a useful validation improvement because runtime GLSL errors are harder to diagnose than feature-attributed declaration conflicts.

**Improvement:** make emission pure by assembling deferred integrators into a local list; validate same-name declarations against both schema and injected entries, allow identical declarations deliberately, and reject incompatible shapes with feature attribution. Verify a repeated build produces identical source. Keep compiler diagnostics mapped back to the injecting feature.

**GMT difficulty 3.8 / effectiveness 5.9. 2D adoption difficulty 4.8 / effectiveness 6.5:** applicable to optional shader feature composition and Vulkan interface validation. RFF must still check descriptor binding, push-constant layout and host/shader type agreement; string deduplication alone cannot validate those contracts.

Sources: [ShaderBuilder.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/engine/ShaderBuilder.ts) (`buildFragment`, `addUniform`, `buildUniformsString`) and [ShaderFactory.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/engine/ShaderFactory.ts) (`buildShader`).

### Variant contracts still requiring follow-up

- Main emits headers before math/coloring, while Physics and Histogram emit them afterward. The source explicitly documents this latent ordering hazard; the current cited preprocessor-only header avoids it. Normalize stage order or declare dependencies before expanding feature hooks.
- Mesh emission omits the accumulated defines, replaces rotation/precision helpers with stubs, fixes the iteration hard cap to 100 and disables numeric-DE kernel gates. Inspect each exporter consumer and injected formula requirement before treating the mesh library as equivalent to the live surface. These may be deliberate export-space policies, but require explicit capability checks.
- Physics uses its own perspective ray and coarse march. This is intentional navigation probing rather than rendered-image equivalence. Test its consumer's handling of missed/thin surfaces separately from visual raymarch correctness.

[Builder probe results](gmt-audit/builder-probe-results.json) execute the actual builder with imported shader chunks replaced by inert sentinels. They prove assembly-state behavior only, not complete program compilation. Histogram numerical examples are guarded against the inspected expressions; they are not GPU captures.

## Reproduction and remaining work

## Direct-reflection implementation follow-up

### S19 — The throughput cutoff excludes ordinary normal-incidence dielectric reflections

Raymarched Direct reflections require both `roughness <= uReflRoughnessCutoff` and `dot(currentThroughput,currentThroughput) >= 0.01`. At normal incidence, with nonmetallic F0=(0.04,0.04,0.04), uSpecular=1 and deterministic mirror sampling, throughput is (0.04,0.04,0.04). Its squared magnitude is **0.0048**, below the cutoff. Thus the branch uses environment lighting instead of tracing scene geometry even when roughness passes its separate cutoff. The equivalent neutral-channel threshold is about **0.057735**.

This is a deliberate cost heuristic, not a zero-reflection result: environment reflection remains. However, low throughput does not bound missing radiance. A reflected emitter with radiance 100 can contribute 4 after a 0.04 factor, before other attenuation/composition. The example is arithmetic, not a measured scene brightness. A hard decision boundary can also change the represented scene discontinuously as Fresnel, specular strength or sampled direction changes.

**Improvement:** expose/document the cutoff as a biased approximation; provide a quality path without it. If stochastic termination is used, derive its survival weights and fallback combination so it does not silently double-count or replace geometry. Benchmark matched images and sample budgets. **GMT difficulty 5.0 / effectiveness 8.1. 2D adoption difficulty 5.5 / effectiveness 6.6:** relevant only to optional glossy relief rendering; preserve weak reflections of bright features rather than rejecting solely on a material coefficient. It is not a reason to introduce ray tracing into ordinary 2D coloring.

### S20 — Inaccurate-color reflections retain trap side channels from earlier evaluations

With `accurateColors=false` (the default), `traceReflectionRay` returns `(hitT,0,0,0)` and omits the full `DE` evaluation at the reflected hit. This zeros the tuple channels but does **not** initialize the global `g_orbitTrap` or `g_geomTrapFinal`. Mapping modes 10–13 read the former, and mode 14 reads the latter. Geometry-only `mapDist` preserves those final color side channels. Consequently those modes can use data left by an earlier full-map evaluation rather than either the reflected point or a defined default. Which earlier point wins depends on the active path, including primary marching and recovery.

The fast path's description as a gradient-default color is therefore not a complete contract for every mapping mode. Even modes using the tuple are mode-dependent: a zero orbit trap passes through a log floor, while position-based modes still depend on the reflected position. **Improvement:** use an explicit color payload/default policy, or require accurate evaluation for modes dependent on trap side channels. Keep fallback colors deterministic and define capability requirements per mapping mode. Verify both layers, geometric traps and recovery paths.

**GMT difficulty 5.4 / effectiveness 7.3. 2D adoption difficulty 5.6 / effectiveness 7.6:** store orbit channels with the sample/tile they belong to, including validity masks. Avoid implicit mutable shader globals when reusing material code across main, preview and auxiliary passes. This complements the render-channel proposal.

### S21 — Reflection-cone filtering measures only one curvature direction

Both environment-only and raymarched reflection integrations estimate a tangent using `cross(n,v)`, evaluate one neighboring normal, and set the cone angle to `2*length(nOff-n)+pixelSizeScale`. This captures useful curvature at one additional normal-evaluation cost, but it is not a bound over the pixel footprint. On the surface `z=k*x*x/2` at the origin, the normal is constant along y and varies along x. A view selecting the y tangent yields zero measured curvature despite a nonzero reflected-direction spread along x. Grazing projection can also stretch the surface footprint beyond the simple scalar estimate.

The primary normal in Direct uses a forward-difference estimator with a wider epsilon, whereas the offset sample uses `GetNormal` (tetrahedral for analytic DE). Their difference can include estimator/scale disagreement as well as curvature. The fallback tangent at normal incidence is an approximate construction, not an explicitly orthogonalized frame. These observations qualify the filter's quality claims; they do not invalidate its practical anti-aliasing benefit.

**Improvement:** compare two orthogonal tangents with consistent normal estimators/epsilon, or propagate ray differentials/cones with a stated conservative approximation. Use the larger directional spread or an anisotropic filter; assess the extra DE calls and cold compilation cost. Keep a lower-cost one-direction option clearly labeled. **GMT difficulty 6.7 / effectiveness 7.2. 2D adoption difficulty 5.9 / effectiveness 7.5:** height-field relief can derive directional normal variation from neighboring field samples, often without expensive fractal re-evaluation. Filter display/lighting data without blurring set classification or reducing orbit precision.

### Reflection semantics and estimator cautions

- **Off means no added tracing:** the reflection feature's Off/disabled path leaves the direct shader's basic Fresnel-weighted environment fallback active. This is intentional in source comments but the label can be clearer. True zero specular response is a separate material/lighting decision.
- **Bounded VNDF still needs a matching weight:** the Direct reflection path uses the same bounded sampler as S06 and assumes cancellation to F*G1(L). Do not apply an unbounded-VNDF cancellation proof unchanged. Review the sampler, its support, the approximate G1 and the below-horizon deterministic fallback together.
- **Glossy sampling plus roughness mip filtering is an approximation:** misses already sample directions from a rough lobe and then query a roughness-blurred environment. This can broaden the effective filter again; distinguish sampling variance control from the intended BRDF integral.
- **Candidate fade is not measured geometric coverage:** recovery blends toward the environment using a squared function of DE/epsilon over 1–6 footprints. It is a useful artistic confidence heuristic, not a probability derived from the reflection cone. Compare silhouette bias against additional march work.
- **Compile gates are useful:** accurate reflected colors add a full map evaluation only when emitted; refinement likewise omits its additional evaluation site when off. Apply this specialization idea to expensive optional 2D display features, while validating pipeline/cache variants and descriptor layouts.

Sources: full [reflection integration and controls](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/reflections/index.ts), [reflection marcher](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/reflections/shader.ts), [direct shading](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/lighting/shading.ts), [material evaluation](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/material_eval.ts), [mapping modes](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/coloring/MappingModes.ts) and [DE globals](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/de.ts).

[Reflection probe results](gmt-audit/reflection-probe-results.json) check the actual generated reflection source, default/accurate color gates and the isolated throughput threshold. The curvature counterexample is analytical. No GPU image or performance result is claimed.

## Validation records and outstanding scope

## AO and atmosphere feature follow-up

### S22 — Stochastic distance-probe AO darkens an unoccluded plane

The AO shader samples a cosine-weighted hemisphere direction, then compares the distance estimate at `p+dir*h` against the travel h. For a true plane SDF and an exact surface origin, that distance is `h*mu`, where mu is the direction's cosine with the normal. Its accumulated deficit is therefore `h*(1-mu)` even though the ray encounters no occluder. Cosine-weighted sampling has E[mu]=2/3, so the normalized deficit tends to 1/3; with intensity 0.2, the shader's factor 2.5 gives mean AO near **0.8333**, instead of unoccluded visibility 1. The source-guarded CPU example with five samples and spread 0.5 gives **0.83337**, including the denominator epsilon. Fixed normal sampling returns 1 on the same plane.

This is a distance-deficit shading heuristic, not a hemisphere visibility estimator. It can be visually useful, but its mode switch changes the baseline meaning. A larger iteration count cannot remove that systematic plane deficit. **Improvement:** either keep the artistic heuristic and explain/calibrate its baseline, or implement a visibility/horizon estimator with a clear geometric contract. Simply replacing h by a projected distance is not a complete fix for arbitrary geometry; validate planes, convex/concave corners and thin occluders.

**GMT difficulty 6.5 / effectiveness 7.8. 2D adoption difficulty 6.0 / effectiveness 7.4:** use a defined height-field horizon/visibility approximation for relief shading, with an unoccluded flat-field baseline test. Do not interpret the 2D escape field itself as a true signed distance without justification.

### S23 — AO navigation and accumulation evaluate different shading rules

For Direct rendering, `isMoving` disables the fully random direction and replaces it with `normalize(mix(n,hemisphereDirection,0.5))`. It also removes the radial jitter. The accumulation path uses the full hemisphere direction and nonzero radial jitter. In the same plane example, the half-angle direction raises mean AO to approximately **0.95524**, versus **0.83337** after accumulation. This difference follows from the estimator, independently of whether the noise texture is temporally frozen elsewhere. The shader always requests `getBlueNoise4(gl_FragCoord.xy)` rather than explicitly selecting its stable counterpart.

**Improvement:** keep the same direction distribution/estimator while navigating and freeze only its sample sequence, or label and intentionally transition a different preview approximation. Check reset/first-frame handling before quantifying final-image bias. Local fragment coordinates also repeat the image-tile consistency question from S12.

**GMT difficulty 4.9 / effectiveness 7.1. 2D adoption difficulty 4.8 / effectiveness 7.2:** stable preview noise should not change the expected relief shading when progressive rendering starts.

### S24 — AO sample count changes spatial reach, and small spread is clamped

Probe distances are `(0.1+0.125*i)*spread+jitterBias`; samples extend the interval rather than subdividing a fixed interval. With spread 0.5 and zero jitter, five probes reach 0.3 and 32 reach 1.9875. Weights decay by 0.8 per probe, so distant additions are weaker, but the control still changes the spatial kernel as well as computation cost. The compile cap allows 256 while the declared runtime Samples control stops at 32; raising the cap alone does not extend that declared UI range. Other programmatic setters remain to be checked.

The Spread control declares a minimum 0.0003, but the shader floors it to 0.001. Its additive normalization epsilon 0.0001 further prevents exact scale invariance at small spreads. **Improvement:** separate radius, sample count and distribution; derive samples over a fixed radius and normalize consistently. Align declared ranges with shader behavior. Support intentional world-space versus pixel/field-space radius policies instead of assuming zoom should always rescale physical AO.

**GMT difficulty 4.8 / effectiveness 6.7. 2D adoption difficulty 4.7 / effectiveness 7.3:** expose a meaningful relief radius while quality controls change sampling density. Preserve legacy settings with a versioned mode if output changes.

### S25 — Disabling glow also removes basic density-fog accumulation

The atmosphere feature always injects fog post-processing, but injects `ATMOSPHERE_VOLUME_BODY` only when `state.glowEnabled` is true. That body contains both glow accumulation and `accDensity += ...` for basic volumetric fog. With glow disabled, the inspected atmosphere path leaves density at its initialized zero while the fog composite still consumes it. Executing the actual feature injection confirms the volume body is present with glow on and absent with glow off. Distance fog remains active; the separate god-ray scatter feature is not the missing basic density accumulator.

This conflicts with the advertised independence of fog and glow at the post-process boundary. **Improvement:** separate fog-density and glow injection gates, preserving a genuinely absent expensive glow path. Verify all combinations of glow compilation, zero glow intensity, fog density and scattering. Also document that the basic density sum is a proximity-weighted artistic quantity: its per-step weight includes `uFudgeFactor` but not the actual jittered/damped segment length, so it is not a physical Beer-Lambert optical-depth integral. Glow's scalar/vector modes similarly change color semantics, not merely performance.

**GMT difficulty 4.2 / effectiveness 7.8. 2D adoption difficulty 4.6 / effectiveness 6.4:** keep optional bloom/glow independent of depth haze, and give each effect an explicit input field and enable condition. A 2D depth-haze effect can be a simple compositing operation without a 3D volume integrator.

**Additional composition policy:** Direct shading multiplies its final sum, including emission and direct/specular light, by the AO tint. This is a broad artistic occlusion treatment, not only ambient-light visibility. A physically motivated option would apply visibility to the relevant incident-light terms and preserve local emission. Existing presets may depend on the current appearance.

Sources: full [AO shader](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/ao/shader.ts), [AO parameters/injection](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/ao/index.ts), [atmosphere accumulation](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/atmosphere/shader.ts), [atmosphere controls/injection](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/atmosphere/index.ts) and [direct composition](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/lighting/shading.ts).

[AO/atmosphere probe results](gmt-audit/ao-probe-results.json) execute the actual source emitters/injection and evaluate an isolated plane-SDF model on the CPU. The model assumes a surface origin, unit normal and uniformly distributed underlying samples; it is not a GPU image or a characterization of the supplied blue-noise texture.

## Probe records and remaining audit scope

## Bloom and lens post-processing follow-up

### S26 — The bright-pass soft knee has a discontinuity at its threshold

The bright pass computes a squared soft ramp, then selects `max(soft,step(threshold,brightness))`. Just below a positive threshold the soft contribution is approximately 0.25; at the threshold the step forces it to 1. With threshold 0.25, an input of 0.24999999 produces approximately **0.06245**, while 0.25 produces **0.25**. This is approximately a fourfold jump in the extracted bloom signal, not a fourfold change in the final image. Bilinear filtering before thresholding does not remove the discontinuity in this function.

**Improvement:** derive a continuous extraction curve with explicit knee width and brightness normalization; test its limits, continuity and monotonicity, including threshold zero and HDR inputs. Evaluate threshold-before-downsampling versus threshold-after-downsampling: they represent different highlight extraction and can lose small bright features differently. Use a moving subpixel highlight to assess temporal behavior. **GMT difficulty 3.7 / effectiveness 8.0. 2D adoption difficulty 3.8 / effectiveness 8.1:** directly relevant to bloom on narrow fractal filaments and animation. Preserve crisp base imagery and apply bloom as a separate display layer.

### S27 — Bloom Spread also changes constant-field gain

Every normalized blur level preserves a constant input. Upsampling then adds `high+weight*low`, so contributions accumulate without normalization. For constant extracted input, the current ideal arithmetic gain is 1 at radius 0.5, 2 at 1.5, 5.5 at 5, and 7 at 6.5–7. The output is subsequently multiplied by Bloom intensity and added to the image. Thus Spread controls brightness as well as spatial extent; 6.5–7 is a plateau in this implementation.

This may be an intended artistic glow, but it is not an energy-preserving spread control. **Improvement:** offer normalized mip weights or a separately named legacy additive mode, keeping intensity and spread independently predictable. Preserve existing presets rather than silently reducing old bloom. Verify an impulse and a flat field at multiple resolutions, including dimensions that collapse several mip levels to 1x1. **GMT difficulty 4.3 / effectiveness 7.1. 2D adoption difficulty 4.0 / effectiveness 7.8:** useful for consistent image/video export and predictable palette grading.

### S28 — Bloom builds inactive levels and its documented draw budget is inaccurate

The implementation always performs six downsample passes before choosing active levels. With L active levels it performs one bright pass, six downsample passes, 2L blur passes and L-1 upsample passes: **6+3L draws**, ranging from 12 to 27. The feature's default radius 7 selects all seven levels and therefore 27 draws. The file's approximate 19-draw description and the comment that radius 5 uses all seven levels do not match this implementation (radius 5 selects six).

**Improvement:** determine active levels first and downsample only what is consumed. Consider lazy allocation and stop creating duplicate 1x1 levels when appropriate, while preserving the chosen gain policy. Zero bloom is skipped by the inspected caller, not by an intensity argument inside BloomPass.render. Existing output should be compared before optimizing; fewer draw calls alone does not prove a GPU-time improvement. **GMT difficulty 3.7 / effectiveness 6.6. 2D adoption difficulty 4.4 / effectiveness 6.9:** multi-resolution blur is reusable, but select active work before dispatching Vulkan passes and verify synchronization/resource reuse.

### Lens and export contracts retained for follow-up

The chromatic-aberration shader uses twelve wavelength-like RGB weights, distorts UV by radial distance and normalizes each output channel by its weight sum. This is an artistic spectral approximation. It replaces `col` by newly sampled raw `map` values, then adds bloom at undistorted UV. The ordering can be intentional, but all preceding post-effect stages, alpha handling and edge addressing must be traced before claiming compositional equivalence. Distortion strengths can sample beyond [0,1]; texture wrap policy determines the boundary appearance.

Bucket export's inspected bloom call uses either viewport dimensions or tile-composite dimensions while sampling the current composite. A bloom kernel reaches beyond the central pixel; verify tile halos/full-image context and radius scaling before promising seamless exports. No tile seam is claimed reproduced. The live worker and export caller sections were selectively read, not marked fully reviewed.

Source-reported distinctive Gaussian weights and the named chromatic-aberration formula need an original-source/license check before adaptation. This audit describes mechanisms; it does not grant copying clearance. A separable symmetric blur or independently derived spectral approximation can be implemented with documented provenance.

Sources: full [BloomPass.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine/BloomPass.ts), [post-effect shader and controls](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine/features/post_effects.ts), and selected caller sections in [handleRenderTick.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/engine/worker/handleRenderTick.ts) and [GmtBucketHost.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/engine/GmtBucketHost.ts).

[Bloom probe results](gmt-audit/bloom-probe-results.json) contain source-guarded CPU arithmetic for threshold continuity, constant-field gain and draw counts. They are not GPU execution or timing measurements.

## Validation and remaining scope

## Color-domain and UV-composition follow-up

### S29 — Saturation extrapolation feeds negative channels into the ACES approximation

Color grading clamps the levels input nonnegative, then applies `mix(luma,col,saturation)` with saturation allowed up to 2. This extrapolation can produce negative channels. The following component-wise ACES-fit function does not sanitize them. With green (0,1,0), neutral levels/gamma and saturation 2, grading returns (-0.7152,1.2848,-0.7152). The current ACES rational curve maps that to approximately **(1,0.85182,1)** before output encoding. Thus a saturated green example acquires maximum red/blue components, a strong hue failure rather than simple gamut clipping.

**Improvement:** establish a valid-domain/gamut policy between grading and tone mapping. Clamping negative values is a simple safety option but is not a complete hue-preserving gamut mapper; evaluate a gamut-aware saturation operation or compression, with explicit legacy behavior for presets. Test primary/secondary colors, near-black values, HDR values and saturation endpoints. **GMT difficulty 4.7 / effectiveness 8.3. 2D adoption difficulty 4.8 / effectiveness 8.4:** directly relevant to fractal palettes; numerical precision in the orbit does not protect against display-stage color-domain errors.

### S30 — The Reinhard luminance denominator floor crushes near-black values

The implementation computes `mapped=lum/(1+lum)` and multiplies RGB by `mapped/max(lum,0.001)`. For positive lum<0.001 it adds an unintended factor lum/0.001. A gray input 0.0001 returns about **0.000009999**, whereas the algebraically cancelled expression `RGB/(1+lum)` returns **0.00009999**. The current result is one tenth of that intended basic luminance-Reinhard expression. It also changes slope at the floor boundary. The label says extended Reinhard, but no white-point extension appears in this function.

**Improvement:** use a numerically stable algebraic expression with a defined nonnegative input contract, and accurately name the implemented variant. Negative/HDR inputs need their own policy; do not merely move a denominator epsilon. **GMT difficulty 2.8 / effectiveness 6.9. 2D adoption difficulty 3.0 / effectiveness 7.5:** preserve faint palette bands and dark backgrounds while avoiding unnecessary divisions.

### S31 — Droste masking and smoothing are not preserved by later raw resampling

The post shader applies UV modifications, samples/filters the source, and multiplies its RGB by `mask`. The chromatic-aberration feature subsequently replaces `col` with a weighted set of raw `map` samples, bypassing the already applied mask and smooth/preview-box filtering. Bloom then adds its texture independently of mask. Finally beauty output forces alpha to 1. Therefore the Droste option named Transparent is not transparency in this beauty shader, and masked regions can be filled by later effects. Alpha/depth export branches return before the beauty mask multiplication, so their warped samples also do not consume that mask.

**Improvement:** define whether the mask represents crop coverage, black padding or transparency; apply a corresponding alpha/compositing contract consistently. Route lens samples through a shared sampling/filtering function, or deliberately specify which effects precede the crop. Bloom spilling across a crop can be intentional, but should not arise solely from overwritten intermediate state. Validate Droste with CA, bloom, smooth upscaling, preview downsampling and auxiliary exports together. **GMT difficulty 6.1 / effectiveness 7.6. 2D adoption difficulty 6.0 / effectiveness 8.0:** coordinate warps are directly reusable on 2D renders, provided coverage, filtering and export semantics are retained.

### Droste mechanisms and limits

The effect maps coordinates through complex log/exp, applies a conformal spiral transform, rescales into an annulus and optionally mirrors alternate levels. This is a display warp; it creates recursive imagery without new high-precision fractal iteration. It is a useful optional 2D creative feature, distinct from an actual deep zoom.

The twisted path calls `log(length(z))` and `atan(z.y,z.x)` at the exact center without a finite-center policy. Annulus normalization uses at most twelve scaling steps and does not verify convergence afterward. When the enforced outer/inner ratio is close to 1, twelve steps cannot bring arbitrary radii into range. A log-domain level calculation could avoid repeated scaling and overflow, but must preserve rotation, mirroring parity and boundary conventions. Automatic periodicity silently keeps the manual period if its square-root domain is invalid. These deserve explicit edge-case behavior and UI feedback rather than a claim of unrestricted infinite recursion.

**2D feature difficulty 6.2 / effectiveness 7.2:** independently implement and validate coordinate/footprint mathematics; preserve the original render for export/re-editing. The header reports a Tom Beddard Pixel Bender port, so its provenance/license needs verification before copying distinctive code. No external licensing clearance is claimed.

Sources: [color grading](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine/features/color_grading.ts), [post shader](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/post_process.ts), [lens/bloom injection](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine/features/post_effects.ts), [Droste math](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/droste/shader.ts) and [Droste settings/injection](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/droste/index.ts).

[Color probe results](gmt-audit/post-color-probe-results.json) reproduce the isolated saturation/ACES and Reinhard equations and check mask ordering against source. These are CPU/source checks, not GPU screenshots or a standards-conformance validation of the named tone mappers.

## Remaining validation and scope

## Water geometry/material follow-up

### S32 — Fast reflected colors discard water material identity

Water composition marks `result.w` with `MATERIAL_WATER=10`; material evaluation applies the water override when that channel is at least 5. The Direct reflection marcher's default inaccurate-color path returns `(hitT,0,0,0)`. It preserves the geometric hit but loses the material sentinel, so the reflected hit does not receive water albedo, roughness or normal overrides. This is more than an approximate palette lookup: a control named Accurate Colors changes material identity. The accurate path's full DE call preserves the sentinel.

**Improvement:** separate geometry/material identity from optional expensive color channels. At minimum identify the winning primitive at a reflected hit even when skipping orbit-trap evaluation. Validate mixed water/fractal intersections and all reflection quality modes. **GMT difficulty 5.5 / effectiveness 7.9. 2D adoption difficulty 5.2 / effectiveness 7.4:** render buffers should carry stable material/mask identifiers independently of palette data, especially for layered relief and compositing.

### S33 — Water view tint mixes coordinate spaces and assumes the primary camera

`applyWaterMaterial` receives `p_fractal`, but computes its view direction as `normalize(uCameraPosition-p)`. The normal render path explicitly sets `uCameraPosition` to zero while carrying scene offsets separately. Therefore this direction points toward the fractal-space origin rather than necessarily toward the camera. For an illustrative scene-space point (0,1,0) and camera (0,3,0), the consistent direction is up; the zero-uniform subtraction points down. With an up normal, the tint multiplier changes from the intended expression's 0.4 to 1 in that example.

Moreover a reflected hit should use its incoming ray/view direction, not the primary camera even after fixing coordinate conversion. **Improvement:** pass the shading view vector explicitly through material evaluation, with a stated coordinate system; or move the view-dependent tint into lighting. It is labeled Fake Depth Absorption in the source and should not be confused with actual path-length-dependent absorption. **GMT difficulty 4.8 / effectiveness 7.2. 2D adoption difficulty 4.7 / effectiveness 6.8:** explicit coordinate and view contracts are essential when reusing relief materials in transformed previews/export passes.

### S34 — Water's normal and step bound do not fully match its implicit surface

The wave displacement uses 3D simplex noise at `p*frequency`, including p.y. The implicit field is `F(p)=p.y-level-H(p.x,p.y,p.z)`, whose gradient is `(-Hx,1-Hy,-Hz)`. The material's height-field tangent construction uses only x/z differences and assumes `(-Hx,1,-Hz)`. The two are not generally equivalent when Hy is nonzero. A fixed finite-difference step 0.05 also becomes large relative to the fine octave as frequency increases.

The constant multiplier 0.6 is described as a Lipschitz bound, but the code does not derive it from amplitude, frequency or noise-gradient bounds. Even the sine layer's maximum x derivative scales as `0.5*strength*frequency`, reaching 7.5 at the declared upper settings before adding noise. That observation does not prove a particular complete-noise overstep, but it rules out treating the fixed coefficient's comment as a general proof. Switching displacement off abruptly outside `abs(distPlane)<2*strength` also requires continuity/conservativeness analysis.

**Improvement:** choose an actual x/z height field, or differentiate the full implicit field; use analytic noise derivatives or consistent scale-aware finite differences. Derive a conservative step policy from the supported parameter range or bound a displaced plane with explicit refinement. Compare normals and intersections against a dense reference. **GMT difficulty 7.0 / effectiveness 7.8. 2D adoption difficulty 6.0 / effectiveness 7.4:** multi-scale sine/noise fields can animate ripple normals or palette warps cheaply, but their derivatives and sampling footprints must match the visible field. This does not require modifying high-precision Mandelbrot iteration.

### Auxiliary-variant behavior

Actual feature-injection execution confirms water contributes hooks only to Main, not Physics, Histogram or Mesh. Thus the auxiliary geometry histogram omits water that can cover the visible fractal; mesh export and navigation probes also do not represent the same combined scene. This may be an intentional export/probe scope, but should be explicit in those contracts. It further limits S17's histogram equivalence.

The mechanism itself is a useful extension pattern: combine distances with a minimum, attach material identity to the winning surface, then evaluate its material separately. For 2D this maps to layered scalar fields/masks and procedural overlays, with care around interpolation of discrete identities. Generic field composition is reusable; the current water implementation should not be copied as a validated SDF model.

Sources: full [water feature](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/water_plane.ts), [reflection result emission](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/reflections/shader.ts), [material injection](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/material_eval.ts), and camera-space setup in [UniformManager.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/engine/managers/UniformManager.ts).

[Water probe results](gmt-audit/water-probe-results.json) execute feature injection for four variants and record illustrative vector arithmetic. The derivative example is a contract counterexample, not a sampled noise realization. No GPU artifact or performance claim is made.

## Outstanding validation and scope

[CPU probe results](gmt-audit/shader-probe-results.json) record the attenuation, negative palette coordinate, GGX peak, bounded-sampler endpoint and volume-gate counterexamples. The local scratch runner is `debug/gmt-deep-shader-probes.mjs`; it asserts that the relevant source expressions still exist before evaluating the isolated arithmetic. Those assertions do not substitute for full GPU tests or dynamic call-path coverage.

This audit is **not complete**. Remaining work includes the rest of shader construction/integrators, every formula implementation, renderer/worker lifecycle, exports/importers, shared engine/store/animation, palette and sibling applications, UI behavior, test implementations, bundled reference code and documentation reconciliation. The ledger intentionally leaves these pending or partial rather than inheriting a false completion status from the earlier feature survey.

[Batch 2 probe results](gmt-audit/path-probe-results.json) add transformed-uniform sampling, actual CDF reduction and mask recurrence checks. The scratch runner is `debug/gmt-deep-path-probes.mjs`. Its initial roulette assertion overestimated the discrepancy (>0.01); the observed discrepancy was about 0.00556, so that harness threshold was corrected. No GMT source was changed. The CDF function was executed with a minimal texture-storage stub, which validates its CPU arrays/arithmetic but not Three.js upload behavior.
