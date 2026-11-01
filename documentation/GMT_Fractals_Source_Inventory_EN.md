<!-- Modified by GPT-6 on 2026-10-07, 2026-10-08. -->
<!-- Modified by Opus 5.5 on 2026-11-01. -->

# GMT Source Inventory and Control Catalogue

> **Final review entry point (2026-10-08):** [Improving RFF_Super with GMT as a reference](GMT_to_RFF_Review_EN.md). All 3,727 inventoried GMT files have completed inspection records and unchanged hashes. This companion preserves chronological evidence: earlier progress counts, pending review statements and provisional priorities describe their recording time, not current coverage. Later corrections and the final entry point govern recommendations. Source inspection does not establish runtime correctness or measured GPU benefit.

Date: 2026-10-07. Companion to [the detailed review](GMT_Fractals_Feature_Review_EN.md).

This mechanically extracted inventory anchors coverage to current registration code, not historical README counts. It lists every directly registered feature in the GMT registration function and palette registration seam, every entry in the native formula array, and all generated box-fold inputs. Labels include group names and option labels as well as controls; they are not a count of parameters. Dynamic/computed labels and imported helper descriptions may not be captured. This is a navigation and coverage artefact, not proof of runtime correctness or a mathematical audit of every formula.

## Registered features (31 registration calls)

### CoreMathFeature

Source: [engine-gmt/features/core_math.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/core_math.ts). Registration: [engine-gmt/features/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/index.ts).

**Labels and options found in the declaration:** Formula; Iterations; Param A; Param B; Param C; Param D; Param E; Param F; Vec2 A; Vec2 B; Vec2 C; Vec3 A; Vec3 B; Vec3 C; Vec4 A; Vec4 B; Vec4 C.

### GeometryFeature

Source: [engine-gmt/features/geometry/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/geometry/index.ts). Registration: [engine-gmt/features/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/index.ts).

**Labels and options found in the declaration:** Pick Coordinate; Geometry Modifiers; Geometry Engine; Enable Rotation; Burning Mode; Burning Active; Mix; Local Rotation; Pre Rotation X; Pre Rotation Y; Pre Rotation Z; Pre Rotation; Post Rotation X; Post Rotation Y; Post Rotation Z; Post Rotation; World Rotation X; World Rotation Y; World Rotation Z; World Rotation; Julia Mode; Julia X; Julia Y; Julia Z; Julia Coordinate.

**Declared behavior** (source descriptions; implementation limitations are discussed in the review):

- Master switch for geometry modifiers (Julia, Rotation, Burning).
- Compiles rotation matrix logic. Disable for speed.
- Applies absolute value to coordinates every iteration. Creates "Burning Ship" variations.
- Blend between original coordinates (0) and absolute-value coordinates (1).
- Replaces the iterating variable with a fixed coordinate, producing connected Julia set slices.

### WeaveFeature

Source: [engine-gmt/features/weave.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/weave.ts). Registration: [engine-gmt/features/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/index.ts).

**Labels and options found in the declaration:** Weave Active; Weave Live Timing; Weave Slot Banks.

**Declared behavior** (source descriptions; implementation limitations are discussed in the review):

- Whole-weave enable: off renders the base formula only (all layers dormant). Live — no recompile.

### LightingFeature

Source: [engine-gmt/features/lighting/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/lighting/index.ts). Registration: [engine-gmt/features/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/index.ts).

**Labels and options found in the declaration:** Light; Lighting Engine; Shadows; Light Engine; Path Tracing Core; Active Mode; Direct (Fast); Path Tracing (GI); Max Bounces; GI Strength; Specular Model; Blinn-Phong (Fast); Cook-Torrance (Quality); Shadow Engine; Shadow Quality; Lite (Fast); HQ (Robust); Soft Shadow Jitter; True Area Lights; Sample All Lights; Env Sampling; Off; Env MIS; Env MIS + IS; Sobol Bounce Sampling; Environment NEE (legacy); Firefly Clamp; Enable; Shadow Jitter; Opacity; Hardness; Quality (Steps); Bias; Light List.

**Declared behavior** (source descriptions; implementation limitations are discussed in the review):

- Per-light shadow tracing and softness.
- Master switch for lighting logic. Disabling provides stubs only.
- Compiles the Path Tracing module. Disable to reduce shader size.
- Switches between fast direct lighting and physically based Global Illumination.
- Recursion depth. Higher = Brighter interiors, Slower render.
- Artistic boost for bounced light intensity.
- BRDF model for direct lighting. Cook-Torrance is physically accurate, with negligible compile cost.
- Compiles the shadow raymarching loop. Disable to save ~5s compile time.
- Soft-shadow penumbra quality (runtime): Lite = fast step-floor, HQ = accurate IQ penumbra (~2x slower). Toggle via the HQ button next to Jitter.
- Deprecated — jitter capability is always compiled with shadows now. Use the runtime Jitter button (areaLights). Retained for scene back-compat.
- Physically-correct sphere area light integration (sphere-surface sampling + MIS). Required for lights with type=Sphere. Enable + change a light type to Sphere via the per-light menu. Costs ~1s compile + one extra sphere intersection per bounce.
- Evaluates every active light per bounce instead of one random light. Reduces shadow noise at the cost of N× more shadow rays.
- Direct env-map sampling with MIS for path-traced reflections. Env MIS handles broad skies; Env MIS + IS adds importance sampling for HDR maps with sun discs / concentrated lights. Replaces the older Environment NEE (which only handled diffuse).
- Low-discrepancy 2D sequence (Sobol) with per-pixel rotation for the bounce-direction seed. Measurably less variance on shiny surfaces; no effect on rough/diffuse-heavy scenes.
- Deprecated — kept so old scene files load without dropping state. Migrated to ptReflMode at boot.
- Clamps per-sample luminance to suppress bright firefly spikes. Lower = cleaner but slightly biased. Raise to effectively disable.
- Toggle shadow casting at runtime without recompiling.
- Stochastic soft-shadow jitter for Point/Directional lights (penumbra via accumulation). Off = sharp analytic shadows.
- How dark the shadowed regions become (1 = fully black).
- Higher values give crisper shadows; lower values give wider penumbras. Affects Point and Directional lights only — Sphere area lights derive shadow softness from physical sphere sampling.
- Quality vs Performance.
- Too low: acne. Too high: detached.

### LightSpheresFeature

Source: [engine-gmt/features/lighting/light_spheres.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/lighting/light_spheres.ts). Registration: [engine-gmt/features/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/index.ts).

**Labels and options found in the declaration:** Light Spheres.

**Declared behavior** (source descriptions; implementation limitations are discussed in the review):

- Compiles visible emitter sphere rendering for point lights with radius > 0.

### AOFeature

Source: [engine-gmt/features/ao/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/ao/index.ts). Registration: [engine-gmt/features/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/index.ts).

**Labels and options found in the declaration:** Ambient Occlusion; Spread; Samples; Stochastic Mode; AO Tint; Max Samples (Hard Cap); Load Stochastic Sampling; Enable AO.

**Declared behavior** (source descriptions; implementation limitations are discussed in the review):

- Darkens crevices where surfaces close in on themselves.
- Iterations per pixel. Runtime controlled.
- Switches between Fixed and Stochastic sampling at runtime.
- Black = classic darkening. Custom color = tinted occlusion in shadowed areas.
- Compile-time limit. Increasing this allows higher runtime samples but compiles slower.
- Compiles High-Quality noise logic into the shader.

### ReflectionsFeature

Source: [engine-gmt/features/reflections/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/reflections/index.ts). Registration: [engine-gmt/features/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/index.ts).

**Labels and options found in the declaration:** Reflection Tracing; Reflections; Reflection Method; Off; Environment Map; Raymarched (Quality); Bounce Shadows; Raymarch Mix; Roughness Cutoff; Trace Steps; Accurate Colors; Reflection Filtering; Enable Reflections.

**Declared behavior** (source descriptions; implementation limitations are discussed in the review):

- Screen-space reflection tracing for glossy surfaces.
- Reflection technique. Higher quality = longer compile time. Raymarched adds ~3s of compile.
- Compute shadows on reflected surfaces. Negligible extra compile time.
- Blends between Raymarched Reflections (1.0) and Environment Map (0.0).
- Surfaces rougher than this will skip raymarching to save performance.
- Precision of the reflection ray.
- Sample the true surface colour (orbit traps / colour smoothing) at reflected hits instead of the gradient default. Adds a little compile time.
- Filter the environment map by how fast the reflection sweeps across the surface, so mirrors on curved geometry stop aliasing bright highlights. Adds ~1.5s of compile and one extra normal estimate per pixel; the gain is mostly in the live preview, since accumulation already smooths much of it.

### AtmosphereFeature

Source: [engine-gmt/features/atmosphere/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/atmosphere/index.ts). Registration: [engine-gmt/features/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/index.ts).

