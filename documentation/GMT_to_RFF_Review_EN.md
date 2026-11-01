<!-- Modified by GPT-6 on 2026-10-08. -->
<!-- Modified by Opus 5.5 on 2026-11-01. -->

# Improving RFF_Super with GMT as a reference

This is the concise entry point to the detailed review. The target is **RFF_Super**, not a GMT repair project. The downloaded GMT tree contains 3,727 files; all have completed inspection records. A full re-enumeration and SHA-256 comparison on 2026-10-08 found no changed, missing or additional files. Companion reports preserve the detailed chronological evidence; this document states the consolidated recommendations.

The strongest appearance extension for RFF's existing relief materials is an **editable gradient/image environment with roughness-aware reflection**. For flat 2D images, **footprint-aware image-layer filtering** takes precedence, especially in animation. The smallest isolated shader improvement is a **continuous bloom threshold knee**. These are judgments about incremental usefulness, not measured performance or universal visual rankings.

## Implementation status (as of 2026-11-01)

This review and its companions were written on 2026-10-07 and 2026-10-08 and still describe every proposal as proposed. The table below records what RFF_Super has implemented since; [CHANGELOG](../CHANGELOG.md) describes each feature and [NOTICE](../NOTICE) records its provenance. Dates are the earliest local record (NOTICE entry or file creation tag); all six ship in 3.2.0.

| GMT reference | RFF_Super feature | First recorded | Note |
|---|---|---|---|
| Rank 1 below; adoption plan section G | Studio Environment Source (gradient or panorama texture, roughness- and footprint-aware blur) | 2026-10-08 | Implemented as proposed; no GMT code or constants |
| Angle-based coloring modes | Escape Angle Coloring (Shader > Other) | 2026-10-08 | Uses arg z at escape from RFF's own perturbators |
| GMT's Pick Focus control | Pick Focus Depth (Fog, Focus Band) | 2026-10-12 | Reads the clicked pixel's iteration count |
| Polygonal bokeh in `ray.ts` | Chaos Bokeh Shape, Rotation and Aspect | 2026-10-12 | The adoption plan held that true polygonal bokeh needs a depth/occlusion model; RFF instead shapes the Chaos Blur aperture with its own area-preserving map |
| Fragmentarium Mandelbrot-Cellular example | Orbit Trap Coloring: Cellular | 2026-10-12 | Worley F1/F2 applied to orbit points |
| Geometric orbit trap (Point, Sphere, Cross, Plane) | Orbit Trap Coloring: Shape | 2026-10-16 | 2D reading with further RFF-only shapes |

Not implemented as of this date: the continuous bloom threshold knee, and ranks 2 to 10 of the table below. Texture layers gained mipmaps for blurring, which is not the footprint-aware sampling of rank 2. The improvements outside the shader are not assessed here.

## What RFF already provides

RFF already has relief shading, GGX-related material response, clearcoat, key and fill lighting, hemispheric environment light, procedural emission, ordered shader layers, image layers, domain/palette warp, color interpolation, animation and output resampling. The proposals below extend those systems. They do not require replacing its renderer or its deep-zoom mathematics.

Current target anchors: [studio surface](../shdsrc/studio_surface.glsl), [GGX/environment helpers](../shdsrc/studio_ggx.glsl), [palette and image sampling](../shdsrc/palette_sampling.glsl), [effects](../shdsrc/effects.glsl), and [final resampling](../shdsrc/vk_resample.frag).

**MPA is not BLA.** RFF's approximation uses reference-orbit periodic structure and variable period-derived skips. GMT's raymarcher and separate LA/reference-orbit implementation are not drop-in alternatives. Appearance work must retain precise orbit data, compressed-map fidelity and existing recipe regeneration.

## Ten shader-related improvements beyond bloom

Ranking criterion: expected visible creative benefit when using RFF's relief/material features. Cost, scene relevance and overlap are shown explicitly; these are not ten independent projects to add together.

