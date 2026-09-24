<!-- Created by GPT-6 on 2026-09-24. -->
# Visual settings comparisons

[24 additional shader comparisons](shader-comparisons.md) · [Back to the guide](SETTINGS_GUIDE.md) · [Field reference](settings-reference.md) · [Validation](validation.md)

All images use the same location, framing, and output size. Captions list the edited values; unrelated appearance effects are disabled in the shared baseline. Each source image is 800 × 450. Click an image to view it at full size. Each downloadable RFC includes the complete example state. Animation images also require the indicated playback time.

Style choices apply multi-control recipes. Other comparisons either change one value or explicitly list the setup needed to expose that effect. The Rain comparison is deliberately identified as subtle.

[Download baseline](examples/baseline.rfc) · [Unmodified supplied source 1](examples/source-1.rfc) · [Unmodified supplied source 2](examples/source-2.rfc)

## Palette

<details>
<summary>Palette Offset</summary>

Colors move around the cycle while the fractal stays in place.

| Before: Palette Start Offset: original | After: Palette Start Offset: original + 0.25 cycle |
| --- | --- |
| ![Palette Offset before](images/palette-offset-before.png) | ![Palette Offset after](images/palette-offset-after.png) |

[Before settings](examples/palette-offset-before.rfc) · [After settings](examples/palette-offset-after.rfc)

</details>

<details>
<summary>Palette Interval</summary>

Halving the iteration intervals makes color repeat more often.

| Before: Color intervals: saved values | After: Color intervals: half the saved values |
| --- | --- |
| ![Palette Interval before](images/palette-interval-before.png) | ![Palette Interval after](images/palette-interval-after.png) |

[Before settings](examples/palette-interval-before.rfc) · [After settings](examples/palette-interval-after.rfc)

</details>

<details>
<summary>Cycle Bias</summary>

Bias changes the proportion of the cycle occupied by different colors.

| Before: Cycle Bias: 1 | After: Cycle Bias: 2 |
| --- | --- |
| ![Cycle Bias before](images/cycle-bias-before.png) | ![Cycle Bias after](images/cycle-bias-after.png) |

[Before settings](examples/cycle-bias-before.rfc) · [After settings](examples/cycle-bias-after.rfc)

</details>

<details>
<summary>Band Lines</summary>

Black boundaries separate palette bands.

| Before: Band Line: Off | After: Band Line: On; Count 8; Width 0.09 |
| --- | --- |
| ![Band Lines before](images/band-lines-before.png) | ![Band Lines after](images/band-lines-after.png) |

[Before settings](examples/band-lines-before.rfc) · [After settings](examples/band-lines-after.rfc)

</details>

## Lighting

<details>
<summary>Slope</summary>

Relief darkens the side facing away from the light.

| Before: Slope Opacity: 0 | After: Slope Opacity: 1; Depth 0.02 |
| --- | --- |
| ![Slope before](images/slope-before.png) | ![Slope after](images/slope-after.png) |

[Before settings](examples/slope-before.rfc) · [After settings](examples/slope-after.rfc)

</details>

<details>
<summary>Light Direction</summary>

Reversing the light reverses the lit/shadow orientation.

| Before: Light Direction: 45 degrees | After: Light Direction: 225 degrees |
| --- | --- |
| ![Light Direction before](images/light-direction-before.png) | ![Light Direction after](images/light-direction-after.png) |

[Before settings](examples/light-direction-before.rfc) · [After settings](examples/light-direction-after.rfc)

</details>

<details>
<summary>Specular</summary>

A specular highlight appears on the lit surface.

| Before: Specular Intensity: 0 | After: Specular Intensity: 0.8 |
| --- | --- |
| ![Specular before](images/specular-before.png) | ![Specular after](images/specular-after.png) |

[Before settings](examples/specular-before.rfc) · [After settings](examples/specular-after.rfc)

</details>

<details>
<summary>Rim</summary>

Blue-white light emphasizes rim regions.

| Before: Rim Intensity: 0 | After: Rim Intensity: 0.8; blue-white Rim Color |
| --- | --- |
| ![Rim before](images/rim-before.png) | ![Rim after](images/rim-after.png) |

[Before settings](examples/rim-before.rfc) · [After settings](examples/rim-after.rfc)

</details>

<details>
<summary>Roughness</summary>

The tight reflection becomes broad and soft. Studio and a nonzero specular contribution are enabled on both sides.

| Before: Studio On; Specular Intensity 0.8; Roughness: 0.1 | After: Studio On; Specular Intensity 0.8; Roughness: 0.8 |
| --- | --- |
| ![Roughness before](images/roughness-before.png) | ![Roughness after](images/roughness-after.png) |