**Labels and options found in the declaration:** Volumetric Glow; Fog; Glow; Enable Glow; Glow Algo; Accurate (Vector); Fast (Scalar); Fog Color; Fog Intensity; Fog Start; Fog End; Fog Tint; Fog Density; Glow Strength; Tightness; Glow Source; Surface; Color; Glow Color.

**Declared behavior** (source descriptions; implementation limitations are discussed in the review):

- Distance-based fog that fades the scene toward a colour.
- Soft halo emitted around bright surfaces.
- Vector accumulates color per-step. Scalar accumulates intensity only (faster).
- The custom fog colour (and the Solid sky colour). Fog fades toward this when Fog Tint is above 0.
- Master fog amount; fades distant geometry toward the fog colour.
- Distance where fog begins to appear.
- Distance where fog reaches full opacity.
- Blends the fog colour away from the sky toward the custom Fog Color below. 0 = fog matches the sky (aerial perspective); 1 = fully the custom colour.
- Basic volumetric fog absorption density. For god rays and scatter, enable Volumetric Scattering in Engine.
- Brightness of the volumetric glow accumulated along each ray.
- Low values give a wide haze; high values hug the surface like neon outlines.
- Whether the glow inherits the surface colour or uses a fixed tint.
- Tint applied to the glow when Glow Source is Color.

### VolumetricFeature

Source: [engine-gmt/features/volumetric/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/volumetric/index.ts). Registration: [engine-gmt/features/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/index.ts).

**Labels and options found in the declaration:** Volumetric Scattering; Volumetric Scatter; Volume Scatter; Enabled; Density; Anisotropy (g); Light Sources; Scatter Tint; Fog Height Falloff; Height Origin; Surface Color Scatter; Surface Falloff; Quality; Step Jitter.

**Declared behavior** (source descriptions; implementation limitations are discussed in the review):

- Henyey-Greenstein single scatter. Enables god rays, colored haze, and directional fog.
- How thick the participating medium is along each ray.
- 0=isotropic, +0.9=forward (god rays), -0.9=back scatter.
- Max lights for shadow rays. More = more expensive.
- Tint applied to scattered light along each ray.
- Density varies with Y. Creates ground fog, rising mist.
- Y level where height-based fog density peaks.
- Orbit trap color field scattered through the volume. No shadow rays needed.
- Concentrate color near fractal surface.
- 0 = 1/128 cheap preview (clean after many frames). 1 = 1/8 full sampling (converges fast, ~16× per-frame cost). Same final image either way; tradeoff is per-frame FPS vs frames-to-converge.
- 1 = smooth (temporal accumulation removes noise). 0 = fixed slicing pattern (artistic, broken fog look).

### MaterialFeature

Source: [engine-gmt/features/materials.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/materials.ts). Registration: [engine-gmt/features/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/index.ts).

**Labels and options found in the declaration:** Shader; Surface; Environment; Self-Illumination; Diffuse (Color); Metallic; Reflectivity; Roughness; Rim Light; Rim Sharpness; Rim Color; Sky Visibility; Environment Light; Source; Solid; Gradient; Sky Image; Upload Texture; Env Profile; Use Env Map; Rotation; Sky Gradient; Self-illumination; Emission Source; Full Surface; Layer 1; Layer 2; Layer 3; Solid Color; Illumination Power.

**Declared behavior** (source descriptions; implementation limitations are discussed in the review):

- Direct-light shading on the fractal surface.
- Image-based lighting and sky source for indirect light.
- Surface glow that emits its own light independently of scene lighting.
- Strength of the gradient colour applied to lit surfaces.
- Tints highlights with the surface colour as the surface gets more metallic.
- Brightness of direct-light specular highlights.
- Spreads highlights wider as roughness increases.
- Adds a glow along edges that face away from the camera.
- Higher values keep the rim glow tight to the silhouette.
- Tint of the rim glow.
- Brightness of the sky behind the fractal — independent of the environment light strength. 0 = black backdrop.
- How strongly the sky lights the scene (dome light on surfaces and reflections).
- What the sky is — a solid colour, a procedural gradient, or a panorama image. Shared by the backdrop, the environment light, reflections, and the fog Sky Tint.
- Equirectangular HDR or LDR image used as the sky.
- Spins the sky image around the vertical axis.
- Vertical sky gradient: ground colour to zenith.
- How brightly the surface glows on its own, independent of lights.
- Which part of the surface colour drives the glow.
- Constant glow tint when Emission Source is Solid Color.
- Path-tracing only: boosts how much light the glow casts onto other surfaces.

### WaterPlaneFeature

Source: [engine-gmt/features/water_plane.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/water_plane.ts). Registration: [engine-gmt/features/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/index.ts).

**Labels and options found in the declaration:** Water Plane; Enable Water; Visible; Height (Y); Water Color; Roughness; Wave Height; Wave Speed; Wave Freq.

**Declared behavior** (source descriptions; implementation limitations are discussed in the review):

- Toggle the water plane on or off without recompiling.
- World-space Y position of the water surface.
- Albedo tint of the water (deep colour).
- Surface roughness; low values give mirror-like reflections.
- Amplitude of the wave displacement.
- How fast the waves animate over time.
- Spatial frequency of the wave pattern; higher = tighter ripples.

### ColoringFeature

Source: [engine-gmt/features/coloring/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/coloring/index.ts). Registration: [engine-gmt/features/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/index.ts).

**Labels and options found in the declaration:** Gradient; Mapping; Histogram; Output; Noise; Geometric Orbit Trap; Scale; Offset; Repeats; Phase; Gamma; Color Iterations; Twist; Arms; Escape Radius; Gradient 2; Scale 2; Offset 2; Blend Mode; Mix; Add; Multiply; Overlay; Screen; Bump (Normal); Blend Amount; Noise Color; Noise Scale; Mix Strength; Bump; Turbulence; Load Noise (Layer 3); Shape; Point; Sphere; Cross; Plane; Center; Radius; Normal.

**Declared behavior** (source descriptions; implementation limitations are discussed in the review):

- How fractal data feeds the gradient lookup.
- Final adjustments applied after the gradient lookup.
- Layer 2 mapping mode and blend.
- High-frequency noise mixed into the surface colour.
- Geometric shape for orbit trap accumulation (pairs with Geometric Orbit Trap mapping mode).
- Colour ramp sampled by Layer 1 mapping values.
- Which fractal quantity drives the gradient lookup.
- Stop orbit trap capture at this iteration (0 = use all iterations)
- Winds the gradient into a logarithmic spiral around the fractal axis. Scale-invariant (looks the same at any zoom). 0 = off. Pair with Arms for multi-armed spirals.
- Number of angular colour arms. Integer values are seam-free. 0 = concentric rings; combine with Twist for N-armed spirals.
- Orbit magnitude² at which a point is treated as escaped, used by the Potential, Decomposition and Green\'s-Flow coloring modes (and the smooth-iteration normalization). Purely a coloring control — the geometry bailout is the separate Quality → DE Bailout.
- Colour ramp sampled by Layer 2 mapping values, blended over Layer 1.
- Which fractal quantity drives the Layer 2 gradient lookup.
- Winds Layer 2 into a logarithmic spiral around the fractal axis. Scale-invariant. 0 = off. Pair with Arms for multi-armed spirals.
- Number of angular colour arms for Layer 2. Integer values are seam-free. 0 = concentric rings.
- How Layer 2 combines with Layer 1 (mix, add, multiply, overlay, screen, bump).
- Strength of the Layer 2 blend over Layer 1.
- Tint of the procedural noise overlay.
- Frequency of the noise pattern; higher = finer detail.
- How strongly the noise tint mixes into the surface colour.
- Perturbs surface normals with the noise pattern for fake displacement.
- Adds higher-frequency octaves on top of the base noise.
- Compiles simplex noise into the shader. Disable to reduce compile time when Layer 3 is not needed.
- Compiles per-iteration geometric trap distance into the shader. Select shape + mapping mode "Geometric Orbit Trap" to colour by trap distance.
- Geometric shape to measure orbit distance against.

### TexturingFeature

Source: [engine-gmt/features/texturing.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/texturing.ts). Registration: [engine-gmt/features/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/index.ts).

**Labels and options found in the declaration:** Image; Mapping; Transform; Use Texture; Select Image; Color Profile; Texture U Mode; Orbit Trap; Iterations; Radial; Z-Depth; Angle; Normal; Decomposition; Raw Iterations; Potential (Log-Log); Green's Flow; Texture V Mode; Scale UV; Texture Offset.

**Declared behavior** (source descriptions; implementation limitations are discussed in the review):

- Image texture sampled instead of the gradient for layer 1.
- How the image projects onto fractal surfaces.
- Rotation and scaling applied to the texture lookup.

### QualityFeature

Source: [engine-gmt/features/quality.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/quality.ts). Registration: [engine-gmt/features/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/index.ts).