| Rank | Incremental improvement | What changes for the user | 2D transfer and limits |
|---:|---|---|---|
| 1 | Editable environment source and filtered reflection | Place bright strips and colored regions in metallic/coated bands | Uses existing relief normals; no real reflected fractal geometry |
| 2 | Footprint-aware image sampling | Less shimmer and moire in compressed or warped images | Direct flat-2D benefit; highest priority for image-heavy animation |
| 3 | Reusable field-driven material masks | Assign roughness, metalness or coating to selected bands/features | Relief plus reusable 2D masks; extend existing masks |
| 4 | Independent U and V field selection | Combine two signals in image/color lookup | Flat and relief; reuse available fields before adding orbit statistics |
| 5 | Field histogram and palette-range assistance | Place color variation where the chosen data actually lies | Flat and relief; includes authoring work, not only a shader |
| 6 | Palette/image routing into emission | Make a chosen pattern luminous independently of its paint visibility | Flat and relief; emission already exists, indirect light is not implied |
| 7 | Vector noise warp of decorative coordinates | Flowing/marbled patterns independent of palette displacement | Flat and relief; distinguish from existing scalar turbulence/warp |
| 8 | Extend the existing key/fill lighting controls | Additional independently directed/tinted accents | Relief only; key/fill and coating softboxes already exist |
| 9 | Mirror/sector repetition of decorative coordinates | Kaleidoscopic and radial material patterns | Flat and relief; filter fold boundaries |
| 10 | Radius-dependent spiral mapping | Controllable spiral image/palette structures | Flat and relief; start with canvas-relative coordinates |

For rank 8, the inspected studio path already has separate key/fill directions. Its fill tint uses the sky color, and both share a direct-intensity factor. The concrete extension is independent tint/intensity routing and optional additional lights, not simply “add multiple lights.” Rank 3 should expose a shared mask path rather than replace the existing procedural, wetness or boundary masks.

## Detailed mechanisms and integration requirements

### 1. Environment reflection on a 2D relief surface

RFF computes a reflected direction from its relief normal and view vector, then samples a rotated two-color hemisphere. The existing helper has no roughness input. GMT offers a useful source abstraction: solid color, gradient or panorama, with illumination independent of backdrop visibility.

Add an explicit legacy/gradient/image source choice. Evaluate the source along `reflect(-view, normal)`, with independent base-surface and clearcoat roughness. Cache source preprocessing; rotating it should change the lookup transform, not rebuild its image. Keep the current two-color mode and calibration for old scenes. An image source also needs decoding, ownership, persistence and export support; an HDR shader attachment alone does not establish HDR file support.

Roughness and pixel footprint answer different questions. GMT estimates an environment footprint using angular cone width, map dimensions and latitude, then chooses a mip level from the larger of footprint and roughness demands. Near the poles the longitude footprint expands. A simple roughness-to-mip mapping is an artistic approximation; it is not automatically GGX convolution or a complete image-based-lighting solution. Test constant radiance, labeled axes, poles/seams, narrow bright strips, distinct coat roughness, and preview/video/tile agreement.