[Before settings](examples/roughness-before.rfc) · [After settings](examples/roughness-after.rfc)

</details>

## Surface styles

<details>
<summary>Y2K Chrome / Liquid Metal</summary>

Selecting this style applies its full surface recipe, including associated line/finishing choices.

| Before: Original surface | After: Base Style: Y2K Chrome / Liquid Metal |
| --- | --- |
| ![Y2K Chrome / Liquid Metal before](images/style-1-before.png) | ![Y2K Chrome / Liquid Metal after](images/style-1-after.png) |

[Before settings](examples/style-1-before.rfc) · [After settings](examples/style-1-after.rfc)

</details>

<details>
<summary>Cyber Sigilism</summary>

Selecting this style applies its full surface recipe, including associated line/finishing choices.

| Before: Original surface | After: Base Style: Cyber Sigilism |
| --- | --- |
| ![Cyber Sigilism before](images/style-2-before.png) | ![Cyber Sigilism after](images/style-2-after.png) |

[Before settings](examples/style-2-before.rfc) · [After settings](examples/style-2-after.rfc)

</details>

<details>
<summary>PHONK / Drift Phonk</summary>

Selecting this style applies its full surface recipe, including associated line/finishing choices.

| Before: Original surface | After: Base Style: PHONK / Drift Phonk |
| --- | --- |
| ![PHONK / Drift Phonk before](images/style-3-before.png) | ![PHONK / Drift Phonk after](images/style-3-after.png) |

[Before settings](examples/style-3-before.rfc) · [After settings](examples/style-3-after.rfc)

</details>

<details>
<summary>Black Metal Album Art</summary>

Selecting this style applies its full surface recipe, including associated line/finishing choices.

| Before: Original surface | After: Base Style: Black Metal Album Art |
| --- | --- |
| ![Black Metal Album Art before](images/style-4-before.png) | ![Black Metal Album Art after](images/style-4-after.png) |

[Before settings](examples/style-4-before.rfc) · [After settings](examples/style-4-after.rfc)

</details>

<details>
<summary>Deep Sea Bioluminescence</summary>

Selecting this style applies its full surface recipe, including associated line/finishing choices.

| Before: Original surface | After: Base Style: Deep Sea Bioluminescence |
| --- | --- |
| ![Deep Sea Bioluminescence before](images/style-5-before.png) | ![Deep Sea Bioluminescence after](images/style-5-after.png) |

[Before settings](examples/style-5-before.rfc) · [After settings](examples/style-5-after.rfc)

</details>

<details>
<summary>Ukiyo-e Fractal</summary>

Selecting this style applies its full surface recipe, including associated line/finishing choices.

| Before: Original surface | After: Base Style: Ukiyo-e Fractal |
| --- | --- |
| ![Ukiyo-e Fractal before](images/style-6-before.png) | ![Ukiyo-e Fractal after](images/style-6-after.png) |

[Before settings](examples/style-6-before.rfc) · [After settings](examples/style-6-after.rfc)

</details>

## Decoration

<details>
<summary>Pattern</summary>

A generated checker appears over exterior color. No image file is used.

| Before: Pattern Layer 1: Off | After: Checker; Screen UV; Repeat U/V: 12/8; Opacity: 0.6 |
| --- | --- |
| ![Pattern before](images/pattern-before.png) | ![Pattern after](images/pattern-after.png) |

[Before settings](examples/pattern-before.rfc) · [After settings](examples/pattern-after.rfc)

</details>

<details>
<summary>Warp</summary>

The enabled checker and palette are displaced by noise warping.

| Before: Warp: Off (checker enabled) | After: Warp: On; Amount: 0.4 (checker enabled) |
| --- | --- |
| ![Warp before](images/warp-before.png) | ![Warp after](images/warp-after.png) |

[Before settings](examples/warp-before.rfc) · [After settings](examples/warp-after.rfc)

</details>

<details>
<summary>Stripe</summary>

Iteration-based stripes darken the exterior.

| Before: Stripe Opacity: 0 | After: Opacity: 0.65; Intervals: 10 and 20 |
| --- | --- |
| ![Stripe before](images/stripe-before.png) | ![Stripe after](images/stripe-after.png) |

[Before settings](examples/stripe-before.rfc) · [After settings](examples/stripe-after.rfc)

</details>

<details>
<summary>Rain</summary>

This rain example is subtle: compare the fine surface highlights at full size. It changes only a small portion of this view; it is not a global blur.

| Before: Rain Layer 1: Off | After: Rain Layer 1: On; Opacity: 1; Drop Size: 3; Surface Depth: 2; Glow: 1 |
| --- | --- |
| ![Rain before](images/rain-before.png) | ![Rain after](images/rain-after.png) |

