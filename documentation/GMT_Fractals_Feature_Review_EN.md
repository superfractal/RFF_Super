<!-- Modified by GPT-6 on 2026-10-07, 2026-10-08. -->
<!-- Modified by Opus 5.5 on 2026-10-21, 2026-11-01. -->

# GMT-fractals: Feature Review and Opportunities for RFF_Super

> **Final review entry point (2026-10-08):** [Improving RFF_Super with GMT as a reference](GMT_to_RFF_Review_EN.md). All 3,727 inventoried GMT files have completed inspection records and unchanged hashes. This companion preserves chronological evidence: earlier progress counts, pending review statements and provisional priorities describe their recording time, not current coverage. Later corrections and the final entry point govern recommendations. Source inspection does not establish runtime correctness or measured GPU benefit.

> **Expanded audit in progress:** the later request covers all code in detail. This earlier feature survey is not evidence of that broader completion. See [the file-by-file deep audit](GMT_Fractals_Deep_Code_Audit_EN.md) for coverage, new arithmetic findings and corrections.

Review date: **2026-10-07**

Source snapshot: [gamazama/GMT-fractals at ad28b8b](https://github.com/gamazama/GMT-fractals/tree/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1) (package version **0.9.8.5**).

Target comparison: the current local RFF_Super source tree. This report proposes work; it does not implement changes to either application.

For shader-level calculations, sampling/filtering mechanisms, implementation caveats and individually rated 2D adaptations, see [Shader Mechanisms and Practical 2D Adaptations](GMT_Fractals_Shader_Mechanisms_EN.md). It includes explicit source tracing and separates verified code behavior from comments and untested mathematical assumptions.

## 1. Main findings

### Target clarification: improve RFF_Super using GMT as a reference

See the [RFF shader adoption plan](GMT_to_RFF_Shader_Adoption_EN.md) for current target-side comparisons and concrete integration/verification steps. It puts RFF improvements first; GMT defects remain supporting evidence in the audit appendix.

The intended deliverable is an actionable comparison for **RFF_Super**, not primarily a list of defects to fix in GMT. The deep-audit appendix records mechanisms and hazards so they are not copied blindly. A defect in GMT is not evidence that RFF has the same defect. Each adoption recommendation must identify RFF's existing implementation, the incremental change, its user benefit and a verification plan.

**Current concrete shader priority: continuous bloom highlight extraction.** A fresh read of RFF's `shdsrc/vk_bloom_threshold.frag` confirms a binary luminance threshold. `shdsrc/vk_bloom.frag` independently repeats that threshold for the sharp component, then blends sharp/blurred glow using the existing Softness control. Therefore bloom, softness, HDR handling and linear-light addition already exist; they are not new GMT-inspired features. The useful improvement is a separately defined continuous threshold knee shared by both extraction sites. Softness currently controls the sharp/blurred mixture and should retain that meaning. GMT suggests the soft-knee objective, but S26 shows its implementation is discontinuous and should not be copied.

**Expected benefit:** steadier thin highlights during palette animation and less abrupt bloom onset. **Difficulty 4.2 / effectiveness 8.1**, provisional until rendered comparisons. Preserve knee=0 legacy output, existing scene-linear/HDR behavior and save-file defaults; append any new persisted setting under ConfigIO's rules. Verify a threshold sweep, moving filament, preview/video agreement and unchanged output when bloom is disabled. Do not change MPA or orbit precision.

Sources: [RFF threshold shader](../shdsrc/vk_bloom_threshold.frag), [RFF bloom composition](../shdsrc/vk_bloom.frag), and GMT [BloomPass](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine/BloomPass.ts). This is a proposal only; application code has not been modified.

Other shader proposals remain comparison tasks before implementation: user-facing inspection/export of existing render fields; histogram-guided palette mapping; broader material masks (distinct from its existing rim/noise masks); and optional field-driven image lookup. RFF already has ordered shader layers, GGX-related shading and relief masks, so generic claims that these are missing are inappropriate. The next review should establish the exact missing behavior rather than duplicate those systems.

GMT is primarily a 3D fractal application, but many of its strongest ideas transfer to a 2D Mandelbrot renderer. The highest-value transfers are **gradient authoring, recoverable image exports, audio modulation, parameter capability metadata, and explicit rendering diagnostics**. Its 3D coloring and material architecture also suggests useful 2D and 2.5D extensions.

RFF_Super already has substantial appearance and animation functionality. Adding generic “PBR,” “a timeline,” “audio,” “GPU batching,” or “OKLab interpolation” would misdescribe the gap. The useful opportunities are more specific: image-derived palettes, user-shaped animation curves, audio-driven appearance parameters, material masks, diagnostic/export channels, and consistent capability explanations.

**Do not replace RFF_Super's deep-zoom mathematics with GMT's renderer.** GMT's 3D distance-estimation/raymarching path and its separate 2D reference-orbit/LA path solve different problems. RFF_Super's MPA uses the reference orbit's periodic structure and variable period-derived skip lengths. It is not BLA, and GMT's LA implementation is not a drop-in replacement.

### Recommended order and numerical ratings

| Priority | Proposal | Relationship to 3D / 2D | Difficulty / 10 | Effectiveness / 10 |
|---|---|---|---|---|
| P1 | Image-to-palette extraction and broader gradient import | Direct 2D transfer | 5.5 | 8.5 |
| P1 | PNG with restorable settings | Dimension-independent | 5.0 | 8.8 |
| P1 | Capability explanations and shared parameter metadata | Dimension-independent | 6.0 | 8.3 |
| P1 | Render-channel inspection and selected channel export | 3D pass separation adapted to 2D | 6.8 | 8.6 |
| P2 | Gradient curves, paint tools, optional stop reduction | Direct 2D transfer | 6.7 | 8.1 |
| P2 | Audio-reactive appearance tracks and reusable modulators | Direct 2D transfer | 8.0 | 8.4 |
| P2 | Editable Bezier curves and explicit parameter interpolation spaces | Direct 2D transfer | 7.2 | 8.0 |
| P2 | Material masks and consistent surface evaluation | 3D architecture adapted to 2.5D | 7.0 | 8.2 |
| P2 | Adaptive work scheduling and render-state diagnostics | 3D scheduling adapted to existing 2D batching | 6.5 | 7.6 |
| P2 | Histogram-guided palette mapping | 3D coloring workflow adapted to 2D data | 5.8 | 7.8 |
| P3 | Orbit-trap channels | 3D iterative coloring adapted to complex orbits | 9.0 | 8.0 |
| P3 | Structured formula presets and restricted formula sequences | 3D formula composition adapted to 2D | 8.8 | 7.5 |
| P3 | Heightfield/relief mesh export | 3D mesh workflow adapted to a 2D scalar field | 7.8 | 6.4 |
| Defer | Full 3D raymarcher, path tracing, volumetrics | Mostly 3D-specific | 9.8 | 3.0 |

**Rating scale:** both scores run from **1.0 to 10.0**, with decimal values. Difficulty increases with engineering effort, mathematical uncertainty, integration, migrations and verification. Effectiveness measures the expected **additional benefit to RFF_Super's existing 2D users**, considering creative reach, reproducibility, reliability and workflow improvement. It is not GMT's implementation quality, predicted speedup, or a percentage. A high difficulty score is unfavorable; a high effectiveness score is favorable.

Difficulty anchors: 1.0 = isolated minor change; 3.0 = a small self-contained feature; 5.0 = several existing components plus persistence/tests; 7.0 = significant cross-component or GPU work; 9.0 = substantial mathematical/architectural research; 10.0 = a new renderer/product-scale undertaking. Effectiveness anchors: 1.0 = negligible marginal benefit; 3.0 = narrow optional use; 5.0 = useful to a subset; 7.0 = broadly useful; 9.0 = major recurring creative/workflow improvement; 10.0 = transformative for almost all target users.

These are reasoned planning estimates, not measurements. Decimal precision expresses ordering rather than statistical confidence; expect roughly ±1 point as prototypes reveal costs. Scores assume preserving precision and old files. Orbit traps score 9.0 because acceleration-compatible statistics are difficult; a direct-iteration-only prototype would be easier but is a narrower feature. The complete 3D renderer scores low in effectiveness specifically because this target is a mature 2D tool. Section 12 rates additional opportunities found in the expanded review, and Section 13 rates improvements to GMT itself separately.

## 2. Scope and evidence quality

The review followed the repository entry documents, subsystem rules, selected architecture notes and plans, then inspected implementation paths for rendering, exporting, palettes, animation, modulation, formulas, compatibility, and deep zoom. RFF_Super's corresponding implementation and file-format notes were checked to distinguish existing functionality from proposed additions.

The expanded review inventories **all 27 feature registrations in the GMT feature entry point, all four palette feature registrations, all 48 native formula-array entries, and all seven box-fold generator inputs**. It also covers user-facing functionality outside those registries and companion applications. See [Source Inventory and Control Catalogue](GMT_Fractals_Source_Inventory_EN.md) for declaration labels, descriptions and individual source links; Section 12 maps the feature families to adoption decisions.

This is an all-feature **product/source review**, not a claim that every source line, generated shader permutation, historical document, bundled Fragmentarium example or third-party asset was audited. Formula declarations are catalogued individually; mathematical transfer is assessed by family, with representative implementation inspection. A complete proof of every formula and every imported shader would be a separate mathematical and licensing audit. Registry coverage is not used as evidence of full runtime correctness.

Evidence is separated into:

- **Implemented:** directly visible in the inspected source. This does not imply full runtime or visual validation.
- **Documented:** stated by a document or source comment; historical benchmarks and known issues retain that status unless independently checked here.
- **Proposed:** this report's adaptation or improvement.
- **Locally checked:** exercised by the small isolated probes described in Section 10.

The downloaded tree has no installed `node_modules`. The full TypeScript build, browser suite, GPU rendering, mesh generation, and export workflows were not run. No screenshots or GPU timings were produced. The supplied download did not expose a Git checkout at its root, so package version and local paths identify the reviewed snapshot rather than a verified commit hash.

The RFF_Super workspace changed concurrently during the review: perturbator, palette, shader and configuration files appeared as modified after the initial clean status check. Those edits were not made by this review. A follow-up check found new escape-angle output and Binary Decomposition/Twist coloring declarations (`ShdPalEscapeAngleColoring.h` and the custom perturbator's `escapeAngle` output). These are now treated as existing/concurrent work, not new proposals. An escape endpoint angle is still different from an orbit-wide minimum trap statistic. The comparisons remain bounded by inspected source, not an immutable repository snapshot or verified release build.

### Documentation drift matters

The root README and `app-gmt/README.md` describe historical formula/feature counts and parts of an older layout. The app README itself later corrects the obsolete `formulas/built-in/` path. `docs/DOCS_INDEX.md` marks older narrative documentation as historical.

Likewise, `plans/tiled-progressive-rendering.md` begins with a later status update identifying implemented work and remaining items. Reading only its original design section would incorrectly classify all tiling as future work. Licensing notes also contain later decisions that change the interpretation of their opening audit.

Use source symbols in the evidence index below; do not turn old plan text, file comments, or a test's name into proof of current end-to-end behavior.

## 3. What RFF_Super already provides

| Area | Evidence in RFF_Super | Consequence for this proposal |
|---|---|---|
| Deep zoom | `LightMandelbrotPerturbator`, `DeepMandelbrotPerturbator`, reference selection in `RenderScene.cpp` | Preserve the existing reference/MPA foundation. |
| Centered reference and minibrot location | `CENTERED_REFERENCE` branch and `MandelbrotLocator::locateMinibrot` | Reference relocation is an improvement/validation subject, not an entirely missing feature. |
| GPU work budgeting | `GpuLightPerturbator.cpp`, `sizer.adapt(seconds, settings.batchSeconds)` | Extend existing budgeting rather than introduce a second scheduler blindly. |
| Video GPU submission splitting | `VidExportAttribute::gpuSubmitSplit` | Band-splitting ideas overlap existing export controls. |
| Gradient editing and perceptual interpolation | `PaletteStopEditor.hpp`, `ShdPaletteAttribute.h`, `blendPaletteColors` | Add authoring operations and imports, not a duplicate basic editor. |
| Palette sources | `PaletteSources.cpp`, `readPaletteSource` | Existing RFC/RFSP/KFR/KFP handling is the starting point for new formats. |
| Timeline and color interpolation | `TimelineEvaluator.cpp`, `TimelineParams.hpp`, animation guide | Existing Step/Linear/Smooth/Cubic and OKLab color interpolation remain intact. |
| Audio clips and export | `AudioTimeline.cpp`, `AudioExport.hpp`, animation guide | Audio-reactive parameter analysis is the proposed addition, not music playback itself. |
| Pseudo-3D surfaces | `vk_slope.frag`, `studio_ggx.glsl`, `chrome_surface.glsl`, `lustre_relief.glsl` | GGX-style specular, roughness, rim lighting and pseudo-AO are already present. |
| Timing and diagnostics | `RenderSceneRenderer.hpp`, `GpuPassTimer`, diagnostics in `RenderScene.cpp` | Improve visibility and regression coverage around existing instrumentation. |

**Tiling qualification:** RFF_Super comments refer to a “tiled export,” but the inspected `applyCreateImage` implementation renders and reads a full offscreen image. A complete current arbitrary-size tiled still-export path was not verified. Boundary-fill tiles, GPU submission bands, and memory-bounded large-image export are different mechanisms and must not be conflated.

## 4. Detailed feature opportunities

### 4.1 Image-derived palettes and gradient interchange — P1

**GMT implementation.** `palette/core/img2grad/index.ts` exposes `extract()` with three workflows: **Distill** for weighted dominant colors, **Tone** for a brightness-ordered color progression, and **Trace** for colors along a selected image path. The pipeline produces a 256-sample gradient and an OKLab ribbon. `importFormats.ts` implements MAP, GPL, GGR, CPT, CSS and JSON parsing.

**2D transfer.** All three workflows apply directly to a 2D fractal palette. A photograph, painting, or previous render becomes a starting color treatment without altering the Mandelbrot calculation. A trace across an existing fractal render could recover a useful visual progression, although it cannot recover the original iteration-to-color mapping.

**Proposed implementation.** Extend `PaletteSources` and the existing palette workspace. Start with MAP/GPL and Trace, then add Tone and Distill. Keep the extracted ramp as authoritative sampled data when a compact stop representation would lose narrow bands. Add explicit cyclic-seam handling because a fractal palette normally repeats.

**Important differences.** GMT's 256-texel representation is a product choice, not a requirement for RFF_Super. Preserve the target's required color resolution and alpha semantics. Importing a format does not mean redistributing its example palette collection. Do not modify the RNG order or interpretation of existing `{presetId, seed}` recipes.

**Acceptance criteria.** Deterministic extraction from identical input; predictable cyclic seam; import/export agreement within an explicitly selected approximation tolerance; old seeded palettes remain unchanged; canceled edits restore the prior palette.

### 4.2 PNG files that restore the scene — P1

**GMT implementation.** `SceneFormat.ts` connects scene serialization to PNG metadata, including `SceneData` and the legacy `FractalData` key. `pngMetadata.ts` writes UTF-8 iTXt data. GMT's formula format adds formula-aware loading to this generic mechanism.

**2D transfer.** Export an image with the precise RFF_Super settings needed to recreate it. Opening or dropping that image can offer “Restore settings” alongside ordinary image viewing. This is particularly useful for sharing deep locations and troubleshooting rendering differences.

**Proposed implementation.** Use a distinct versioned RFF metadata envelope containing exact coordinate strings, calculation settings, palette identity/data, appearance settings, render time and application version. Reuse established settings serializers where practical. Include an external-asset manifest; embedding settings does not embed textures, music or keyframe maps. Keep a setting to export pixels only.

**Hardening requirement.** Do not copy GMT's metadata reader unchanged. The local probes confirm incomplete PNG-signature checking and ignored metadata CRCs. Its signed chunk-length arithmetic also permits a non-advancing parser offset for a malformed length. Use validated unsigned lengths, bounds checks, full signature checking, metadata-size limits, supported compression handling and validated scene data before applying anything. Do not automatically execute embedded formula code or follow arbitrary external asset paths.

**Acceptance criteria.** Exact coordinate-string round trips at deep zoom; Unicode names; correct restoration of time-dependent appearance; clear missing-asset reporting; invalid or truncated metadata leaves the active scene untouched. Document that external image tools may strip metadata.

### 4.3 Capability-aware settings and parameter metadata — P1

**GMT implementation.** `FeatureSystem.ts` describes parameter types, ranges, scale mappings and optional shader bindings. `evaluateCompat()` compares declared feature requirements with formula capability tokens and produces reasons for disabled features.

**2D transfer.** A common capability description can explain why an operation is unavailable for a custom formula, a PNG-only source, a compressed reference, a GPU mode, or an iteration-map source. This is more useful than leaving a control active when it cannot affect the result.

**Proposed implementation.** Extend `TimelineParamDesc` and the existing workspace field descriptors incrementally with value scale, invalidation scope, source requirements and a human-readable reason. Reuse these descriptions in the settings panel, timeline, Local AI exchange and export validation. Avoid replacing the entire Win32 application with GMT's React/plugin architecture.

**Caution.** A declaration is not enforcement. GMT's own `FeatureSystem` comments explain soft failure behavior for dependency cycles and some registration mistakes. RFF_Super should validate its generated tables at development time and enforce unsupported operations at the execution boundary.

**Acceptance criteria.** The same parameter has the same range and meaning everywhere; unavailable controls explain the reason; unknown future track IDs remain handled by established compatibility rules; descriptor changes do not reorder binary fields.

### 4.4 Render-channel inspection and auxiliary exports — P1

**GMT implementation.** `WorkerExporter.renderFrameImageSequence()` selects beauty, alpha and depth passes through `uOutputPass`. The inspected path stores byte buffers for PNG/JPG-oriented output. This is not evidence of a general floating-point EXR AOV exporter.

**3D-to-2D adaptation.** Separate meaningful RFF_Super outputs: final image, palette-only color, smoothed iteration values, interior/coverage mask, pseudo-height, pseudo-normal, material mask and emission. Start with a few diagnostic views; add file export where external compositing provides value.

**Why it helps.** A user can tell whether a line or patch originates in iteration computation, palette mapping, relief normals, or post-processing. Artists can composite glow and relight a height-derived surface externally.

**Constraints.** Iteration count is not camera depth. Label a derived height pass “pseudo-height” and document its mapping and range. Preserve numerical channels in an appropriate high-precision representation rather than encoding them through a display tone curve. Distinguish antialiased coverage from a binary interior mask. Use the same underlying evaluated fields for beauty and auxiliary passes.

**Acceptance criteria.** All outputs share framing and sample positions; pseudo-normals match the lit image; invalid/interior pixels have documented values; no hidden recomputation with different settings; exported channel precision is explicit.

### 4.5 Advanced gradient authoring — P2

**GMT implementation.** `channelCurve.ts` converts ramps into channel tracks, including Bezier and stepped forms. `waveGen.ts` provides waveform editing. `paintRamp.ts` defines paint, smudge, soften, sharpen, tone, clone and restore tools. `reduceStops.ts` searches fewer-stop representations and measures worst sampled OKLab error; `gradientSeam.ts` preserves a ramp when fitting would require too many stops.

**2D transfer.** Add an optional expanded palette editor with lightness/chroma or channel curves, direct painting on the palette strip, controlled periodic wave generation, and “Simplify stops” with a before/after preview.

**Constraints.** Stop reduction is deliberately approximate. Keep it user-invoked, reversible, and separate from saving or loading. GMT's tests measure a finite 256-sample grid; that is not a continuous-domain guarantee and can miss narrower features at higher target resolution. Validate at the actual RFF palette sampling resolution and preserve step boundaries. An edited recipe becomes explicit authored palette data rather than silently changing the recipe generator.

**Acceptance criteria.** Undo covers one gesture as one action; wraparound is intentional; an unchanged palette is unchanged after opening and closing the editor; reduction reports the actual measured error; preview and renderer agree on interpolation space.

### 4.6 Audio-reactive appearance and reusable modulation — P2

**GMT implementation.** `AudioAnalysisEngine` separates transport from worklet-based analysis. `paramMapping.ts` composes modulation in the parameter's displayed control space. `applyAt.ts` supports applying modulation at a controlled time, and shared routing avoids separate live/export target logic.

**2D transfer.** Use bass/mid/treble energy or a selected frequency band to control palette phase, bloom, emission, texture intensity or lighting. Add LFOs for periodic motion independent of music. RFF_Super already plays and exports audio clips; the missing candidate is an analysis-to-parameter layer.

**Proposed implementation.** Begin with offline analysis of existing timeline audio and a restricted set of appearance-only targets. Cache analyzed envelopes by audio content, clip timing, analysis configuration and sample rate. Evaluate them from the timeline's authoritative time so seeking, holds and slow exports produce the same result. Provide gain, attack/release, bounds and an optional bake-to-track operation.

**Constraints.** Do not initially modulate precision, iteration limits, approximation thresholds or formula identity. GMT's controlled-time helper alone does not prove every live audio source exports deterministically. Live microphone input would require recorded input/envelopes for reproducible offline export.

**Acceptance criteria.** Preview and export sample the same envelope; seeking does not depend on prior playback history; holds continue appropriate time-based motion; disabled modulation restores base values; safety bounds prevent accidental invalid parameters.

### 4.7 Bezier animation and explicit interpolation spaces — P2

**GMT implementation.** `AnimationMath.interpolate()` handles explicit Bezier tangents and logarithmic value interpolation. Rotation can choose a shortest path. Its channel-curve editor reuses animation mathematics.

**2D transfer.** Add editable tangent handles to selected RFF_Super tracks, with per-parameter interpolation policy. A scale or frequency often benefits from ratio-based interpolation; exposure may be better represented in stops; angles need a user-visible distinction between shortest-path rotation and deliberate multiple turns.

**Constraints.** RFF_Super animation keys are tied to source-keyframe depth, while audio is timed in seconds and holds alter the schedule. Preserve that distinction. Existing Cubic interpolation is a neighboring-value Catmull–Rom calculation, not user-authored Bezier tangents. Add a new mode and versioned tangent data rather than changing what existing Cubic keys mean. Keep boolean/enum tracks stepped and explicit multi-turn rotation intact.

**Acceptance criteria.** Old timelines render unchanged; interpolation and handle drawing use the same coordinate space; held depths and speed edits behave predictably; overshoot and value clamping are visible and tested.

### 4.8 Material masks and shared surface evaluation — P2

**GMT implementation.** `getSurfaceMaterial()` assembles normal, albedo, emission and roughness for both direct lighting and path tracing. It reads two coloring layers and procedural noise. The material feature can select different emission sources.

**3D-to-2D adaptation.** Build on RFF_Super's existing pseudo-normal/GGX surface. Let a normalized field modulate roughness, metallic response, emission or layer opacity independently of palette color. Candidate fields include iteration bands, a texture, a procedural pattern or a curvature-like measure derived from the relief.

For example, a single fractal can have matte valleys, narrow glossy ridges and emitting bands while retaining its current palette. These are **2.5D material effects**, not physically reconstructed 3D geometry.

**Proposed implementation.** Introduce a small shared surface-data structure and reusable field/mask evaluation before expanding controls. Make mask origin and scale explicit: screen coordinates, fractal coordinates or iteration bands. Align zoom behavior and animation semantics with the existing texture/pattern/warp system.

**Constraints.** Avoid duplicating the already implemented GGX/rim/AO features. Do not assume GMT's constants are suitable for RFF's relief scale. Additional shader data must match C++ descriptors/push constants exactly. Derivative estimates must remain stable across tile/band boundaries and changes of resolution.

**Acceptance criteria.** Neutral masks reproduce the current image; beauty and debug normal outputs agree; no new band seams; roughness/emission bounds and color-space assumptions are documented.

### 4.9 Responsive rendering with unchanged mathematics — P2

**GMT implementation.** `BandScheduler` distributes horizontal bands in center-out order. All bands in a complete pass use the same sample weight. A requested band-count change is applied at a pass boundary. `BucketRunner` separately handles large-image tiles and GPU buckets, with post-processing per image tile. `InteractionSessionMachine` tracks gestures separately from rendering dirtiness.

**2D transfer.** Extend the existing GPU perturbation batch budget and video submission splitting with measured work budgeting and explicit states: interacting, waiting for exact calculation, updating appearance, exporting and complete. These ideas can improve UI responsiveness without reducing arithmetic precision.

**Constraints.** RFF_Super already calls `sizer.adapt`; performance improvement must be demonstrated against it. A scheduler cannot shorten an indivisible expensive operation merely by assigning fewer rows. The longest command, transfer and synchronization cost must be measured. Keep presentation resolution, sample count and fractal arithmetic precision as separate controls.

For large still export, verify the actual existing path before implementing a new one. GMT's two-level design is a useful reference, but it still allocates a per-image-tile HDR composite. Independent image-tile post-processing requires halos or a global pass for bloom, blur, normals and other neighborhood effects; otherwise seams are possible.

**Acceptance criteria.** Compare interaction latency, worst GPU submission duration and total render time; exact output remains unchanged under different work partitions; cancellation is prompt; global pixel coordinates and time remain stable across tiles; no falsely complete partial result.

### 4.10 Histogram-guided color mapping — P2

**GMT implementation.** Coloring declares histogram controls, and `WorkerHistogram.handleHistogramReadback()` samples geometry/color information through a 128×128 readback. This is an interactive diagnostic approximation.

**2D transfer.** Display an iteration-value distribution beside the palette, with draggable mapping endpoints or percentile suggestions. Separately show output luminance for exposure/grade adjustment. A luminance histogram and an iteration histogram answer different questions and should be labeled separately.

**Constraints.** Exclude or separately classify interiors and unfinished pixels. Extremely large iteration counts need a sensible plotting transform. Per-frame automatic normalization can cause video flicker; offer a locked distribution or sequence-wide mapping. RFF's existing shading-rate histogram is not a user-facing color histogram and should not be repurposed as if it were one.

**Acceptance criteria.** Histogram sample scope is disclosed; locked mappings are temporally stable; no blocking full-resolution readback during dragging; automatic suggestions never silently alter saved palettes.

### 4.11 Geometric orbit traps — P3

**GMT implementation.** Coloring exposes a geometric orbit trap; `DE_MASTER` provides per-iteration injection and separates geometry-only evaluation from coloring work. Trap data can influence coloring independently of the surface's final position.

**3D-to-2D adaptation.** A plane, sphere or point trap in 3D becomes a line, circle, cross or point in the complex plane. Record a statistic over the orbit, then use it as a palette coordinate or mask. This provides genuinely different coloring from an escape-iteration palette or a post-process pattern.

**Critical mathematical constraint.** MPA skips intermediate iterates. A minimum trap distance over the entire orbit cannot generally be reconstructed from the skipped interval's endpoints. Keeping MPA active while sampling only visited iterates would change the definition of the coloring. Start with a restricted direct-iteration mode, or derive and validate suitable block statistics/bounds for explicitly supported traps. Do not present an approximate trap as exact.

**Data constraint.** Existing iteration-only maps cannot reconstruct missing orbit history. Additional channels require a new optional data representation or sidecar with explicit compatibility. Never redefine an existing RFMZ stream mode or make video keyframes lossy.

**Acceptance criteria.** Match a direct high-precision orbit baseline at selected pixels, including off-axis views; disclose which acceleration modes are supported; the disabled feature leaves runtime cost and output essentially unchanged.

### 4.12 Structured formula presets and formula sequences — P3

**GMT implementation.** The formula registry, capability system, Workshop importer and `weave/schedule.ts` support structured formula composition. `buildCountsPlan` and `buildBlockPlan` represent introductory segments and repeating sequences. The Fragmentarium V3 adapter explicitly rejects sources without a usable DE function; arbitrary 2D shaders are not automatically importable.

**2D transfer.** Introduce parameterized 2D formula presets with named controls and validated ranges, then consider a restricted sequence editor such as repeated blocks of supported complex maps. A separate Julia/Mandelbrot paired explorer is another useful direction; GMT's shared 2D reference code already distinguishes their initial conditions.

**Constraints.** RFF_Super's inspected custom path evaluates a parsed expression with `std::complex<double>` and a dummy reference. A new formula UI does not grant deep-zoom perturbation or MPA support. Alternating maps, history-dependent formulas and nonanalytic folds need their own recurrence/derivative/error analysis. Keep the mature Mandelbrot path separate and explicitly advertise shallow/custom limitations.

**Acceptance criteria.** Saved formula identity and parameters reproduce a scene; unsupported capabilities are explained; no automatic assumption that the Mandelbrot period/MPA logic applies; syntax failure is clearly distinguished from a legitimate interior image.

### 4.13 Relief mesh export — P3, optional 2.5D branch

**GMT implementation.** The mesh pipeline samples dense or sparse fields, builds meshes, applies filters/post-processing, and exports through GLB/STL and VDB-related code paths. These are implemented source paths, not mesh-quality guarantees from this review.

**3D-to-2D adaptation.** Export a bounded relief from an existing 2D scalar field. A regular-grid heightfield mesh is a smaller first step than porting 3D dual contouring: map iteration-derived height to vertices, add a base and boundary walls, and optionally bake vertex colors or a texture.

**Constraints.** A heightfield is an artistic interpretation of a Mandelbrot image. Iteration values are not a signed distance field. Interior heights, steep steps, edge closure, minimum feature size, physical units, holes and memory use all need explicit handling. Geometry simplification changes the exported artifact and should be user-controlled.

**Acceptance criteria.** Watertightness for print-oriented export; finite vertices/normals; stated dimensions; predictable interior mapping; preview before export; documented error after simplification. Leave volumetric VDB and full 3D fractal meshing for a separate project decision.

## 5. Explicit 3D → 2D transfer matrix

| GMT 3D feature / technique | Useful 2D equivalent | Transfer verdict | Main limitation |
|---|---|---|---|
| Two coloring layers and procedural material inputs | Independent palette/mask fields | Strong, with adaptation | Avoid duplicate or inconsistent existing layer semantics. |
| Roughness, emission and reflection controls | Masks on RFF's existing relief material | Strong 2.5D adaptation | Not true volumetric geometry or transport. |
| Surface normals | Stable pseudo-normal and normal-map export | Strong, partly existing | Derived from a chosen height field, not a 3D fractal surface. |
| Geometric orbit traps | Point/line/circle traps in complex orbits | Strong artistic potential | Skipped MPA intervals cannot be ignored. |
| HDR environment lighting | Environment-driven reflections on pseudo-normals | Possible extension | First compare existing studio/environment-style shading; requires stable filtering. |
| Beauty/alpha/depth passes | Beauty, coverage, iteration, pseudo-height/normal | Strong workflow transfer | Do not label iterations as physical depth. |
| Center-out progressive bands | Bounded exact computation/shading work | Strong, existing overlap | Need real timing evidence and correct completion state. |
| Bucket rendering | Memory-bounded still export | Strong architecture transfer | Halos/global processing are necessary for neighborhood effects. |
| Formula weaving | Restricted 2D sequence presets | Possible research feature | No automatic deep-zoom support. |
| 3D navigation/picking | Pick a palette band, trap origin or material mask source | Selective interaction transfer | Full fly/orbit camera is unnecessary for a planar renderer. |
| 3D mesh generation | Heightfield relief export | Useful optional extension | Heightfield does not reproduce a true 3D fractal. |
| Soft shadows and AO | Existing relief AO; optional heightfield shadows | Mostly existing or 2.5D | Occlusion is only as meaningful as the chosen relief. |
| Depth of field | Focus/blur based on pseudo-height | Optional artistic effect | No unique physically correct depth in a 2D fractal. |
| Full path tracing / VNDF bounce sampling | Little benefit to ordinary flat palette rendering | Defer | Requires geometry, visibility and transport beyond existing 2D needs. |
| Volumetric scattering / water intersections | Stylized fog or overlays | Weak direct transfer | RFF already has fog/effects; real volume integration is a different renderer. |
| Distance-estimator raymarching | No direct replacement for pixel escape iteration | Do not transplant | Solves a different geometric problem. |

Gradient editing, PNG scene recovery, audio modulation, animation mathematics and capability metadata are dimension-independent and can transfer without adding a 3D renderer.

## 6. Improvement opportunities found in GMT itself

These findings also identify behavior that should not be copied into RFF_Super.

### A. PNG metadata parser hardening — confirmed limited probes + static analysis

`extractMetadata()` checks only the first two signature bytes, does not verify the metadata CRC, and reads chunk lengths with signed bitwise arithmetic. The first two behaviors were reproduced against the actual function. For length bytes `FF FF FF F4`, the signed length is `-12`, so `pos += 12 + len` does not advance. The arithmetic was checked; the intentionally nonterminating full parser case was not executed.

Both read and write traversal should reject malformed chunks, lengths outside the remaining buffer, missing terminators and unsupported compressed text. A valid round trip does not prove robust handling of imported images.

### B. Share serialization mutates its argument — source-confirmed behavior

`generateShareStringFromPreset()` deletes quality fields, and optionally animation fields, directly from the supplied preset. Callers that pass a detached copy may be safe; this review did not demonstrate corruption of the live UI store. Nevertheless, a serializer with this behavior is easy to misuse. Build a detached share payload before removing nonportable fields, and test that the input object remains unchanged.

### C. Compilation completion is not compilation success — API improvement

`CompileScheduler.awaitCompile()` resolves when `IS_COMPILING` becomes false. The failure handler also emits that state after `COMPILE_FAILED`. The file documents that `hasCompiledShader` is not a reliable last-success flag. Prefer a result tied to a compilation generation, with explicit success/failure and diagnostic information. A spinner stopping must not be sufficient proof that a new shader is usable.

This is an API-semantic observation, not a reproduced claim that the current application always uses the result incorrectly.

### D. Environment filtering coverage and stale commentary — corrected after wider tracing

`sampleEnvBicubic()` uses explicit LOD for its bicubic taps, avoiding derivatives of discontinuous reconstructed coordinates. Its introductory comments still describe missing minification information, but the wider implementation already contains `envConeLod()` and combines it with roughness using `max(...)`. `reflEnvShading(coneAA)` and `reflRaymarchShading(coneAA)` can supply a normal-variation-based reflected cone. The earlier, narrower review overstated the missing mechanism; it is **already implemented as an optional reflection-filtering path**.

The remaining opportunity is to reconcile the stale comments and test which call paths supply meaningful cone information, especially direct versus path-traced reflections, near mirrors, HDR maps and animation. Do not recommend adding a mechanism without first tracing both its producer and its consumer. No visual filtering defect was reproduced here.

### E. Preserve ancestry when editing palettes — design improvement

`catalogOrigin.ts` appends export credit only while the current content key matches the stamped original. Editing a stop causes the original credit to disappear from the export name. That is useful for identifying an exact catalogue match, but is not a complete derivation/provenance model. Preserve immutable source ancestry separately from whether an item is unchanged, and retain it in formats or sidecars that can carry it.

### F. Make test coverage match the actual application path — documented gap

Subsystem rules identify several misleading test names: some smokes boot sibling apps, not `app-gmt`; a palette-core test does not validate the palette UI; historical hybrid modes can produce identical shaders. These are documented findings, not results rerun here. Keep a maintained matrix of entry point → renderer → feature → assertions. Add rendered-image assertions to distinguish a correct scene from a blank but technically successful render.

### G. Refresh onboarding and implementation-status summaries

Generate formula counts from the registry or omit the count. Remove obsolete path instructions from the primary onboarding route while retaining historical records. Separate implemented, experimental, known-limited and planned items in feature summaries. RFF_Super should follow the same discipline in its own user guides.

## 7. Precision, memory and compatibility requirements

### Preserve the deep-zoom contract

- Keep exact coordinate storage and GMP-based calculations. A JavaScript number, float32 texture or double-double view center must not become the new storage precision limit.
- GMT's shared `engine/fractal/deepZoom` code uses a high-precision reference builder but float32-packed orbit output and a double-double view-center interface. Its rules identify a center-precision wall around a view scale of `1e-30`; that is not the capability target for RFF_Super.
- `app-gmt` uses the separate `engine-gmt` renderer; the shared 2D deep-zoom path is reached by sibling tools. Do not describe the 3D app as if it uses that 2D pipeline everywhere.
- Preserve RFF's FPG, periodic MPA and reference compression. Reference selection diagnostics and off-axis tests are transferable; acceleration mathematics requires independent review.
- Test non-real-axis orbits. The GMT LA test's own history explains why real-axis-only fixtures can hide imaginary-part sign errors.

### Bound additional memory

At 3840×2160, one extra 32-bit scalar image is approximately **31.6 MiB**; four such channels are approximately **126.6 MiB**, before padding, supersampling, staging, history buffers or frames in flight. A four-component float image alone costs that latter amount. Allocate optional channels only when required, reuse evaluated results, and define an explicit memory budget before adding a rich surface buffer.

These are buffer-size calculations, not measurements of either renderer's actual allocation.

### Preserve saved work

- **Palette recipes:** do not alter existing generation order/RNG behavior or reuse IDs. Store authored/imported palettes explicitly, or introduce a new recipe ID where appropriate.
- **RFC:** follow the existing append-only trailing-field convention, EOF guards, validation and default restoration. Do not reorganize the binary stream to match a new parameter schema.
- **RFSP:** retain existing appearance-preset behavior; add new optional appearance data through its established compatibility approach.
- **RFMZ:** preserve existing lossless modes and stream-size accounting. New orbit/material data needs a separately defined representation, not hidden reinterpretation of old maps.
- **Timeline:** preserve parameter IDs and existing interpolation meanings. New Bezier tangents and audio envelopes need explicit serialization and migration design.
- **Shader interfaces:** C++ and GLSL layouts must match. Include capability fallbacks and disabled-feature parity checks.

## 8. Licensing and provenance findings

The downloaded root `LICENSE` contains GPL version 3. RFF_Super also identifies GPLv3 as its project license. This is useful initial inventory information, **not a file-by-file clearance to copy all bundled code, formulas, data or assets**.

GMT contains heterogeneous provenance: palette collections, imported Fragmentarium examples, MB3D-derived implementation comments, and named sampling algorithms. `vndf.ts`, for example, cites several publications and a basis-construction method; a citation is not a substitute for verifying the actual source and license of code being adapted.

The palette licensing plan's later **2026-09-23 owner decision** is especially relevant. It records per-source credits and excluded/nondefault collections, but also retains some families whose terms were not fully verified. Those are the source project's recorded choices, not approvals transferable to RFF_Super. The report's opening historical complaints should not be treated as an unchanged inventory, either.

Recommended adoption method:

1. Prefer independently implementing the user-facing concept in the existing C++ architecture.
2. For adapted code or distinctive constant sets, inspect the exact originating file, upstream source and license first.
3. Retain relevant notices and add the required dated modification tag; update RFF_Super's `NOTICE` for named algorithms and third-party-derived implementations.
4. Audit palette assets separately from palette-editor code. Do not bulk-import GMT's catalogue or reference shader collection.
5. Keep source ancestry after user edits instead of using “unmodified” as the sole provenance condition.

No new third-party formulas, palettes or shader implementations were copied into RFF_Super by this review. Upstream licensing terms were not independently re-researched on the web; unresolved origins remain adoption prerequisites rather than conclusions of this report.

## 9. Suggested delivery sequence

### Phase 1 — useful additions around existing computation

Deliver image-derived palettes/basic interchange, validated PNG settings recovery, capability explanations, and a small diagnostic-view selector. Reuse existing undo, palette, settings and rendering infrastructure. Establish old-file fixtures and representative visual baselines first.

### Phase 2 — stronger authoring and animation

Add gradient curves/painting, material masks, histogram-guided mapping and selected high-precision auxiliary exports. Introduce Bezier tracks and offline audio modulation only after their time/serialization contracts are explicit.

### Phase 3 — optional mathematical or 2.5D expansion

Prototype orbit traps without silently skipping required orbit samples. Evaluate structured custom formulas as a separate supported mode. Build a simple closed relief mesh exporter before considering any 3D volumetric meshing.

### Defer until there is an explicit product requirement

Full 3D navigation, a distance-estimator renderer, path tracing, water intersections and volumetric transport would expand the application substantially. They are poor prerequisites for the directly useful 2D improvements above.

## 10. Verification performed and future acceptance suite

The local isolated probes load the actual self-contained GMT TypeScript modules using Node 24's type stripping, without installing the application or altering the downloaded source.

| Probe | Result | What it establishes |
|---|---|---|
| Four-band pass followed by a requested three-band layout | Passed | Change takes effect at a pass boundary; sampled bands cover the normalized range and use consistent weights. |
| PNG metadata with Unicode and a long decimal-string coordinate | Passed | The tested metadata helper round-trips those strings. It does not prove an entire GMT/RFF scene round trip. |
| Invalid PNG signature with the first two bytes retained | Accepted by reader | Confirmed validation gap. |
| Corrupted iTXt CRC | Accepted by reader | Confirmed validation gap. |
| Signed length `0xfffffff4` | Evaluates to `-12` | Static arithmetic confirms the possible non-advancing traversal. The hanging parser call was not run. |

The final probe run passed its assertions. During harness development, an exact-equality assertion on the sum of thirds was corrected to a one-ULP tolerance, and a malformed embedded image fixture was replaced with the existing valid project icon. Those were test-harness issues, not GMT regressions.

Future implementation validation should include:

- Saved-file round trips with old RFC/RFSP/RFMZ/timeline fixtures and recipe identity checks.
- Pixel/iteration comparisons at exterior, boundary, interior and deep off-axis views.
- Disabled-feature parity and correct shader interfaces on representative GPU capabilities.
- Whole-image versus partitioned output comparisons, including bloom/blur/normal edges.
- Timeline preview/export equality at explicit times, around holds, after seeking, and at different export rates.
- Adversarial metadata/import inputs with bounded processing and no partial state application.
- Per-subsystem tests plus end-to-end image-content assertions; a green boot/compile result alone is insufficient.

## 11. Source evidence index

Paths below identify the reviewed files. Symbols and search phrases are included so the evidence remains findable after line numbers move. Local GMT links point to the supplied download and will need adjustment if it is moved.

### GMT documentation and architecture

| Source | Relevant evidence |
|---|---|
| [README](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/README.md), [app README](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/app-gmt/README.md) | App separation; historical layout and formula-count claims. |
| [Documentation index](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/docs/DOCS_INDEX.md), [codebase map](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/CODEBASE_MAP.md) | Authority model and directory structure. |
| [FeatureSystem](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine/FeatureSystem.ts) | `ParamType`, `ScaleType`, registry assumptions. |
| [Compatibility evaluation](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/engine/compat/evaluateCompat.ts) | `evaluateCompat`, requirement/rejection reasons. |
| [Interaction state](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine/InteractionSessionMachine.ts) | `InteractionSessionState`, gesture/render-intent separation. |

### GMT rendering and 3D transfer

| Source | Relevant evidence |
|---|---|
| [BandScheduler](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/engine/BandScheduler.ts) | `next`, `setBandCount`, `resumeFrom`. |
| [Tiling plan/status](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/plans/tiled-progressive-rendering.md) | Later “BUILT, with follow-ups open” qualification. |
| [BucketRunner](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine/export/BucketRunner.ts), [GmtBucketHost](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/engine/GmtBucketHost.ts) | `start`, `beginImageTile`, `beginGpuBucket`, full-output coordinates. |
| [WorkerExporter](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/engine/worker/WorkerExporter.ts) | `renderFrameImageSequence`, `uOutputPass`, `EXPORT_HEARTBEAT_MS`. |
| [WorkerHistogram](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/engine/worker/WorkerHistogram.ts) | `HISTOGRAM_SIZE`, `handleHistogramReadback`. |
| [CompileScheduler](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/engine/CompileScheduler.ts) | `awaitCompile`, `COMPILE_FAILED`, `hasCompiledShader`, `keepCurrent`. |
| [Surface evaluation](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/material_eval.ts), [material feature](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/materials.ts) | `getSurfaceMaterial`, `MATERIAL_LOGIC`, `emissionMode`. |
| [Coloring feature](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/coloring/index.ts), [DE kernel](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/de.ts) | `trapShape`, `perIterInject`, `DE_MASTER`, geometry/color separation. |
| [Environment sampling](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/lighting/env.ts) | `sampleEnvBicubic`, roughness versus footprint commentary. |
| [Path tracer](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/pathtracer.ts), [VNDF sampler](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/vndf.ts) | `getPathTracerGLSL`, `getVNDFSamplerGLSL`, named algorithm provenance. |
| [Mesh pipeline](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/mesh-export/pipeline/mesh-pipeline.ts) | `runMeshPipeline`, dense/sparse sampling, mesh writer imports. |

### GMT palettes, animation and persistence

| Source | Relevant evidence |
|---|---|
| [Image extraction](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/palette/core/img2grad/index.ts) | `extract`, `IMG2GRAD_MODES`. |
| [Gradient imports](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/palette/core/importFormats.ts) | `IMPORT_EXTENSIONS`, `parseGradientText`. |
| [Gradient seam](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/palette/core/gradientSeam.ts), [stop reduction](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/palette/core/reduceStops.ts) | `entryToGradientConfig`, sampled error and ramp preservation. |
| [Channel curves](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/palette/core/channelCurve.ts), [paint tools](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/palette/core/paintRamp.ts), [wave generator](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/palette/core/waveGen.ts) | `rampToBezierTrack`, `PAINT_TOOLS`, `WAVE_SHAPES`. |
| [Animation mathematics](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine/math/AnimationMath.ts) | `interpolate`, `isLog`, tangent handling. |
| [Audio facade](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine/features/audioMod/AudioAnalysisEngine.ts), [modulation timing](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine/features/modulation/applyAt.ts), [parameter mapping](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine/features/modulation/paramMapping.ts) | Transport/analysis split, `applyModulationsAt`, displayed-value mappings. |
| [Scene format](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/utils/SceneFormat.ts), [PNG metadata](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/utils/pngMetadata.ts), [sharing](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/utils/Sharing.ts) | `SCENE_METADATA_KEY`, `injectMetadata`, `extractMetadata`, `generateShareStringFromPreset`. |

### GMT mathematics, coverage and provenance

| Source | Relevant evidence |
|---|---|
| [Reference builder](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine/fractal/deepZoom/referenceOrbit.ts), [nucleus search](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine/fractal/deepZoom/nucleus.ts) | `computeReferenceOrbit`, `RefOrbitInput`, `detectPeriod`, `newtonNucleus`. |
| [Deep-zoom rules](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/.claude/rules/deep-zoom.md), [LA test](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/debug/smoke-deep-zoom-la.mts) | Actual app reachability, center precision, off-axis and coverage-floor tests. |
| [Weave schedules](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/engine/weave/schedule.ts), [formula registry list](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/index.ts), [import compatibility](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/fragmentarium_import/v3/compat.ts) | `buildCountsPlan`, `buildBlockPlan`, `detectFormulaV3`. |
| [Renderer rules](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/.claude/rules/gmt-renderer.md), [palette rules](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/.claude/rules/palette.md), [modulation rules](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/.claude/rules/modulation.md) | Coverage limitations and shared routing contracts. |
| [Root license](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/LICENSE), [palette licensing history](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/plans/palette-catalogue-licensing.md), [catalogue origin](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/palette/core/catalogOrigin.ts) | Supplied license text, later owner decision, `unmodifiedOrigin`, `exportNameFor`. |

### RFF_Super comparison anchors

| Source | Relevant evidence |
|---|---|
| [RenderScene](../src/rff2/ui/RenderScene.cpp) | `applyCreateImage`, `CENTERED_REFERENCE`, computation diagnostics. |
| [Renderer](../src/rff2/ui/RenderSceneRenderer.hpp), [GPU perturbator](../src/rff2/vulkan/GpuLightPerturbator.cpp), [video export settings](../src/rff2/attr/VidExportAttribute.h) | Pass timing, `sizer.adapt`, `gpuSubmitSplit`. |
| [Palette sources](../src/rff2/ui/workspace/PaletteSources.cpp), [palette attributes](../src/rff2/attr/ShdPaletteAttribute.h) | `readPaletteSource`, recipe identity, `blendPaletteColors`. |
| [Timeline evaluator](../src/rff2/video/TimelineEvaluator.cpp), [parameter descriptors](../src/rff2/video/TimelineParams.hpp), [animation guide](../guide/animation-and-export.md) | Existing interpolation, dirty masks, holds/audio/source semantics. |
| [Custom formula implementation](../src/rff2/formula/CustomFormulaPerturbator.cpp) | `iterate`, standard-double expression evaluation. |
| [Slope shader](../shdsrc/vk_slope.frag), [GGX shading](../shdsrc/studio_ggx.glsl), [relief](../shdsrc/lustre_relief.glsl) | Existing material/normal/AO/specular behavior. |
| [Config compatibility](notes/config-file-format.md), [palette recipe stability](notes/palette-recipe-stability.md), [compressed maps](notes/compressed-map-format.md), [NOTICE](../NOTICE) | Constraints on implementation and provenance. |

## 12. Expanded all-feature coverage and additional opportunities

### 12.1 Coverage ledger: every registered GMT feature

The declaration-level controls for each row are reproduced in the [companion catalogue](GMT_Fractals_Source_Inventory_EN.md). “Existing overlap” means additional benefit must be demonstrated against RFF_Super's inspected implementation. It does not certify equivalence between two renderers.

| Registered feature | What the feature contributes | 2D/RFF_Super assessment and recommendation |
|---|---|---|
| `CoreMathFeature` | Iteration limit and formula-dependent scalar/vector parameter slots | Preserve typed, named parameters and validate bounds. RFF's iteration/precision semantics remain authoritative; no transplant of GMT's small 3D iteration budget. See 4.3/4.12. |
| `GeometryFeature` | Julia/offset mode, per-iteration pre/post rotations, world rotation, absolute-coordinate burning mix and coordinate picking | Separate view transforms from changes to the iteration map. A 2D rotation of the view is already supported; a burning/fold formula requires new mathematics. A paired Julia parameter picker is useful as a separate mode. |
| `WeaveFeature` | Formula-slot banks, enable state and runtime timing for composition | Supports the restricted sequence proposal in 4.12. Structural rebuilds must preserve parameter identity, values and animation targets. |
| `LightingFeature` | Light lists, direct/PT modes, direct BRDF selection, shadow settings, sphere-area-light integration, environment sampling, bounce controls and firefly limiting | RFF already has sophisticated artistic shading. Reuse light editing and material organization ideas; true multiple scattering/visibility is a 3D undertaking. Firefly limiting is biased radiance processing, not a precision correction. |
| `LightSpheresFeature` | Visible emitter spheres, ray intersections and fog-aware compositing | A visual light-position handle transfers; rendering actual emitter geometry is optional 2.5D/3D work. Do not confuse visible spheres with the separate physical area-light integrator. |
| `AOFeature` | Occlusion intensity/spread/tint, fixed/stochastic samples, runtime count and compile cap | Existing pseudo-AO overlaps. The useful transfer is an explicit quality/cost distinction and well-defined diffuse-only application, not a duplicate AO slider. |
| `ReflectionsFeature` | Environment or raymarched reflections, bounce shadows, roughness cutoff, accurate-color option and optional filtering | Environment filtering can improve a future extension of RFF's reflection model. Direct raymarching is one-bounce here; the old SSR mode is retired, despite a surviving “screen-space” description. |
| `AtmosphereFeature` | Distance fog, sky/custom tint, absorption and ray-accumulated surface/color glow | RFF fog and bloom overlap. 2D can use iteration/height masks for haze, explicitly artistic rather than geometric distance fog. Ray glow is not the same operation as image bloom. |
| `VolumetricFeature` | Single scattering with anisotropy, tint, height falloff, surface-color scattering, jitter and quality | Defer the actual volume integrator. A stylized height-mask haze or light-shaft overlay is possible, with different semantics and without claiming volumetric accuracy. |
| `MaterialFeature` | Diffuse/metallic/specular/roughness/rim controls, sky source, environment strength/rotation and selectable emission source | Strong overlap with RFF's existing surface. Independent material-channel masks and environment presentation are useful extensions; see 4.8. |
| `WaterPlaneFeature` | Intersecting water plane, surface color/roughness and animated waves | A separate 3D surface participates in distance and material evaluation. For 2D, only ripple/refraction/mask ideas transfer directly; do not call a post-warp a water-surface simulation. |
| `ColoringFeature` | Two gradient layers, multiple field mappings, blend modes, histograms, procedural noise/bump and geometric traps | Highest transfer relevance. Preserve independent scalar fields, perceptual editing and source capabilities. New endpoint-angle coloring already exists in concurrent RFF work; orbit-wide traps remain distinct. |
| `TexturingFeature` | Image input/profile, separate U and V field mappings, offset/scale | RFF already has texture layers. Independent mapping-field selection is useful if its existing modes cannot express a desired combination; depth/normal/trap inputs need actual source data. |
| `QualityFeature` | Ray-step budget, DE metric/estimator/bailout, step size/jitter, hit detail, numerical-DE controls, refinement, adaptive resolution and distance probe | Adopt clear cost/quality explanations and validation. Most numerical controls govern a raymarcher and must not be relabeled as Mandelbrot precision controls. |
| `DrosteFeature` | Recursive image remapping with radii, period, spiral arms, mirroring, rotation, zoom, transparency and smoothing | Direct 2D post-effect opportunity; see 12.4. It remaps finite image content and does not calculate new deep-zoom detail. |
| `PostEffectsFeature` | Bloom intensity/threshold/radius and chromatic aberration | Existing bloom/color effects overlap. Improve effect-space documentation, filtering, halos and temporal consistency before adding another post stack. |
| `ColorGradingFeature` | Tone-mapping choices, saturation, black/white points and gamma | RFF already supports grading/HDR paths. The opportunity is comparative previews and clearly specified transfer functions, not renaming an artistic approximation as a full standard. |
| `OpticsFeature` | Perspective, orthographic and skybox projection, FOV and focus blur | RFF planar/360 projection is already present. Pseudo-height focus blur is optional; 3D focus distance cannot be inferred uniquely from a 2D image. |
| `NavigationFeature` | Fly speed, automatic slowdown and cursor-anchored orbit/zoom | RFF already supports planar exploration. Reuse coherent anchoring and gesture state; collision-aware flying only matters if a 3D/relief viewer is added. |
| `CameraManagerFeature` | Saved view management, delegated to a state-library/camera slice | A thumbnail location library with exact coordinates, duplicate/update and selected recall scopes is a valuable 2D counterpart. See 12.5. |
| `AudioFeature` | Audio transport, multiple input/deck concepts and frequency analysis | Existing RFF clip playback is not frequency modulation. Offline envelopes and deterministic mapping are the high-value additions in 4.6. |
| `DrawingFeature` | Rectangles/circles, color, measurement labels, axes and a drawing plane | Adapt to annotations and pixel/fractal-coordinate rulers. Screen overlays must stay separate from beauty export unless explicitly included. See 12.6. |
| `RotationGizmoFeature` | On-canvas rotation handles, with window state kept outside saved scene parameters | Adapt to 2D rotation, light direction and trap placement handles. Keep display-only state out of saved fractal mathematics; one drag should be one undo action. |
| `ModulationFeature` | Rules/LFOs and common target routing | Strong dimension-independent transfer, conditional on reproducible time and parameter bounds. See 4.6/4.7. |
| `WebcamFeature` | Live DOM overlay with position, crop, opacity, blend and CRT-style display | Narrow value for tutorials/streaming; does not improve fractal rendering. Its overlay declaration alone does not prove inclusion in worker-exported video. Prefer optional presentation composition. |
| `DebugToolsFeature` | State/shader/interaction diagnostic overlays | Extend RFF's existing diagnostics to expose why a mode is unavailable and what work is active. Do not equate a debug menu with validated performance. |
| `ShaderCompilerFeature` | A host for the compiler panel; registered profiles own quality bundles | Useful separation of compile-time capabilities and runtime edits. RFF's Vulkan shader distribution differs; avoid unnecessary runtime compilation infrastructure. |

### 12.2 Palette registrations and the Gradient Explorer

| Feature/tool | Inspected functionality | 2D opportunity / constraint |
|---|---|---|
| `PaletteGeneratorFeature` | Generator parameters and modification controls | Add deterministic authored variants without altering old palette recipes; store the generated output or a newly versioned generator recipe. |
| `PaletteFiltersFeature` | Catalogue filtering, source and quality controls | Search by hue/lightness/chroma, use case and source. Catalogue ingestion is separately license-sensitive. |
| `PaletteImageFeature` | Image extraction controls | Direct transfer; 4.1. A source image is not automatically licensed for redistribution with a saved preset. |
| `PaletteEditorFeature` | Standalone stop-editor mode and document settings | Extend the existing RFF editor rather than introduce incompatible stop storage. |
| My Gradients/Favients and sets | Reusable gradients, groups, recent use, drag/drop and cross-app destinations | Good workflow model for local collections. Preserve stable IDs and ancestry when editing, moving or exporting. |
| Browse/Curves/Adjust/Paint/Image surfaces | Focused views for selecting and shaping a working gradient | Direct creative transfer; prioritize a compact integrated tool rather than duplicating the whole browser application. |
| Gradient previews | Linear, radial, conic and live fractal presentations in the geometry selector | Compare palettes on representative fractal regions. The fractal preview uses a separate renderer, not the pure CPU geometry sampler. |
| Ramp geometry, spline/liquify-related tooling | Source contains parametric field and geometry controls used by preview/editor tooling | Inspect the actual mounting path before claiming every historical geometry mode is exposed in the current v2 shell. Do not propose these as new fractal mathematics. |
| Export/interchange | Text/ramp/swatch subjects and DCC-related exporters | Choose a small supported format set and validate round trips. Stop-based formats can approximate sampled ramps; make that loss explicit. |
| Global contribution and sharing | Optional live source/community contribution and compact shared gradients | Local collections provide most immediate value without backend operations. A public service needs storage, moderation, privacy and ongoing maintenance. |
| Session and GMT round trip | Working-session capture/restore plus a way to visit the Explorer and return to GMT | Preserve working state and return location when opening an auxiliary editor; apply/cancel must be unambiguous. |

The companion catalogue records declared controls but deliberately does not assert that every historical helper is reachable from the current Explorer shell. The current v2 shell, not an old mode list, determines the product surface.

### 12.3 Functionality outside the feature registry

| Functional area | Source evidence / inspected behavior | Adoption decision |
|---|---|---|
| Formula catalogue | Current `formulas` array has 48 entries; seven `FOLD_LIST` inputs create extra formula definitions; runtime imports can add more | Preserve IDs/aliases and query the actual registry. Do not quote “42 formulas” as the current count. A static array count is not the runtime total. |
| Formula Workshop | `FormulaWorkshop.tsx`: source editing, V3/V4 analysis, parameter promotion/mapping, temporary preview, library access | A constrained 2D formula editor with named parameters and a safe preview is useful. Arbitrary GLSL DE import is a separate feature from expression parsing. |
| Modular graph | `GraphCompiler.ts`: topology/live-node selection and shader/uniform packing | A small mapping/material graph may be more useful than a general formula graph. Preserve shared slot allocation and meaningful errors; a graph UI alone does not validate mathematics. |
| MB3D import | `loadMB3DScene.ts`: parse → fused-hybrid emission → registration → scene load, with a ledger/reasons | Do not import a 3D scene into a 2D renderer as if it were equivalent. Transfer the explicit compatibility report and parameter-retargeting workflow. The inspected loader rejects unsupported external-code/mode cases. |
| GMF and scene files | Formula-aware scene state plus generic SceneIO transport | Transfer versioning, recovery and asset reporting. Do not adopt embedded shader execution automatically. |
| Timeline | Tracks, graph/dope-sheet editing, interpolation, key binding, audio and modulation | Improve RFF's existing timeline with specific missing operations; see 4.6/4.7 and DCC export below. |
| Still/bucket/video export | Different viewport, bucket and video paths; progress, cancellation, encoding and image-pass selection | Validate parity across paths. Tiling, convergence and accumulation are not interchangeable with exact orbit computation. |
| After Effects / FBX | `AfxRenderAdjunct.tsx`, `FbxRenderAdjunct.tsx`, shared `sampleScene` | Prefer a 2D transform/parameter sidecar first; FBX camera export only when the target motion has a meaningful 3D interpretation. |
| Saved views and thumbnails | `cameraSlice.ts`, state-library integration | Direct 2D location-bookmark transfer; separate camera/location recall from appearance recall. |
| Sky library | Registration of `SkyLibrary`; material image/gradient/solid sources | Optional environment asset browser; useful only with an environment-aware target shader. Preserve HDR data and missing-file behavior. |
| Undo/redo, shortcuts, panel composition | Engine plugins and feature/panel descriptors | Existing RFF infrastructure overlaps. Improve gesture grouping, scope and discoverability instead of porting framework code. |
| Help and tutorials | `Tutorial.tsx`, `app-gmt/tutorial/lessons.ts`: trigger-driven steps, highlighted targets, setup edits | Direct transfer for explaining calculation versus appearance versus export. Tutorial setup must be reversible. |
| Recovery / session saving | `Session.ts`: adapter/envelope validation, compact fallback; GMT's dirty-scene restore is a separate path | Improve restoration completeness and missing-asset reporting. Do not confuse Explorer automatic session restore with GMT scene restore. |
| Gallery / My Fractals / authentication | Current `submitGalleryItem.ts` uses signed-in JWT submission; `sharedScene.ts` supports unlisted anonymous links | Optional service rather than a prerequisite for file sharing. Older gallery README admin-token instructions are historical, not the current client contract. |
| Web updates / mobile / clean presentation | PWA update plugin, viewport and app shell | Transfer non-disruptive update/recovery behavior and clean presentation mode selectively. Browser service workers and touch layouts are not native Win32 requirements. |
| Runtime recovery | Compile-failure state, context-loss overlay and worker lifecycle | Provide explicit recoverable states and preserve last useful image/settings. A completed asynchronous action must be distinguished from a successful one. |

### 12.4 Additional scored opportunities

All scores use the Section 1 scale and estimate the stated scope, not a hypothetical full implementation of the originating 3D subsystem.

| ID | Proposed addition/improvement | Difficulty / 10 | Effectiveness / 10 | Why / scope |
|---|---|---|---|---|
| A01 | Droste / recursive spiral image remapping | 6.0 | 7.2 | Visually distinct and independent of orbit calculation; needs resampling, cyclic seams, alpha, animation and provenance work. |
| A02 | Exact-coordinate thumbnail location library | 5.2 | 8.2 | Frequently useful navigation/reuse improvement on top of existing settings saves. |
| A03 | On-canvas measurement and annotation tools | 4.8 | 6.6 | Helps comparison/tutorials; coordinate precision and export inclusion require clear semantics. |
| A04 | Guided first-render / surface / video lessons | 4.5 | 7.6 | Reduces confusion across existing workflows without new numerical code. |
| A05 | DCC-friendly 2D motion and parameter export | 6.8 | 6.8 | Useful for compositing users; timing, unit conversion and source-image coverage are substantial work. |
| A06 | Independent U/V data-field texture mappings | 6.3 | 7.1 | Richer textures; benefit depends on gaps in existing mapping modes and available scalar channels. |
| A07 | Environment asset browser and footprint-aware reflection extension | 7.0 | 6.7 | Useful for surface artists; overlaps existing studio shading and requires HDR/filtering validation. |
| A08 | Pseudo-height focus blur | 5.8 | 5.4 | Attractive optional effect; no physically unique depth and potential halo/artifact costs. |
| A09 | Paired Mandelbrot/Julia parameter explorer, initially bounded precision | 7.4 | 7.7 | Strong exploration value; explicit formula mode, coordinate handling and saved-state support needed. |
| A10 | Small scalar-field/material node graph | 8.0 | 6.9 | Powerful but introduces a new editor/compiler/storage system. Start with masks before a graph. |
| A11 | Deterministic fractal-guided particles | 7.8 | 6.3 | New animation styles from field gradients; requires simulation state, seeds, seeks and export determinism. |
| A12 | Full fractal-driven fluid simulation | 9.0 | 5.5 | Expensive simulation and export integration; useful to a smaller creative audience. |
| A13 | Local collection/asset packaging and restoration checks | 6.0 | 8.0 | Complements existing presets/recovery by reducing missing-texture/audio/source problems. |
| A14 | Hosted gallery / short share links | 8.4 | 5.8 | Sharing benefit, but servers, identity, moderation and retention add ongoing costs. File/PNG sharing first. |
| A15 | Webcam/presentation overlays | 4.5 | 3.8 | Relevant to demonstrations, with little improvement to fractal creation itself. |
| A16 | Quality-profile explanation and live cost diagnostics | 4.8 | 7.4 | Makes existing options understandable; estimates need real target-GPU evidence. |
| A17 | Better tone-map comparison and output-space preview | 5.2 | 6.9 | Helps existing SDR/HDR workflow; preserve actual transfer functions and avoid duplicate grading controls. |
| A18 | Optional height-mask haze/ripple effects | 5.7 | 5.2 | Artistic 2D reinterpretation of fog/water, with substantial overlap in existing effects. |

These are **18 additional scopes**, not 18 commitments. Some should be combined with the main proposals: A06/A07/A10 with material masks, A13 with PNG recovery, and A16 with capability metadata. Their effectiveness scores must not be summed; benefits overlap.

### 12.5 Implementation notes for the new high-value candidates

**A01 — Droste.** GMT's `droste/index.ts` injects a UV transform into the post-processing shader. `shader.ts` handles radius ratios, complex transforms, spirals, mirroring and a transparency mask. For RFF, implement a separate optional image-space stage; keep source iteration data untouched. Use the full output's coordinates for tiled renders, filter magnified regions, and define behavior at singularities and aspect-ratio changes. Its source explicitly says it was ported from Tom Beddard's Pixel Bender script, so the actual upstream terms must be resolved before copying that implementation. A conceptual independent implementation remains an option. Verify neutral-state parity, loop continuity, alpha behavior and repeatable animation.

**A02 — locations.** GMT's camera library captures a view, optics and thumbnail, with duplicate/update/reorder and a modified indicator. The 2D counterpart should store arbitrary-precision center strings, log zoom, rotation and formula identity. Offer “Location only,” “Appearance only” and “Both” recall scopes. RFF already saves locations/settings, so the incremental feature is indexed visual recall and comparison. Thumbnail generation should never silently replace the current location or serialize a reduced-precision preview center. Verify deep-coordinate round trips and one-step undo after recall.

**A03 — annotations.** Adapt the 3D drawing tool's rectangle/circle and ruler concepts to screen-pixel or complex-plane coordinates. Keep a screen annotation distinct from one anchored to a fractal location. Deep differences must use high-precision subtraction before display scaling. Store annotations separately from calculation settings; optional exported overlays should use the same final-resolution transform. Useful additions include comparison regions and labels, not just decorative marks.

**A04 — lessons.** GMT's lessons wait for actual actions/values and can prepare a known scene. RFF lessons could explain: calculate a view → change palette without recalculating → compare relief → save settings → create a short video. Use sample files and existing help links first. Record an undoable snapshot before lesson setup, make exit safe, and do not force a costly deep render as the first interaction. This is source-based workflow advice; no visual usability test was run.

**A05 — external animation.** `sampleScene()` samples a common neutral representation and rebases coordinates before AFX/FBX conversion. For RFF, export a documented frame/time table with local translation, rotation, log scale, selected appearance values and an exact reference-center string. Converting deep absolute coordinates directly to ordinary DCC doubles loses meaningful motion. Baked 2D motion only works within the detail and coverage of the supplied rendered footage; it cannot manufacture unsampled zoom detail. Verify frame endpoints, handedness, FPS, holds and matching footage dimensions. Executable export scripts should be user-invoked artifacts, not automatically run.

**A06/A07 — mapping and environments.** GMT separates U and V mapping sources and distinguishes environment brightness from sky visibility. RFF can expose a small explicit field selector if its current modes cannot express these combinations. Endpoint escape angle can be reused where available; a historical map without it must not fabricate it from iteration count. For environments, test both bright narrow sources and smooth skies, and ensure a filter path actually receives a footprint. A library browser alone does not supply a physically correct environment integrator.

**A09 — Julia exploration.** A linked parameter picker shows how choosing a Mandelbrot parameter changes a Julia view. Initial-condition differences matter: Mandelbrot varies `c` with `z₀ = 0`, while Julia fixes `c` and varies `z₀`. A shallow direct evaluator is an explicitly limited first delivery; arbitrary-depth Julia needs its own correct perturbation/reference/MPA analysis. Share palettes across the views without assuming identical orbit statistics. Keep saved coordinates/formula mode explicit.

**A11/A12 — dynamics.** The sibling fluid app supports fractal-derived forces, several source fields, collision masks, vorticity/dissipation, dye, brushes/particles and compositing. RFF could start with particles advected by a normalized smooth field. Clamp forces near singular gradients and define interior behavior. Repeatability requires fixed simulation steps, deterministic seeds, explicit starting state and cached checkpoints for seeking. Fluid simulation is a separate large step; changing only render timestamps is not enough to reconstruct a history-dependent state at an arbitrary frame.

**A13 — complete working collections.** GMT's generic session adapter validates an envelope and provides a compact fallback when storage fails, while the main GMT app has a different scene recovery route. RFF already has recovery; improve restoration of the working collection and asset manifest. Prefer portable relative asset references and content identifiers, with explicit missing-file resolution. Never silently call a settings-only file a self-contained project archive. Test partial writes, missing assets and old versions without modifying the original recovery copy.

### 12.6 Formula-family assessment, covering the full native array

Individual source links and declared parameters for **every array entry** appear in the companion inventory. The grouping below is for transfer decisions rather than a formal mathematical taxonomy; unusual formulas can combine several mechanisms.

| Native entries / generated family | Review interpretation | 2D transfer decision |
|---|---|---|
| Mandelbulb, Mandelbar3D, Buffalo, Bristorbrot, MakinBrot, Tetrabrot, MandelBolic | Power/coordinate-map and folding variants exposed through the 3D formula contract | Do not obtain a mathematically equivalent 2D formula merely by deleting a coordinate. Offer separately defined complex-plane maps and validate their recurrence. Covered by 4.12 (8.8 / 7.5). |
| JuliaMorph, Phoenix, SineJulia3D, Julia3D | Julia or history/transcendental variants | Julia parameter browsing is strong; history-dependent maps need additional state and distinct perturbation logic. Covered by A09 (7.4 / 7.7) and 4.12. |
| Julia3DLattes, Julia3DKucera, Julia3DZorich | Specialized 3D Julia constructions with extensive mathematical context | The Lattès source discusses stereographic/rational-map construction and limits of folded off-axis behavior. Study the actual map rather than treating the name as proof of ordinary Mandelbrot dynamics. Research only; no promised deep 2D port. |
| Quaternion | Four-component iteration and 4D rotation/slice controls | A 2D slice viewer is possible but is a different formula family, not a new coloring mode. Preserve all hidden state. Included in the high-difficulty formula scope. |
| AmazingBox, AmazingSurface, AmazingSurf, KaliBox, BoxBulb, MarbleMarcher | Box/surface/fold or combined transforms | A 2D fold-map playground is plausible. GMT/MB3D similarly named formulas are not necessarily identical; import mapping must report substitutions. |
| MixPinski, SierpinskiTetrahedron, MengerSponge, MengerAdvanced | IFS/fold families with repeated geometric structure | A separate 2D IFS/carpet mode can be useful, but requires a different renderer/model from Mandelbrot perturbation. Do not rebrand as a Mandelbrot optimization. |
| Kleinian, KleinianMobius, KleinianJos, PseudoKleinian, PseudoKleinian06, PseudoKleinianMod4, Apollonian | Inversion/Möbius/Kleinian-style families | 2D circle-inversion or Möbius exploration is plausible research. New bailout/estimator and singularity policies must be defined. |
| Dodecahedron, Octahedron, Icosahedron, Cuboctahedron, TruncatedIcosahedron, RhombicDodecahedron, Coxeter, RhombicTriacontahedron, GreatStellatedDodecahedron | Polyhedral/fold-and-cut-oriented entries | Low direct benefit to planar Mandelbrot rendering; transfer symmetry and mask design ideas, not the 3D solids wholesale. |
| Mandelorus, MandelMap, Borromean, Appell, MandelTerrain, Claude | Other specialized native maps/terrain/structures | Keep as individually named experimental domains. A rendered slice or height interpretation is not automatically equivalent to a complex-plane iteration. Prototype only with a concrete desired image. |
| Modular | A graph-assembled formula rather than a single fixed recurrence | Potentially useful architecture; a restricted field/material graph is a smaller and more directly useful starting point (A10: 8.0 / 6.9). |
| BoxFoldStandard, BoxFoldMirror, BoxFoldKali, BoxFoldTetra, BoxFoldOcta, BoxFoldIcosa, BoxFoldMenger | Seven generated fold definitions, with retired stable codes preserved separately | Preserve schedule/parameter identity and report unsupported legacy cases. A generic fold sequence still needs independent 2D mathematics. |

The formula family rows enumerate the 48 array names exactly once. Seven generated folds are separate. Legacy aliases and imported runtime formulas are not counted as new native algorithms. No claim is made that every shader variant has been visually or mathematically verified.

### 12.7 Companion apps and infrastructure: final disposition

- **Gradient Explorer:** directly useful authoring functionality; prioritize image extraction, editing and collections. Its own working-session model and preview renderer must not be confused with GMT's scene/renderer.
- **fluid-toy:** meaningful new 2D creative direction, with high simulation complexity. Reuse concepts selectively; full fluid integration is A12, not a small post-effect.
- **fractal-toy:** minimal main-thread Mandelbulb/Mandelbox renderer example. Useful as an architectural reference and isolated testbed; no unique reason to add it to RFF_Super as a second renderer.
- **mesh-export:** optional 2.5D/3D export workflow. Dense/sparse sampling, filtering, smoothing/welding and format writers require memory and topology validation. Source comments explicitly flag expensive per-vertex adjacency storage; do not assume “sparse” makes the full pipeline memory-safe.
- **demo:** verifies generic feature/panel/animation/save/undo plumbing. Transfer the idea of a small integration fixture; it is not an end-user fractal feature.
- **Browser backend/PWA infrastructure:** necessary to GMT's web distribution and community features, largely unnecessary to the native application's core. No accounts were accessed, files uploaded, services deployed or external messages sent during this review.

## 13. Numerical ratings for improvements to GMT itself

Unlike the RFF adoption scores, effectiveness here is for **GMT users/developers**. Keep the two columns of recommendations separate; a valuable GMT fix may have no benefit to RFF unless the component is ported.

| Finding from Section 6 | Difficulty / 10 | Effectiveness / 10 | Rationale |
|---|---|---|---|
| A. Harden PNG metadata traversal/validation | 4.0 | 8.7 | Small utility with a confirmed validation gap and an arithmetic path to nontermination; preserves imported-file reliability. |
| B. Make sharing serialization non-mutating | 2.5 | 6.8 | A localized payload-copying contract plus tests; live-state impact depends on caller ownership. |
| C. Return explicit per-generation compile results | 5.0 | 7.6 | Crosses scheduler/worker/caller boundaries; prevents “finished” from being treated as “successful.” |
| D. Reconcile filtering comments and test cone coverage | 4.8 | 6.4 | Filtering machinery already exists. Audit call-path coverage and rendered motion before changing math. |
| E. Retain palette derivation ancestry after edits | 4.5 | 7.5 | Requires collection/export schema handling; provenance is more complete than exact-match credit alone. |
| F. Align test names/coverage with actual app paths | 5.5 | 8.2 | Prevents misleading green tests and catches scene-content failures; includes substantive end-to-end work. |
| G. Refresh onboarding and generated status summaries | 2.8 | 6.3 | Low risk and recurring benefit; use registration-derived inventories instead of manual counts. |
| H. Clarify 3D versus 2D import support in the Workshop | 3.2 | 6.0 | The DE adapter rejects unsupported sources; explain those limits before users attempt a 2D shader import. |
| I. Unify memory budgeting across mesh stages | 6.5 | 7.2 | Dense samples, sparse blocks, adjacency, export buffers and temporary arrays all contribute; requires end-to-end accounting. |
| J. Clarify DOM-overlay versus exported-pixel capabilities | 3.8 | 6.2 | Drawing/webcam declarations mount DOM overlays; export availability needs a separately verified capability. |

## 14. Completion scope and reproducibility

The deliverables now comprise the detailed review and the separate all-registration/control/formula inventory. Together they cover main-app feature declarations, rendering and editing subsystems, nonregistry workflows, formula families and companion apps, with explicit 2D transfer decisions and numerical ratings for adoption/fix proposals.

The inventory is a **source coverage checklist**, not a substitute for the implementation tracing, limited probes and constraints in this report. The observations remain static/source-level except where Section 10 explicitly reports a probe. GPU performance, visual quality, backend behavior, unexamined imported examples and file-by-file license clearance remain unverified and must not be inferred from the coverage counts.

The source-level breadth is intentional: proposals with poor 2D value are included and explicitly deferred, rather than silently omitted. The ratings are comparative estimates and should be revised after a concrete prototype or measurement on RFF_Super's target hardware.