**Labels and options found in the declaration:** Quality; Loop Limits & Precision; Quality Core; Hard Loop Cap; Ray Precision; High (Desktop); Standard (Mobile); Texture Buffer; Float32 (HDR); HalfFloat16; Max Ray Steps; Distance Metric; Euclidean (Sphere); Chebyshev (Box); Manhattan (Diamond); Minkowski 4 (Rounded); Estimator; DE Bailout; Step Size; Step Jitter; Ray detail; Pixel threshold; Overstep Fix; DE Scale; DE Smoothing; Surface Refinement; Refine Active; Refine Steps; DE Sub; Adaptive Resolution; UI Responsiveness; Distance Probe; GPU Probe; Manual; Manual Distance.

**Declared behavior** (source descriptions; implementation limitations are discussed in the review):

- Safety limit for ray/DE loops (MAX_HARD_ITERATIONS define). Requires recompile but does not affect compile time — ANGLE/D3D does not unroll define-bounded loops.
- Sets the minimum epsilon (ray hit distance). Standard prevents GPU hangs on mobile.
- Controls render target bit-depth. 16-bit is faster and required on some mobile GPUs.
- Runtime limit. Rays stop after this many steps. Artistic tool for limiting depth. Maximum is limited by Hard Loop Cap.
- The shape of "distance". Changes the aesthetic of the fractal surface.
- Algorithm for calculating distance. Log=Smooth, Linear=Sharp/IFS, Pseudo=Artifact Fix, Cutting Plane=Knighty fold-and-cut polyhedra, Numerical=finite-difference (no analytic DE needed; ~4× slower, fixes formulas that render as dust/noise).
- Radius² at which the raymarch DE stops iterating. High keeps surfaces sharp and true to the boundary; low bails early, slicing the fractal into rounded shells. FOLD formulas (box/IFS) whose orbit oscillates back need a HIGH bailout to develop structure — MB3D imports map this from rStop² (often ~1e6). Fast-escaping formulas only respond near their structure scale.
- March step multiplier (MB3D ZstepDiv). Lower = finer march, higher quality but slower. Set to < 0.2 for deep zooms. Also paces shadow / visibility rays.
- Stochastic step variation. Breaks banding artifacts. Higher = softer edges, artistic blur.
- Tightens the hit threshold; higher values resolve finer surface detail.
- Pixel size at which a ray is considered to have hit the surface.
- Recovers details missed by the raymarcher. 0=Off. Higher values fix more holes but may create noise.
- Magnitude calibration (MB3D dDEscale) for the Numerical (Finite-Diff) estimator. Too high overshoots/misses the surface; too low converges slowly and can exhaust the step budget (blank). ~0.2–0.5 works for the MB3D imports; if a scene reads as noise, lower Ray detail (~1.5) so the rougher numeric DE clears the hit threshold. The probe is auto-derived, so this is the main numeric-DE dial. Only affects that estimator.
- Primary-ray probe width for the Numerical (Finite-Diff) estimator. The escape field is chaotic in deep/high-iteration regions, so a narrow probe (1.0) makes those sections flicker black under any motion. Wider = averages the sub-pixel chaos → a stable, flicker-free surface, trading a little deep-detail sharpness. Raise if deep sections shimmer/flicker; lower toward 1.0 for maximum detail. Only affects that estimator.
- Binary-searches the first ray hit onto the surface — resolves 'dust' from overshooting / discontinuous distance estimators (hard hybrid & imported formulas). Compiling it in/out recompiles; on/off + step count are live after.
- Bisection steps at the hit — higher converges tighter onto the surface. Live (no recompile).
- MB3D msDEsub — per-step DE safety-subtraction (0 unless the imported scene set iOptions bit 2). Higher = more cautious near surfaces; too high can under-step a scene to empty — lower it if a scene renders blank.
- Drop resolution while moving and during slow frames; restore when idle.
- Target frame rate the adaptive resolver + progressive banding hold during interaction. Higher = smoother UI (more downscaling/banding, less GPU load). 0 = off: full-resolution, un-banded frames — chugs the GPU for the fastest accumulation.
- GPU Probe: Reads distance from render target. Manual: Fixed value for orbit control.
- Manual distance value. Used for orbit control calculations.

### DrosteFeature

Source: [engine-gmt/features/droste/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/droste/index.ts). Registration: [engine-gmt/features/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/index.ts).

**Labels and options found in the declaration:** Droste Effect; Tiling Mode; Repeat; Mirror; Clamp; Transparent; Center Shift; Smooth Upscaling; Inner Rad; Outer Rad; Strands; Mirror Strand; Auto Period; Periodicity; Zoom; Spiral Rotate; Image Spin; Polar Rotate; Twist (Conformal); Hyper Droste; Fractal Points.

**Declared behavior** (source descriptions; implementation limitations are discussed in the review):

- Enable the recursive Droste image-in-image post effect.
- How sampled coordinates wrap outside the source rectangle.
- Pixel offset of the spiral centre on screen.
- Softens the magnified recursion. 0 = raw pixels; low = a crisp bicubic smoothing; higher widens into a broad blur.
- Inner ring radius of the recursion.
- Outer ring radius; ratio with inner controls scaling per loop.
- Number of spiral arms (sign flips chirality).
- Mirrors alternating strands for kaleidoscopic symmetry.
- Solve periodicity automatically from inner/outer radii.
- How many recursion loops complete before tiling repeats.
- Zoom along the spiral; animate to drive infinite-zoom motion.
- Twist of the spiral path in degrees.
- Rotates the sampled image content independently of the spiral.
- Rotates the polar coordinate map before applying the spiral.
- Use the conformal Escher mapping; off gives a non-conformal spiral.
- Modular multi-point variant of the Droste mapping.
- Number of fractal centres in the Hyper Droste mapping.

### PostEffectsFeature

Source: [engine/features/post_effects.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine/features/post_effects.ts). Registration: [engine-gmt/features/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/index.ts).

**Labels and options found in the declaration:** Bloom; Threshold; Spread; Chromatic Aberration.

### ColorGradingFeature

Source: [engine/features/color_grading.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine/features/color_grading.ts). Registration: [engine-gmt/features/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/index.ts).

**Labels and options found in the declaration:** Color Correction; Tone Mapping; ACES; AgX; Reinhard; Neutral; None; Saturation; Black Point; White Point; Gamma.

### OpticsFeature

Source: [engine-gmt/features/optics.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/optics.ts). Registration: [engine-gmt/features/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/index.ts).

**Labels and options found in the declaration:** Scene; Projection; Perspective; Orthographic; 360° Skybox; Field of View; Ortho Scale; Camera Blur; Focus Distance.

**Declared behavior** (source descriptions; implementation limitations are discussed in the review):

- Camera projection: perspective, orthographic, or equirectangular skybox.
- World-space size of the orthographic view rectangle.
- Aperture strength for depth-of-field blur.
- Distance from the camera that stays in sharp focus.

### NavigationFeature

Source: [engine-gmt/features/navigation.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/navigation.ts). Registration: [engine-gmt/features/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/index.ts).

**Labels and options found in the declaration:** Fly Speed %; Auto-slow on collision; Orbit/zoom around cursor.

### CameraManagerFeature

Source: [engine-gmt/features/camera_manager/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/camera_manager/index.ts). Registration: [engine-gmt/features/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/index.ts).

**Labels and options found in the declaration:** Camera Manager.

### AudioFeature

Source: [engine/features/audioMod/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine/features/audioMod/index.ts). Registration: [engine-gmt/features/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/index.ts).

**Labels and options found in the declaration:** Audio; Audio Modulation; Enable Audio Engine; Response; Gate Threshold; Auto Gain; Global Attack; Global Decay; High Pass; Low Pass; Volume; Input Trim; Analysis Detail; Fast (23 Hz, punchy); Balanced (12 Hz); Fine (6 Hz, bass detail); Floor; Ceiling; Band Width; Wide — 1/3 octave; Medium — 1/6 octave; Narrow — 1/12 octave; Balance Bands; Tilt.

**Declared behavior** (source descriptions; implementation limitations are discussed in the review):

- Total time band levels take to follow the music. Detail is accounted for inside this, so changing it does not make the rig slower.
- Track the input level and normalise it, so thresholds keep working when the music gets quieter or louder.
- Finer bins separate bass better; coarser bins react faster to attacks.
- Quietest level the spectrum shows. Raise it to keep room noise out of the bottom.
- Loudest level before the spectrum saturates. Lower it for a hotter, more contrasty reading.
- How finely the spectrum is divided. Narrower bands separate more, but need a finer Detail setting to stay honest in the bass.
- Let every frequency band self-calibrate, so quiet bands react as readily as loud ones. Trades spectral contrast for consistency — off usually reads better.
- Lift the high bands to compensate for music\'s natural roll-off. A fixed offset, so it costs no dynamics — 3 is neutral for typical material, 0 is the raw spectrum.

### DrawingFeature

Source: [engine-gmt/features/drawing/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/drawing/index.ts). Registration: [engine-gmt/features/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/index.ts).

**Labels and options found in the declaration:** Drawing; Drawing Tools; Show Tab; Enable Tool; Tool Type; Origin Plane; Global Zero; Surface (Probe); Line Color; Line Width; Show Measurements; Show Axis Ruler; Shapes; Refresh Trigger.