[Before settings](examples/rain-before.rfc) · [After settings](examples/rain-after.rfc)

</details>

<details>
<summary>Flame</summary>

The material adds flowing surface highlights. Both views are evaluated at 2 seconds; only the enabled material contribution changes.

| Before: Flame Layer 1: Off | After: Flame Layer 1: On; Opacity 0.85; Glow 1; t = 2 s |
| --- | --- |
| ![Flame before](images/flame-before.png) | ![Flame after](images/flame-after.png) |

[Before settings](examples/flame-before.rfc) · [After settings](examples/flame-after.rfc)

</details>

## Finishing

<details>
<summary>Gamma</summary>

Midtones become brighter without changing the fractal geometry.

| Before: Gamma: 1 | After: Gamma: 1.7 |
| --- | --- |
| ![Gamma before](images/gamma-before.png) | ![Gamma after](images/gamma-after.png) |

[Before settings](examples/gamma-before.rfc) · [After settings](examples/gamma-after.rfc)

</details>

<details>
<summary>Exposure</summary>

The positive color-grading Exposure strongly brightens the picture. This is not the HDR exposure-in-stops control.

| Before: Exposure: 0 | After: Exposure: 0.6 |
| --- | --- |
| ![Exposure before](images/exposure-before.png) | ![Exposure after](images/exposure-after.png) |

[Before settings](examples/exposure-before.rfc) · [After settings](examples/exposure-after.rfc)

</details>

<details>
<summary>Hue</summary>

A quarter-turn hue shift replaces the color relationships.

| Before: Hue: 0 | After: Hue: 0.25 turn |
| --- | --- |
| ![Hue before](images/hue-before.png) | ![Hue after](images/hue-after.png) |

[Before settings](examples/hue-before.rfc) · [After settings](examples/hue-after.rfc)

</details>

<details>
<summary>Saturation</summary>

The same structure becomes grayscale.

| Before: Saturation: 0 | After: Saturation: -1 (grayscale) |
| --- | --- |
| ![Saturation before](images/saturation-before.png) | ![Saturation after](images/saturation-after.png) |

[Before settings](examples/saturation-before.rfc) · [After settings](examples/saturation-after.rfc)

</details>

<details>
<summary>Contrast</summary>

Highlights and dark regions separate more strongly.

| Before: Contrast: 0 | After: Contrast: 0.5 |
| --- | --- |
| ![Contrast before](images/contrast-before.png) | ![Contrast after](images/contrast-after.png) |

[Before settings](examples/contrast-before.rfc) · [After settings](examples/contrast-after.rfc)

</details>

<details>
<summary>Bloom</summary>

Bright details produce soft halos.

| Before: Bloom Intensity: 0 | After: Intensity: 1.2; Threshold: 0.3; Radius: 0.2; Softness: 1 |
| --- | --- |
| ![Bloom before](images/bloom-before.png) | ![Bloom after](images/bloom-after.png) |

[Before settings](examples/bloom-before.rfc) · [After settings](examples/bloom-after.rfc)

</details>

<details>
<summary>Fog</summary>

The broad fog contribution softens the image.

| Before: Fog Opacity: 0 | After: Fog Opacity: 0.8; Radius: 0.12 |
| --- | --- |
| ![Fog before](images/fog-before.png) | ![Fog after](images/fog-after.png) |

[Before settings](examples/fog-before.rfc) · [After settings](examples/fog-after.rfc)

</details>

<details>
<summary>Focus</summary>

Most visible detail lies outside this narrow selected depth band and becomes defocused.

| Before: Focus Amount: 0 | After: Amount: 1; Depth: 0.3; Range: 0.05; Blur: 16 |
| --- | --- |
| ![Focus before](images/focus-before.png) | ![Focus after](images/focus-after.png) |

[Before settings](examples/focus-before.rfc) · [After settings](examples/focus-after.rfc)

</details>

<details>
<summary>Chaos</summary>

Intricate structure softens while broad smooth regions retain their form.

| Before: Chaos Amount: 0 | After: Amount: 1; Threshold: 0.15; Blur Radius: 12 |
| --- | --- |
| ![Chaos before](images/chaos-before.png) | ![Chaos after](images/chaos-after.png) |

[Before settings](examples/chaos-before.rfc) · [After settings](examples/chaos-after.rfc)

</details>

## Animation

<details>
<summary>Animation</summary>

Only time changes: the palette advances by 16 iterations in one second.

| Before: Linear animation: t = 0 s, Speed = 16 | After: Linear animation: t = 1 s, Speed = 16 |
| --- | --- |
| ![Animation before](images/animation-before.png) | ![Animation after](images/animation-after.png) |

[Before settings](examples/animation-before.rfc) · [After settings](examples/animation-after.rfc)

</details>
