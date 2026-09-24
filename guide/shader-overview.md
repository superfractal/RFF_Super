<!-- Created by GPT-6 on 2026-09-24. -->
# Shader settings: a complete feature overview

[Main guide](SETTINGS_GUIDE.md) · [Individual fields and defaults](settings-reference.md) · [Rendered comparisons](visual-comparisons.md) · [24 additional shader comparisons](shader-comparisons.md)

Use this page to understand the purpose of each shader feature before looking up individual fields. It covers the current Shader menu, the Surface editor, and the custom compositing stack. Repeated Texture, Pattern, and Effect layers share controls but hold independent values. The pictures are existing verified RFF_Super renders; a style picture shows a recipe, not an isolated test of every control listed beside it.

## Find a feature from the Shader menu

| Menu entry | What it opens or does | Overview |
| --- | --- | --- |
| Palette | Iteration colors, band decoration, stops, recipes, and imports | [Palette and band lines](#palette-and-band-lines) |
| Texture | Four image texture layers | [Textures](#textures) |
| Pattern | Four generated pattern layers | [Patterns](#patterns) |
| Warp | Displacement of appearance coordinates | [Domain warp](#domain-warp) |
| Stripe | Iteration-based stripe shading | [Stripe](#stripe) |
| Slope | Lighting & Relief controls | [Lighting and relief](#lighting-and-relief) |
| Color | Color Correction controls in Finishing | [Color correction](#color-correction) |
| Effects | Four Animated Materials layers | [Animated materials](#animated-materials) |
| Fog | Fog, with Focus Band and Chaos Blur in Finishing | [Fog and selective blur](#fog-and-selective-blur) |
| Bloom | Bright-area glow | [Bloom](#bloom) |
| HDR | HDR & Tone Mapping in the output settings | [HDR and tone mapping](#hdr-and-tone-mapping) |
| Shader Layers | Order and visibility of compositing contributions | [Shader Layers](#shader-layers) |
| Load KFR Color | Opens the palette import workflow | [Imports and presets](#imports-and-presets) |
| Import Color | Opens the palette import workflow | [Imports and presets](#imports-and-presets) |
| Local AI appearance | Optional local-model appearance workflow | [Imports and presets](#imports-and-presets) |
| Save Shader Preset | Saves appearance settings as `.rfsp` | [Imports and presets](#imports-and-presets) |
| Load Shader Preset | Restores an `.rfsp` appearance | [Imports and presets](#imports-and-presets) |

**Surface editing is also part of shader appearance**, even though its material/style controls are presented separately from the Shader menu's Slope form. See [Studio and surface styles](#studio-and-surface-styles). Color animation controls live in Animation; they are summarized under [Movement](#movement).

## Palette and band lines

Palette controls turn iteration values into colors. Changing them usually recolors the existing fractal without changing its mathematical location.

| Feature | Controls and result |
| --- | --- |
| Cycle spacing and phase | **Cycle Length (R), Cycle Length (G), Cycle Length (B)** set repetition per channel. Smaller lengths repeat colors more often. **Start Offset** shifts phase. **Iteration Coloring** changes the mapping before cycling. |
| Color distribution | **Cycle Bias** and **Cycle Curve** redistribute positions within a cycle. **Seamless (Mirror)** mirrors repetition. **Color Smoothing** selects discrete, normal, or reversed transitions; **Color Interpolation** selects RGB, OKLab, or Linear RGB blending. |
| Interior and palette highlights | **Mandelbrot Color** colors interior/iteration-limit pixels. **Palette Gloss** and its **Gloss Color** add cycle-linked highlights. This is separate from relief Gloss below. |
| Basic band lines | **Band Lines**, **Lines per Cycle**, **Line Width**, **Line Opacity**, **Line Softness**, and **Line Color** define the repeated contours. Opacity 0 hides the contribution. |
| Recessed grooves | **Recessed Grooves** adds a relief groove. **Auto Groove** derives the shape from the band-line setup; **Groove Depth** and **Groove Width** shape the manual response. |
| Spines and branches | **Spine Amount**, **Spine Length**, **Spine Density**, and **Branch Amount** decorate the contours. Amount 0 removes the decoration. They use the band-line color, width, and opacity; Studio is not required. |
| Ornaments | **Ornament Amount**, **Ornament Size**, **Ornament Density**, and **Ornament Inset** control ornamental detail and placement along the lines. They are also band-line controls, independent of Studio. |
| Line glow | **Line Glow**, **Glow Width**, and **Glow Color** add contour glow. These share underlying values with the legacy surface rim controls; they are not the directional Rim Light controls. |
| Editable stops | **Selected Stop**, **Stop Position**, **Stop Color**, and **Stop Easing** shape an explicit stop palette. Select and apply a stop before editing it. **Create 8 Stops (Lossy)** replaces the previous palette representation with a resampled one. |
| Generated palettes | **Palette Preset** and **Seed** choose a repeatable recipe. Applying it replaces the recipe's palette settings, not merely a preview swatch. |

![Band-line example with eight lines per cycle](images/band-lines-after.png)

[Palette before/after comparisons](visual-comparisons.md#palette) · [All palette fields](settings-reference.md#palette)

**Example: Spines**

| Before: Spine Amount: 0; band lines enabled | After: Spine Amount: 1; Spine Length: 1.5 |
| --- | --- |
| ![Spines before](images/spines-before.png) | ![Spines after](images/spines-after.png) |

Branch-like extensions appear along the band contours. The band-line color, opacity, and framing stay fixed.

[Full comparison and settings](shader-comparisons.md#spines)

**Example: Ornaments**

| Before: Ornament Amount: 0; band lines enabled | After: Ornament Amount: 1; Ornament Size: 1.5 |
| --- | --- |
| ![Ornaments before](images/ornaments-before.png) | ![Ornaments after](images/ornaments-after.png) |

Look at the sparse ornaments near the left and right edges. This localized change is easy to miss in a reduced thumbnail; click to inspect the full image.

[Full comparison and settings](shader-comparisons.md#ornaments)

## Lighting and relief

Lighting works with the apparent surface derived from iteration data. Test these controls with visible relief; a nearly flat normal or disabled contribution can hide a large numeric change.

| Feature | Controls and result |
| --- | --- |
| Shading and compositing | **Shading Depth** controls relief strength; **Slope Opacity** controls its ordinary contribution. **Shading Blend** chooses Overlay or OKLab Lightness. **Surface Blend** chooses Mix, Add, Multiply, or Screen for legacy surface compositing. **Replace Surface Style Completely** requires Studio and a non-Original style and bypasses the legacy palette tint/blend/opacity route. |
| Light–shadow transition | **Shadow Floor** lifts the unlit side; **Terminator Softness** softens the lit/shadow boundary; **Relief Lightness** controls directional lightness independently of the tint contribution. |
| Main and fill lights | **Light Zenith** and **Light Direction** aim the main light. **Fill Intensity**, **Fill Zenith**, and **Fill Direction** control a second diffuse light. Zero fill intensity makes its direction ineffective. |
| Specular highlights | **Specular Intensity**, **Specular Power**, **Relief Response**, and **Specular Color** control highlight strength, concentration, response, and color. **Independent Specular Light**, **Specular Zenith**, and **Specular Direction** separate its direction from the main light. **Specular Anisotropy** stretches highlights; **Anisotropy Angle** rotates that stretch. |
| Broad relief and cavities | **Macro Relief** and **Macro Radius** emphasize broader structure; **Cavity Intensity** darkens recessed detail. |
| Lit/shadow tint | **Tint Intensity**, **Light-Facing Color**, **Shadow-Facing Color**, **Tint Response**, and **Tint Blend** color the two sides. **Shadow Chroma** affects OKLab Tint. A zero tint intensity hides the tint colors. |
| Local light tone | **Slope Brightness**, **Slope Gamma**, **Highlight Knee**, and **Light Blend** shape the lighting response. These are distinct from final Color Correction and HDR tone mapping. |
| Directional rim light | **Rim Intensity**, **Rim Power**, and **Rim Color** brighten rims; power changes concentration. This rim footprint can also mask Fog. |
| Relief Gloss | **Gloss Intensity**, **Gloss Source**, **Gloss Relief**, **Gloss Bands**, **Gloss Sharpness**, **Gloss Phase**, and **Gloss Color** create repeated highlights from the chosen relief source. They do not edit Palette Gloss. |
| Lustre relief | **Lustre Relief**, **Lustre Depth**, **Normal Smoothing**, **AO Radius**, and **Boundary Reflection Guard** control the alternate relief and its boundary/reflection response. **Invert Relief** reverses its orientation. **Relief Waves** and **Wave Frequency** add modulation. |
| Zoom compensation | **Auto Zoom Compensation** and **Zoom Reference** keep Lustre behavior relative to a reference zoom. **Use Current Zoom** captures that reference. |

[Light-direction and slope comparisons](visual-comparisons.md#lighting) · [All lighting fields](settings-reference.md#lighting--relief)

**Example: Lit and shadow tint**

| Before: Tint Intensity: 0; warm/cool tint colors set | After: Tint Intensity: 0.8; same tint colors |
| --- | --- |
| ![Lit and shadow tint before](images/light-tint-before.png) | ![Lit and shadow tint after](images/light-tint-after.png) |

The same warm light-facing and cool shadow-facing colors are configured on both sides; only Tint Intensity changes.

[Full comparison and settings](shader-comparisons.md#lit-and-shadow-tint)

**Example: Relief Gloss**

| Before: Gloss Intensity: 0 | After: Gloss Intensity: 0.8; Bands: 4; Sharpness: 12 |
| --- | --- |
| ![Relief Gloss before](images/relief-gloss-before.png) | ![Relief Gloss after](images/relief-gloss-after.png) |

Bright repeated highlights follow the relief. This is the relief Gloss control, separate from Palette Gloss.

[Full comparison and settings](shader-comparisons.md#relief-gloss)

## Studio and surface styles

**Studio** enables the material response. **Base Style** chooses Original or one of six recipes. **Style Application** selects Legacy Compositing or Complete Replacement. A recipe sets several related controls together; loading a recipe is not an isolated change to one material property.

| Liquid Metal: chrome and reflections | Cyber Sigilism: ink and reflective contours | PHONK: colored chrome and damage |
| --- | --- | --- |
| ![Liquid Metal recipe](images/style-1-after.png) | ![Cyber Sigilism recipe](images/style-2-after.png) | ![PHONK recipe](images/style-3-after.png) |

| Black Metal: frost and monochrome | Deep Sea: body and emission | Ukiyo-e: flat print colors and waves |
| --- | --- | --- |
| ![Black Metal recipe](images/style-4-after.png) | ![Deep Sea recipe](images/style-5-after.png) | ![Ukiyo-e recipe](images/style-6-after.png) |

**Original** retains the original surface style. The six mix controls—**Liquid Metal Mix**, **Cyber Sigilism Mix**, **PHONK Mix**, **Black Metal Mix**, **Deep Sea Light**, and **Ukiyo-e Wave Mix**—let contributions be combined. Editing a related surface control can activate its contribution; inspect the mixes when an edit changes more than expected. The editor's Basic view hides advanced controls; switch to its Detail view and expand a group to see those controls.

The reset menu has different scopes: reset one selected value/color, **Reset effect** for the selected category, **Clear added effects (keep base style)**, or **Reset all appearance (includes palette)**. Read the scope before using it; the last action also replaces palette settings.

### Material, reflection, and color

| Feature | Controls and result |
| --- | --- |
| Material | **Roughness** spreads Studio reflections; **Metalness** changes the metallic response; **Index of Refraction** controls dielectric reflection. **Clearcoat** adds a coating; **Coat Roughness** spreads that coating's reflections. |
| Studio lighting | **Direct Light** controls direct illumination; **Studio Reflections** controls the environment contribution. **Environment Rotation** rotates the environment; **Follow Light Direction** couples it to the main light direction, from independent at 0 to full follow at 1. |
| Reflection shaping | **Reflection Detail**, **Reflection Contrast**, **Reflection Brightness**, and **Reflection Curvature** shape reflection structure. **Specular Antialiasing** reduces harsh specular variation; it is not a substitute for image SSAA. |
| Palette and surface | **Palette Amount** (reference name **Palette Color**) mixes palette color into the style. **Chrome Strength** controls the legacy surface contribution. **Surface Phase** shifts surface/reflection structure. **Surface Color Amount** scales **Surface Color**. Complete Replacement bypasses the legacy composite controls described above. |
| Reflection/background tint | **Reflection Color Amount** scales **Reflection Color**; **Background Color Amount** scales **Background Color**. Raising an amount makes its selected tint contribute more. |
| Sigil ink | **Preserve Palette Ink** retains dark palette ink; **Sigil Background** sets the sigil background brightness. These have a useful effect when the sigil contribution is visible. |

**Example: Metalness**

| Before: Studio On; Metalness: 0 | After: Studio On; Metalness: 1 |
| --- | --- |
| ![Metalness before](images/metalness-before.png) | ![Metalness after](images/metalness-after.png) |

Studio is enabled with Specular Intensity 0.8 on both sides. Increasing Metalness changes both the reflected color and the balance of the material response.

[Full comparison and settings](shader-comparisons.md#metalness)

### Film, prism, and glints

**Thin Film Color** is displayed as **Iridescence** in the Surface inspector; a separate advanced field is also named Iridescence. See the [name mapping](settings-reference.md#surface-editor-display-names) to distinguish them. **Film Thickness (nm)** changes thin-film hues, while **Film Hue** shifts their phase. **Prism Width / Prism Spread** are the inspector names for **Rainbow Width / Rainbow Spread**. They shape the prismatic region. **Glint Strength** and **Glint Size** control small bright reflections. Changing thickness is not simply a brightness adjustment.

**Example: Film thickness**

| Before: Advanced Iridescence: 1; Film Thickness: 200 nm | After: Advanced Iridescence: 1; Film Thickness: 800 nm |
| --- | --- |
| ![Film thickness before](images/film-thickness-before.png) | ![Film thickness after](images/film-thickness-after.png) |

With the advanced Iridescence field set to 1, changing thickness shifts the reflected hues. This uses the separate thickness-related field, not the Surface inspector alias for Thin Film Color.

[Full comparison and settings](shader-comparisons.md#film-thickness)

### Chrome flames, frost, and noise

| Feature | Controls and result |
| --- | --- |
| Chrome flames | **Chrome Flames**, **Red / Purple**, **Shadow Crush**, and **PHONK Surface Damage** shape the PHONK/chrome-flame treatment. This is separate from the Flame type in Animated Materials. |
| Frost | **Frost Strength** and **Frost Threshold** control the Black Metal frost contribution. Threshold changes the selected regions; strength changes how strongly they appear. |
| VHS and channel shift | **VHS / Nearest Effects** enables the finish contribution; **VHS Noise** (reference **VHS Damage**) adds disturbance and **Chromatic Shift** offsets channels. |
| Pixelation | **Nearest Mix** blends the block-sampled result; **Pixel Size** (reference **Nearest Block Size**) changes block size. A zero mix hides the block-size change. |
| Grain and monochrome | **Monochrome / Grain Effects** controls this finishing contribution. **Film Grain**, **Grunge Scale**, and **Black Metal Monochrome** set grain strength/scale and monochrome amount. |

**Example: Frost strength**

| Before: Black Metal; Frost Strength: 0 | After: Black Metal; Frost Strength: 2 |
| --- | --- |
| ![Frost strength before](images/frost-before.png) | ![Frost strength after](images/frost-after.png) |

Frost fills more of the fine structure in the Black Metal treatment. Its other recipe settings, including monochrome finish, remain fixed.

[Full comparison and settings](shader-comparisons.md#frost-strength)

**Example: Nearest pixelation**

| Before: VHS / Nearest Effects: 1; VHS Noise: 0; Chromatic Shift: 0; Nearest Mix: 0 | After: Nearest Mix: 1; Pixel Size: 12; noise and channel shift remain 0 |
| --- | --- |
| ![Nearest pixelation before](images/pixelation-before.png) | ![Nearest pixelation after](images/pixelation-after.png) |

Nearest Mix adds intentional block sampling. These square blocks are the selected effect, not a loss of fractal calculation precision.

[Full comparison and settings](shader-comparisons.md#nearest-pixelation)

### Emission and dense detail

**Emission**, **Emission Threshold**, and **Emission Color Balance** are the Surface editor's names for **Sea Glow**, **Sea Glow Threshold**, and **Sea Color Balance**. They set the emission strength, selection, and balance between **Emission Color A** and **Emission Color B**. **Body Light / Body Color** affect the underlying sea body; Body Light is listed as **Sea Body Light** in the registry. **Particle Amount / Particle Size** control luminous particles, with registry names **Sea Particles / Sea Particle Size**.

**Detail Brightness**, **Dense Detail Suppression**, and **Density Threshold** tune fine highlights and suppress overly bright dense regions. Their reference names are Detail Light and Detail Density Threshold where different. The old surface rim fields **Surface Rim Strength** (alias **Detail Rim Light**), **Surface Rim Width**, and **Detail Rim Color** are exposed with Palette's **Line Glow**, **Glow Width**, and **Glow Color**. They are routed to Band Line rather than an additional independent Surface inspector group.

**Example: Emission**

| Before: Deep Sea; Emission: 0 | After: Deep Sea; Emission: 3 |
| --- | --- |
| ![Emission before](images/emission-before.png) | ![Emission after](images/emission-after.png) |

Deep Sea emission brightens the body highlights and luminous detail. Emission is the Surface editor name for Sea Glow.

[Full comparison and settings](shader-comparisons.md#emission)

**Example: Luminous particles**

| Before: Deep Sea; Particle Amount: 0 | After: Deep Sea; Particle Amount: 1; Particle Size: 2 |
| --- | --- |
| ![Luminous particles before](images/sea-particles-before.png) | ![Luminous particles after](images/sea-particles-after.png) |

Particle Amount enables more visible luminous dots; Particle Size is also increased as stated. This is an effect setup example, not an isolated size test.

[Full comparison and settings](shader-comparisons.md#luminous-particles)

### Print and color quantization

**Color Quantization** controls the print finish independently of Base Style. **Color Count** (reference **Print Color Count**) selects 3–6 print colors; **Flatness** (reference **Print Flatness**) controls the flattened result. **Print Tone Balance** redistributes print tones. **Wave Foam** adds pale wave detail; **Woodcut Grain** adds print texture.

The six print colors are **Ink, Indigo, Asagi, Pale Blue, Foam, and Paper**. Their visible use depends on color count and the selected tones. A color not selected by the current picture can change numerically without an obvious visual difference.

[All surface fields](settings-reference.md#surface-effects) · [Style before/after pairs and RFC files](visual-comparisons.md#surface-styles)

**Example: Print color count**

| Before: Ukiyo-e; Color Count: 3 | After: Ukiyo-e; Color Count: 6 |
| --- | --- |
| ![Print color count before](images/print-colors-before.png) | ![Print color count after](images/print-colors-after.png) |

Three print colors produce fewer tonal regions; six permit additional blue/green tones. The location and Ukiyo-e recipe otherwise stay fixed.

[Full comparison and settings](shader-comparisons.md#print-color-count)

## Textures

For each of four layers, turn **Enabled** on, choose **Image File**, and raise **Opacity**. **Blend Mode** offers Multiply, Overlay, and Replace: multiplication darkens by the texture, overlay changes contrast, and replace blends toward the texture color. Keep external image files available when sharing the preset.

**UV Source** chooses Screen, Cycle × Band, Cycle × Angle, or Cycle × Screen placement. **Texture Period** 0 follows the palette; a positive value gives an independent iteration period. **Size** enlarges/shrinks texture features, **Keep Aspect** preserves source proportions, and **Repeat U / Repeat V** change repetition. **Palette Follow** couples movement to palette animation; **Scroll U / Scroll V** add signed motion. A still image cannot show a speed difference at one fixed time.

Texture, Pattern, and Animated Materials panels also offer **Reset Layer**, **Copy to Next Layer**, **Move Toward Bottom**, and **Move Toward Top**. Copy overwrites the destination layer; moving swaps neighboring slots. See [layer actions](settings-reference.md#palette-and-layer-actions).

[Texture fields](settings-reference.md#textures)

## Patterns

Each of four layers generates **Stripes, Checker, Grid, Dots, Diamond, Honeycomb, Waves, or Cloud** without an external image. Select the **Pattern**, enable the layer, and use **Opacity** and **Blend Mode** to control compositing.

**Ink Mode** chooses **Solid Color**, using **Ink Color**, or **Palette Shift**, using a shifted palette sample. **Sharpness** controls the shape boundary. **UV Source**, **Pattern Period**, **Repeat U / Repeat V**, **Palette Follow**, and **Scroll U / Scroll V** provide the corresponding placement and motion controls.

**Edge Enabled**, **Edge Color**, **Edge Width**, and **Edge Opacity** add an independent outline. **Relative Edge Width** normalizes the outline against the shape's field range, so its effect can differ between pattern types. Test edge controls with the edge enabled and nonzero opacity.

[Pattern fields](settings-reference.md#patterns)

**Example: Pattern outlines**

| Before: Dots; Screen UV; Repeat 12/8; Opacity 0.8; Edge Enabled: Off | After: Edge Enabled: On; red Edge Color; Edge Width: 0.25; Edge Opacity: 1 |
| --- | --- |
| ![Pattern outlines before](images/pattern-outline-before.png) | ![Pattern outlines after](images/pattern-outline-after.png) |

A red ring appears around the same procedural dots. Edge color, width, and opacity are set explicitly in the enabled example.

[Full comparison and settings](shader-comparisons.md#pattern-outlines)

## Domain warp

**Warp Enabled** and **Warp Amount** turn coordinate displacement on and set its strength. **Warp Source** chooses procedural Noise or Texture 1–4; texture-based warp needs the corresponding image source. **Warp Detail** controls the noise detail layers. **UV Source**, **Repeat U / Repeat V**, **Warp Period**, **Palette Follow**, and **Scroll U / Scroll V** place and animate the displacement.

Warp changes appearance coordinates, not the Mandelbrot formula or saved location. Amount 0 produces no displacement. It is not itself a reorderable color layer in Shader Layers.

[Warp example](SETTINGS_GUIDE.md#textures-patterns-warp-and-stripes) · [Warp fields](settings-reference.md#domain-warp)

## Stripe

**Stripe Type** selects None, Single Direction, Smooth, or Squared. **Interval 1 / Interval 2** set iteration spacing; **Opacity** sets strength; **Offset** shifts phase; **Animation Speed** moves it over time. Type None disables the effect.

This is different from both a Pattern layer whose shape is Stripes and Palette Band Lines. Its controls affect iteration-based shading rather than an image texture.

[Stripe fields](settings-reference.md#stripe)

## Animated materials

Each of four independent layers has **Enabled**, **Effect Type**, **Blend**, **Mask**, and **Opacity**. Blend offers Normal, Add, Screen, and Multiply. Mask restricts coverage to Surface, Exterior, No Surface, Bands, or Iteration Edges; a mask can exclude the visible area.

| Effect Type | Visual purpose |
| --- | --- |
| Rain | Rain-shaped streaks/drops; **Rain Shape** and **Drop Size** specialize this type. |
| Flame | Flowing flame-like surface highlights. |
| Embers | Scattered ember-like luminous details. |
| Mist | Soft mist-like coverage. |
| Heat Haze | Heat-like surface distortion treatment. |
| Ripples | Repeating ripple-like structure. |
| Flow Light | Flowing bands of light. |
| Aurora | Aurora-like colored curtains. |

**Surface Scale**, **Density**, **Shape Length**, **Shape Width**, **Surface Depth**, **Glow**, **Primary Color**, **Secondary Color**, and **Seed** shape the selected effect. Their interpretation depends on the type; Rain Shape does not select a flame shape. **Speed**, **Evolution**, and **Flow Bend** control its motion. **Sync Color Animation** couples its clock to color animation. **Band Period** sets the iteration period used by the band mask. Check the mask and motion clock before deciding a control is ineffective.

[Flame comparison](SETTINGS_GUIDE.md#textures-patterns-warp-and-stripes) · [All Animated Materials fields](settings-reference.md#animated-materials)

**Example: Mist**

| Before: Mist Layer 1: Off; t = 2 s | After: Mist Layer 1: On; Opacity: 1; Density: 0.8; pale blue Primary Color; t = 2 s |
| --- | --- |
| ![Mist before](images/mist-before.png) | ![Mist after](images/mist-after.png) |

The pale-blue material adds a soft, subtle change at 2 seconds. A still pair demonstrates the enabled layer, not its animation speed.

[Full comparison and settings](shader-comparisons.md#mist)

## Color correction

**Gamma** shapes midtones; **Exposure** uses the color-grading curve; **Hue** rotates colors; **Saturation** increases/decreases colorfulness; **Brightness** adds an offset; **Contrast** expands/compresses differences. Neutral values are Gamma 1 and all five others 0. Saturation −1 produces grayscale.

Color Correction's Exposure is not measured in stops. **HDR Exposure** is a different setting, measured in stops. Reordering Color Correction in the custom stack changes which earlier contributions it grades.

[Color comparison and controls](SETTINGS_GUIDE.md#color-correction-bloom-and-blur) · [Color Correction fields](settings-reference.md#color-correction)

## Fog and selective blur

| Feature | Controls and result |
| --- | --- |
| Fog | **Radius** and **Opacity** mix blurred color. **Center Start** and **Invert Falloff** shape screen-space coverage. |
| Rim-masked fog | **Rim Mask**, **Rim Mask Boost**, and **Rim Blur** confine/soften fog around the directional rim footprint. A useful rim contribution is needed to see the mask. |
| Focus Band | **Focus Amount**, **Focus Depth**, **Focus Range**, **Focus Falloff**, and **Focus Blur** keep an iteration-depth band sharp and blur outside it. This is not physical camera-distance depth of field. |
| Chaos Blur | **Chaos Amount**, **Chaos Detail Scale**, **Chaos Threshold**, **Chaos Transition**, **Chaos Feather**, and **Chaos Blur Radius** select and soften intricate regions. **Chaos Highlight Detail** retains some bright detail; **Chaos Shade** darkens the selected regions. |
| Blur Quality | Speed caps relevant full-resolution radii at 16 texels. Appearance permits larger radii, up to the shader's 4096-texel safety limit, and denser sampling. Small ordinary local blurs can match in both modes; Chaos Blur changes sampling density even below the 16-texel cap. |

Focus Band and Chaos Blur are groups within Finishing and participate in the Fog contribution; they do not appear as separate Shader menu entries or reorderable layers.

[Blur examples](visual-comparisons.md#finishing) · [Finishing fields](settings-reference.md#finishing)

## Bloom

**Threshold** chooses bright pixels, **Radius** spreads them, **Intensity** sets strength, and **Softness** blends sharp thresholded glow toward blurred glow. Softness 1 is fully blurred. **Linear Light** changes the light-addition method. A threshold above all visible brightness values yields no bloom.

Bloom is separate from Line Glow and Animated Materials Glow: those create local bright contributions; bloom spreads selected bright areas. In custom order, only suitable earlier contributions are available for later processing.

[Bloom before/after](SETTINGS_GUIDE.md#color-correction-bloom-and-blur) · [Bloom fields](settings-reference.md#bloom)

## HDR and tone mapping

**HDR Rendering** retains above-white light through the HDR chain. **Exposure** adjusts linear light in stops: +1 doubles it. **Highlight Headroom** controls the highlight range for applicable conventional curves. **MFR Mastering Peak (nits)** sets MFR display metadata/curve behavior, not source illumination. It is separate from the export **HDR Peak Brightness (nits)**.

| Tone Mapping choice | Purpose |
| --- | --- |
| Clip | Cuts off values outside the display range; useful for seeing clipping. |
| Reinhard | Compresses bright values with a gradual roll-off. |
| ACES (Narkowicz fit) | Uses Narkowicz's curve approximation, not a complete ACES transform; see [NOTICE](../NOTICE). |
| Filmic | Uses a filmic highlight/shadow curve. |
| MFR Shoulder | MFR display rendering with a highlight shoulder. |
| MFR Log View | Logarithmic inspection/display view of the light range. |
| MFR Linear Clip | Linear display view with clipping. |
| MFR False Color | Diagnostic colors for exposure ranges rather than a natural-color image. |

The MFR SDR display modes ignore Highlight Headroom. For PQ/HLG video, enable HDR Rendering and select **Video Transfer** in export settings; the selected SDR preview curve does not by itself select an HDR video format.

[HDR fields](settings-reference.md#hdr-and-tone-mapping) · [Detailed MFR behavior](../docs/mfr-hdr-preview.md)

**Example: HDR shoulder**

| Before: HDR On; Exposure: +2 stops; Tone Mapping: Clip | After: HDR On; Exposure: +2 stops; Tone Mapping: MFR Shoulder |
| --- | --- |
| ![HDR shoulder before](images/hdr-shoulder-before.png) | ![HDR shoulder after](images/hdr-shoulder-after.png) |

These are SDR PNG previews of HDR source light at +2 stops. MFR Shoulder compresses highlights differently from Clip; they are not HDR-encoded image files.

[Full comparison and settings](shader-comparisons.md#hdr-shoulder)

**Example: HDR false color**

| Before: HDR On; Exposure: 0 stops; Tone Mapping: MFR Shoulder | After: HDR On; Exposure: 0 stops; Tone Mapping: MFR False Color |
| --- | --- |
| ![HDR false color before](images/hdr-false-color-before.png) | ![HDR false color after](images/hdr-false-color-after.png) |

False Color visualizes exposure ranges. Its blue/gray/yellow colors are diagnostic output, not corruption or a natural-color palette.

[Full comparison and settings](shader-comparisons.md#hdr-false-color)

## Shader Layers

**Custom Layer Order** enables ordered compositing. **Selected Layer** chooses a contribution; **Open Layer List** opens the list. Move Up/Down, Bring to Front, Send to Back, or dragging changes order. Higher layers are applied later. Moving or toggling visibility enables custom order. Hiding a contribution retains its values, so it can be restored without rebuilding the settings.

The stack has **29 layer slots**. The list can show only currently active contributions; a stored slot is not a guarantee of a visible effect.

| Layer name(s) | What the contribution contains |
| --- | --- |
| Band Line | Palette contour lines and their decoration. |
| Texture 1–4 | The four image texture layers, separately ordered. |
| Pattern 1–4 | The four generated pattern layers, separately ordered. |
| Stripe | Iteration-based stripe shading. |
| Lighting & Base Material | Base surface lighting/material response. |
| Metal | Liquid Metal contribution. |
| Cyber Sigilism | Sigil styling. |
| PHONK | PHONK contribution. |
| Frost | Black Metal frost contribution. |
| Sea | Deep Sea contribution. |
| Print Material | Ukiyo-e surface/wave material. |
| Material Color & Rim | Material color/rim contribution, distinct from the base light stage. |
| Effect 1–4 | Animated Materials layers. |
| Color Correction | Final-style color grading at its chosen position. |
| Fog | Fog, Focus Band, and Chaos Blur processing. |
| Bloom | Bright-area glow. |
| HDR / Tone Mapping | Display/tonal mapping at its position in the ordered stack. |
| Print Color Quantization | Print color reduction, separately ordered from Print Material. |
| VHS Finish | VHS/nearest-sampling finish. |
| Monochrome Finish | Monochrome/grain finish. |

For example, placing a pattern above a blur keeps its newly added edges sharper; placing it below lets the later blur affect them. This is an ordering example, not a promise that every layer interacts identically with every mask. **Restore Original Order** resets order and visibility and disables custom compositing. Turning Custom Layer Order off alone restores the original rendering path while retaining the saved custom arrangement.

[Layer controls](settings-reference.md#shader-layer-controls)

**Example: Layer order**

| Before: Custom Layer Order: On; Pattern 1 below Color Correction; Saturation: -1 | After: Pattern 1 moved to front, above Color Correction; same settings |
| --- | --- |
| ![Layer order before](images/layer-order-before.png) | ![Layer order after](images/layer-order-after.png) |

When the pattern is below desaturation, it turns gray too. Moving Pattern 1 to the front adds colored dots after desaturation. Both images use Custom Layer Order.

[Full comparison and settings](shader-comparisons.md#layer-order)

## Movement

Shader animation includes more than the **Color Animation Speed** slider. **Animation Mode**, **Flow Amount**, **Flow Scale**, **Flow Speed**, and **Swirl** shape palette motion. **Frozen Iteration Values** and **Freeze Match Tolerance** hold selected bands still. Texture/Pattern/Warp have their own follow/scroll settings; Stripe has Animation Speed; Animated Materials has its own speed/evolution controls. A static image at a fixed timestamp cannot demonstrate each speed difference.

The Timeline Editor can animate supported parameters by depth. Selecting a track, its interpolation, or a zoom hold changes animation behavior rather than adding another shader layer. See the [illustrated timeline walkthrough](SETTINGS_GUIDE.md#timeline-controls) and [Animation fields](settings-reference.md#animation).

## Imports and presets

**Color Settings File** accepts RFC, RFSP, KFR, or KFP in the palette import workflow. Read the source, then apply it. **Include Color Correction** additionally imports Gamma, Exposure, Hue, Saturation, Brightness, and Contrast. This is narrower than loading a full shader preset.

**Save Shader Preset / Load Shader Preset** work with `.rfsp` appearance settings. A full `.rfc` also saves the artwork's location/configuration. External texture/audio sources are separate assets. The **Preset → Shader** menu provides preset families for palette, stripe, slope, color, fog, bloom, and full examples; a family preset replaces that component while a full example replaces the shader settings.

**Local AI appearance** is an optional helper using a configured local-model server. It proposes/applies appearance settings; it is not an additional GPU effect. The [detailed Local AI chapter](local-ai.md) covers setup, Generate/Apply/Undo, refinement, AI zoom, locator retries, and troubleshooting, with actual images of both tabs. The separate [Timeline AI Edit](animation-and-export.md#timeline-ai-edit) exchanges timeline JSON and image sheets.

## Coverage and verification

This overview distinguishes **feature coverage** from **image-test coverage**: the guide contains 54 measured before/after render pairs, not an individual visual test for every shader field. Defaults, ranges, and alternate displayed names are in the [field reference](settings-reference.md). The [coverage checklist](shader-coverage.md) records the source groups checked and points to each description.