**Declared behavior** (source descriptions; implementation limitations are discussed in the review):

- Where the drawing plane starts.
- Displays a reference grid at the drawing origin.

### RotationGizmoFeature

Source: [engine-gmt/features/rotation_gizmo/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/rotation_gizmo/index.ts). Registration: [engine-gmt/features/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/index.ts).

**Labels and options found in the declaration:** .

### ModulationFeature

Source: [engine/features/modulation/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine/features/modulation/index.ts). Registration: [engine-gmt/features/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/index.ts).

**Labels and options found in the declaration:** Rules; Selection.

### WebcamFeature

Source: [engine/features/webcam/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine/features/webcam/index.ts). Registration: [engine-gmt/features/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/index.ts).

**Labels and options found in the declaration:** Webcam Overlay; Enabled; Opacity; Position X; Position Y; Width; Height; Crop Left; Crop Right; Crop Top; Crop Bottom; Blend Mode; Normal; Screen; Overlay; Lighten; Difference; CRT Scanlines; 3D Tilt; Overlay Font Size.

### DebugToolsFeature

Source: [engine/features/debug_tools/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine/features/debug_tools/index.ts). Registration: [engine-gmt/features/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/index.ts).

**Labels and options found in the declaration:** State Debugger; Interaction Session; GLSL Debugger.

### ShaderCompilerFeature

Source: [engine-gmt/features/engine/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/engine/index.ts). Registration: [engine-gmt/features/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/index.ts).

**Labels and options found in the declaration:** Shader Compiler; Show Shader Compiler Tab.

### PaletteGeneratorFeature

Source: [palette/features/paletteGenerator.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/palette/features/paletteGenerator.ts). Registration: [palette/registerPaletteUI.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/palette/registerPaletteUI.ts).

**Labels and options found in the declaration:** Generator; Modify; Noise; Generator mode; Mixer; ColorBox; Stops; Lightness (L); Chroma (C); Hue (h); Hue rotate; Chroma ×; Contrast; Lightness; Posterize bands; Scale; Phase; Mirror; Reverse output; Amount; Frequency; Noise → lightness; Noise → chroma; Noise → hue.

**Declared behavior** (source descriptions; implementation limitations are discussed in the review):

- Global modifier chain applied to the mixed result.
- High-frequency grain added to the result.
- Blend the light/dark structure A↔B.
- Blend the vividness A↔B.
- Blend the colours A↔B.
- Rotate the whole gradient hue (degrees).
- Scale colourfulness (0 = greyscale).
- Contrast lightness around mid.
- Make the whole gradient lighter or darker.
- Quantize into N colour bands (0 = off).
- How many times the gradient runs across — fractions end part-way through, below 1 shows part of it.
- Offset the gradient along t.
- Ping-pong the gradient (seamless tile).
- Flip the mixed result.
- Grain strength.
- Grain coarseness (smaller = coarser).

### PaletteFiltersFeature

Source: [palette/features/paletteFilters.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/palette/features/paletteFilters.ts). Registration: [palette/registerPaletteUI.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/palette/registerPaletteUI.ts).

**Labels and options found in the declaration:** Filters; Reverse order; Swatch size; Padding.

**Declared behavior** (source descriptions; implementation limitations are discussed in the review):

- Width × height of each gradient swatch (px).
- Gap between swatches (px). 0 = flush.

### PaletteImageFeature

Source: [palette/features/paletteImage.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/palette/features/paletteImage.ts). Registration: [palette/registerPaletteUI.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/palette/registerPaletteUI.ts).

**Labels and options found in the declaration:** Image; Mode; Distill; Tone; Trace; Colours; Frequency ↔ saliency; Tonal detail; Chroma boost; Band width; Smoothing; Smooth path; Golden hour; Even ↔ dwell; Reverse.

**Declared behavior** (source descriptions; implementation limitations are discussed in the review):

- Distill = dominant colours · Tone = colour-by-brightness · Trace = a line through the image.
- How many dominant colours to distill.
- ← common colours · standout colours →
- Number of brightness buckets along the spine.
- Vividness of the tonal spine.
- Width of the perpendicular sampling band.
- Smooth the sampled colour journey along its length.
- Fit a Catmull-Rom curve through the freehand points instead of straight segments.
- ← shadows · highlights →
- Even spacing ↔ linger on the dominant colours.
- Flip the extracted gradient.

### PaletteEditorFeature

Source: [palette/features/paletteEditor.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/palette/features/paletteEditor.ts). Registration: [palette/registerPaletteUI.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/palette/registerPaletteUI.ts).

**Labels and options found in the declaration:** Stops.

## Native formula array (48 entries, including Modular)

These are array entries, not a claim about the total runtime registry: folds and imported/user formulas register separately, and aliases are additional keys rather than independent algorithms.

### Mandelbulb

Source: [engine-gmt/formulas/Mandelbulb.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Mandelbulb.ts).

- Literal IDs: Mandelbulb, paramA, vec2A, paramD, vec2B, 2, 1767363622003, 1, 1767363615540, hor, zen.
- Parameter/option labels: Power; Phase (θ, φ); Z Twist; Radiolaria.
- Source descriptions: The classic 3D extension of the Mandelbrot set. Features standard Power controls plus the "Radiolaria" mutation for skeletal/hollow effects..
- Capability tokens found: shape:per-iteration.

### Mandelorus

Source: [engine-gmt/formulas/Mandelorus.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Mandelorus.ts).

- Literal IDs: Mandelorus, paramA, paramB, paramC, vec2A, paramF, vec2B.
- Parameter/option labels: Ring Radius; Twist (Sym); Power; Phase (Ring, Cross); Vert Scale; Radiolaria.
- Source descriptions: Wraps the fractal iteration around a Torus (Donut). Creates a Solenoid structure. Twist is linked to Power: 1.0 Twist = 1 Symmetry Shift (360/Power)..
- Capability tokens found: shape:per-iteration.

### MixPinski

Source: [engine-gmt/formulas/MixPinski.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/MixPinski.ts).

- Literal IDs: MixPinski, paramA, paramC, paramB, vec3A, vec3B, vec2A, vec3C, 1773421493405_7, 1773421493405_6, 1773421493405_5, 1773421493405_4, 1773421493405_3, 1773421493405_2, 1773421493405_1, 1773421493405_0, 1, 2, sky, hor, zen.
- Parameter/option labels: Sierpinski Scale; Menger Scale; W (4th Dim); Sierpinski Offset; Menger Offset; 4D Offsets; Rotation.
- Source descriptions: Darkbeam\'s MixPinski — a 4D hybrid combining Sierpinski tetrahedron folds (extended to 4D with w-component) and a Menger-like fold-scale transform. The interplay of these two IFS systems produces extraordinary geometric complexity..
- Capability tokens found: shape:per-iteration.

### SierpinskiTetrahedron

Source: [engine-gmt/formulas/SierpinskiTetrahedron.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/SierpinskiTetrahedron.ts).

- Literal IDs: SierpinskiTetrahedron, vec3C, paramB, vec3B, vec3A, paramF, 1766253802332_11, 1766253802332_10, 1766253802332_9, 1766253802332_8, 1766253802332_7, 1766253802332_6, 1766253802332_5, 1766253802332_4, 1766253802332_3, 1766253802332_2, 1766253802332_1, 1766253802332_0, 1, 2.
- Parameter/option labels: Scale; Offset; Rotation; Shift; Twist.
- Source descriptions: The Sierpinski Tetrahedron (Tetrix) — a 3D IFS fractal built from reflective folds across tetrahedron face planes. Supports per-axis scale, rotation, shift and twist..
- Capability tokens found: shape:per-iteration, estimator:cutting-plane.

### MandelMap

Source: [engine-gmt/formulas/MandelMap.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/MandelMap.ts).

- Literal IDs: MandelMap, paramA, paramB, paramC, paramD, vec2A.
- Parameter/option labels: Power; Height Amp; Map Scale; Projection; Spherical; Cylindrical; Toroidal; Phase (θ, φ).
- Source descriptions: Maps the Mandelbulb 3D structure onto a 2D plane. Use "Projection" (Param D) to switch between Spherical (Standard), Cylindrical (Infinite Vertical), and Toroidal (Seamless) mappings..
- Capability tokens found: shape:per-iteration.

### Borromean

Source: [engine-gmt/formulas/Borromean.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Borromean.ts).

- Literal IDs: Borromean, paramA, paramB, paramC, paramD, paramE, paramF.
- Parameter/option labels: Power; Connection; Repulsion; Balance; Phase; Invert.
- Source descriptions: Treats 3D space as three coupled 2D planes (XY, YZ, ZX). The output of one plane becomes the input of the next, creating a "Rock-Paper-Scissors" feedback loop. Produces tetrahedral symmetries and solid, non-spherical shapes..
- Capability tokens found: shape:per-iteration.

### Appell

Source: [engine-gmt/formulas/Appell.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Appell.ts).

