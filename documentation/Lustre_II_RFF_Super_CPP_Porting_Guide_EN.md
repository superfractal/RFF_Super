# Lustre II → RFF_Super
## Feature Delta and C++ / Vulkan Porting Specification

**Document date:** 2026-09-10  
**Implementation under review:** `lustre_II_rfc.html` — the latest Lustre II HTML delivered in this conversation  
**Intended use:** engineering reference for selectively bringing the HTML implementation's additions into RFF_Super  
**Status:** source audit and porting specification; **not an implemented or compiled C++ patch**

> **Recommended direction:** retain RFF_Super's fractal solver, precision, legacy appearance, native palette storage, file readers, and export infrastructure. Add an opt-in **Studio GGX** material path and selected editing improvements. Do not replace the native application with the browser renderer or interpret matching control names as numerically interchangeable settings.

## Contents

1. [Baselines and evidence](#1-baselines-and-evidence)
2. [What is genuinely different](#2-what-is-genuinely-different)
3. [Rendering architecture and data contracts](#3-rendering-architecture-and-data-contracts)
4. [Relief, normals, occlusion, and boundary treatment](#4-relief-normals-occlusion-and-boundary-treatment)
5. [Studio GGX material and reflections](#5-studio-ggx-material-and-reflections)
6. [Artistic shading and surface gloss](#6-artistic-shading-and-surface-gloss)
7. [Palette authoring and evaluation](#7-palette-authoring-and-evaluation)
8. [Color processing, bloom, and output](#8-color-processing-bloom-and-output)
9. [RFC / RFSP color import](#9-rfc--rfsp-color-import)
10. [Additional effects and editing workflow](#10-additional-effects-and-editing-workflow)
11. [Native integration map](#11-native-integration-map)
12. [C++ implementation sketches](#12-c-implementation-sketches)
13. [Compatibility and serialization strategy](#13-compatibility-and-serialization-strategy)
14. [Defects and limitations discovered in the audit](#14-defects-and-limitations-discovered-in-the-audit)
15. [Port order and acceptance tests](#15-port-order-and-acceptance-tests)
16. [Verification performed and not performed](#16-verification-performed-and-not-performed)
17. [Complete HTML control reference](#17-complete-html-control-reference)
18. [Non-control state and presets](#18-non-control-state-and-presets)
19. [Local implementation index](#19-local-implementation-index)
20. [Pinned upstream source index](#20-pinned-upstream-source-index)

---

## 1. Baselines and evidence

### 1.1 Upstream baseline

The repository was checked on 2026-09-10. The comparison baseline is commit **`a20d3c3f8b651b69cb36dba26a7334ed614daee7`**, titled **“2.2.1 - Faster Coloring”**, dated 2026-09-06. All upstream links at the end of this document are pinned to that commit rather than to a moving `master` branch. [Upstream revision][U01]

This is the baseline inspected for this document. The exact upstream revision used when the earlier HTML versions were originally generated was not recorded; this document does not reconstruct that historical Git diff.

### 1.2 Local artifacts examined

| Artifact | Role | Bytes | SHA-256 |
|---|---|---:|---|
| `lustre.html` | First browser material study | 90,179 | `43610db8d9af477f6e1465af68283ab46bf3659065b42cc97696c0d832451d79` |
| `lustre_II.html` | Expanded palette / slope editor | 152,713 | `4d19b6ffb04f9c559ef58d41dbd2367e632e140352c7a7529c2c235ec1b76f53` |
| `lustre_II_rfc.html` | Authoritative implementation for this specification | 193,759 | `54afbdc624e7891fb6b8480c54afa89edbe8d5bec82098aa29b0116cea68df19` |

The bundled importer, synthetic `.rfc` fixture, existing test report, and both retained README files were also examined. Statements about **implemented HTML behavior** below come from the actual latest HTML, not solely from previous conversational descriptions. The local source index provides script IDs, symbols, and line locations.

### 1.3 Classification used throughout

| Label | Meaning |
|---|---|
| **Added** | Present in Lustre's inspected rendering/editor path, beyond the corresponding inspected RFF path. Not a claim of an invention of the underlying graphics technique. |
| **Adapted** | An RFF concept was recreated in the HTML, possibly with different arithmetic, units, defaults, or ordering. |
| **Preserved upstream** | Already implemented by RFF; retain it rather than presenting it as a new native feature. |
| **Approximation** | Intentionally simplified or artist-directed; not a physically exact or pixel-compatible implementation. |
| **Audit finding** | A defect or risk in the current code identified during this review; not a correction already applied to the HTML. |
| **Proposed** | Advice for the future C++ port. No such native change is claimed to have been made. |

The document distinguishes **raw palette preservation**, **matching parameter semantics**, and **matching rendered pixels**. These are different guarantees. Keeping RGB float32 entries unchanged does not make the entire shaded image match RFF.

## 2. What is genuinely different

### 2.1 Feature delta matrix

| Area | Classification | Actual Lustre II behavior | Native port action |
|---|---|---|---|
| Palette-derived color vs iteration-derived relief | Preserved upstream | Palette changes do not rebuild or change relief normals. | Keep this separation; do not describe it as a newly fixed RFF defect. |
| RGB intervals, offset, smoothing, cycle bias / curve, iteration coloring | Adapted | Exposed independently in the HTML editor. | Keep native implementations and native precision; translate only when reading Lustre settings. |
| Palette animation, frozen iterations, palette gloss, band lines | Adapted | Browser controls reproduce the concepts, with limitations described below. | Retain the native algorithms for native presets. |
| Color-stop editor | Added editor representation | 2–32 movable, non-uniformly positioned stops, HEX editing, easing, reversal, and equal spacing. | Add optional authoring metadata; do not force existing raw / recipe palettes into 32 stops. |
| Linear RGB palette interpolation | Added mode | Three spaces: encoded sRGB, linear RGB, OKLab. | Extend enum handling safely; fix the packed-bit collision before enabling this mode. |
| Macro relief, independent specular direction, anisotropy, fill, tint, rim, terminator, relief response, surface gloss | Adapted | Exposes these concepts in HTML rather than omitting them. | These are not missing native features. Keep legacy behavior and offer new material semantics separately. |
| Fine-normal smoothing, explicit AO radius, relief waves, inversion | Added / changed | Dedicated controls and a different height/normal construction. | Add a separate relief model or explicit opt-in controls; do not rescale existing Depth globally. |
| Studio GGX | Added material path | Anisotropic GGX distribution, Smith visibility, Schlick Fresnel, roughness, metalness, and IOR. | Add alongside the native artistic reflection model. |
| Analytic studio reflections | Added / approximation | Four softbox shapes and a hemispheric background evaluated in reflection direction. | Optional procedural environment; no external HDR image is required. |
| Clearcoat | Added / approximation | Separate roughness, extra reflection, and attenuation of the base layer. | Port with the basis/Fresnel corrections in Section 14. |
| Thin-film tint | Added / approximation | Three-wavelength cosine modulation of Fresnel color. | Label as artistic iridescence, not spectral thin-film transport. |
| Specular antialiasing | Added heuristic | Screen-space normal derivatives broaden roughness. | Make it optional and validate preview/export sampling behavior. |
| Surface-gloss filtering | Added heuristic | Derivative-based fading of unresolved repeated highlights. | Port independently from changing the existing gloss waveform. |
| Distance-based reflection mask | Added heuristic | Suppresses reflection within approximately a pixel of the set boundary. | Requires a trustworthy distance field; not obtainable from iteration count alone. |
| HDR processing and bloom | Changed implementation | Linear floating-point material/bloom buffers and final peak-based compression. | Extend the native HDR pipeline rather than claiming HDR or linear bloom is absent. |
| Tone-map choices / OKLab grading | Changed / added choices | Peak shoulder, peak Reinhard, peak filmic fit, and a separate OKLab grade. | Add new output modes without reinterpreting existing native enum values. |
| RFC/RFSP color import | Added to HTML; native workflow proposal | Preview first, apply palette and optionally color correction, preserve the scene/material. | Reuse existing C++ loaders; add selective merge and preview rather than another binary parser. |
| Large palettes | Preserved upstream concept; browser extension | Keeps up to 1,048,576 raw RGB float32 entries instead of reducing to editor stops. | Keep native SSBO/recipe capabilities; browser limits are not recommended C++ limits. |
| Pattern, warp, stripe, fog | Adapted / reduced scope | A small procedural layer and simplified effects. | Do not replace the native multilayer effects with the browser subset. |
| JSON state, undo/redo, comparison and diagnostics | Added browser workflow | Reproducible settings, explicit lossy conversion, display/debug views. | Port selectively as UX features; comparison is not a native-RFF image comparison. |

**Upstream evidence for existing capabilities:** palette attributes and upload path [U04], [U08]; slope attributes and host bindings [U02], [U09]; HDR attributes, bloom host, and final output path [U21], [U22], [U23]; native shader bundle and IO APIs [U15], [U16], [U25].

### 2.2 What the latest HTML does not implement

It does not implement RFF's full solver/deep-zoom machinery, complete native shader compatibility, all palette recipes, full image-texture/pattern stacks, native timeline/video export, PQ/HLG output, or writing `.rfc`/`.rfsp` files. It is a 2.5D normal-shaded surface, not displaced 3D geometry, a ray tracer, a spectral renderer, or a measured material model.

A native implementation should preserve capabilities that are more complete upstream. In particular, the native output shader already contains SDR, PQ, and HLG paths; Lustre's display and PNG output are SDR only. [Native output][U22]

## 3. Rendering architecture and data contracts

### 3.1 Actual HTML pass graph

```text
view / iteration settings
         │
         ▼
fractalShader ───────► field: RGBA32F, nearest sampling
                           │
raw RGB palette or         │
authored-stop LUT ──────────┤
                           ▼
                    materialShader
                      │         │
                      │         └─ optional decorated-base comparison render
                      ▼                       ▼
                 scene: RGBA16F          flat: RGBA16F
                      │
                      ▼
              threshold + horizontal blur
                      │
                  bloomA: RGBA16F, quarter dimensions
                      │
                      ▼
                  vertical blur
                      │
                  bloomB: RGBA16F
                      │
                      ▼
 displayShader: scene + bloom → grade → tone map → sRGB → optional RFF grade
                      │
                      ▼
                 SDR canvas / PNG
```

`flat` is the name of a comparison target, not a native RFF render. The comparison pass sets `viewMode = 1`, so the left image is the decorated base color rather than the same diffuse shading with reflection disabled. It also omits bloom on that side. This is **Base vs Shaded**, not a strictly isolated one-variable BRDF comparison.

### 3.2 Field-channel contract

| Channel | Meaning in the latest HTML |
|---|---|
| R | Escaped smooth iteration value, clamped to at least 1 for escaped pixels. |
| G | `log2(distanceEstimate)`, bounded approximately to `[-100, 100]` in the solver. |
| B | Unused, written as zero. |
| A | Escaped/exterior mask: 1 outside, 0 for the interior. Not fractional pixel coverage. |

The browser solver uses double-single orbit arithmetic, a float derivative, a maximum of 2,000 iterations, and a minimum view span of `2e-7`. These are implementation constraints of the demonstration. **Do not backport its lower precision or solver limits.**

The native iteration descriptor already stores iteration data as doubles. Keep that precision for relief differences and palette phase reduction. A float normal is appropriate after a stable relative-height computation, not as a replacement for the source iteration values. [Native iteration descriptor][U10]

### 3.3 Color-space contract

The HTML's material pass operates principally in **linear RGB**, and the scene/bloom attachments contain linear values that may exceed 1. The stop palette is baked into linear RGB; an imported raw palette remains encoded RGB in its storage and is converted explicitly during lookup. HEX color controls are decoded before being bound to material uniforms.

Important exceptions are intentional artistic operations: overlay shading, native-style palette gloss, band-color mixing in the imported path, and `lightBlend = 0` temporarily operate in encoded RGB. These exceptions must remain explicit rather than being hidden behind an attachment format.

For a Vulkan port, document the signal carried by every attachment. An `RGBA16F` image can still contain encoded, headroom-scaled values; its storage format alone does not establish that the contents are scene-linear. The native final pass decodes its input and restores headroom in its existing HDR path. A new true-linear path must bypass or adapt that decoding, not perform it twice. [Native output contract][U22]

### 3.4 Invalidation and resource behavior

The HTML separates geometry, palette upload, material, and display dirty flags. Moving/zooming the fractal or changing iterations recomputes the field. Relighting, recoloring, and palette animation reuse the field. Exposure and several finishing controls only redraw the display pass. Bloom threshold/radius changes currently request more work than strictly necessary; do not call this an optimally scheduled graph.

The renderer uses GPU fences, reduced interactive resolution, and a later settled-resolution redraw. The importer may use asynchronous file reading, but parsing/recipe generation runs on the main JavaScript thread. Neither feature is a worker-based renderer or importer.

For the native port, prefer existing request/invalidation infrastructure and immutable material/palette snapshots during background rendering. The browser's event-loop strategy is not a native threading design.

## 4. Relief, normals, occlusion, and boundary treatment

### 4.1 Height function — exact HTML behavior

Let `m` be a neighbor's iteration value and `c` the center's value. Define:

```text
x = (max(m, 1) - max(c, 1)) / (max(c, 1) + 4)

h = [x - x²/2 + x³/3 - x⁴/4 + x⁵/5] / ln(2)     when |x| < 0.2
h = log2((max(m, 1) + 4) / (max(c, 1) + 4))       otherwise

h += 0.24 * reliefWaves * [sin(m * waveFrequency) - sin(c * waveFrequency)]

if invertRelief: h = -h
```

The Taylor branch avoids subtracting nearly equal absolute logarithms. The `+4` bias changes the shape near low iteration counts. Relief waves are part of the **height**, not the palette; changing them changes normals and AO. Their frequency is in radians per iteration, not palette cycles.

Invalid/interior neighbor samples contribute a height difference of zero. Samples are clamped to the field edge. This is a local visual approximation, not reconstruction of an unknown interior surface.

### 4.2 Sobel sampling and scale

For a sampling radius `r`, the eight-neighbor Sobel gradient is divided by `8*r`. Its eight height differences are also averaged into a mean used by AO. Radius is reduced near the viewport boundary, with a minimum radius of one texel.

```text
s          = imageHeight / 800
fine       = Sobel(1)
smoothG    = Sobel(2)
r          = max(1, macroRadius * s)
wide       = [Sobel(r) + Sobel(0.66*r) + Sobel(0.33*r)] / 3
grad       = mix(mix(fine, smoothG, normalSmooth), wide, macroRelief)
gained     = grad * (68 * depth * s)
magnitude  = length(gained)
limited    = 2.5 * [1 - exp(-0.55 * magnitude^0.70)]
N          = normalize((-gained * limited / max(magnitude, 1e-9), 1))
```

The limiter is not linear near zero. It changes small-slope response as well as bounding steep slopes. `macroRadius` and `aoRadius` are reference-height pixel controls; they are not world-space distances.

The current source evaluates six Sobel neighborhoods per material pixel: fine, smoothed, three macro radii, and AO. This is approximately 48 height taps plus the center fetch before considering compiler elimination/caching. It does not skip all optional neighborhoods when their blend weights are zero. The comparison path can repeat the material pass.

### 4.3 Native compatibility boundary

RFF's slope uses a different height baseline, width-based `1280` reference scale, Sobel normalization, depth gain, and slope limiter. Its small-difference log calculation and edge-aware macro sampling already exist. Consequently, **there is no single reliable conversion factor from RFF Depth to Lustre Depth**, and copying the numeric value will not preserve the image. [Native relief implementation][U03]

**Proposed:** retain a `LegacyRff` relief model and add a separate `LustreRelief` model. Alternatively, reuse native normals for the first GGX milestone and add Lustre's relief model later. That gives a useful material improvement without simultaneously changing the geometry implied by every preset.

### 4.4 Ambient occlusion

The HTML AO is a concavity heuristic:

```text
aoRadiusPixels = max(1, aoRadius * s)
concavity     = max(0, 0.65 * meanAtAoRadius + 0.35 * meanAtSecondMacroRadius)
AO            = 1 - aoIntensity * [1 - exp(-14 * depth * concavity)]
```

This is not ray-traced visibility, horizon-based AO, or a screen-space depth-buffer AO algorithm. `macroRadius` can affect AO even when `macroRelief` is zero because the second macro-radius mean is still used. A native UI should either document that coupling or separate the AO support radii.

### 4.5 Boundary reflection attenuation

```text
pixelWorldSize = span / imageHeight
away           = exp2(clamp(log2Distance - log2(pixelWorldSize), -20, 20))
reflectionMask = smoothstep(0.14, 1.15, away)
```

This mask multiplies the reflection contribution, including rim/gloss, before composition. It does not antialias the interior silhouette or provide fractional field coverage.

**Native dependency:** supply a distance estimate in the same coordinate scale as `span / height`. Iteration values alone are insufficient to recreate it. Until that field is available and verified for the native projection/formula, disable this mask rather than derive an arbitrary surrogate. Projection-dependent distance/pixel relationships require separate treatment.

## 5. Studio GGX material and reflections

### 5.1 Material controls and normal roles

`renderModel = 0` selects Studio GGX. The principal additions are `roughness`, `metalness`, `ior`, `coat`, `coatRoughness`, `environmentIntensity`, `softness`, `lightIntensity`, `iridescence`, and `filmThickness`. Existing specular/anisotropy controls are reused with **new semantics** in this path.

There are two normals:

- `N`: actual relief normal, used for diffuse and parts of clearcoat.
- `Ns`: artist-adjusted reflection normal, interpolating its tilt between a half-light-angle anchor and the actual relief tilt.

```text
V          = normalize(((uv - 0.5) * (-0.10, -0.06), 1))
actualTilt = acos(clamp(N.z, -1, 1))
anchorTilt = radians(activeSpecularZenith) / 2
tilt       = mix(anchorTilt, actualTilt, reliefResponse)
Ns         = (normalize(N.xy + (1e-12, 1e-12)) * sin(tilt), cos(tilt))
```

At `reliefResponse = 1`, reflection follows the actual relief. At zero, it follows an anchored artistic tilt. Independent specular lighting selects its own azimuth/zenith without moving the diffuse key light. A native port should preserve the existing linkage UI but take care with its inverted GPU link flag. See Section 11.

### 5.2 Screen-space specular antialiasing

```text
variance = min(0.12,
    0.19 * (dot(dFdx(N), dFdx(N)) + dot(dFdy(N), dFdy(N))))

r = clamp((roughness^4 + variance)^0.25, 0.055, 0.96)
```

The fourth-power combination widens the effective distribution where normals vary quickly. It can reduce isolated sharp sparkle, but it is a heuristic. It is not temporal antialiasing, a mipmapped normal-distribution filter, or a guarantee against flicker.

For a compute implementation, fragment derivatives are not a portable drop-in operation. Use explicit neighbor differences or a precomputed normal/variance image, and make its pixel-footprint convention agree with the fragment path. Evaluate derivatives outside divergent per-pixel branches in the fragment implementation.

### 5.3 F0 and anisotropic roughness

All following RGB quantities are linear unless an exception is stated.

```text
F0dielectric = ((ior - 1) / (ior + 1))²
F0          = mix((F0dielectric, F0dielectric, F0dielectric), decoratedBase, metalness)

alpha       = max(0.018, r² * sqrt(64 / max(specularPower, 1)))
aspect      = sqrt(max(0.12, 1 - 0.88 * abs(anisotropy)))
lightBlur   = 0.025 + 0.07 * softness
alphaX      = sqrt((alpha / aspect)² + lightBlur²)
alphaY      = sqrt((alpha * aspect)² + lightBlur²)
```

`specularPower` is an additional roughness-width control in Studio mode, not an exponent applied to `N·H`. Raising it narrows the lobe, subject to the width floor. `softness` broadens both the analytic light appearance and the direct GGX lobe, so it is not solely an environment control.

The tangent seed is the projected 2D gradient direction rotated by `anisotropyAngle`; the seed is projected onto the plane perpendicular to `Ns`, normalized to `T`, and `B = cross(Ns, T)`. The current zero-gradient guard is incomplete; use the corrected construction proposed in Section 12.

### 5.4 Distribution, visibility, Fresnel

For normalized view/light vectors `V`, `L`, and `H = normalize(V + L)`:

```text
q = (dot(H,T)/alphaX)² + (dot(H,B)/alphaY)² + dot(H,Ns)²
D = 1 / max(PI * alphaX * alphaY * q², 1e-6)

Nv = max(dot(Ns,V), 0.001)
Nl = max(dot(Ns,L), 0.001)

Gv = Nl * length((alphaX*dot(T,V), alphaY*dot(B,V), Nv))
Gl = Nv * length((alphaX*dot(T,L), alphaY*dot(B,L), Nl))
Visibility = 0.5 / max(Gv + Gl, 1e-6)

F = F0 + (1 - F0) * (1 - clamp(dot(V,H), 0, 1))^5
specularLight = D * Visibility * F * lightRGB * max(dot(Ns,L), 0)
```

**Do not divide by `4 * Nv * Nl` again.** `Visibility` here already includes that factor together with Smith masking/shadowing. This is a common integration error when moving between a `G` implementation and a visibility-function implementation.

The key light uses `specularColor * lightIntensity * 3`. The fill uses `(0.7, 0.85, 1.0) * fillIntensity * lightIntensity * 1.7`. These multipliers are artistic exposure choices, not calibrated photometric units. The width inflation is an area-light approximation, not integration over a rectangular emitter.

### 5.5 Procedural studio environment

The reflection vector is `reflect(-V, Ns)`. It is rotated around Z according to the active specular azimuth. A low-intensity hemispheric background is combined with four projected rectangles:

| Rectangle | Direction before common azimuth rotation | Half-size | Linear intensity/color |
|---|---|---|---|
| Main | `lightDirection(132°, activeZenith)` | `(0.09 + 0.32*softness, 0.85)` | `5 * specularColor` |
| Cool side | `normalize(0.85, 0.12, 0.65)` | `(0.08 + 0.16*softness, 0.75)` | `2.6 * (0.63, 0.80, 1)` |
| Warm lower | `normalize(-0.12, -0.88, 0.57)` | `(0.65, 0.045 + 0.08*softness)` | `2.8 * (1, 0.77, 0.48)` |
| Broad upper | `normalize(0.2, 0.95, 0.3)` | `(1.1, 0.24)` | `0.8 * (0.8, 0.93, 1)` |

The hemisphere interpolates between `(0.015, 0.023, 0.04)` and `(0.18, 0.22, 0.25)` using `smoothstep(-0.6, 0.85, reflectedDirection.y)`.

For each rectangle, the reflection direction is projected into its local tangent/bitangent plane, divided by the forward dot product, and feathered around the rectangle bounds. A front-facing fade removes backward reflections. A size/blur ratio approximately compensates intensity as the rectangle softens.

```text
blur        = 0.018 + 0.88 * r²
blurXY      = (blur * (1 + 1.9*abs(anisotropy)), blur)
Fenv        = F0 + (max(1-r, F0) - F0) * (1-Nv)^5
environment = studio(reflection, r) * Fenv * (1 - 0.25*r) * mix(1, AO, 0.3)
```

`studio()` includes `environmentIntensity`. Thin-film tint may also modulate `Fenv`. This is not an HDR cubemap, a prefiltered environment convolution, or visibility-aware reflection. The current environment blur does **not** rotate with `anisotropyAngle`; only direct GGX uses the full tangent orientation.

### 5.6 Clearcoat layer

The HTML computes:

```text
coatFresnel = F0dielectric + (1-F0dielectric) * (1-Nv)^5
attenuation = (1 - coat * coatFresnel)²

coatReflection = studio(reflect(-V,N), filteredCoatRoughness) * coatFresnel * coat
               + directCoatSpecular * coat

reflection = (direct + environment) * specularIntensity * attenuation
           + coatReflection

diffuse = shadedBase * (1 - 0.65*metalness) * attenuation
```

The coat's environment roughness has a derivative floor; its direct roughness has a `0.065` floor. Direct coat intensity uses `1.3 * lightIntensity * specularColor` and power `64`.

Two behaviors matter when porting. First, `specularIntensity = 0` does not disable clearcoat, rim, or surface gloss. Second, the current coat calculation mixes the actual normal with a tangent basis and Fresnel angle derived for `Ns`; these disagree when relief response is not one. Correct that deliberately, with a compatibility option for reproducing the existing HTML.

The layer is an artistic approximation. It is not a complete energy-conserving dielectric coating model.

### 5.7 Thin-film color approximation

```text
eta = 1.38
cosTransmitted = sqrt(max(0, 1 - (1-cosView²)/eta²))
filmRGB = 0.55 + 0.45 * cos(
    TAU * 2 * eta * filmThickness * cosTransmitted / (650,510,475)
    + (0,0.15,0.32))

F = mix(F, F * (0.35 + 1.15*filmRGB), iridescence)
```

Thickness and the three sample wavelengths are in nanometers. This produces an angle-dependent color impression, not a spectral integration over illuminant and sensor responses. It can amplify a Fresnel term beyond one. Keep the UI label **“artistic thin-film tint”** unless a physical model is implemented separately.

## 6. Artistic shading and surface gloss

### 6.1 Diffuse and tint pipeline

The following is the HTML's order, not a proposed reordering of existing native shading:

```text
raw       = dot(N, keyLight)
rawFilled = clamp(raw + max(dot(N, fillLight),0)*fillIntensity, 0, 1)
shade     = max(reflectionRatio, rawFilled)
wrap      = clamp((rawFilled + terminatorSoftness)/(1+terminatorSoftness), 0, 1)
shade     = mix(shade, mix(reflectionRatio,1,wrap), terminatorSoftness)
shade     = mix(1, max(shade,1e-5)^(1/slopeGamma), lumaAmount)
```

The shading composite has three HTML values:

| HTML value | Operation |
|---:|---|
| 0 | Overlay-style composition in encoded RGB using `shade/2` as the overlay input. |
| 1 | `linearBase * shade`. |
| 2 | Convert linear base to OKLab, multiply L by `sqrt(shade)`, then convert back. |

Tint interpolates the **linear** ground and sky controls using `clamp(rawFilled,0,1)^tintResponse`. Multiply mode mixes from white to that tint. OKLab tint mode scales L with `ambientIntensity*0.45`, adds tint chroma with `ambientIntensity*0.6`, and applies `shadowChroma` toward the shadow end. The result is multiplied by `slopeBrightness` and AO.

RFF's native shading enum reserves value 1 for a different historical purpose; only 0 and 2 are currently exposed there. Do not assign the HTML's linear-multiply meaning to native value 1 without a versioned extension. [Native shading enum][U07]

### 6.2 RFF-style artistic highlight

`renderModel = 1` replaces the Studio reflection contribution with an artistic highlight and rim. It does not reproduce native output exactly.

```text
aspect = atan(N.y,N.x)
power  = max(0.1,
    specularPower * (1 + 3*anisotropy)^(-cos(2*(aspect-angle))))
sp     = smoothstep(0,1,clamp(dot(Ns,normalize(specLight+V)),0,1))^power
sp    *= specularIntensity
sp    *= sin(actualTilt)/(sin(actualTilt)+0.05)
sp    *= clamp(shade*slopeBrightness,0,1)
```

Rim uses the ungained fine gradient:

```text
detail = length(fine) * (imageHeight/800) * 12
rim    = [detail/(detail+1)]^rimPower * rimIntensity
```

The native implementation differs in its normal/aspect conventions and uses a tangent-slope flat fade. Its OKLab lightness shading also uses a different strength. Treat this HTML mode as **RFF-inspired**, not as the compatibility path for existing native presets. [Native artistic shading][U03]

### 6.3 Surface gloss modes

Surface gloss is distinct from palette gloss. It is added after the selected reflection model and is evaluated from surface-derived coordinates.

| `glossSource` | HTML coordinate `g` | Detail |
|---:|---|---|
| 0 | Processed `shade` | Depends on diffuse wrap/gamma/luma settings. |
| 1 | `detail / (detail+1)` | Fine-relief density. |
| 2 | `atan(N.y,N.x)/TAU + 0.5` | Slope direction; band count is rounded to an integer. |
| 3 | `max(dot(normalize((-fine*2^glossRelief*s,1)), keyLight),0)` | Fine normal with a gain independent of Depth and macro relief. |

```text
peak = [0.5 + 0.5*cos(TAU*(g*glossBands + glossPhase))]^glossSharpness
unresolvedFade = 1 - smoothstep(0.12, 0.7, fwidth(g)*glossBands)
gloss = peak * glossIntensity * flatFade * litFade * unresolvedFade
```

For source 3, `flatFade` is derived from the independent normal's XY length and `litFade` from its lighting. Other modes use the actual tilt and processed shade. All contribute through `glossColor`.

The native waveform is sine-based and its shading coordinate is not always the HTML coordinate. With an otherwise identical coordinate only, `phaseHTML = fract(phaseRFF - 0.25)` aligns cosine to sine. This identity is **not** a complete visual conversion. [Native gloss][U03]

### 6.4 Composition and bypass behavior

`lightBlend = 1` adds diffuse and reflection in linear RGB. Value 0 encodes both terms, adds them, and decodes the sum before the final display pass. The latter is an artistic encoded-space composite, not an exact legacy hard-clipping path.

`depth <= 0` sets the effective slope opacity to zero. Otherwise `slopeOpacity` blends between the decorated base and the shaded surface. Iteration fog is applied afterward. Diagnostic modes may replace the result, but still pass through finishing and display conversion.


## 7. Palette authoring and evaluation

### 7.1 Two representations, not one reduced palette

The latest HTML deliberately separates:

| Representation | Authoritative data | Editing | GPU representation |
|---|---|---|---|
| Authored stops | 2–32 `{pos, color}` records | Positions, HEX colors, insert/delete, reverse, equal spacing | 4,096-sample, one-row, linear-RGB `RGBA16F` LUT |
| Imported raw/recipe palette | Full RGB float32 array or a recipe/seed descriptor | Mapping controls remain editable; stop editing is locked | `RGBA32F` texture, manual entry interpolation |

For raw palettes, texture width is `min(1024, count)` and height is `ceil(count/width)`. Alpha is set to one for GPU storage. This arrangement is a WebGL storage workaround, not a recommended replacement for RFF's native palette SSBO. [Native palette upload][U08]

**Explicitly lossy actions:** converting an imported palette into 32 editable stops, or exporting a sampled 256-color `.map`. The original imported descriptor survives undo after conversion. A normal settings/JSON save preserves the imported representation instead of silently converting it.

### 7.2 Color-stop interpolation

Positions are sorted in normalized palette space. Interactive positioning is bounded to approximately `[0, 0.9999]`; validation rejects almost coincident neighboring stops. Each stop stores an 8-bit-per-channel HEX color.

The local interpolation fraction is transformed by a separate stop-easing control:

```text
Linear:      t
Smoothstep:  t²(3 - 2t)
Smootherstep:t³(t(6t - 15) + 10)
```

The eased fraction is used in one of three spaces:

```text
HTML 0: mix(encodedRGB_A, encodedRGB_B, t), then decode to linear RGB
HTML 1: mix(linearRGB_A,  linearRGB_B,  t)
HTML 2: mix(OKLab_A,     OKLab_B,     t), then convert to linear RGB
```

The authored-stop CPU result is clamped to `[0,1]` before LUT upload. Samples are evaluated at `(index + 0.5) / 4096`; the shader uses the texture's linear filtering. This adds a finite LUT approximation on top of the chosen interpolation. It is not direct arbitrary-stop interpolation per fragment.

**Stop easing is not Color Smoothing.** The latter transforms fractional iteration counts before palette mapping; stop easing only shapes blending between color knots. Likewise, the `coloring` smoothstep modes reshape the *cycle coordinate*, not the interpolation fraction between adjacent color entries.

### 7.3 Seamless has two different meanings

For authored stops, `seamless` connects the last stop to the first across the cycle boundary. With it disabled, the CPU sampler holds endpoint colors outside their stop interval.

For an imported palette of `N` entries, the seamless logical sequence contains `2N` entries:

```text
C0, C1, ... C(N-1), C(N-1), ... C1, C0
```

The duplicated endpoint entries are intentional. The GPU computes the mirrored index without allocating a doubled source array. These semantics must not be collapsed into a single “wrap on/off” flag in a native editor.

Even a non-seamless authored-stop LUT is sampled cyclically by the material lookup, so it can have a discontinuity at the cycle boundary. Do not equate endpoint holding during LUT baking with non-cyclic fractal coloring.

### 7.4 Iteration mapping

For each channel period `P`, first apply `smoothing`:

```text
0 / None:     I = floor(m)
1 / Normal:   I = m
2 / Reversed: I = floor(m) + 1 - fract(m)
```

Then evaluate `x = max(I/P, 0)` and the selected coloring transform:

| HTML `coloring` | Transform |
|---:|---|
| 0 | `x` |
| 1 | `sqrt(x)` |
| 2 | `cbrt(x)` |
| 3 | `log2(1+x)` |
| 4 | `log(1+log(1+x)) * 1.8990140986` |
| 5 | `floor(x) + smoothstep(0,1,fract(x))` |
| 6 | `floor(x) + smootherstep(fract(x))` |

Take the fractional cycle coordinate and apply `cycleCurve`:

```text
Power: y = q^cycleBias
Wave:  y = q + ((cycleBias-1)/(cycleBias+1)) * sin(TAU*q)/TAU
```

Offset and animation/warp shifts are added after this bias, and then wrapped. With independent RGB periods, each output component comes from a separate lookup of the same palette. `rgbLinked` copies the shared `cycleLength` into the three channel controls; it is an editor convenience.

RFF already has the underlying interval, curve, smoothing, and coloring concepts. Native porting should generally retain native evaluation rather than replacing its double-precision phase handling with the browser's float shader arithmetic. [Palette mapping implementation][U05]

### 7.5 Enum translation and the packed-bit hazard

The HTML and native interpolation IDs are **not aligned**:

| Meaning | HTML | Current native | Proposed extended native |
|---|---:|---:|---:|
| Encoded RGB | 0 | 0 | 0, unchanged |
| OKLab | 2 | 1 | 1, unchanged |
| Linear RGB | 1 | Not present | 2, new and explicitly versioned |

The proposed third column extension is only a storage/API proposal. It is not safe to feed it unchanged into the current packed GPU word.

The native packed word allocates low 8 bits to smoothing, **bit 8** to interpolation, bit 9 to cycle curve, and bits 10–13 to iteration coloring. Writing interpolation value 2 with `value << 8` sets bit 9 and collides with the cycle curve. Both the uniform fallback and specialization constants use this packing. [Native interpolation enum][U06]; [mode packing][U11]

**Proposed safe approach:** keep existing IDs stable and provide an additional, separately represented interpolation mode for the new renderer, or introduce a versioned wider bitfield after auditing every reader. Update both `vk_iteration_palette.frag` and `vk_2_map_iter_stripe.comp`, their pipeline-specialization keys, and dynamic/timeline updates. Do not fix only the fragment shader or only the UI dropdown. [Mode consumers][U11], [U12]

### 7.6 CPU / GPU color conversion consistency

The HTML CPU stop interpolation and material helpers use the piecewise sRGB transfer:

```text
decode(c) = c/12.92                         if c <= 0.04045
          = ((c+0.055)/1.055)^2.4            otherwise

encode(c) = 12.92*c                         if c <= 0.0031308
          = 1.055*c^(1/2.4) - 0.055         otherwise
```

The inspected native CPU `blendPaletteColors()` uses a `2.2` power approximation for its OKLab conversion, while the palette fragment shader has piecewise sRGB conversion. That is a concrete consistency issue to test when matching CPU previews/baked bands to GPU interpolation. [CPU helper][U04]; [GPU helper][U05]

**Proposed correction:** share a precisely specified transfer implementation and OKLab matrices between CPU reference tests and shaders. Introduce it deliberately: changing baked CPU interpolation may change legacy palette previews or resampled band colors. It is not a no-risk cosmetic refactor.

### 7.7 Palette gloss and band lines

The ordinary stop path adds an adjustable one-cycle cosine highlight in linear light:

```text
paletteHighlight = [0.5 + 0.5*cos(TAU*ratio)]^paletteGlossPower
base += paletteGlossColor * paletteGloss * paletteHighlight * derivativeFade
```

The imported RFF path instead applies the native-style three-wave, power-20 gloss to encoded source entries before interpolation. It uses `rffGloss`, not the ordinary adjustable `paletteGloss` amount. Its color input is converted back to encoded RGB for this operation. Keeping both paths is necessary to avoid changing imported palettes merely by opening them.

Imported band lines are evaluated as a continuous coverage function after adjacent-entry interpolation. The source-array RGB remains intact. Native upload can bake/resample bands; therefore a raw source-array match does not guarantee identical band edges. [Native upload][U08]

The ordinary stop path uses `fwidth`-based line smoothing/fading. The imported band path does not have the same derivative antialiasing. Very high imported line counts can alias; see Section 14.

### 7.8 Animation and frozen colors

Ordinary Lustre animation speed is in **palette cycles per second**. Imported `rffSpeed` is in **iterations per second**, and the imported shader shift has the opposite-sign phase convention:

```text
ordinary shift = palettePhase
RFF shift      = -rffPhase / cycleLength
```

Psychedelic, breathing, and turbulence modes are procedural browser approximations; they are not native-coordinate reimplementations. The HTML uses viewport-derived coordinates for spatial flow. The import plan maps native animation IDs `0,1,3,4` to HTML `0,1,2,3` and divides imported flow amount by the red interval.

Up to 16 iteration values can be frozen. The matching metric is the maximum circular cycle-distance across RGB periods. A smooth tolerance mask blends between animated and non-animated palette lookup. In imported mode the match is evaluated before cycle bias; in ordinary mode it is evaluated after bias.

A frozen iteration does **not** freeze the final shaded pixel forever. Editing the palette, changing the material/lighting, or using warp can still change it. Warp remains present in both the animated and frozen lookups. Imported tolerance zero disables freezing in the imported path.

The importer sets `animatePalette` from whether the base animation speed is nonzero, then leaves global playback paused after applying the file. A nonlinear flow with zero base speed is not automatically enabled, even if its own flow speed is nonzero.

## 8. Color processing, bloom, and output

### 8.1 Exact finishing order

The latest HTML display pass performs:

```text
linear scene + linear bloom
    → multiply by 2^exposure * brightness
    → vignette
    → OKLab hue rotation, chroma saturation, and L contrast
    → peak-based tone mapping
    → sRGB encoding
    → ordinary output gamma
    → optional imported RFF-style color correction
    → output dither / grain
    → clamp to SDR [0,1]
```

For the ordinary OKLab grade, hue is in degrees, saturation is a multiplier on a/b chroma, and contrast applies `(L-0.5)*contrast + 0.5`. Ordinary `brightness` is a multiplier; ordinary exposure is measured in stops. None of these should be populated directly from native additive/rational color settings.

The vignette is a screen-space multiplier, not an optical lens simulation. Normal/AO/reflection diagnostic views still go through this finishing chain; their displayed pixels are not raw numerical buffers.

### 8.2 Peak-based tone maps

Let `p = max(R,G,B)` for a nonnegative linear RGB value `C`.

| HTML `tonemap` | Mapping |
|---:|---|
| 0 / Soft shoulder | For `p > k`, `q = k + (1-k)*(1-exp(-(p-k)/max(1-k,0.001)))`; return `C*q/max(p,1e-5)`. Below knee, leave C unchanged. |
| 1 / Peak Reinhard | `C / (1+p)`. |
| 2 / Filmic peak fit | `q = clamp(p*(2.51*p+0.03)/(p*(2.43*p+0.59)+0.14),0,1)`; scale all components by `q/max(p,1e-5)`. |
| 3 / Clip | No tone-map compression in this stage; the final output is still clamped. |

Scaling components together preserves their ratios **at this stage**. Subsequent grading, gamma, and gamut clipping can still alter perceived hue/saturation. The filmic curve is a simple fit evaluated on the peak, not the full ACES transform. The HTML Reinhard variant is not component-wise Reinhard or the native headroom-normalized extended Reinhard mode.

RFF already has highlight compression and output tone mapping. The changes here are the chosen curve, its peak-based use, and placement in Lustre's pipeline—not the invention of highlight rolloff. [Native slope][U03]; [native final output][U22]

### 8.3 Bloom

The HTML bloom is a two-pass separable blur at one-quarter width and height. A threshold/soft-knee extraction is performed only in the first pass:

```text
p = max(R,G,B)
k = max(0.001, bloomSoftness * bloomThreshold)
s = clamp(p - bloomThreshold + k, 0, 2*k)
s = s² / (4*k)
extracted = C * max(p-bloomThreshold, s) / max(p, 0.0001)
```

Each blur pass uses the center and two symmetric offset pairs:

| Offset | Weight per sample |
|---:|---:|
| 0 | 0.227027 |
| ±1.384615 | 0.316216 |
| ±3.230769 | 0.070270 |

The host scales the direction by radius/texel size. Bloom is added to the linear scene with `bloomAmount` before the output curve. This is a compact glow filter, not a multiscale optical glare model. The native bloom already exposes a linear-add option; the browser shader is an alternative implementation, not a reason to remove that option. [Native bloom][U21]

### 8.4 Imported RFF-style color correction

The optional `rffGrade` branch is separate from the ordinary grade. It acts on the SDR encoded image after ordinary tone mapping and gamma. Each stage clamps to `[0,1]`:

```text
1. gamma:      C = C^(1/max(rffGamma, 0.001))
2. exposure:   C = C * (1+e)/(1-e), e = min(rffExposure,0.999)
3. hue:        HSV-sector hue rotation in cycles, preserving min/max for this step
4. saturation: gray = 0.3R + 0.59G + 0.11B
               C = C + (C-gray)*rffSaturation
5. brightness: C = C + rffBrightness
6. contrast:   C = (C-0.5)*(1+q)/(1-q) + 0.5, q = min(rffContrast,0.999)
```

The source for the compatibility intent is the native color shader. Placement in Lustre's overall rendering chain differs, so this is parameter-level compatibility, not a full native-image reconstruction. [Native color correction][U19]

When optional color correction is imported, the ordinary grade is reset to neutral: exposure 0, contrast/saturation/brightness/gamma 1, hue 0. Tone mapping, vignette, bloom, and material settings remain unchanged. When the checkbox is off, existing ordinary and imported-grade settings are preserved.

For a single unclipped gain stage only, a rational exposure `e` corresponds to `EV = log2((1+e)/(1-e))`, for `-1 < e < 1`. This does not convert the complete pipeline because clamps and operation ordering are not equivalent.

### 8.5 Dither and output restrictions

The HTML always adds a deterministic sub-code-value noise term:

```text
noiseAmplitude = (1 + 5*grain) / 255
```

The noise itself is centered around zero with a range of approximately `[-0.5,0.5]`. Therefore `grain = 0` still leaves half-an-8-bit-step dither. It is not a true dither-off state. Native float-buffer regression tests must bypass this display noise, and a future UI should separate dither enable from artistic grain amount.

Export is SDR PNG. There is no float EXR, HDR metadata, PQ, or HLG output in the HTML. Preserve the native HDR output options when integrating the material path.

## 9. RFC / RFSP color import

### 9.1 Scope and user-visible contract

The latest HTML adds a color-only import workflow with preview, warnings, cancel, and a single undoable apply action. It accepts `.rfc` and `.rfsp` through the import dialog, main load action, or file drop.

**Applied:** palette data/recipe, mapping, interpolation, smoothing, cycle controls, palette gloss, interior color, band settings, animation/freeze settings, and optionally color correction.

**Preserved:** center, zoom/span, iteration budget, rotation, lighting, slope, material, clearcoat, fog, bloom, pattern, and warp settings already in Lustre. Global playback is paused on apply; imported palette phases are reset.

**Not imported into the render:** native image textures, pattern stacks, slope parameters, fog, bloom, HDR output, formula, projection, and video/timeline settings. Some are parsed or skipped solely to reach later palette fields without losing byte alignment.

The presence of parsed `slope` or `bloom` objects in `RFFImport.parse()` is not evidence that the UI applies those settings.

### 9.2 Supported versions and byte representation

| File family | Numeric magic | Supported versions | Legacy palette | Hybrid palette | Frozen iterations |
|---|---|---|---|---|---|
| RFC configuration | `0x52464643` | 3, 4, 5 | v3 | v4–5 | v5 |
| RFSP shader preset | `0x52465350` | 1, 2, 3 | v1 | v2–3 | v3 |

The importer reads little-endian fields explicitly with `DataView`. The magic is a serialized integer: its little-endian bytes are `43 46 46 52` for RFC and `50 53 46 52` for RFSP. Checking literal ASCII `RFFC`/`RFSP` bytes in that order would be wrong for this representation. Big-endian input is rejected rather than guessed. Current upstream constants are in [ConfigIO.h][U16] and [ShaderPresetIO.h][U15].

For an RFC file, the shader section follows variable-length center/formula strings and calculation/render settings. Its location must be reached by typed parsing; it is **not a fixed byte offset**. In the retained synthetic fixture it happens to begin at byte 191, which is not a format constant.

### 9.3 Palette binary layouts

```text
Legacy palette (RFC v3 / RFSP v1):
    uint64 colorCount
    repeated colorCount times: float32 R, G, B, A

Hybrid palette (RFC v4–5 / RFSP v2–3):
    int32 recipeId
    if recipeId < 0:
        uint64 colorCount
        repeated colorCount times: float32 R, G, B
    else:
        uint32 seed
        colors regenerated from recognized recipe
```

After the palette come its core mapping fields, then stripe/slope/color/fog/bloom data and optional tails. The legacy alpha is consumed for alignment but discarded from the imported RGB rendering; a non-opaque source alpha produces a warning.

The browser parser still consumes deprecated/removed fields where the native binary stream retains them. Repacking an old record into an imagined C++ struct would shift every subsequent field. The inspected native serializers remain the format authority. [Shader serialization][U13]; [configuration serialization][U14]

### 9.4 Recognized palette recipes

| ID | Recipe | Generated entry count | HTML handling |
|---:|---|---:|---|
| 1 | LongRandom64 | 640,000 | Regenerated from seed. |
| 2 | LongRandom64 2 | 64,000,000 | Rejected with an explicit unsupported-size message. |
| 3 | Long Rainbow 7 | 70,000 | Regenerated from seed. |
| 4 | Legacy KFR Banded | 16 | Regenerated from seed. |
| Other | Unknown | Unknown | Rejected; never replaced silently with a different palette. |

The browser reproduces the MT19937 stream, RGB draw order, and float32 rounding points. Recipe 1 begins with 64 random colors, recipe 3 with a fixed seven-color set, and both perform two 100× stages. Recipe 4 normalizes 16 random colors by their maximum component. The implementation is in the HTML's `MT19937`, `lerp`, and `recipe` functions. Native recipe definitions remain upstream. [Palette recipes][U17]

For C++, **reuse the native recipe generation rather than porting the JavaScript generator back into C++**. The browser recipe-reference log reports small floating-point differences for recipes 1 and 3; only raw-array serialization is claimed byte-exact in the fresh audit.

### 9.5 Optional trailers

The browser reader follows the known append-only order, including these markers:

| Symbolic block | Numeric marker | Purpose relevant to navigation |
|---|---|---|
| L2SH | `0x4C325348` | Later shading/fog/pattern fields. |
| VTLB | `0x424C5456` | RFC timeline block, skipped with bounded counts. |
| GLSS | `0x474C5353` | Surface-gloss fields. |
| PCLR | `0x50434C52` | Iteration-coloring mode. |
| PRJC | `0x50524A43` | RFC projection fields. |
| GLRL | `0x474C524C` | Gloss relief. |

These names identify integer markers; they are not instructions to search for printable byte strings. Earlier unmarked tails and these marked blocks are not a generic length-prefixed TLV stream. At an unknown marked boundary, the reader stops applying the remainder and reports it; it does not scan ahead to guess a later marker. Absence at an allowed field boundary is tolerated; a partial required scalar is rejected.

The timeline skip bounds tracks at 4,096, keys per track at 65,536, and holds at 65,536, with byte-availability checks for their payloads. These limits belong to the browser parser, not a proposed restriction on native projects.

### 9.6 Applied-field mapping

| Parsed field / concept | Lustre destination or action | Compatibility note |
|---|---|---|
| Raw colors or recipe metadata | `rffPalette` | Full representation retained. |
| Source interpolation 0 / 1 | `interpolation = 0 / 2` | Native RGB / OKLab to HTML IDs. |
| Color smoothing | `smoothing` | Keeps 0/1/2 semantics. |
| RGB intervals | `redCycle`, `greenCycle`, `blueCycle`; red also seeds `cycleLength` | Linked only when the three source values are exactly equal. |
| Offset | `offset = fract(sourceOffset)` | Negative/large phase is normalized into one cycle. |
| Iteration coloring | `coloring` | Missing/unsupported mode falls back with appropriate defaults/warnings. |
| Cycle bias and curve | `cycleBias`, `cycleCurve` | UI bounds apply. |
| Seamless | `seamless` | Imported forward-plus-reverse behavior. |
| Palette gloss enable | `rffGloss` | Ordinary `paletteGloss` set to zero. |
| Gloss / interior / band colors | HEX color controls | Quantized to 8-bit/channel and clamped for the UI; not lossless float color controls. |
| Animation speed | `rffSpeed` | Iterations/second, unlike ordinary `paletteSpeed`. |
| Animation mode 0/1/3/4 | HTML 0/1/2/3 | Nonlinear flows are approximations. |
| Flow amount | Source amount divided by red interval | Then bounded to the HTML range. |
| Flow scale/speed/swirl | Corresponding flow controls | Missing-tail defaults applied. |
| Frozen iteration values | `frozen` | Up to 16; values above `1e12` are capped with warning. |
| Freeze tolerance | `freezeTolerance` | Zero disables imported matching. |
| Band enabled/count/width/opacity/softness/color | Band controls | Continuous browser coverage rather than identical native resampling. |
| Color gamma/exposure/hue/saturation/brightness/contrast | `rff*` grade controls when requested | Resets ordinary grade only when checked. |
| Parsed slope/effects/HDR/timeline | No render-state replacement | Used for navigation and warnings only. |

The import plan sets `rffMapping = true`, `easing = 0`, `paletteReverse = false`, and resets palette/flow/RFF phases. A warning is emitted when supported numeric settings are clipped to the browser UI range. Long periods are retained within those bounds even when they make the current shallow view look almost uniform; “fit period to 36” is a separate explicit action.

### 9.7 Preservation and memory

A raw palette descriptor uses:

```json
{
  "encoding": "rff-rgb-f32-v1",
  "count": 257,
  "recipeId": -1,
  "seed": 0,
  "source": "example.rfc",
  "data": "<base64 of little-endian interleaved RGB float32 bytes>"
}
```

A recipe descriptor omits `data` and keeps recipe ID, count, and seed. Raw payload length is exactly `count * 12` bytes, or `count * 16` Base64 characters. The latter needs no padding because each RGB record is 12 bytes.

For 1,048,576 entries, the RGB source alone occupies 12 MiB, its RGBA32F GPU expansion 16 MiB, and its Base64 representation 16 MiB of characters before JavaScript string/object/copy overhead. Parsing, snapshots, undo, staging, and the original file add memory. A 64 MiB file-size cap is **not** a 64 MiB process-memory cap.

Finite negative and above-one source RGB components survive raw serialization. Rendering may clamp or transform them, especially in OKLab/HEX-related paths; do not interpret retention of the bytes as unrestricted HDR palette reproduction.

The undo stack has a 45-entry cap plus a heuristic 48-million-character budget for raw Base64 history. Recipe/raw caches retain only a small number of decoded palettes. These are browser safeguards, not a substitute for a native resource budget or immutable shared palette objects.

### 9.8 Input validation and transaction behavior

The reader rejects bad magic, unsupported core versions, oversized/zero raw palette counts, non-finite float values, invalid booleans, truncated required data, and unsupported recipes. Integer counts are read as 64-bit integers before conversion to JavaScript numbers. Known unsupported palette enum values in optional fields are warned about and defaulted; this is different from rejecting an unknown file version or recipe.

The import preview does not modify the current state. Cancel leaves it unchanged. Application prepares a validated next state, then commits one undoable action. A token prevents stale asynchronous file reads from overwriting a newer pending import.

Saved source paths/formula text are not evaluated; external texture paths are not opened; no network fetch is performed by the importer. Persisted import metadata excludes the original center and external image paths. The original source file is not overwritten.

### 9.9 What to implement natively instead of another parser

The current native API already provides `ConfigIO::loadShader(path, ShaderAttribute&)` and `ShaderPresetIO::load(path, ShaderAttribute&)`. The former obtains a shader from an RFC using a scratch configuration and does not replace the live view. [Native config API][U16]; [implementation][U14]; [preset API][U15]

The useful backport is therefore a **selective color import dialog**:

```text
native loader → scratch ShaderAttribute → palette preview and optional color-grade checkbox
              → commit scratch.palette and, optionally, scratch.color
              → preserve all other live attributes
```

Keep native float colors, native enum meanings, recipe metadata, and double frozen iterations. Do **not** apply the browser's HEX quantization, 32-stop limit, texture layout, `1e12` frozen-value cap, or reduced recipe list to native data. The code sketch in Section 12 shows this separation.

## 10. Additional effects and editing workflow

### 10.1 Decorations

The HTML offers a single generated pattern layer with stripes, checker, grid, dots, diamond, a honeycomb-like pattern, waves, and cloud noise. `patternFollow` either uses screen coordinates or replaces one coordinate with the palette cycle ratio. Pattern angle, scale, sharpness, color, and opacity are independent controls.

Warp is a small fractal-noise displacement of the **palette phase**, using one to six noise octaves. It does not warp the iteration field or the surface normals. Stripe shading has single sinusoidal, double-period, and squared variants. Iteration fog blends toward a color across a `log2(iteration+1)` interval.

These are useful optional looks, but they are not a complete implementation of RFF's texture/pattern stacks, domain-warp options, or focus/rim fog. The native shader bundle explicitly has separate arrays and attributes for these systems; keep them. [Native shader bundle][U25]

### 10.2 Editor and persistence

The latest HTML includes movable stops, numeric editing, separate material/palette/scene presets, a light-direction disc, camera drag/zoom, optional palette and light animation, freeze eyedropper, JSON import/export, PNG export, undo/redo, split comparison, and five diagnostic views. Controls have English technical names alongside the original Japanese labels.

Full settings JSON uses `format: "lustre-atelier"`, `version: 2`, and a `state` object. The RFC addition does **not** increase that top-level JSON version. Palette JSON can include raw descriptor plus mapping and frozen settings. The latest code also contains a first-version settings migration; it is an approximate look migration, not native file conversion.

The current default has `animateLight = true`, but `playing = false`; startup is paused. `H` hides the UI; Shift-drag changes light direction/zenith in this version. Avoid copying older version-one interaction descriptions into new native documentation.

### 10.3 Diagnostics and native engineering use

| Diagnostic | HTML selection | Native recommendation |
|---|---:|---|
| Finished image | 0 | Preserve the standard output path. |
| Palette/base | 1 | Add a genuinely unlit/undecorated option if needed; HTML base can include decorations. |
| Normal | 2 | Offer ungraded raw normal RGB for debugging, not only the finished display path. |
| AO | 3 | Add a numeric/raw view to avoid gamma/grade ambiguity. |
| Reflection only | 4 | Allow linear-float capture before display processing. |

For exact regressions, add intermediate buffer capture and a dither-off mode to the native port. A PNG of the finished diagnostic cannot establish exact equality of normal vectors, irradiance, or palette float values.


## 11. Native integration map

### 11.1 Existing files to extend or reuse

All file names below refer to the pinned upstream revision. Suggested new types/functions are explicitly marked **proposed**; they are not claimed to exist in the repository.

| Existing file / area | Relevant responsibility | Proposed work |
|---|---|---|
| `src/rff2/attr/ShdSlopeAttribute.h` [U02] | Existing artistic relief/lighting settings | Preserve all existing defaults; reference or extend with a separate relief/material model. |
| `src/rff2/attr/ShaderAttribute.h` [U25] | Shader settings bundle | Add a **proposed** material sub-attribute without deleting slope, HDR, or effect attributes. |
| `src/rff2/attr/ShdPaletteAttribute.h` [U04] | Colors, mapping, recipe and freeze metadata; CPU interpolation | Keep raw colors/recipes; add optional stop authoring metadata and shared transfer helpers. |
| `src/rff2/attr/ShdPalColorInterpolationMethod.h` [U06] | Native interpolation enum | Add Linear RGB with stable old IDs and explicit GPU translation. |
| `src/rff2/attr/ShdSlopeShadingBlend.h` [U07] | Native shading-composite enum | Do not reuse its reserved value 1 for HTML linear multiply. |
| `src/rff2/vulkan/SharedDescriptorTemplate.hpp` [U10] | Descriptor layouts and host storage reservations | Append compatible fields or add an explicitly allocated material descriptor. Keep host/shader offsets synchronized. |
| `src/rff2/vulkan/GPCSlope.cpp` [U09] | Uploads slope attributes to uniforms | Bind new material controls or delegate to a new material pipeline. Preserve the native specular-link inversion. |
| `shdsrc/vk_slope.frag` [U03] | Native relief and artistic lighting | Keep an unchanged legacy branch; add the Studio path or factor a shared relief/material include. |
| `src/rff2/vulkan/GPCIterationPalette.cpp` [U08] | Native palette preparation/upload | Optional stop-LUT generation, Linear RGB mode, shared CPU/GPU validation. Keep large native palettes. |
| `src/rff2/vulkan/ShaderModeSpecialization.hpp` [U11] | Packed palette modes and pipeline variants | Update packed/unpacked mode agreement and specialization identity. |
| `shdsrc/vk_iteration_palette.frag` [U05] | Preview palette mapping | Add safe new interpolation handling without changing legacy branches. |
| `shdsrc/vk_2_map_iter_stripe.comp` [U12] | Compute/video mapping counterpart | Apply the same interpolation semantics, packing, and precision rules. |
| `src/rff2/vulkan/GPCColor.cpp`, `shdsrc/vk_color.frag` [U20], [U19] | Native color-correction pass | Preserve native parameter interpretation; define placement of any new OKLab grade. |
| `src/rff2/vulkan/GPCBloom.cpp` [U21] | Bloom setup and HDR/linear-add controls | Integrate new scene-linear signals without decoding them as encoded RGB. |
| `src/rff2/vulkan/GPCLinearInterpolation.cpp`, `shdsrc/vk_linear_interpolation.frag` [U24], [U22] | Final transform, tone mapping, transfer | Add distinct peak-based modes and a signal-contract-aware input branch. Preserve SDR/PQ/HLG behavior. |
| `src/rff2/attr/ShdHdrAttribute.h` [U23] | Existing HDR exposure/headroom/method | Extend, do not reinterpret existing options as Lustre's indices. |
| `src/rff2/io/ConfigIO.cpp/.h` [U14], [U16] | RFC load/save and shader-only load | Reuse for selective color import; extend serialization only for genuinely new native fields. |
| `src/rff2/io/ShaderPresetIO.cpp/.h` [U13], [U15] | RFSP load/save and shared shader sections | Version and validate material/stop metadata; keep old defaults. |
| `src/rff2/preset/shader/palette/ShdPalettePresets.cpp` [U17] | Native recipe generation | Reuse rather than translating the browser's limited generator back to C++. |
| `src/rff2/ui/CallbackShader.cpp` [U18] | Shader setting callbacks, previews, preset actions | Add material panel and selective-color preview/apply workflow; preserve appropriate panel lifetimes. |
| `CMakeLists.txt` [U26] | Build integration | Register any new C++/shader files and ensure runtime SPIR-V assets are rebuilt. |

The complete native attachment/render-context routing was not exhaustively audited. Before implementation, trace preview, still export, keyframe/video, and HDR image allocation from these entry points. This document does not claim that editing `GPCSlope.cpp` alone updates every route.

### 11.2 Important host/shader translations

| HTML field | Native field or concern |
|---|---|
| `slopeOpacity` | Existing `slope.opacity`; same concept, whole-pipeline result may differ. |
| `slopeBrightness`, `slopeGamma` | Existing slope brightness/gamma; not ordinary/output color grade. |
| `anisotropy`, `anisotropyAngle` | Native `specularAnisotropy`, `specularAnisotropyAngle`; GGX width vs legacy exponent semantics differ. |
| `specularIndependent` | Native CPU boolean has the same intent, but native GPU `specular_link` is **0 when independent, 1 when linked**. |
| `renderModel = 0` | Studio in HTML; **must not** become the default legacy native enum value implicitly. |
| `interpolation = 1 / 2` | Linear RGB / OKLab in HTML; translate, do not cast to native enum. |
| `shadingBlend = 1` | HTML linear multiply; native value 1 is reserved. |
| `paletteAnimation = 2 / 3` | HTML breathing/turbulence; native IDs are 3/4. |
| `exposure`, `hue`, `brightness`, `saturation` | Ordinary Lustre grade; not interchangeable with the native `color` fields. |
| `rffExposure`, `rffHue`, etc. | Browser compatibility grade. Native imports should use the native color attribute directly. |
| `depth`, `macroRadius`, `aoRadius` | Different scale/support conventions; retain model identity in saved settings. |
| `specularPower` | Exponent in artistic mode; roughness-width influence in GGX. |
| `lightBlend` | Similar intent, different placement/clipping; no promise of identical output. |

The native host binding and shading enum establish the two particularly easy-to-miss link/value hazards. [Host slope binding][U09]; [native shading values][U07]

### 11.3 GPU layout, variants, and precision

`DescSlope` currently ends at target 52 (`GLOSS_RELIEF`). Its targets are host reservation identifiers, not a license to insert members in the middle of the shader block. `DescPalette` has a runtime color array that must remain last. A separate extension buffer can avoid shifting that array. [Descriptor definitions][U10]

**Proposed rules:** keep existing byte offsets stable; reserve/bind new fields explicitly; use fixed-width GPU flags; check alignment with shader reflection; and update descriptor stage visibility when a consumer moves to compute. Do not upload a C++ settings struct containing `bool`, enums, and `glm::vec3` by blind `memcpy`.

Keep double iteration/frozen-value data through phase reduction and relative-height calculation. Convert reduced coordinates or final gradients to float only when appropriate. Replacing high-precision phase arithmetic with a large float iteration value can erase the palette variation before lighting even begins.

Palette-mode changes must update both specialization keys and uniform fallback values. Test specialized and unspecialized pipelines against each other. A mode appearing correct in preview but wrong in video is a likely sign of an omitted consumer or stale pipeline cache key.

### 11.4 Linear material transport vs HDR display output

These are separate decisions:

```text
Internal transport:  does this material require unclipped linear radiance buffers?
Output transfer:     is the final target SDR, PQ, or HLG?
```

Studio GGX benefits from linear floating-point transport even for an SDR PNG. Do not make it work only when the user enables HDR video output. Conversely, do not feed a tone-mapped SDR image into the native PQ/HLG encoder and call it HDR.

For the new path, define one owner for each conversion:

```text
native encoded palette/effects result
   → explicit decode to linear base color
   → Studio material / lighting in linear RGB
   → compatible linear fog and bloom
   → grade in its defined domain
   → one output transform
   → SDR sRGB OR native HDR transfer
```

Keep the old route unchanged for old presets. Audit both any slope-local shoulder and final tone map: applying both by accident compresses the highlight twice. Likewise, bypass the native final pass's headroom reconstruction when its input is already true scene-linear. The existing output implementation makes this boundary explicit. [Final output][U22]

### 11.5 Full-frame coordinates and tiled exports

Use full-image UVs for the view vector, procedural environment orientation, patterns, flow, vignette, and dither. Tile-local UVs would move the studio lights or create seams. Use full output dimensions for reference-radius scaling and pixel footprint.

Provide sample halos around tiles for the largest active Sobel/AO radius and for bloom; the required halo depends on the selected filtering implementation. At tile boundaries, do not treat the tile's edge as the image's outer edge. The native iteration descriptor's canvas extent/offset fields are relevant to carrying this information. [Iteration descriptor][U10]

## 12. C++ implementation sketches

These are **proposed integration examples**, not patches already applied or compiled against the native project. The palette import example uses real inspected native APIs. UI history, threading, and render request ownership remain the caller's responsibility.

### 12.1 Preserve legacy mode by construction

```cpp
// Proposed new settings types. These names are not present in the baseline.
#include <cstdint>

enum class SurfaceModel : std::uint32_t {
    LegacyRff = 0,       // Missing extension block always selects this.
    StudioGgx = 1,
    LustreArtistic = 2   // Optional; not a substitute for LegacyRff.
};

enum class ReliefModel : std::uint32_t {
    LegacyRff = 0,
    LustreRelativeLog = 1
};

struct StudioMaterialSettings {
    SurfaceModel model = SurfaceModel::LegacyRff;
    ReliefModel reliefModel = ReliefModel::LegacyRff;

    float roughness = 0.28f;
    float metalness = 0.72f;
    float ior = 1.50f;
    float clearcoat = 0.30f;
    float clearcoatRoughness = 0.14f;
    float directIntensity = 0.80f;
    float environmentIntensity = 0.74f;
    float softboxWidth = 0.55f;
    float iridescence = 0.07f;
    float filmThicknessNm = 410.0f;
};
```

These are CPU settings, **not a file or GPU binary layout**. Existing slope settings supply lights, colors, specular amount/power, anisotropy, and related controls. Use explicit validation and serialization. When importing a Lustre state, translate `renderModel=0` to `SurfaceModel::StudioGgx`; do not cast numeric values directly.

For an exact HTML-looking preset, select the Lustre relief model and apply the HTML slope defaults as well. The structure above intentionally leaves legacy relief selected for a safer first material milestone.

### 12.2 Selective native color import using existing loaders

The following can be placed in a native UI/service implementation with include paths relative to `src/rff2/ui`. It reads into a scratch shader and never assigns the live scene, slope, fog, or material while opening the file.

```cpp
#include "../io/ConfigIO.h"
#include "../io/ShaderPresetIO.h"
#include <filesystem>
#include <optional>
#include <utility>

namespace merutilm::rff2 {

enum class ColorImportFileKind { Rfc, Rfsp };

struct ColorImportPayload {
    ShdPaletteAttribute palette;
    ShdColorAttribute color;
    bool includeColorCorrection = false;
};

[[nodiscard]] std::optional<ColorImportPayload> readColorImport(
    const std::filesystem::path& path,
    ColorImportFileKind kind,
    bool includeColorCorrection)
{
    ShaderAttribute scratch{};
    const bool ok = kind == ColorImportFileKind::Rfc
        ? ConfigIO::loadShader(path, scratch)
        : ShaderPresetIO::load(path, scratch);
    if (!ok) {
        return std::nullopt;
    }

    return ColorImportPayload{
        std::move(scratch.palette),
        scratch.color,
        includeColorCorrection
    };
}

// Call only after the user accepts the preview and the caller has recorded undo.
// The caller must synchronize with rendering and refresh affected settings panels.
void commitColorImport(ShaderAttribute& destination, ColorImportPayload&& payload)
{
    destination.palette = std::move(payload.palette);
    if (payload.includeColorCorrection) {
        destination.color = payload.color;
    }
}

} // namespace merutilm::rff2
```

The caller should handle allocation/IO exceptions, show a bounded preview, preserve a pre-commit undo snapshot, close or refresh UI panels that retain references to replaced palette data, and issue the existing shader redraw request after the commit. Do not emit missing-image warnings for discarded texture settings or load their images merely to preview palette colors.

Using native attributes directly preserves recipe metadata, alpha where supported natively, intervals, band controls, and double frozen iterations. It deliberately avoids the lossy mappings required by browser HEX controls. The native loaders also validate their full supported input, so a damaged unrelated part of a file may still cause the load to fail; this example is not a recovery parser.

### 12.3 Safe tangent frame for the Studio port

This is proposed GLSL intended for the Vulkan shader. It fixes the zero-gradient tangent problem and can be called separately for the base and coat normals.

```glsl
void makeTangentFrame(
    vec3 normal,
    vec2 gradient,
    float angleRadians,
    out vec3 tangent,
    out vec3 bitangent)
{
    vec3 n = normalize(normal);
    vec2 g = dot(gradient, gradient) > 1e-16
        ? normalize(gradient) : vec2(1.0, 0.0);

    float a = atan(g.y, g.x) + angleRadians;
    vec3 seed = vec3(cos(a), sin(a), 0.0);
    vec3 projected = seed - n * dot(n, seed);

    if (dot(projected, projected) < 1e-12) {
        vec3 helper = abs(n.z) < 0.999
            ? vec3(0.0, 0.0, 1.0) : vec3(0.0, 1.0, 0.0);
        projected = cross(helper, n);
    }

    tangent = normalize(projected);
    bitangent = normalize(cross(n, tangent));
}
```

Use `makeTangentFrame(Ns, ...)` for the adjusted base reflection normal and `makeTangentFrame(N, ...)` for a coat computed on the actual surface. Also evaluate the coat's Fresnel angle from its own normal. This changes the current HTML's behavior when `reliefResponse < 1`, so classify it as a correction rather than an exact port.

### 12.4 Versioned palette authoring metadata

```cpp
// Proposed authoring model; not a replacement for native palette.colors.
enum class StopEasing : std::uint32_t {
    Linear = 0, Smoothstep = 1, Smootherstep = 2
};

struct PaletteStop {
    double position;  // Normalized cycle position; validated and sorted.
    float red;        // Explicitly encoded RGB, not implicitly linear.
    float green;
    float blue;
};
```

Store stop metadata separately from baked/native color entries and recipe metadata. Choose one authoritative representation for each palette. Editing a generated/baked array should clear stale recipe IDs and either regenerate or detach stop metadata. Never retain a recipe tag after changing its colors manually.

Preserve a full raw/recipe palette until the user explicitly requests resampling. A native editor can support more than 32 stops; 32 is an HTML UI limit, not a requirement of the interpolation algorithm.

## 13. Compatibility and serialization strategy

### 13.1 Non-negotiable compatibility rules

Old RFC/RFSP files must keep their native rendering mode and defaults when no new extension is present. An old native file must not suddenly become metallic because the HTML defaults use `metalness=0.72`. Keep the original slope calculation, tone-map interpretation, palette IDs, and output ordering available.

Use explicit converters for Lustre JSON rather than treating it as a native shader attribute dump. State should carry both a **material model** and a **relief model** where their arithmetic differs. A value named `Depth` without its model is insufficient to reconstruct the look.

The native save format already has versioned cores and optional appended blocks. It must not be extended by inserting floats into the historical shader prefix. Use existing validation and compatibility tests as the base. [IO layout][U13], [U14], [U15]

### 13.2 Proposed extension format

For new native material/stop data, a future extension can use a marker, extension version, and byte length followed by typed fields. This is a **proposal**, not the current RFC/RFSP layout:

```text
uint32 extensionMagic       // Choose a unique marker after auditing the format.
uint32 extensionVersion
uint64 payloadByteLength
byte[payloadByteLength] payload
```

Bounds-check the length before allocation/read; reject non-finite values and unknown mandatory modes. A length allows a new reader to skip an unfamiliar extension without scanning for magic. Define absent-block defaults explicitly.

Decide and test whether older applications reject the newer file version or safely ignore the extension. Do not promise backward reader compatibility merely because the block is appended. Current readers enforce their own version limits and trailer order.

Keep browser-only state such as modal visibility, panel tabs, GPU quality scale, and import-source display names out of native shader serialization unless there is a deliberate UI-session format.

### 13.3 Save policy for new and legacy looks

| Situation | Recommended behavior |
|---|---|
| Old native file opened without edits | Preserve legacy appearance; do not auto-enable Studio or new tone maps. |
| User selects Studio | Persist explicit model/version and all new material controls. |
| User returns to legacy mode | Keep material values as dormant settings, but use the unchanged legacy renderer. |
| Recipe palette edited manually | Clear recipe identity or store a deliberate detached representation. |
| Non-uniform stops edited | Rebuild the chosen palette representation deterministically and retain stop metadata. |
| New interpolation unsupported by target version | Warn or export a deliberately baked fallback; never reinterpret enum bits. |
| Color-only import canceled | No scene, shader, phase, history, or panel-state mutation. |
| Color-only import accepted | Replace only selected color attributes and record one undo transaction. |

### 13.4 Attribution

The HTML importer carries `SPDX-License-Identifier: GPL-3.0-only` and attributes its binary layout/recipes to RFF_Super. The inspected repository includes `LICENSE` and `NOTICE`. Preserve those notices and the attribution of any adapted source when moving code between the projects; this document does not supply a different license grant. [Repository license][U27]; [notices][U28]

## 14. Defects and limitations discovered in the audit

These items are **not already fixed** in `lustre_II_rfc.html`. They should not be marketed as completed improvements. Reproducing the HTML exactly and correcting it for production are separate objectives.

| ID | Evidence / current behavior | Impact | Proposed treatment |
|---|---|---|---|
| A01 | `reliefResponse` passes its tooltip text as `N()`'s `step` argument. `sliderValue()` divides by that string. | Dragging the slider computes `NaN`; the numeric-entry path is different. | Supply numeric step `0.01`, move text to `tip`, and validate all range metadata. |
| A02 | `tangents()` calls `atan(grad.y,grad.x)` without a zero-gradient fallback. Similar aspect calculations exist in artistic/gloss paths. | Flat regions have undefined orientation; downstream normalization can be unstable. | Use a deterministic tangent/aspect fallback before normalization. |
| A03 | Direct coat uses actual `N` but a basis built for `Ns`; coat Fresnel uses the `Ns` view dot. | Inconsistent coat at relief response below one. | Build a coat frame/Fresnel on its own normal. |
| A04 | Environment anisotropic blur uses anisotropy magnitude but not its angle. | Direct highlight and environment streak orientation are not fully consistent. | Rotate/filter in the same tangent frame, or label environment blur as approximate. |
| A05 | Diffuse remains `35%` of shaded base at metalness 1; film and artist multipliers can amplify reflection. | Not an energy-conserving metallic BRDF. | Keep an artistic mode; add a separately validated physical mode rather than silently changing all presets. |
| A06 | `rawFilled` is clamped before terminator wrapping even with zero fill. | Negative light-facing values collapse before the wrap, limiting its intended range. | Evaluate a corrected signed wrap in a versioned mode and compare dark-side behavior. |
| A07 | Some derivative masks differentiate already wrapped cycle/aspect values. | Discontinuities at wrap seams can over-broaden or suppress detail. | Estimate the footprint from unwrapped phase or a periodic representation. |
| A08 | Imported band lines lack the ordinary path's derivative filtering. | High line counts can alias despite retaining every source color. | Add analytic/minification filtering and mirror it in preview/compute output. |
| A09 | Six Sobel neighborhoods and GGX/coat computations appear in the source even for inactive modes. | Avoidable work; actual elimination depends on compilation and uniform handling. | Add coherent mode-specialized paths or reuse a normal/AO buffer after correctness is established. |
| A10 | Nearest field sampling, radius rounding, reference-height scaling, and screen derivatives depend on resolution. | The look is not rigorously invariant under export size or aspect ratio. | Define full-frame footprint semantics and test several sizes/tiles. |
| A11 | The comparison pass actually sets `viewMode=1` and renders decorated base, not merely specular-off shading. | The UI comparison should not be read as an isolated reflection comparison. | Label it Base vs Shaded, or implement a true reflection-off comparison with identical diffuse/fog/bloom. |
| A12 | Diagnostic views still pass through tone mapping, grade, gamma, and dither. | Displayed normal/AO values cannot be used as raw buffers. | Add ungraded intermediate capture/debug paths. |
| A13 | `grain=0` leaves dither enabled. | Exact pixel regressions can fail despite identical material values. | Separate dither enable from grain and disable it for reference captures. |
| A14 | Raw source RGB is preserved, but imported auxiliary colors become HEX and some numeric values are capped. | Color-only import is not fully lossless at the entire-attribute level. | Native selective import should keep native types and omit browser caps. |
| A15 | Imported animation enable is derived only from base animation speed. | A nonzero nonlinear flow with zero linear speed is not automatically enabled. | Decide animation enable from the selected mode's effective motion, or expose it explicitly. |
| A16 | Whole-file parsing and recipe regeneration run on the main JS thread; snapshots can duplicate large payloads. | Large imports can pause the UI and exceed the file-size memory bound. | Use a native worker and immutable/shared data with explicit memory budgets. |
| A17 | Boundary attenuation requires a distance field; the browser field is solver/projection-specific. | An arbitrary native substitute can erase highlights or create false edges. | Disable until the native distance/pixel metric is verified. |
| A18 | HTML float phase/frozen uniforms have less precision than native double data. | Deep iteration counts and long periods can lose color variation. | Preserve native precision and perform stable phase reduction. |

### 14.1 Reproduced metadata defect

The schema factory signature is:

```javascript
N(key, label, eng, value, min, max, step = 0.01, tip = '', unit = '', log = false)
```

For `reliefResponse`, the current seventh argument is text rather than a number. The audit evaluated the same slider arithmetic at midpoint and obtained `NaN`. This is a deterministic JavaScript-level reproduction; no claim is made that a new browser/GPU failure test was run during this documentation task.

The corrected declaration should pass the numerical step before its explanation, for example:

```javascript
N('reliefResponse', 'Relief response', 'Relief Response',
  1, 0, 1, 0.01,
  '1 follows the actual surface normal; 0 uses the half-light-angle anchor.');
```

Add a metadata invariant test: every numeric control must have finite bounds/default/step, positive step, and a default within its range. Add a runtime finite-value guard before uniform upload as a second line of defense.

### 14.2 Upstream-specific integration risks

Separate from the HTML defects, the native audit found the interpolation bit collision described in Section 7.5 and the CPU/GPU transfer mismatch described in Section 7.6. The first becomes a bug if the new enum is added naively; it is not a defect for the current two interpolation modes. The second already describes differing arithmetic in the inspected helpers, but its visual effect has not been measured against native captures in this task.

## 15. Port order and acceptance tests

### 15.1 Recommended milestones

| Milestone | Deliverable | Acceptance gate |
|---|---|---|
| 0. Preserve baseline | Native legacy reference captures and file corpus | No image or serialization change before features are enabled. |
| 1. Selective color import | Native scratch-load, preview/cancel, palette-only or palette+grade commit | Live view/material untouched; cancel/invalid file unchanged; one undo. |
| 2. Palette consistency | Explicit enum translation, safe third interpolation mode, shared transfer tests | All mode combinations agree in CPU, preview, compute/video, specialized and fallback paths. |
| 3. Studio BRDF on native normals | GGX + direct lights + procedural studio, legacy route intact | Finite output over parameter extremes; no unintended old-preset changes. |
| 4. Linear signal contract | Correct floating transport, bloom, grade, final transform | No double decode/tone map; SDR and native HDR exports remain valid. |
| 5. Coating and filtering | Clearcoat with corrected basis, optional film tint, specular AA | Layer toggles behave as documented; stable animation and export-size comparisons. |
| 6. Optional Lustre relief/editor | Separate relief model, waves, AO radius, authoring stops | No implicit Depth conversion; save/load and tiled-render continuity. |
| 7. Production verification | Real RFF files, stress/fuzz tests, preview/export/timeline coverage | Documented supported combinations and known remaining differences. |

Prioritize correct signal flow over shader optimization. A faster implementation of the wrong color-space contract will still flatten the highlights.

### 15.2 Rendering test matrix

**Legacy regression:** render native presets at several locations, including low-count exterior, high-count filigree, flat regions, and interior boundaries. Compare old/new outputs with the new model disabled. Check all supported native interpolation, stripe, animation, slope, HDR, and export modes. Use a fixed seed and a dither-disabled capture where available.

**Numeric robustness:** sweep roughness, anisotropy, IOR, specular power, coat, film amount, intensity, and light angles at minimum/default/maximum. Include zero gradient, zero depth, zero opacity, near-grazing directions, and high HDR values. Reject NaN/Infinity in every intermediate buffer. Guard degenerate half vectors and tangent frames.

**Layer isolation:** test direct-only, environment-only, coat-only, rim-only, gloss-only, and base-only. Check that `specularIntensity=0` has the documented semantics, and that the master material bypass removes all intended contributions. Test clearcoat with relief response at 0, 0.5, and 1.

**Color correctness:** test dark sRGB ramps around `0.04045`, linear ramps around `0.0031308`, saturated colors, neutral gray, and colors outside the display gamut. Compare CPU stop samples against GPU samples. Test negative source RGB preservation separately from render clipping. Confirm peak tone maps act on the peak rather than accidentally per component.

**Spatial/temporal behavior:** compare 800-, 1600-, and 3200-pixel heights, multiple aspect ratios, device-scale settings, tiled stills, and full-frame stills. Rotate lights slowly through flat/detail regions. Test wrapped palette seams and high band counts. Compare frame time/phase deterministically between preview and video.

**Palette invariance:** recolor without changing the iteration field and compare normal/AO buffers; they should remain unchanged unless an explicitly height-affecting setting changed. Conversely, verify relief waves alter normals but ordinary palette animation does not.

### 15.3 Import and persistence test matrix

Test actual native exports of every supported version, not only synthetic writers. Include raw palettes of 1, 2, 257, 4097, 640000, and 1048576 entries for browser interop, and native-supported larger palettes/recipes for native operation. Include seeds 0 and `UINT32_MAX`, all interpolation/animation/coloring modes, alpha in legacy files, and 0/16 frozen values.

Test truncation at every field boundary, invalid booleans, NaN/Infinity, bad counts, unknown markers/versions/recipes, unknown optional modes, and large strings/timelines. Verify unsupported data never changes the live state. Test that missing external images do not prevent a supported color-only operation solely because image loading was unnecessarily invoked.

Roundtrip raw RGB bytes exactly through the raw descriptor format. For regenerated recipes, compare to native output with a declared float tolerance and log the exact seed/algorithm/compiler. Test full settings, palette-only JSON, explicit lossy MAP export, 32-stop conversion, undo/redo, and repeated imports under memory pressure.

For new native fields, test absent-block defaults, old-reader behavior, native save/load, timeline parameter interpolation, and both runtime shader variants. Validate that changing palette interpolation does not flip the cycle curve.

## 16. Verification performed and not performed

### 16.1 Fresh checks in this documentation task

The latest HTML was inspected directly, and **139 control definitions** were extracted from its schema. The embedded importer was exercised in Node with **17 checks, all passing**:

| Check group | Fresh result |
|---|---|
| Retained synthetic RFC parsing | v5 parsed, 257 raw colors, no unparsed trailing bytes. |
| Raw RGB descriptor serialization | Byte-exact roundtrip; one-color palettes; finite negative and above-one channels retained. |
| Rejection behavior | Bad magic/version, empty/truncated input, non-finite descriptor data, excessive count, recipe 2, and unknown recipe rejected. |
| Random stream reference | MT19937 seed 5489 first output equals 3499211612. |
| Supported recipes | IDs 1, 3, 4 checked for expected size, finite values, and deterministic repeated generation. |

The test runner and results were recorded during the audit as `extract_audit.js` and `audit_checks.json`. These are work records, not native tests. Separately, the invalid `reliefResponse` slider step was reproduced with the schema and slider arithmetic; that finding is not included among the 17 importer checks.

### 16.2 Historical bundled test report

The previously delivered RFC bundle contains a report of 34 synthetic binary checks, 29 browser checks, and 13 additional large-palette browser checks. It records Chromium `144.0.7559.96` and no browser errors in those runs. Those are **retained historical results**, not browser tests rerun for this document.

Its independent C++ recipe-reference comparisons cover IDs 1, 3, 4 at three seeds. Recorded maximum absolute differences are approximately `5.96e-8` for IDs 1 and 3, and zero for ID 4. This does not establish a match against the native RFF application or every platform's math library.

### 16.3 Explicitly not verified here

No native RFF build or C++ patch was compiled, no real user-supplied RFC file was available, no new browser/GPU visual comparison was run, and no native image-difference, HDR monitor, or performance benchmark was performed in this task. The retained synthetic fixture is not an application-exported file.

The equations and limitations are source-level findings. The C++/GLSL sketches and production acceptance tests are proposals. A successfully parsed palette and passing Node tests do not prove correct native rendering, stable animation, or unchanged legacy output.



## 17. Complete HTML control reference

This inventory is generated from the **actual 139-entry schema** in the latest HTML. Defaults are the initial HTML state, not recommended native defaults. They must not overwrite old RFF presets. `Step` is the nominal editor step; numerical entry can have behavior different from the slider. Bounds here are browser/UI bounds, not automatically native validation limits.

The invalid `reliefResponse` step is reported rather than silently replaced in the inventory. All control labels below are English. A dash means not applicable. Color values are encoded sRGB HEX. Angles are in degrees unless the implementation formula explicitly converts them to radians.

### 17.1 Color Mapping

| Key | English label | Initial value | Bounds or enum values | Step / unit |
|---|---|---|---|---|
| `interpolation` | Color Interpolation | `2` | `0` encoded sRGB; `1` Linear RGB; `2` OKLab | — |
| `easing` | Stop Easing | `1` | `0` Linear; `1` Smoothstep; `2` Smootherstep | — |
| `seamless` | Seamless / cyclic | `true` | `false` / `true` | — |
| `cycleLength` | Cycle Length | `36` | `1e-06` to `1e+12`; logarithmic slider | `1e-06` |
| `offset` | Offset Ratio | `0.12` | `0` to `1` | `0.001` |
| `smoothing` | Color Smoothing | `1` | `0` None / integer; `1` Normal / continuous; `2` Reversed fractional iteration | — |
| `coloring` | Iteration Coloring | `0` | `0` Linear; `1` Square root; `2` Cube root; `3` Logarithmic; `4` Log-log; `5` Smoothstep; `6` Smootherstep | — |

### 17.2 Cycle / Rgb

| Key | English label | Initial value | Bounds or enum values | Step / unit |
|---|---|---|---|---|
| `rgbLinked` | Linked RGB intervals | `true` | `false` / `true` | — |
| `redCycle` | Red interval | `36` | `1e-06` to `1e+12`; logarithmic slider | `1e-06` |
| `greenCycle` | Green interval | `36` | `1e-06` to `1e+12`; logarithmic slider | `1e-06` |
| `blueCycle` | Blue interval | `36` | `1e-06` to `1e+12`; logarithmic slider | `1e-06` |
| `cycleCurve` | Cycle Curve | `1` | `0` Power; `1` Wave | — |
| `cycleBias` | Cycle Bias | `1` | `0.01` to `16`; logarithmic slider | `0.01` |
| `paletteReverse` | Reverse palette lookup | `false` | `false` / `true` | — |
| `interiorColor` | Mandelbrot Color | `#06100f` | `#000000` to `#ffffff` | 8-bit per channel |

### 17.3 Band / Gloss

| Key | English label | Initial value | Bounds or enum values | Step / unit |
|---|---|---|---|---|
| `bandAmount` | Band Line Opacity | `0` | `0` to `1` | `0.01` |
| `bandCount` | Band Line Count | `8` | `1` to `1048576` | `1` |
| `bandWidth` | Band Line Width | `0.025` | `0` to `1` | `0.001` |
| `bandSoftness` | Band Line Softness | `0.7` | `0` to `1` | `0.01` |
| `bandColor` | Band Line Color | `#d6c394` | `#000000` to `#ffffff` | 8-bit per channel |
| `paletteGloss` | Palette Gloss | `0` | `0` to `2` | `0.01` |
| `paletteGlossPower` | Palette Gloss Sharpness | `60` | `1` to `256` | `1` |
| `paletteGlossColor` | Palette Gloss Color | `#fff4d3` | `#000000` to `#ffffff` | 8-bit per channel |

### 17.4 Palette Animation

| Key | English label | Initial value | Bounds or enum values | Step / unit |
|---|---|---|---|---|
| `animatePalette` | Animate palette | `false` | `false` / `true` | — |
| `paletteSpeed` | Animation Speed | `0.025` | `-0.3` to `0.3` | `0.001` |
| `paletteAnimation` | Animation Mode | `0` | `0` Linear; `1` Psychedelic; `2` Breathing; `3` Turbulence | — |
| `flowAmount` | Flow Amount | `0.12` | `0` to `1e+09` | `0.01` |
| `flowScale` | Flow Scale | `3` | `0.001` to `100000` | `0.1` |
| `flowSpeed` | Flow Speed | `0.45` | `-100000` to `100000` | `0.01` |
| `flowSwirl` | Flow Swirl | `0.4` | `0` to `100000` | `0.01` |

### 17.5 Freeze Colors

| Key | English label | Initial value | Bounds or enum values | Step / unit |
|---|---|---|---|---|
| `freezeTolerance` | Static Color Tolerance | `0.018` | `0` to `1` | `0.001` |

### 17.6 Relief

| Key | English label | Initial value | Bounds or enum values | Step / unit |
|---|---|---|---|---|
| `depth` | Depth | `0.82` | `0` to `5` | `0.01` |
| `slopeOpacity` | Opacity | `1` | `0` to `1` | `0.01` |
| `normalSmooth` | Normal Smoothing | `0.75` | `0` to `1` | `0.01` |
| `macroRelief` | Macro Relief | `0.2` | `0` to `1` | `0.01` |
| `macroRadius` | Macro Radius | `10` | `1` to `60` | `1` / px |

### 17.7 Relief Detail

| Key | English label | Initial value | Bounds or enum values | Step / unit |
|---|---|---|---|---|
| `reliefWaves` | Relief Waves — extension | `0.5` | `0` to `1` | `0.01` |
| `waveFrequency` | Relief Wave Frequency | `0.53` | `0.03` to `1.5` | `0.01` |
| `invertRelief` | Invert relief | `false` | `false` / `true` | — |
| `aoIntensity` | AO Intensity | `0.26` | `0` to `1` | `0.01` |
| `aoRadius` | AO Radius — extension | `8` | `1` to `50` | `1` / px |

### 17.8 Key Light

| Key | English label | Initial value | Bounds or enum values | Step / unit |
|---|---|---|---|---|
| `azimuth` | Azimuth | `115` | `-180` to `180` | `1` / deg |
| `zenith` | Zenith | `48` | `0` to `89` | `1` / deg |
| `reflectionRatio` | Reflection Ratio | `0.22` | `0` to `1` | `0.01` |
| `terminatorSoftness` | Terminator Softness | `0.55` | `0` to `1` | `0.01` |
| `slopeBrightness` | Brightness | `1.15` | `0.1` to `3` | `0.01` |
| `slopeGamma` | Gamma | `1.08` | `0.2` to `3` | `0.01` |

### 17.9 Fill / Chromatic

| Key | English label | Initial value | Bounds or enum values | Step / unit |
|---|---|---|---|---|
| `fillIntensity` | Fill Intensity | `0.25` | `0` to `1` | `0.01` |
| `fillAzimuth` | Fill Azimuth | `-28` | `-180` to `180` | `1` / deg |
| `fillZenith` | Fill Zenith | `62` | `0` to `89` | `1` / deg |
| `lumaAmount` | Luma Amount | `0.9` | `0` to `1` | `0.01` |
| `shadingBlend` | Shading Blend | `2` | `0` Overlay in sRGB; `1` Linear multiply; `2` OKLab lightness | — |
| `ambientIntensity` | Ambient Intensity | `0.15` | `0` to `1` | `0.01` |
| `skyColor` | Sky Color | `#b9dbd0` | `#000000` to `#ffffff` | 8-bit per channel |
| `groundColor` | Ground Color | `#4f5967` | `#000000` to `#ffffff` | 8-bit per channel |
| `tintBlend` | Tint Blend | `1` | `0` Multiply; `1` OKLab chromatic tint | — |
| `tintResponse` | Tint Response | `1` | `0.1` to `4` | `0.01` |
| `shadowChroma` | Shadow Chroma | `1.15` | `0` to `2.5` | `0.01` |

### 17.10 Light In Motion

| Key | English label | Initial value | Bounds or enum values | Step / unit |
|---|---|---|---|---|
| `animateLight` | Animate light | `true` | `false` / `true` | — |
| `lightSpeed` | Light speed | `8` | `-45` to `45` | `0.1` / deg/s |

### 17.11 Reflection Model

| Key | English label | Initial value | Bounds or enum values | Step / unit |
|---|---|---|---|---|
| `renderModel` | Shading Model | `0` | `0` Studio / anisotropic GGX; `1` RFF-style artistic highlight | — |
| `roughness` | Roughness | `0.28` | `0.055` to `0.95` | `0.005` |
| `metalness` | Metalness | `0.72` | `0` to `1` | `0.01` |
| `specularIntensity` | Specular Intensity | `0.75` | `0` to `3` | `0.01` |
| `specularPower` | Specular Power | `64` | `1` to `512`; logarithmic slider | `1` |
| `specularColor` | Specular Color | `#fff1d5` | `#000000` to `#ffffff` | 8-bit per channel |

### 17.12 Specular / Anisotropy

| Key | English label | Initial value | Bounds or enum values | Step / unit |
|---|---|---|---|---|
| `specularIndependent` | Specular Independent | `false` | `false` / `true` | — |
| `specularAzimuth` | Specular Azimuth | `125` | `-180` to `180` | `1` / deg |
| `specularZenith` | Specular Zenith | `50` | `0` to `89` | `1` / deg |
| `anisotropy` | Specular Anisotropy | `0.28` | `0` to `0.9` | `0.01` |
| `anisotropyAngle` | Specular Anisotropy Angle | `22` | `-180` to `180` | `1` / deg |
| `reliefResponse` | Relief Response | `1` | `0` to `1` | **Invalid text step**; proposed: `0.01` (A01) |

### 17.13 Studio / Clearcoat

| Key | English label | Initial value | Bounds or enum values | Step / unit |
|---|---|---|---|---|
| `lightIntensity` | Direct Light — GGX | `0.8` | `0` to `3` | `0.01` |
| `environmentIntensity` | Environment — GGX | `0.74` | `0` to `3` | `0.01` |
| `softness` | Softbox Width — GGX | `0.55` | `0.05` to `1.4` | `0.01` |
| `coat` | Clearcoat — GGX | `0.3` | `0` to `1` | `0.01` |
| `coatRoughness` | Coat Roughness — GGX | `0.14` | `0.04` to `0.6` | `0.005` |
| `ior` | Index of Refraction — GGX | `1.5` | `1.05` to `2.5` | `0.01` |
| `iridescence` | Iridescence — approximation | `0.07` | `0` to `1` | `0.01` |
| `filmThickness` | Film Thickness — 3 wavelengths | `410` | `100` to `1000` | `1` / nm |

### 17.14 Rim Light

| Key | English label | Initial value | Bounds or enum values | Step / unit |
|---|---|---|---|---|
| `rimIntensity` | Rim Intensity | `0.12` | `0` to `2` | `0.01` |
| `rimPower` | Rim Power | `2` | `0.25` to `8` | `0.05` |
| `rimColor` | Rim Color | `#d1e0cf` | `#000000` to `#ffffff` | 8-bit per channel |

### 17.15 Relief Gloss

| Key | English label | Initial value | Bounds or enum values | Step / unit |
|---|---|---|---|---|
| `glossIntensity` | Gloss Intensity | `0` | `0` to `2` | `0.01` |
| `glossSource` | Gloss Source | `3` | `0` Shading; `1` Relief density; `2` Aspect; `3` Fine shading / independent normal | — |
| `glossBands` | Gloss Bands | `3` | `1` to `24` | `0.1` |
| `glossSharpness` | Gloss Sharpness | `45` | `1` to `256` | `1` |
| `glossPhase` | Gloss Phase | `0.15` | `0` to `1` | `0.001` |
| `glossRelief` | Gloss Relief | `8` | `0` to `16` | `0.1` |
| `glossColor` | Gloss Color | `#f5d9a6` | `#000000` to `#ffffff` | 8-bit per channel |
| `lightBlend` | Light Blend | `1` | `0` Direct / encoded-space addition; `1` Linear HDR addition | — |

### 17.16 Color Grade

| Key | English label | Initial value | Bounds or enum values | Step / unit |
|---|---|---|---|---|
| `exposure` | Exposure | `0.05` | `-3` to `3` | `0.05` / EV |
| `contrast` | Contrast | `1.03` | `0.4` to `1.8` | `0.01` |
| `saturation` | Saturation | `1.06` | `0` to `2` | `0.01` |
| `hue` | Hue — OKLab | `0` | `-180` to `180` | `1` / deg |
| `brightness` | Brightness | `1` | `0.1` to `3` | `0.01` |
| `colorGamma` | Color Gamma | `1` | `0.4` to `2.2` | `0.01` |

### 17.17 Tone / Bloom

| Key | English label | Initial value | Bounds or enum values | Step / unit |
|---|---|---|---|---|
| `tonemap` | Tone Mapping | `0` | `0` Soft peak shoulder; `1` Peak Reinhard; `2` Filmic peak fit; `3` Clip / no compression | — |
| `highlightKnee` | Highlight Knee | `0.68` | `0.1` to `0.99` | `0.01` |
| `bloomAmount` | Bloom Intensity | `0.05` | `0` to `0.6` | `0.005` |
| `bloomThreshold` | Bloom Threshold | `1.05` | `0.1` to `4` | `0.05` |
| `bloomSoftness` | Bloom Softness | `0.6` | `0.01` to `1` | `0.01` |
| `bloomRadius` | Bloom Radius | `1.8` | `0.3` to `6` | `0.1` |
| `vignette` | Vignette | `0.14` | `0` to `0.7` | `0.01` |
| `grain` | Grain | `0` | `0` to `1` | `0.01` |

### 17.18 Stripe

| Key | English label | Initial value | Bounds or enum values | Step / unit |
|---|---|---|---|---|
| `stripeType` | Stripe Type | `0` | `0` None; `1` Single sinusoid; `2` Double period; `3` Squared | — |
| `stripeOpacity` | Opacity | `0.15` | `0` to `1` | `0.01` |
| `stripeFirst` | First Interval | `5` | `0.1` to `100` | `0.1` |
| `stripeSecond` | Second Interval | `13` | `0.1` to `100` | `0.1` |
| `stripeOffset` | Offset | `0` | `0` to `100` | `0.1` |
| `stripeSpeed` | Animation Speed | `0` | `-10` to `10` | `0.1` |

### 17.19 One Procedural Layer

| Key | English label | Initial value | Bounds or enum values | Step / unit |
|---|---|---|---|---|
| `patternAmount` | Pattern Opacity | `0` | `0` to `1` | `0.01` |
| `patternType` | Pattern Type | `0` | `0` Stripes; `1` Checker; `2` Grid; `3` Dots; `4` Diamond; `5` Honeycomb-like; `6` Waves; `7` Cloud | — |
| `patternFollow` | UV Mode | `1` | `0` Screen; `1` Cycle / Screen | — |
| `patternScale` | Pattern Scale | `18` | `1` to `80` | `1` |
| `patternAngle` | Pattern Angle | `0` | `-180` to `180` | `1` / deg |
| `patternSharpness` | Pattern Sharpness | `1` | `0.2` to `8` | `0.05` |
| `patternColor` | Pattern Color | `#d4c399` | `#000000` to `#ffffff` | 8-bit per channel |
| `warpAmount` | Warp Amount | `0` | `0` to `0.5` | `0.005` |
| `warpScale` | Warp Scale | `5` | `0.2` to `20` | `0.1` |
| `warpOctaves` | Warp Octaves | `4` | `1` to `6` | `1` |
| `warpSpeed` | Warp Speed | `0.15` | `-1` to `1` | `0.01` |

### 17.20 Iteration Fog

| Key | English label | Initial value | Bounds or enum values | Step / unit |
|---|---|---|---|---|
| `fogAmount` | Fog Opacity | `0` | `0` to `1` | `0.01` |
| `fogStart` | Fog Start — log₂(iteration) | `6` | `0` to `12` | `0.1` |
| `fogRange` | Fog Range | `3` | `0.1` to `10` | `0.1` |
| `fogColor` | Fog Color | `#b4cbbe` | `#000000` to `#ffffff` | 8-bit per channel |

### 17.21 Render / View

| Key | English label | Initial value | Bounds or enum values | Step / unit |
|---|---|---|---|---|
| `viewMode` | Diagnostic View | `0` | `0` Finished image; `1` Decorated base / palette; `2` Surface normal; `3` AO; `4` Reflection | — |
| `iterations` | Max Iterations | `720` | `360` 360; `720` 720; `1200` 1200; `2000` 2000 | — |
| `rotation` | Rotation | `-15` | `-180` to `180` | `1` / deg |

### 17.22 Rfc / Rfsp

| Key | English label | Initial value | Bounds or enum values | Step / unit |
|---|---|---|---|---|
| `rffMapping` | RFF mapping | `false` | `false` / `true` | — |
| `rffGloss` | RFF Palette Gloss | `false` | `false` / `true` | — |
| `rffSpeed` | RFF Animation Speed | `0` | `-1e+09` to `1e+09` | `0.01` / iter/s |

### 17.23 Imported Color Grade

| Key | English label | Initial value | Bounds or enum values | Step / unit |
|---|---|---|---|---|
| `rffGrade` | RFF color correction | `false` | `false` / `true` | — |
| `rffGamma` | Gamma | `1` | `0.001` to `100` | `0.001` |
| `rffExposure` | Exposure — rational | `0` | `-1` to `0.999` | `0.001` |
| `rffHue` | Hue — cycles | `0` | `-1000` to `1000` | `0.001` |
| `rffSaturation` | Saturation — additive | `0` | `-10` to `10` | `0.001` |
| `rffBrightness` | Brightness — additive | `0` | `-10` to `10` | `0.001` |
| `rffContrast` | Contrast — rational | `0` | `-1` to `0.999` | `0.001` |

### 17.24 Dependency notes

`rffMapping` is an indicator controlled by the presence of an imported descriptor, not an ordinary editable switch. `rffGloss` requires an imported palette. Raw imports lock the stop editor and hide stop easing. Studio-only controls are disabled in artistic mode. Linked RGB disables independent intervals; linked specular lighting disables its separate angle controls.

`paletteGloss` is the ordinary stop-path effect; `rffGloss` is the imported entry-based effect. The `rff*` color controls are active only when `rffGrade` is enabled. All these controls remain represented in saved state even when a UI group is inactive.

## 18. Non-control state and presets

### 18.1 State outside the numeric-control schema

| State key / concept | Initial value or constraint | Native interpretation |
|---|---|---|
| `cx`, `cy` | `-0.761194`, `0.084676`; browser validation keeps each within ±8 | View position, not a color-import target. Preserve native arbitrary precision. |
| `span` | `0.00075`; browser range `2e-7` to `4.2` | Vertical view span; do not import these browser bounds into the native solver. |
| `quality` | `1.25`; choices `0.75, 1.25, 1.75, 2.5` | Browser rendering scale, not a material parameter. |
| `palette`, `material`, `scene` | Preset-selection indices; initially `0,0,0` | UI state; material/palette values remain authoritative. |
| `stops` | 2–32 sorted `{pos,color}` records | Optional authoring metadata, not the full imported array. |
| `frozen` | Initially empty; at most 16 values | Native implementation should retain its double values without browser caps. |
| `palettePhase`, `flowPhase`, `stripePhase`, `warpPhase`, `rffPhase` | Initially zero | Accumulated animation state; units differ by phase. |
| `rffPalette` | Initially `null` | Raw/recipe descriptor preserving imported palette representation. |
| `rffInfo` | Initially `null` | Source identity, original color/palette metadata, warnings; not live shading data. |
| `playing` | `false` | Runtime playback state, outside the settings schema. |
| `comparing`, `split` | `false`, `0.5` | Runtime comparison state; not native image-reference data. |
| `selected`, `activeTab`, `picking` | Editor selection/tab/eyedropper state | UI-only concerns. |

### 18.2 Material presets

These presets change only the listed fields. They do not reset every light, tint, relief, or grading control. Every material preset uses HTML `renderModel=0` (Studio GGX).

| Preset | Roughness | Metalness | Coat | Coat roughness | Specular intensity | Anisotropy | Iridescence | Film thickness, nm |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Bronze | 0.28 | 0.72 | 0.3 | 0.14 | 0.75 | 0.28 | 0.07 | 410 |
| Pearl | 0.32 | 0.24 | 0.72 | 0.12 | 0.85 | 0.12 | 0.4 | 360 |
| Obsidian | 0.16 | 0.87 | 0.34 | 0.1 | 0.85 | 0.38 | 0.02 | 410 |
| Ceramic | 0.43 | 0.02 | 0.83 | 0.17 | 0.63 | 0 | 0 | 410 |

### 18.3 Palette presets

Colors are listed in stop order. Except for Verdigris, positions are uniform at `i/N` for `i=0..N-1`. They are not sampled native recipe palettes.

| Preset | Encoded RGB stops | Positions |
|---|---|---|
| Verdigris | `#102b39` `#226769` `#699892` `#c2cbb2` `#dfbd7c` `#9c643d` `#464032` `#123537` | `0, 0.15, 0.3, 0.44, 0.57, 0.7, 0.82, 0.92` |
| Nacre | `#263553` `#628799` `#a4b9c5` `#e0d9d6` `#d2b1c2` `#9b95b9` `#5c8d9f` `#dae0c6` | `i/N` |
| Ember | `#07151c` `#16333f` `#326475` `#a7844e` `#ecc18a` `#a45e34` `#492e28` `#152332` | `i/N` |
| Aurora | `#111e40` `#383865` `#866c8c` `#deb6b3` `#c5d2b2` `#5dafa4` `#287278` `#1c374f` | `i/N` |
| Ivory | `#3a5556` `#719386` `#b9c2a2` `#f1e4bd` `#cbbb91` `#899e89` `#426c68` | `i/N` |
| Carmine | `#211f2a` `#532f40` `#99584f` `#d09b70` `#e2c998` `#b98460` `#67454c` `#2d3843` | `i/N` |

### 18.4 Browser scene presets

These are convenient shallow-view bookmarks, not additions to the native fractal algorithm.

| Preset | Center real | Center imaginary | Span | Rotation, deg |
|---|---:|---:|---:|---:|
| Spiral | `-0.761194` | `0.084676` | `0.00075` | `-15` |
| Seahorse | `-0.743643887037151` | `0.13182590420533` | `0.0001` | `-10` |
| Filigree | `-0.10109636384562` | `0.95628651080914` | `0.003` | `12` |
| Origin | `-0.63` | `0` | `2.45` | `0` |

## 19. Local implementation index

Line numbers are **one-based in the exact hashed `lustre_II_rfc.html` file**, not the extracted work files. Several functions are intentionally compacted onto single lines. Search by script ID/function name when an editor wraps long lines. These locations make the implementation claims above reproducible without treating this document as a replacement for the source.

### 19.1 Embedded scripts

| Script ID / block | HTML lines | Purpose |
|---|---:|---|
| `vertexShader` | 37–43 | Full-screen triangle. |
| `fractalShader` | 44–66 | Browser double-single fractal field and distance estimate. |
| `materialShader` | 67–183 | Palette mapping, relief, GGX, artistic reflection, decorations, diagnostics. |
| `bloomShader` | 184–192 | Soft-threshold extraction and separable blur. |
| `displayShader` | 193–212 | Composition, ordinary grade, tone map, imported grade, dither. |
| `rffBinaryReader` | 892–969 | RFFImport parser, recipes, raw descriptor serialization. |
| `Main application script` | 970–1269 | Controls, state, rendering host, import UI, export, interaction. |

### 19.2 Key source symbols

| Symbol | HTML line | What to inspect |
|---|---:|---|
| `heightDifference()` | 125 | Relative-log bias, Taylor branch, waves, inversion. |
| `sobel()` | 127 | Edge clamping, radius reduction, normalization, mean. |
| `reliefNormal()` | 128 | Gain and nonlinear limiter. |
| `compositeShade()` | 129 | Three shading-space implementations. |
| `tint()` | 130 | Ambient tint and shadow chroma. |
| `distribution()` | 135 | Anisotropic GGX D. |
| `visibility()` | 136 | Smith visibility including the BRDF denominator. |
| `tangents()` | 137 | Current flat-gradient problem. |
| `specLight()` | 138 | Direct lobe, power/roughness coupling, film tint. |
| `rectangle()` | 139 | Procedural softbox projection and blur. |
| `studio()` | 140 | Four lights, background, environment scale. |
| `rffEntry()` | 114 | Mirrored raw palette indexing and RFF entry gloss. |
| `lut()` | 117 | Manual imported-entry interpolation and bands. |
| `frozenWeight()` | 123 | Freeze matching across RGB periods. |
| `rffCorrect()` | 201 | Imported encoded-space color grade. |
| `prepareStops()` | 1081 | CPU encoded/linear/OKLab stop preparation. |
| `paletteSample()` | 1083 | Stop positions, easing, seam handling. |
| `sliderValue()` | 1099 | NaN reproduction for invalid step metadata. |
| `uploadPalette()` | 1156 | 4096-entry stop LUT vs full raw RGBA32F upload. |
| `drawGeometry()` | 1157 | Browser field uniforms and precision split. |
| `drawMaterial()` | 1158 | Bindings and actual base-color comparison behavior. |
| `drawBloom()` | 1159 | Pass size, directions, threshold selection. |
| `drawDisplay()` | 1160 | Display inputs and split. |
| `frame()` | 1163 | Animation phases, dirty flags, GPU synchronization. |
| `validateState()` | 1186 | State bounds and imported-descriptor validation. |
| `saveSettings()` | 1185 | Lustre JSON format/version. |
| `exportPNG()` | 1197 | SDR export and temporary render size. |
| `rffSample()` | 1228 | CPU imported palette preview. |
| `rffPlan()` | 1242 | Actual applied fields, translations, clipping warnings. |
| `applyRffPending()` | 1255 | Validate then apply one state transaction. |
| `convertRff()` | 1240 | Explicit lossy 32-stop conversion. |
| `parse()` | 919 | Binary parser in the rffBinaryReader block. |
| `recipe()` | 957 | Browser regeneration of supported native palettes. |
| `descriptor()` | 965 | Raw/recipe descriptor construction. |
| `decode()` | 964 | Raw float32 / recipe descriptor validation. |

### 19.3 Other examined local evidence

`Lustre_II_README.md`, `Lustre_II_RFC_README.md`, and the retained bundle's `developer/rff-import.js`, `developer/TEST_RESULTS.json`, and `demo_synthetic_color_import.rfc` were examined. The fresh audit extracted the control schema and executed the importer checks described in Section 16. The supplied preview images were not used as proof of native rendering equivalence.

## 20. Pinned upstream source index

All source-file links below point to commit `a20d3c3f8b651b69cb36dba26a7334ed614daee7`. A file listed as an integration target is not evidence that a patch has been made there. The local HTML remains the authority for implemented Lustre behavior; proposed C++ work is identified separately.

- **[U01]** — Commit: 2.2.1 - Faster Coloring: `a20d3c3f8b651b69cb36dba26a7334ed614daee7`.
- **[U02]** — Slope attributes: `src/rff2/attr/ShdSlopeAttribute.h`.
- **[U03]** — Native slope shader: `shdsrc/vk_slope.frag`.
- **[U04]** — Palette attributes and CPU interpolation: `src/rff2/attr/ShdPaletteAttribute.h`.
- **[U05]** — Native palette fragment shader: `shdsrc/vk_iteration_palette.frag`.
- **[U06]** — Native palette interpolation enum: `src/rff2/attr/ShdPalColorInterpolationMethod.h`.
- **[U07]** — Native slope shading enum: `src/rff2/attr/ShdSlopeShadingBlend.h`.
- **[U08]** — Native palette upload: `src/rff2/vulkan/GPCIterationPalette.cpp`.
- **[U09]** — Native slope host binding: `src/rff2/vulkan/GPCSlope.cpp`.
- **[U10]** — Descriptor layouts and target reservations: `src/rff2/vulkan/SharedDescriptorTemplate.hpp`.
- **[U11]** — Palette mode packing / specialization: `src/rff2/vulkan/ShaderModeSpecialization.hpp`.
- **[U12]** — Compute/video palette and stripe mapping: `shdsrc/vk_2_map_iter_stripe.comp`.
- **[U13]** — Shader preset serialization implementation: `src/rff2/io/ShaderPresetIO.cpp`.
- **[U14]** — Configuration serialization implementation: `src/rff2/io/ConfigIO.cpp`.
- **[U15]** — Shader preset IO API and format constants: `src/rff2/io/ShaderPresetIO.h`.
- **[U16]** — Configuration IO API and format constants: `src/rff2/io/ConfigIO.h`.
- **[U17]** — Native palette recipes: `src/rff2/preset/shader/palette/ShdPalettePresets.cpp`.
- **[U18]** — Shader settings UI and preview callbacks: `src/rff2/ui/CallbackShader.cpp`.
- **[U19]** — Native color shader: `shdsrc/vk_color.frag`.
- **[U20]** — Native color host: `src/rff2/vulkan/GPCColor.cpp`.
- **[U21]** — Native bloom host: `src/rff2/vulkan/GPCBloom.cpp`.
- **[U22]** — Native final output / tone mapping / SDR-PQ-HLG: `shdsrc/vk_linear_interpolation.frag`.
- **[U23]** — Native HDR settings: `src/rff2/attr/ShdHdrAttribute.h`.
- **[U24]** — Native final-output host: `src/rff2/vulkan/GPCLinearInterpolation.cpp`.
- **[U25]** — Native shader settings bundle: `src/rff2/attr/ShaderAttribute.h`.
- **[U26]** — Build manifest: `CMakeLists.txt`.
- **[U27]** — Repository license: `LICENSE`.
- **[U28]** — Repository notices: `NOTICE`.

---

**Implementation takeaway:** the most useful native additions are an opt-in Studio material model, explicit linear-light transport, consistent palette interpolation, non-destructive palette authoring, and selective color import. Preserve existing RFF algorithms and defaults; port the intended improvements, not browser limitations or the audit findings left unresolved in the current HTML.

[U01]: https://github.com/superfractal/RFF_Super/commit/a20d3c3f8b651b69cb36dba26a7334ed614daee7 "Commit: 2.2.1 - Faster Coloring"
[U02]: https://github.com/superfractal/RFF_Super/blob/a20d3c3f8b651b69cb36dba26a7334ed614daee7/src/rff2/attr/ShdSlopeAttribute.h "Slope attributes"
[U03]: https://github.com/superfractal/RFF_Super/blob/a20d3c3f8b651b69cb36dba26a7334ed614daee7/shdsrc/vk_slope.frag "Native slope shader"
[U04]: https://github.com/superfractal/RFF_Super/blob/a20d3c3f8b651b69cb36dba26a7334ed614daee7/src/rff2/attr/ShdPaletteAttribute.h "Palette attributes and CPU interpolation"
[U05]: https://github.com/superfractal/RFF_Super/blob/a20d3c3f8b651b69cb36dba26a7334ed614daee7/shdsrc/vk_iteration_palette.frag "Native palette fragment shader"
[U06]: https://github.com/superfractal/RFF_Super/blob/a20d3c3f8b651b69cb36dba26a7334ed614daee7/src/rff2/attr/ShdPalColorInterpolationMethod.h "Native palette interpolation enum"
[U07]: https://github.com/superfractal/RFF_Super/blob/a20d3c3f8b651b69cb36dba26a7334ed614daee7/src/rff2/attr/ShdSlopeShadingBlend.h "Native slope shading enum"
[U08]: https://github.com/superfractal/RFF_Super/blob/a20d3c3f8b651b69cb36dba26a7334ed614daee7/src/rff2/vulkan/GPCIterationPalette.cpp "Native palette upload"
[U09]: https://github.com/superfractal/RFF_Super/blob/a20d3c3f8b651b69cb36dba26a7334ed614daee7/src/rff2/vulkan/GPCSlope.cpp "Native slope host binding"
[U10]: https://github.com/superfractal/RFF_Super/blob/a20d3c3f8b651b69cb36dba26a7334ed614daee7/src/rff2/vulkan/SharedDescriptorTemplate.hpp "Descriptor layouts and target reservations"
[U11]: https://github.com/superfractal/RFF_Super/blob/a20d3c3f8b651b69cb36dba26a7334ed614daee7/src/rff2/vulkan/ShaderModeSpecialization.hpp "Palette mode packing / specialization"
[U12]: https://github.com/superfractal/RFF_Super/blob/a20d3c3f8b651b69cb36dba26a7334ed614daee7/shdsrc/vk_2_map_iter_stripe.comp "Compute/video palette and stripe mapping"
[U13]: https://github.com/superfractal/RFF_Super/blob/a20d3c3f8b651b69cb36dba26a7334ed614daee7/src/rff2/io/ShaderPresetIO.cpp "Shader preset serialization implementation"
[U14]: https://github.com/superfractal/RFF_Super/blob/a20d3c3f8b651b69cb36dba26a7334ed614daee7/src/rff2/io/ConfigIO.cpp "Configuration serialization implementation"
[U15]: https://github.com/superfractal/RFF_Super/blob/a20d3c3f8b651b69cb36dba26a7334ed614daee7/src/rff2/io/ShaderPresetIO.h "Shader preset IO API and format constants"
[U16]: https://github.com/superfractal/RFF_Super/blob/a20d3c3f8b651b69cb36dba26a7334ed614daee7/src/rff2/io/ConfigIO.h "Configuration IO API and format constants"
[U17]: https://github.com/superfractal/RFF_Super/blob/a20d3c3f8b651b69cb36dba26a7334ed614daee7/src/rff2/preset/shader/palette/ShdPalettePresets.cpp "Native palette recipes"
[U18]: https://github.com/superfractal/RFF_Super/blob/a20d3c3f8b651b69cb36dba26a7334ed614daee7/src/rff2/ui/CallbackShader.cpp "Shader settings UI and preview callbacks"
[U19]: https://github.com/superfractal/RFF_Super/blob/a20d3c3f8b651b69cb36dba26a7334ed614daee7/shdsrc/vk_color.frag "Native color shader"
[U20]: https://github.com/superfractal/RFF_Super/blob/a20d3c3f8b651b69cb36dba26a7334ed614daee7/src/rff2/vulkan/GPCColor.cpp "Native color host"
[U21]: https://github.com/superfractal/RFF_Super/blob/a20d3c3f8b651b69cb36dba26a7334ed614daee7/src/rff2/vulkan/GPCBloom.cpp "Native bloom host"
[U22]: https://github.com/superfractal/RFF_Super/blob/a20d3c3f8b651b69cb36dba26a7334ed614daee7/shdsrc/vk_linear_interpolation.frag "Native final output / tone mapping / SDR-PQ-HLG"
[U23]: https://github.com/superfractal/RFF_Super/blob/a20d3c3f8b651b69cb36dba26a7334ed614daee7/src/rff2/attr/ShdHdrAttribute.h "Native HDR settings"
[U24]: https://github.com/superfractal/RFF_Super/blob/a20d3c3f8b651b69cb36dba26a7334ed614daee7/src/rff2/vulkan/GPCLinearInterpolation.cpp "Native final-output host"
[U25]: https://github.com/superfractal/RFF_Super/blob/a20d3c3f8b651b69cb36dba26a7334ed614daee7/src/rff2/attr/ShaderAttribute.h "Native shader settings bundle"
[U26]: https://github.com/superfractal/RFF_Super/blob/a20d3c3f8b651b69cb36dba26a7334ed614daee7/CMakeLists.txt "Build manifest"
[U27]: https://github.com/superfractal/RFF_Super/blob/a20d3c3f8b651b69cb36dba26a7334ed614daee7/LICENSE "Repository license"
[U28]: https://github.com/superfractal/RFF_Super/blob/a20d3c3f8b651b69cb36dba26a7334ed614daee7/NOTICE "Repository notices"