Detailed source discussion: [adoption journal, section G](GMT_to_RFF_Shader_Adoption_EN.md#g-editable-environment-illumination-for-2d-relief) and [mechanisms appendix](GMT_Fractals_Shader_Mechanisms_EN.md).

### 2. Filter the image lookup before aliasing is created

RFF's inspected image upload disables mipmaps, its relevant samplers clamp to LOD zero, and the common image-layer helper samples level zero. Final output downsampling already exists, but cannot reliably remove aliasing introduced by that earlier lookup.

For unwrapped image coordinates q and image dimensions W,H, estimate the texel footprint across one output pixel:

```text
gx = (W,H) * difference_x(q)
gy = (W,H) * difference_y(q)
rho = max(length(gx), length(gy))
lod = clamp(log2(max(rho, 1)), 0, lastMip)
```

Coordinate image upload, mip generation, image views, sampler limits and shader lookup. Compute differences before wrapping, with double-precision iteration reduction retained; include aspect fit and domain-warp variation. Fragment derivatives need suitable control flow. Compute export needs explicit differences or a deliberately supported derivative facility. Define pixel units globally so tile boundaries do not change the filter.

Filter alpha/coverage with a declared color-space convention and account for fitted-image boundaries. Image-driven displacement has different semantics from ordinary color: filtering displacement and then evaluating a nonlinear palette need not match averaging the final colors. Strongly anisotropic footprints may need an anisotropic or bounded multi-tap refinement. Compare checkerboards, transparent edges, angular seams and slow movement with a high-sample reference.

Detailed target files and requirements: [adoption journal, section H](GMT_to_RFF_Shader_Adoption_EN.md#h-footprint-aware-filtering-for-existing-image-layers).

### 3. Separate fields, coordinates, masks and material properties

GMT's transferable idea is the separation of the signal being observed from the appearance applied to it. RFF currently offers fixed decorative coordinate pairs such as cycle/band, cycle/screen, screen/screen and cycle/angle. Independent selectors would allow `q=(mapU(fieldA), mapV(fieldB))` without multiplying fixed mode combinations.

A reusable mask can be expressed as `m=remap(field)` and used to blend a material property between two authored values. Define the field's valid range, interior/invalid behavior, normalization, sampling footprint and whether it depends on the current palette. Preserve layer order and masking semantics. A new orbit-history field is not “free”: MPA skips must supply a mathematically valid statistic, or the feature needs a clearly defined separate calculation path. Never silently disable acceleration or invent statistics from skipped endpoints.

A histogram should inspect the selected field, exclude invalid values and distinguish linear from logarithmic bins. Provide a freeze/apply operation so animation or navigation does not continuously recolor itself through auto-normalization. Diagnostic channels need documented meanings and export handling; raw data must bypass beauty-only grading, bloom and dithering where those would corrupt it.

### 4. Decorative transforms without changing the Mandelbrot set

Apply optional vector-noise, sector folds, mirrored repetition or spiral mapping to decorative coordinates, keeping the precise fractal map untouched. Transform order matters: a rotation before a fold differs from one after it. Track the mapping's local footprint through those operations and handle seams explicitly.

The historical spiral uses a term proportional to `log(1+r)`. It is regular at the center but only approaches constant winding per radius decade at large r; it is not exactly scale-invariant everywhere. Mirrored repetition can remove an endpoint value jump while still introducing a derivative change. Neither operation is antialiasing by itself.

The spline reference blends arc coordinates using segment-length-weighted inverse-square distance, then samples one palette. Length weights reduce sensitivity to uneven tessellation but do not make a finite closest-point sum an exact integral. This is a lower-priority creative 2D field, not a generic sharpness improvement.

### 5. Bloom knee: a small, separate change

RFF's bright-pass and sharp-glow composition independently use a binary threshold. Define one continuous extraction law for both, with knee zero preserving the legacy behavior. Keep the existing Softness parameter's sharp/blurred-mixture meaning. GMT provides the motivation, but the audit's source-guarded arithmetic probe found a discontinuity in its own extraction; do not copy that formula unchanged.

Test continuity around threshold, thin moving highlights, constant-field behavior, disabled/legacy identity and scene-linear/HDR ordering. The [bloom probe](gmt-audit/bloom-probe-results.json) is CPU equation evidence, not a GPU image test or measured RFF improvement.

## Valuable improvements outside the shader

| Opportunity | Concrete scope | Compatibility or verification condition |
|---|---|---|
| Image-derived palettes | Deterministic color/path analysis producing an ordinary editable RFF palette | Keep source color interpretation explicit; do not alter old recipe IDs/RNG order |
| Palette channel curves and local wave tools | Edit a selected span; preview against the operation's initial state; commit once | Cancel restores exactly; zero strength bypasses lossy refitting; preserve hard steps |
| Restorable image exports | Embed/version the rendering settings needed to reopen an image | Test every save/load entry point and resource dependencies; never substitute a thumbnail for a lossless map |
| Parameter capability metadata | Explain whether a control is active, requires recompilation or lacks a needed field | Share definitions across UI, serialization and rendering; preserve stable IDs |
| Appearance animation/modulation | Editable interpolation and audio-driven appearance using existing animation infrastructure | Explicit time units, deterministic offline evaluation, one value/track mapping when reordered |
| Render-state diagnostics | Distinguish compilation, ready pipeline, submitted work, finished frame and export progress | CPU/UI timing is not GPU timing; stale results must not replace current state |
| Palette collections and imports | Search/filter personal palettes; consistent import/export and reduction warnings | Unique local editing IDs; actual format budgets; batch operations form one undo step |

Palette fitting deserves dark-ramp and discontinuity tests. An 8-bit sRGB step near black can have a much larger OkLab distance than a midtone step; a single small perceptual threshold can turn quantization into false bands. Sequential/diverging/cyclic presets describe a shape, not scientific or accessibility certification. Changing only lightness does not establish full-palette uniformity or cyclic RGB continuity.

## What should remain 3D-specific or deferred

Full raymarching, geometric reflection/refraction, volumetric transport, sphere-light MIS and environment importance sampling require genuine scene geometry and probability-density contracts. Deterministic environment lookup over RFF relief does not need that path-tracing machinery. Optional heightfield/mesh export is a separate feature with geometry, resolution and topology requirements.

Formula composition and new orbit observables have much higher mathematical/integration cost than decorative shaders. GMT's formula names, imported code and finite cross-checks do not prove compatibility with RFF perturbation or MPA. Smooth geometric folds are not image antialiasing. Shader emission and successful compilation are not proof of correct rendered appearance or unbiased transport.

## Evidence, limits and implementation order

1. Preserve current presets and establish reference images/data for the chosen feature.
2. For the smallest shader change, prototype the bloom knee. For image-heavy animation, prioritize image-footprint filtering. For relief art direction, prioritize the environment source.
3. Introduce shared field/mask semantics before independent coordinates and additional routing proliferate.
4. Validate preview, compute/video and tiled output at matched resolution, including dark gradients, discontinuities and high iteration counts.
5. Measure GPU time and memory on representative scenes before claiming a speedup. Retain exact C++/GLSL layouts, append-only ConfigIO and lossless compressed maps.

This review proposes changes; it does not implement them. The audit contains source inspection, selected generated-source checks and isolated CPU arithmetic probes. It does not establish new GPU benchmarks, rendered A/B comparisons or production service behavior. Historical GMT tests are attributed records, not tests run here. Distinctive third-party implementations require checking the original source/license and updating RFF's NOTICE when actually adapted; no external shader implementation is copied by this document.

## Detailed evidence

- [Target-facing adoption journal](GMT_to_RFF_Shader_Adoption_EN.md): extensive mechanisms, target comparisons and chronological follow-ups; later entries may supersede earlier ones.
- [Feature survey](GMT_Fractals_Feature_Review_EN.md): broader workflow opportunities and initial ratings.
- [Shader mechanisms](GMT_Fractals_Shader_Mechanisms_EN.md): equations and 3D-to-2D boundaries.
- [Deep code audit](GMT_Fractals_Deep_Code_Audit_EN.md): counterexamples and implementation hazards, supporting the RFF recommendations.
- [Formula audit](GMT_Fractals_Formula_Audit_EN.md) and [source inventory](GMT_Fractals_Source_Inventory_EN.md).
- [File-by-file coverage ledger](gmt-audit/coverage.json) and [full-source hash verification](gmt-audit/final-source-verification.json).