- Literal IDs: Appell, paramA, paramB, paramC, paramD, paramE.
- Parameter/option labels: Interference; Power; Ghost Shift; Cloud Density; Phase.
- Source descriptions: Implements a simplified Appell polynomial: P(x) = x^n - k\|x\|^2, where the non-conformal subtraction destabilizes the surface, revealing skeletal interference patterns. The "Interference" parameter k controls how much structure is stripped away. Best viewed as a volumetric cloud..
- Capability tokens found: shape:per-iteration.

### AmazingBox

Source: [engine-gmt/formulas/AmazingBox.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/AmazingBox.ts).

- Literal IDs: AmazingBox, paramA, paramB, paramC, paramD, vec3A, 0, 1, 2, 3, 4, 5, 6.
- Parameter/option labels: Scale; Min Radius; Folding Limit; Fixed Radius; Pre-Rotation.
- Source descriptions: Also known as the Mandelbox (Tglad). A folding fractal that creates complex, machine-like architectural structures..
- Capability tokens found: shape:per-iteration.

### AmazingSurface

Source: [engine-gmt/formulas/AmazingSurface.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/AmazingSurface.ts).

- Literal IDs: AmazingSurface, paramA, paramB, vec3A, vec3B, paramE, paramF, sky, hor, zen, 2, 1768067662570, 1768067659362, 1768067719427, 1768067722341, 1768067729100, 1768067900586, 1.
- Parameter/option labels: Scale; Inv Max; Box Params; Offset Params; Pre-Scale; Thickness.
- Source descriptions: A "Menger-Kleinian" hybrid. Uses 3-axis sorting (Menger) followed by a Box Fold and Sphere Inversion. Capable of creating non-orthogonal, organic machinery..
- Capability tokens found: shape:per-iteration.

### MengerSponge

Source: [engine-gmt/formulas/MengerSponge.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/MengerSponge.ts).

- Literal IDs: MengerSponge, paramA, vec3B, vec3A, paramC, 1767569325432_0, 1767569325432_1, 1767569325432_2, 1767569325432_3, 1767569325432_4, 1767569325432_5, 1767569325432_6, 1, 2.
- Parameter/option labels: Scale; Offset; Rotation; Center Z.
- Source descriptions: The canonical Menger Sponge (Level N). Set Scale to 3.0 and Offset to 1.0 for the classic mathematical shape. Use "Center Z" to toggle between a corner fractal and the full cube..
- Capability tokens found: shape:per-iteration, estimator:cutting-plane.

### MarbleMarcher

Source: [engine-gmt/formulas/MarbleMarcher.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/MarbleMarcher.ts).

- Literal IDs: MarbleMarcher, paramA, vec3A, vec3B, 1766936009557_0, 1766936025161, 1766936020401, 1766936032564, 1766936039597, 1, 2, 0, 3, 4.
- Parameter/option labels: Scale; Shift; Rotation (Z, X, Y).
- Source descriptions: The dynamic fractal from the game Marble Marcher. A specialized Menger Sponge IFS with rotation and shifting steps..
- Capability tokens found: shape:per-iteration.

### KaliBox

Source: [engine-gmt/formulas/KaliBox.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/KaliBox.ts).

- Literal IDs: KaliBox, paramA, paramB, vec3A, paramF, 1773423501447_0, 1773423501447_1, 1773423501447_2, 1773423501447_3, 1773423501447_4, 1773423501447_5, 1773423501447_6, 1773423501447_7, 1773423501447_8, 1773423501447_9, kb2_0, kb2_1, kb2_2, 1773423550251_0, 1773423550251_1, 1773423550251_2, 1773423550251_3, 1773423550251_4, 1773423550251_5, 1773423550251_6, 1773423550251_7.
- Parameter/option labels: Scale; MinRad2; Translation; Rotation.
- Source descriptions: A Mandelbox variant by Kali (fractalforums.com), optimized by Rrrola. Uses rotation, abs-fold + translation, clamped sphere inversion, and scale/minRad rescaling. Produces organic, cave-like structures..
- Capability tokens found: shape:per-iteration.

### Kleinian

Source: [engine-gmt/formulas/Kleinian.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Kleinian.ts).

- Literal IDs: Kleinian, paramA, vec3A, paramB, paramC, 2, 1, 1767121500027, 1767120246151, hor, zen.
- Parameter/option labels: Scale; Offset; Fold Size; K Factor.
- Source descriptions: Based on Kleinian groups and inversion in a sphere. Creates intricate, bubbly, sponge-like structures..
- Capability tokens found: shape:per-iteration.

### AmazingSurf

Source: [engine-gmt/formulas/AmazingSurf.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/AmazingSurf.ts).

- Literal IDs: AmazingSurf, paramA, paramB, paramC, paramD, vec3A, 1767122909918_0, 1767122909918_1, 1767122909918_2, 1, 2, 1767120246151, hor, zen.
- Parameter/option labels: Scale; Min Radius; Wave Freq; Wave Amp; Transform.
- Source descriptions: A variant of the Amazing Box that introduces sinusoidal waves. Now with Wave Twist and Vertical Shift..
- Capability tokens found: shape:per-iteration.

### MandelTerrain

Source: [engine-gmt/formulas/MandelTerrain.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/MandelTerrain.ts).

- Literal IDs: MandelTerrain, paramB, vec2A, paramA, paramC, paramD, paramE.
- Parameter/option labels: Map Zoom; Pan (Real, Imag); Height: Distance Estimator; Height: Layer 2 Gradient; Height: SmoothTrap; Height: Ledge (cosine bands).
- Source descriptions: A 3D Heightmap of the Mandelbrot set. Iterations slider controls terrain detail..
- Capability tokens found: shape:self-contained.

### Claude

Source: [engine-gmt/formulas/Claude.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Claude.ts).

- Literal IDs: Claude, paramA, paramB, paramC, paramD, vec3A, vec3B, paramF, cl_0, cl_1, cl_2, cl_3, cl_4, cl_5, cl_6, cl_7, cl2_0, cl2_1, cl2_2, env_0, env_1, env_2, env_3, env_4.
- Parameter/option labels: Scale; Harmonic; Inner R²; Fix R²; Offset; Rotation; Twist.
- Source descriptions: Icosahedral reflection folds (golden-ratio normals) + a parametric "harmonic" fold (4th plane swept around the golden axis) + clamped sphere inversion. The harmonic fold is unique to this formula — it enriches the icosahedral base like an overtone enriches a fundamental tone. φ appears in fold geometry, harmonic axis, and default parameters..
- Capability tokens found: shape:per-iteration.

### SineJulia3D

Source: [engine-gmt/formulas/SineJulia3D.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/SineJulia3D.ts).

- Literal IDs: SineJulia3D, paramA, vec3A, paramC, vec3B.
- Parameter/option labels: Scale; Inv. Center; Inv. Radius; Pre-Rotation.
- Source descriptions: Julia sets of a 3D extension of the complex family s·sin(z)+c. The 3D map and its distance estimator are from amoser\'s "Sine Fractal 3D" Shadertoy (2026, CC-BY-NC-SA); the family is discussed on fractalforums.org thread 5591, where Pupukuusikko developed an independent DE. The map is not conformal, so the Julia sets stretch and distort — but for many parameters they look strikingly like Kleinian limit groups. A pre-iteration sphere inversion and per-axis rotation control the framing. Only the formula + DE are ported; shading is GMT\'s standard pipeline, not the original Shadertoy\'s post-stack..
- Capability tokens found: shape:per-iteration.

### Julia3D

Source: [engine-gmt/formulas/Julia3D.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Julia3D.ts).

- Literal IDs: Julia3D, paramA, paramB, paramC, paramD, paramE, vec3B.
- Parameter/option labels: Sphere Radius; Centre Distance; Variant; Limit set — Inflated (Γ̃₁); Limit set — True 3D (Γ̃₂); Julia set (escape-time); DE Scale (limit set); Radial Power (Julia set); Rotation.
- Source descriptions: Václav Kučera\'s 3D generalization of quadratic Julia sets (Bridges 2019). The Variant selector switches between the two objects in the paper, which share the same sphere-inversion group. LIMIT SET: the limit set of five radius-√3 spheres orthogonal to the unit sphere — an Apollonian-like 3D packing (Inflated Γ̃₁ drops the third sphere pair; True 3D Γ̃₂ adds it). JULIA SET: a reconstruction of the paper\'s omitted escape-time construction, H(w)=r²·h(w/\|w\|)+c, where h is built from the generators with their shared reflection lifted to 3D as a 180° x-rotation (the recovered detail that prevents cracking). Default c=(-1,0,0) gives the 3D Basilica..
- Capability tokens found: shape:per-iteration.

### Julia3DLattes

Source: [engine-gmt/formulas/Julia3DLattes.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Julia3DLattes.ts).

- Literal IDs: Julia3DLattes, paramA, paramB, vec3B.
- Parameter/option labels: Sym Fold; Off (no fold); amoser 45° (off-axis seams); Dihedral (continuous, BS-class); Aim set along c; Off (true map; off-axis seams); On (rotate on-axis set to c); Post-Rotation.
- Source descriptions: amoser\'s reconstruction of Kučera\'s 3D Julia set as a Lattès map on the Riemann sphere: stereographic-project, apply M(w²) with M a 90° x-rotation, unproject, scale by r² and add c. One global analytic map — no region seams. A 45° fold supplies the sphere symmetry; like amoser\'s original it is faithful for c on the x-axis..
- Capability tokens found: shape:per-iteration.

### Julia3DKucera

Source: [engine-gmt/formulas/Julia3DKucera.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Julia3DKucera.ts).

- Literal IDs: Julia3DKucera, paramA.
- Parameter/option labels: Map; Kučera cover (quadratic); Folded (burning-ship-class).
- Source descriptions: Kučera\'s "inflated" 3D generalization of quadratic Julia sets (Bridges 2019, Γ̃₁): his piecewise-Möbius degree-2 circle map h (built from Kleinian-group generators, topologically conjugate to z²) suspended azimuthally to the sphere, then extended by radial scaling: H(w) = c + r²·Φ(w/r). Φ is a true degree-2 branched cover — no folds, no seams — so the family has genuine quadratic dynamics for arbitrary c ∈ ℝ³: Mandelbrot-class parameter space (turn Julia mode off), quadratic-family Julia sets. The Fold-compare mode folds the azimuth first, flipping the same map to burning-ship-class dynamics..
- Capability tokens found: shape:per-iteration.

### Julia3DZorich

Source: [engine-gmt/formulas/Julia3DZorich.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Julia3DZorich.ts).

- Literal IDs: Julia3DZorich, paramA.
- Parameter/option labels: Power n; n = 2 (quadratic); n = 3 (cubic); n = 4 (quartic).
- Source descriptions: The Zorich–Mayer power-map family in ℝ³: H = c + rⁿ·Φ(w/r), where Φ is multiplication-by-n on the flat square pillowcase (ℝ² mod integer-mirror reflections ≅ S²) — a degree-n² branched cover expanding lengths by n, matching the radial log-expansion. That matched (uniformly quasiregular) coupling is what a degree-2 sphere map can never provide (√2 ≠ 2), and is the structural requirement for genuine zⁿ-like dynamics in 3D: honest escape structure, valid distance estimation, quadratic-class Julia sets and a connected Mandelbrot-type parameter body (turn Julia mode off ⇒ c = z) — for arbitrary c ∈ ℝ³, no seams, no folds. Lattice symmetry supplies the fractal grammar: the four pillowcase corners on the equator are the branch points..
- Capability tokens found: shape:per-iteration.

### KleinianMobius

Source: [engine-gmt/formulas/KleinianMobius.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/KleinianMobius.ts).

- Literal IDs: KleinianMobius, paramA, paramB, paramC, paramD, vec3A, vec2A, vec4A, paramE, paramF, vec3C, km0, km1, km2, km3, km4, 1, 2, bg0, bg1, bg2.
- Parameter/option labels: Klein R; Klein I; Scale; Sphere Inversion; Off; On; Offset; Box Size XZ; Inv Sphere (xyz=center, w=radius); DE Min; Twist; Box Offset.
- Capability tokens found: shape:per-iteration.

### KleinianJos

Source: [engine-gmt/formulas/KleinianJos.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/KleinianJos.ts).

- Literal IDs: KleinianJos, paramA, paramB, paramC, paramD, paramE, paramF, vec2A, vec4A, vec3C, kj0, kj1, kj2, kj3, kj4, 1, 2, bg0, bg1, bg2.
- Parameter/option labels: Klein R; Klein I; Scale; Rot Z; DE Min; Twist; Box Size XZ; Inv Sphere (xyz=center, w=radius); Box Offset.
- Capability tokens found: shape:per-iteration.

### Mandelbar3D

Source: [engine-gmt/formulas/Mandelbar3D.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Mandelbar3D.ts).

- Literal IDs: Mandelbar3D, paramA, vec3B, vec3A, paramF, 1766240207225_0, 1766240207225_1, 1766240207225_2, 1766240207225_3, 1766240207225_4, 1766240207225_5, 1766240207225_6, 1, 2.
- Parameter/option labels: Scale; Rotation; Offset; Twist.
- Source descriptions: The 3D extension of the Tricorn (Mandelbar) fractal: x²-y²-z², 2xy, -2xz. The conjugation on z creates the distinctive tri-corner symmetry. Supports rotation, offset, and twist..
- Capability tokens found: shape:per-iteration.

### Quaternion

Source: [engine-gmt/formulas/Quaternion.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Quaternion.ts).

- Literal IDs: Quaternion, paramA, paramB, paramC, paramD, paramE, vec2A, vec2B, vec3A, 1766241181174_0, 1766241181174_1, 1766241181174_2, 1766241181174_3, 1766241181174_4, 1766241181174_5, 1766241181174_6, 1, 2.
- Parameter/option labels: Julia C (W); Slice W; Damping; Inversion Radius; Inversion Angle; Rot 3D (XY, XZ); Rot 4D (XW, YW); Inversion Center.
- Source descriptions: A 4D Julia set projected into 3D. Features 4D rotations, iteration damping for smooth variants, and optional spherical inversion (Kosalos) for inside-out effects..
- Capability tokens found: shape:per-iteration.

### PseudoKleinian

Source: [engine-gmt/formulas/PseudoKleinian.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/PseudoKleinian.ts).

- Literal IDs: PseudoKleinian, paramA, paramB, paramC, paramD, vec3A, paramF, 1771521894392, 1771519043183, 1771519043723, 1771518360330_2, 1771518360330_3, 1, 1771521834787, 1771521804163, 1771523832902_0, 1771523832902_1, 1771523832902_2.
- Parameter/option labels: Box Limit; Size (C); Power; Magic Factor; Shift; Twist.
- Source descriptions: A modification of the Kleinian group formula. Now supports linear shifting and twisting..
- Capability tokens found: shape:per-iteration.

### PseudoKleinian06

Source: [engine-gmt/formulas/PseudoKleinianAdv.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/PseudoKleinianAdv.ts).

- Literal IDs: PseudoKleinian06, paramB, paramC, vec2A, vec2B, vec3A, vec3B, vec3C, vec4A, 1, 2, 3, 4.
- Parameter/option labels: Size; DEoffset; Thickness; Inversion; Offset; CSize; C; InvCenter.
- Source descriptions: Pseudo Kleinian fractal (Scale 1 JuliaBox + Thingy DE shape) by Knighty and Theli-at.\n\nProduces intricate Kleinian group limit sets, nested spherical lattices, and soap-film-like bubble networks.\n\nEach iteration applies:\n1. Box fold: p = 2·clamp(p, −CSize, CSize) − p\n2. Sphere fold: k = max(Size/r², 1); p *= k\n3. Julia shift: p += C\n\nThe DE uses the "Thingy" shape — a twisted cylinder cross-section.\n\nWith Inversion enabled, the infinite periodic tiling wraps into a bounded sphere via Möbius inversion, producing the classic Kleinian bubble geometry. InvRadius controls the inversion sphere size; InvCenter positions it.\n\nKey parameters:\n• Size: sphere fold radius. 1 = scale-1 Julia box.\n• CSize: box fold half-size per axis. CSize.z also offsets z before the loop.\n• C: Julia constant shift. Zero = pure Kleinian; non-zero = Julia variant.\n• Inversion: toggle + radius. Wraps infinite tiling into bounded sphere.\n• InvCenter: sphere inversion center. Classic value (1.15, 0.5, −2).\n• Thickness: x=TThickness (DE numerator), y=TThickness2 (shell radius).\n• Offset: translates the Thingy DE shape origin.\n• DEoffset: subtracts from the final DE, inflating the surface..
- Capability tokens found: shape:per-iteration.

### PseudoKleinianMod4

Source: [engine-gmt/formulas/PseudoKleinianMod4.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/PseudoKleinianMod4.ts).

- Literal IDs: PseudoKleinianMod4, paramA, paramB, paramC, paramD, paramE, paramF, vec3A, vec3B, vec3C, pk4_0, pk4_1, pk4_2, pk4_3, pk4_4, pk4_5, 1, 2, 0, 3.
- Parameter/option labels: DE Shape; Plane; Cylindrical; Thingy; Size; Thickness; Alt Offset; Inv Scale; Twist; C Shift; Box Size; Box Offset.
- Source descriptions: Extended Pseudo Kleinian based on Mandelbulber\'s Mod4 variant by darkbeam. Adds per-iteration sphere inversion and sign-based box offset to the core Knighty/Theli-at box fold + sphere fold. The sphere inversion maps each iteration through an inversion sphere, creating intricate nested structures. Box offset subtracts a constant times sign(z) per axis, breaking symmetry in controllable ways. Three DE shapes: Plane (Fragmentarium), Cylindrical (Mandelbulber), Thingy (Fragmentarium)..
- Capability tokens found: shape:per-iteration.

### Dodecahedron

Source: [engine-gmt/formulas/Dodecahedron.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Dodecahedron.ts).

- Literal IDs: Dodecahedron, paramA, paramB, vec3B, vec3A, paramF, 1773424223148_0, 1773424223148_1, 1773424223148_2, 1773424223148_3, 1773424223148_4, 1773424223148_5, 1773424223148_6, 1773424223148_7, 1773424223148_8, 1773424223148_9, 1773424223148_10, 1773424223148_11, 1773424223148_12, 1773424223148_13, 1773424223148_14, 1, 2, 0, 3, 4.
- Parameter/option labels: Scale; Offset; Rotation; Shift; Twist.
- Source descriptions: Kaleidoscopic IFS fractal with true dodecahedral symmetry using 3 golden-ratio reflection normals. Based on Knighty\'s method: 3 normals × 3 reflections = 9 fold operations per iteration, producing the icosahedral/dodecahedral reflection group. Supports rotation, twist, and shift..
- Capability tokens found: shape:per-iteration, estimator:cutting-plane.

### Octahedron

Source: [engine-gmt/formulas/Octahedron.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Octahedron.ts).

- Literal IDs: Octahedron, paramA, paramB, vec3B, vec3A, paramF, octa_5, octa_4, octa_3, 1775546936281, 1775546967714, octa_2, octa_1, octa_0, 1, 2, 0, 3, l7, l8, l9.
- Parameter/option labels: Scale; Offset; Rotation; Shift; Twist.
- Source descriptions: Kaleidoscopic IFS fractal with octahedral/cubic symmetry. Uses 4 conditional fold operations per iteration to map points into the octahedral fundamental domain. Based on Knighty\'s method from Fragmentarium. Supports rotation, twist, and shift..
- Capability tokens found: shape:per-iteration, estimator:cutting-plane.

### Icosahedron

Source: [engine-gmt/formulas/Icosahedron.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Icosahedron.ts).

- Literal IDs: Icosahedron, paramA, paramB, vec3B, vec3A, paramF, 1775547195175_0, 1775547195175_1, 1, 2, 0, 3, l10, l11, l12.
- Parameter/option labels: Scale; Offset; Rotation; Shift; Twist.
- Source descriptions: Kaleidoscopic IFS fractal with icosahedral symmetry using golden-ratio fold normals and abs() prefold. Based on Knighty\'s Fragmentarium implementation. The offset points toward an icosahedron vertex, producing 20-fold triangular face patterns distinct from the dodecahedral variant..
- Capability tokens found: shape:per-iteration, estimator:cutting-plane.

### Cuboctahedron

Source: [engine-gmt/formulas/Cuboctahedron.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Cuboctahedron.ts).

- Literal IDs: Cuboctahedron, paramA, paramB, vec3B, vec3A, paramF, cubo_5, cubo_4, cubo_3, cubo_2, cubo_1, cubo_0, 1, 2, 0, 3, l13, l14, l15.
- Parameter/option labels: Scale; Offset; Rotation; Shift; Twist.
- Source descriptions: Kaleidoscopic IFS fractal with true cuboctahedral geometry. Uses Knighty\'s fold-and-cut approach: octahedral symmetry fold combined with cutting-plane distance estimation. The cuboctahedron vertex sits at the edge midpoint of the octahedral Schwarz triangle. Unlike r/dr estimation, the cutting-plane DE naturally handles fold boundary points, producing exact geometry..
- Capability tokens found: shape:per-iteration, estimator:cutting-plane.

### TruncatedIcosahedron

Source: [engine-gmt/formulas/TruncatedIcosahedron.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/TruncatedIcosahedron.ts).

- Literal IDs: TruncatedIcosahedron, paramA, paramB, paramC, vec3B, vec3A, paramF, trico_0, trico_1, trico_2, trico_3, trico_4, trico_5, 1, 2, 0, 3, l16, l17, l18.
- Parameter/option labels: Scale; Offset; Truncation; Rotation; Shift; Twist.
- Source descriptions: Kaleidoscopic IFS fractal with truncated icosahedral geometry (soccer ball / C60 buckyball). Uses icosahedral symmetry folds with an offset pointing toward the truncated vertex position — between the icosahedron vertex and edge midpoint. The truncation parameter controls how much vertex is cut, interpolating between icosahedron (0) and truncated icosahedron (1)..
- Capability tokens found: shape:per-iteration, estimator:cutting-plane.

### RhombicDodecahedron

Source: [engine-gmt/formulas/RhombicDodecahedron.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/RhombicDodecahedron.ts).

- Literal IDs: RhombicDodecahedron, paramA, paramB, vec3B, vec3A, paramF, rd_0, rd_1, rd_2, rd_3, rd_4, rd_5, 1, 2, 0, 3.
- Parameter/option labels: Scale; Offset; Rotation; Shift; Twist.
- Source descriptions: Kaleidoscopic IFS fractal with rhombic dodecahedral geometry. Uses RD face-normal folds instead of the Knighty fold — reflections through (1,±1,0)/√2 and (0,1,±1)/√2 planes, which ARE the RD face planes themselves. This ensures all fold boundaries align with RD faces, producing true rhombic geometry at every iteration level. The fold domain x ≥ y ≥ \|z\| is bounded entirely by RD faces..
- Capability tokens found: shape:per-iteration, estimator:cutting-plane.

### Coxeter

Source: [engine-gmt/formulas/Coxeter.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Coxeter.ts).

- Literal IDs: Coxeter, paramA, paramB, paramC, vec3B, vec3A, paramF, cox_0, cox_1, cox_2, cox_3, cox_4, cox_5, cox_6, cox_7, cox_8, cox_9, cox_10, cox_11, cox_12, cox_13, cox_14, 1, 2, 0, 3.
- Parameter/option labels: Scale; Offset; Symmetry N; Rotation; Shift; Twist.
- Source descriptions: Kaleidoscopic IFS fractal with a parameterized Coxeter fold normal. The Symmetry N parameter continuously interpolates between Coxeter symmetry groups: N=3 gives tetrahedral [3,3], N=4 gives octahedral [3,4], N=5 gives icosahedral [3,5], and non-integer values produce novel intermediate symmetries. Uses cutting-plane DE with the fold\'s edge-midpoint direction as face normal. Explore fractional N values to discover shapes unique to this formula..
- Capability tokens found: shape:per-iteration, estimator:cutting-plane.

### RhombicTriacontahedron

Source: [engine-gmt/formulas/RhombicTriacontahedron.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/RhombicTriacontahedron.ts).

- Literal IDs: RhombicTriacontahedron, paramA, paramB, vec3B, vec3A, paramF, rt_0, rt_1, rt_2, rt_3, rt_4, rt_5, rt_6, 1, 2, 0, 3.
- Parameter/option labels: Scale; Offset; Rotation; Shift; Twist.
- Source descriptions: Kaleidoscopic IFS fractal based on the rhombic triacontahedron — the 3D shadow of a 6-dimensional hypercube and Buckminster Fuller\'s favorite geometric form. 30 golden-ratio rhombic faces with icosahedral symmetry. Uses the Knighty icosahedral fold (whose planes ARE RT face normals) with a single cutting plane z=size in the folded domain, giving true RT geometry at all iteration levels..
- Capability tokens found: shape:per-iteration, estimator:cutting-plane.

### GreatStellatedDodecahedron

Source: [engine-gmt/formulas/GreatStellatedDodecahedron.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/GreatStellatedDodecahedron.ts).

- Literal IDs: GreatStellatedDodecahedron, paramA, paramB, paramC, vec3B, vec3A, paramF, gsd_0, gsd_1, gsd_2, gsd_3, gsd_4, gsd_5, gsd_6, 1, 2, 3, 0.
- Parameter/option labels: Scale; Offset; Stellation; Rotation; Shift; Twist.
- Source descriptions: Kaleidoscopic IFS fractal based on the great stellated dodecahedron — a Kepler-Poinsot star polyhedron. Uses icosahedral symmetry folds followed by a stellation step that pushes points outward along the vertex direction, creating spiky star-shaped geometry. The stellation parameter controls the spike depth..
- Capability tokens found: shape:per-iteration, estimator:cutting-plane.

### Apollonian

Source: [engine-gmt/formulas/Apollonian.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Apollonian.ts).

- Literal IDs: Apollonian, paramA, paramB, paramC, paramD, vec3A, vec3B, vec2A, paramF, apo_0, apo_1, apo_2, apo_3, apo_4, apo_5, apo_6, 1, 2, 0, 3, l7, l8, l9.
- Parameter/option labels: Foam; Foam 2; Modulation; Inversion; Fold Scale; Inv Center; Inv Radius / Angle; Twist.
- Source descriptions: Apollonian gasket fractal using space folding and sphere inversion. Each iteration folds space into a unit cube then applies an inversion sphere, creating a foam-like recursive sphere packing. Based on kosalos\'s Fragmentarium implementation (fractalforums.org). The modulation factor \'t\' is computed once from the initial ray position before iterations, matching the reference. Optional spherical inversion pre-transform mode (Inversion > 0) replicates the reference \'doInversion\' mode with center, radius, and XY angle rotation with proper DE correction..
- Capability tokens found: shape:per-iteration.

### JuliaMorph

Source: [engine-gmt/formulas/JuliaMorph.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/JuliaMorph.ts).

- Literal IDs: JuliaMorph, paramA, paramF, paramB, vec2B, vec2A, paramC, paramD, paramE, 1, 2, 3, 4, 1768818072358, 1768817090653_0, 1768817090653_1, 1768817090653_2, 1768817090653_3, 1768817090653_4, 1768817090653_5, 1768817090653_6, 1768817090653_7, 1768817090653_8, 1768817090653_9, sky, hor, zen.
- Parameter/option labels: Height (Z Scale); Slice Interval; Slice Thickness; Start C; End C; Twist; Bend; Taper.
- Source descriptions: Constructs a 3D object by stacking 2D Julia sets along the Z-axis. Start C and End C define the Julia constants at the bottom and top. The constant smoothly interpolates between them along the height..
- Capability tokens found: shape:self-contained.

### Phoenix

Source: [engine-gmt/formulas/Phoenix.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Phoenix.ts).

- Literal IDs: Phoenix, paramA, paramB, paramC, paramD, vec2A, vec2B, vec3A, vec3B, vec3C, 8, 7, 6, 5, 4, 3, 2, 1, 0, hor, zen, l7, l8, l9.
- Parameter/option labels: Power (p); History Exp; Twist; History Depth; Distortion (Re, Im); Phase (θ, φ); Stretch; Abs Fold; Pre-Rotation.
- Source descriptions: A 3D generalization of the Phoenix Julia set. Now with Z-stretching and spatial twisting..
- Capability tokens found: shape:per-iteration.

### Buffalo

Source: [engine-gmt/formulas/Buffalo.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Buffalo.ts).

- Literal IDs: Buffalo, paramA, vec3A, vec3B, vec3C, 1766251986254_0, 1766252034417, 1766251986254_1, 1766252020600, 1766252024362, 1766251986254_2, 1766251986254_3, 1, 2, 1773425036101_0, 1773425036101_1, 1773425036101_2, 1773425036101_3, 1773425036101_4, 1773425036101_5, 1773425036101_6, 1773425036101_7, 1773425036101_8, 1773425036101_9, 1773425036101_10, 1773425036101_11, 1773425036101_12, 1773425036101_13, 1773425036101_14, 1773425036101_15.
- Parameter/option labels: Power; Abs After Power; Abs Before Power; Rotation.
- Source descriptions: The Buffalo fractal (ported from Mandelbulber via 3Dickulus). A Mandelbulb variant with selective per-axis absolute value folding before and after the power iteration. The default abs on Y+Z creates the distinctive buffalo/horn shape. Based on the original by youhn @ fractalforums.com..
- Capability tokens found: shape:per-iteration.

### BoxBulb

Source: [engine-gmt/formulas/BoxBulb.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/BoxBulb.ts).

- Literal IDs: BoxBulb, paramA, paramB, paramC, paramD, vec3A, 1766255418053_0, 1766255418053_1, 1766255418053_2, 1766255418053_3, 1766255418053_4, 1766258643816, 1766255418053_5, 1766255418053_6, 1, 2.
- Parameter/option labels: Power; Min Radius; Scale; Fixed Radius; Rotation.
- Source descriptions: A hybrid that combines box/sphere folding with the Mandelbulb power function. Now with rotation controls. (Formerly FoldingBrot).
- Capability tokens found: shape:per-iteration.

### MengerAdvanced

Source: [engine-gmt/formulas/MengerAdvanced.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/MengerAdvanced.ts).

- Literal IDs: MengerAdvanced, paramA, paramB, vec3A, paramC, paramD, 0, 1, 2, 3, 4, 5, 6.
- Parameter/option labels: Scale; Offset; Rotation; Inner Fold; Z Scale.
- Source descriptions: An advanced variant of the Menger Sponge. It adds an Inner Box Fold (Param E) to generate machinery-like details inside the voids, and Z-Scale (Param F) for creating towering structures. (Formerly UberMenger).
- Capability tokens found: shape:per-iteration, estimator:cutting-plane.

### Bristorbrot

Source: [engine-gmt/formulas/Bristorbrot.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Bristorbrot.ts).

- Literal IDs: Bristorbrot, paramA, paramB, vec3A, paramC, paramD, 0, 1, 2, 3, 4, 5, 6.
- Parameter/option labels: Scale; Offset; Rotation; Shift X; Twist.
- Source descriptions: A custom polynomial fractal: x²-y²-z², y(2x-z), z(2x+y). No folding — the asymmetric cross-terms between axes create sharp crystalline edges mixed with smooth bulb regions. Supports scale, rotation, twist, and shift..
- Capability tokens found: shape:per-iteration.

### MakinBrot

Source: [engine-gmt/formulas/MakinBrot.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/MakinBrot.ts).

- Literal IDs: MakinBrot, paramA, vec3B, vec3A, paramF, 0, 1, 2, 3, 4, 5, 6.
- Parameter/option labels: Scale; Rotation; Shift; Twist.
- Source descriptions: A 3D fractal variant discovered by Makin. Custom polynomial: x²-y²-z², 2xy, 2z(x-y). Supports rotation, shift, and twist..
- Capability tokens found: shape:per-iteration.

### Tetrabrot

Source: [engine-gmt/formulas/Tetrabrot.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Tetrabrot.ts).

- Literal IDs: Tetrabrot, paramA, paramB, vec3A, 1766256604050_0, 1766256604050_1, 1766256604050_2, 1, 2.
- Parameter/option labels: Julia C (W); Slice W; Rotation.
- Source descriptions: A 4D Mandelbrot set visualization using a specific squaring function. Now with pre-rotation support..
- Capability tokens found: shape:per-iteration.

### MandelBolic

Source: [engine-gmt/formulas/MandelBolic.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/MandelBolic.ts).

- Literal IDs: MandelBolic, paramA, paramB, paramC, paramD, paramE, paramF, vec2A, hor, zen, 1771879786393_0, 1771879786393_1, 1771879786393_2, 1771879786393_3, 1771879786393_4, 1771879786393_5, 1771879786393_6, 1771879786393_7, 1771879786393_8, 1771879786393_9, 1767363622003, 1, 1767363615540.
- Parameter/option labels: Power; Hyp. Scale; Conformal Cone; Phase Twist; Z-Offset; Trap Scale; Height Fold / Shells.
- Source descriptions: A conformal lift of the exact 2D Mandelbrot into a third (height) axis. The z=0 slice is the true Mandelbrot set — exact periodicity, real cardioid — and the height is carried upward by the map\'s own conformal derivative (a Poincaré-style normal-axis extension), giving smooth bulbs with none of the angular smearing of spherical-power 3D fractals like the Mandelbulb. By default the height dynamics are linear (a pure conformal lift, so the object is essentially 2.5D); the "Height Fold" and "Shells" controls add genuine nonlinear dynamics in the height axis — a real 3D escape boundary — while keeping that z=0 slice exact. Power, conformal cone, phase twist and height-scale shape the rest..
- Capability tokens found: shape:per-iteration.

### Modular

Source: [engine-gmt/formulas/Modular.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Modular.ts).

- Literal IDs: Modular.
- Source descriptions: Construct your own fractal equation by chaining operations together. Combine folds, rotations, and logic via the Graph tab..
- Capability tokens found: shape:modular.

## Generated box-fold formulas

Source: [engine-gmt/formulas/boxFolds.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/boxFolds.ts); fold list: [engine-gmt/features/geometry/folds/index.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/geometry/folds/index.ts).

- **standard** — [engine-gmt/features/geometry/folds/standard.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/geometry/folds/standard.ts); labels: Standard (Tglad).
- **mirror** — [engine-gmt/features/geometry/folds/mirror.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/geometry/folds/mirror.ts); labels: Mirror.
- **kali** — [engine-gmt/features/geometry/folds/kali.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/geometry/folds/kali.ts); labels: Kali.
- **tetra** — [engine-gmt/features/geometry/folds/tetra.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/geometry/folds/tetra.ts); labels: Tetrahedral (KIFS).
- **octa** — [engine-gmt/features/geometry/folds/octa.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/geometry/folds/octa.ts); labels: Octahedral (KIFS).
- **icosa** — [engine-gmt/features/geometry/folds/icosa.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/geometry/folds/icosa.ts); labels: Icosahedral (KIFS).
- **menger** — [engine-gmt/features/geometry/folds/menger.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/geometry/folds/menger.ts); labels: Menger (Cubic).

## Coverage interpretation

The main review supplies the 3D-to-2D verdict and scored recommendations for these feature families, plus functionality outside the feature registry: timeline/export, Workshop/imports, gallery/sharing/authentication, help/tutorials, undo/session recovery, saved views, and the Gradient Explorer, fluid-toy, fractal-toy and mesh-export companion applications. Legacy aliases retain their meanings; their presence is not another independently implemented formula. Reference examples, assets and historical plans are not automatically active application features.
