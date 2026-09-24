<!-- Created by GPT-6 on 2026-09-24. -->
<!-- Modified by GPT-6 on 2026-09-25. -->
# RFF_Super complete field reference

Read the [shader feature overview](shader-overview.md) for an explanation of each group, and use the [coverage checklist](shader-coverage.md) to locate features by their source panel. Surface editor names can differ from the registry labels below; see [Surface editor display names](#surface-editor-display-names).

[Back to the illustrated guide](SETTINGS_GUIDE.md) · [Visual comparisons](visual-comparisons.md)

Field names, choices, and defaults were read from current production form models. Defaults mean a fresh session on this machine, not a saved preset. Read the effect/dependency column first. Texture, Pattern, and Animated Material fields are listed once and apply separately to each of their four layers.

## Index

- [Lighting & Relief](#lighting--relief)
- [Textures](#textures)
- [Patterns](#patterns)
- [Warp & Stripe](#warp--stripe)
- [Finishing](#finishing)
- [Animated Materials](#animated-materials)
- [Palette](#palette)
- [Render & Preview](#render--preview)
- [Explore](#explore)
- [Animation](#animation)
- [Zoom Overlay](#zoom-overlay)
- [Surface Effects](#surface-effects)
- [Export and timeline controls](#export-and-timeline-controls)
- [Surface editor display names](#surface-editor-display-names)
- [HDR and tone mapping](#hdr-and-tone-mapping)
- [Shader layer controls](#shader-layer-controls)
- [Palette and layer actions](#palette-and-layer-actions)

## Lighting & Relief

### Slope Shading

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Shading Blend | Overlay | Overlay changes the color composite; OKLab Lightness carries relief mainly through perceptual lightness. | Overlay, OKLab Lightness |
| Replace Surface Style Completely | Off | With Studio and a non-Original style, replaces the palette entirely and bypasses legacy tint/blend/opacity contributions. | On, Off |
| Surface Blend | Mix | Chooses Mix, Add, Multiply, or Screen for legacy compositing; complete replacement bypasses it. | Mix, Add, Multiply, Screen |
| Shading Depth | 0 | Amplifies the relief gradient. High values can saturate the slope, making further increases look similar. | 0 to 10000 |
| Shadow Floor | 0 | Raises the minimum diffuse illumination on the shadow side. | 0 to 1 |
| Slope Opacity | 1 | Blends slope shading into the underlying color; 0 removes its ordinary contribution. | 0 to 1 |
| Terminator Softness | 0 | Softens the transition between the lit and shadowed sides. | 0 to 1 |
| Relief Lightness | 1 | Strength of directional lightness shading; 0 removes that lightness contribution while allowing tint. | 0 to 1 |

### Light Direction

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Light Zenith | 60 | Changes the main light's elevation in degrees. | 0 to 359.99997 |
| Light Direction | 135 | Rotates the main light around the relief in degrees. | 0 to 359.99997 |
| Fill Intensity | 0 | Strength of the secondary diffuse light; 0 disables it. | 0 to 1 |
| Fill Zenith | 60 | Aims the secondary light; requires nonzero Fill Intensity. | 0 to 359.99997 |
| Fill Direction | 315 | Aims the secondary light; requires nonzero Fill Intensity. | 0 to 359.99997 |

### Specular

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Specular Intensity | 0 | Strength of specular highlights. Zero can also make Studio roughness comparisons invisible. | 0 to 1 |
| Specular Power | 32 | Concentrates the highlight as it increases; requires a visible specular contribution. | 1 to 100000 |
| Relief Response | 0 | Moves highlight orientation from its anchored response toward the relief's own tilt. | 0 to 1 |
| Specular Color | 1, 1, 1 | Tints highlights; requires Specular Intensity above zero. | Choose a color, enter #RRGGBB, or R, G, B in 0–1. |
| Independent Specular Light | Off | Allows highlights to use independent light angles. | On, Off |
| Specular Zenith | 60 | Aims the independent highlight; enable Independent Specular Light. | 0 to 359.99997 |
| Specular Direction | 135 | Aims the independent highlight; enable Independent Specular Light. | 0 to 359.99997 |
| Specular Anisotropy | 0 | Stretches highlights away from a round shape. | 0 to 1 |
| Anisotropy Angle | 0 | Rotates the anisotropic stretching direction. | 0 to 359.99997 |

### Relief Detail

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Macro Relief | 0 | Mixes a broader relief normal into the fine normal; 0 uses fine relief only. | 0 to 1 |
| Macro Radius | 8 | Neighborhood sampled for broad relief; requires Macro Relief. | 1 to 12 |
| Cavity Intensity | 0 | Darkens cavities to separate recessed details. | 0 to 1 |

### Lit / Shadow Tint

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Tint Intensity | 0 | Blends light-facing/shadow-facing tint into the surface. | 0 to 1 |
| Light-Facing Color | 1, 0.93, 0.82 | Color for the corresponding lit/shadow side; requires Tint Intensity. | Choose a color, enter #RRGGBB, or R, G, B in 0–1. |
| Shadow-Facing Color | 0.45, 0.55, 0.78 | Color for the corresponding lit/shadow side; requires Tint Intensity. | Choose a color, enter #RRGGBB, or R, G, B in 0–1. |
| Tint Blend | Multiply | Multiply tints RGB; OKLab Tint changes the perceptual color response. | Multiply, OKLab Tint |
| Tint Response | 1 | Shapes the lit/shadow tint transition; larger values favor its lit-side part. | 0.1 to 4 |
| Shadow Chroma | 1 | Scales shadow-side colorfulness in OKLab Tint mode. | 0 to 2 |

### Tone Mapping

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Slope Brightness | 1 | Multiplier for the slope contribution; 0 turns it off. | Brightness multiplier, 0 to 100; 0 turns it off. |
| Slope Gamma | 1 | Adjusts the slope's shadow/midtone curve; 1 preserves it. | Shadow curve, 0.01 to 10; 1 preserves the curve. |
| Highlight Knee | 0.75 | Sets where the highlight shoulder starts in the applicable linear-light blend. | 0 to 1 |
| Light Blend | Direct | Direct uses the direct composite; Linear RGB Add adds lighting in linear light. | Direct, Linear RGB Add |

### Rim Light

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Rim Intensity | 0 | Strength of edge/rim light; 0 disables it. | 0 to 1 |
| Rim Power | 3 | Concentrates the rim highlight as it increases. | 1 to 64 |
| Rim Color | 1, 1, 1 | Tints the rim; requires nonzero Rim Intensity. | Choose a color, enter #RRGGBB, or R, G, B in 0–1. |

### Gloss

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Gloss Intensity | 0 | Strength of relief-based gloss bands; 0 disables them. | 0 to 1 |
| Gloss Source | Fine Shading | Selects the relief coordinate driving gloss, independently of Palette Gloss. | Fine Shading, Shading, Relief Detail, Slope Facing |
| Gloss Relief | 8 | Normal gain used by Fine Shading gloss. | 0 to 16 |
| Gloss Bands | 2 | Number of bright bands across the gloss coordinate's range. | 1 to 32 |
| Gloss Sharpness | 6 | Narrows gloss bands as it increases. | 1 to 256 |
| Gloss Phase | 0.25 | Slides gloss bands along their coordinate without moving geometry. | 0 to 1 |
| Gloss Color | 1, 1, 1 | Colors relief-based gloss bands. | Choose a color, enter #RRGGBB, or R, G, B in 0–1. |

### Lustre Relief

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Lustre Relief | Off | Enables the alternate Lustre relief calculation and its controls. | On, Off |
| Auto Zoom Compensation | On | Compensates Lustre relief against Zoom Reference as zoom changes. | On, Off |
| Invert Relief | Off | Flips relief orientation, exchanging raised/recessed appearance. | On, Off |
| Lustre Depth | 1 | Strength of Lustre relief; requires Lustre Relief. | 0 to 16 |
| Normal Smoothing | 0 | Smooths relief normals to reduce harsh fine variation. | 0 to 1 |
| AO Radius | 8 | Neighborhood size of the cavity/ambient-occlusion response. | 1 to 64 |
| Boundary Reflection Guard | 0 | Suppresses unwanted reflections near iteration boundaries. | 0 to 1 |
| Relief Waves | 0 | Adds wave modulation to relief; 0 disables it. | 0 to 1 |
| Wave Frequency | 0.5 | Frequency of relief waves; requires Relief Waves above zero. | 0.03 to 1.5 |
| Zoom Reference | -1 | Reference zoom for compensation. Use Current Zoom binds it; -1 is unbound. | Saved reference zoom: 0 to 16777216, or -1 for an unbound reference. |

## Textures

These fields apply separately to Layers 1–4. Check Enabled, Opacity, mask, and layer visibility before judging a change.

### Layer 1 (Bottom)

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Enabled | Off | Enables this contribution while retaining its settings when off. | On, Off |
| Image File | (empty) | Image sampled by this texture. Keep the external file available when sharing settings. | Choose an image file. Enable the layer to display it. |
| UV Source | Cycle x Band | Screen aligns to the image; Cycle modes align U to iteration bands and V to angle, screen position, or band direction. | Cycle x Band, Cycle x Angle, Cycle x Screen, Screen |
| Blend Mode | Multiply | Multiply darkens through multiplication; Overlay combines contrast; Replace uses layer color according to opacity. | Multiply, Overlay, Replace |
| Opacity | 1 | Strength of the layer; 0 makes its other visual controls ineffective. | 0 to 1 |
| Texture Period | 0 | Iterations per tile; 0 follows the palette. Positive values give an independent period. | Iterations per tile; 0 follows the palette. |
| Size | 1 | Overall texture size; increasing it enlarges texture features. | 0.1 to 20 |
| Keep Aspect | On | Preserves source-image proportions. | On, Off |
| Repeat U | 1 | Repetition along U; higher values make features repeat more often. | 0 to 20 |
| Repeat V | 1 | Repetition along V; higher values make features repeat more often. | 0 to 20 |
| Palette Follow | 1 | Share of palette movement inherited by U: 1 follows, 0 stays independent, negative moves oppositely. | -2 to 2 |
| Scroll U | 0 | Signed U scrolling rate; compare different animation times. Zero stops this scroll. | -2 to 2 |
| Scroll V | 0 | Signed V scrolling rate; compare different animation times. Zero stops this scroll. | -2 to 2 |

## Patterns

These fields apply separately to Layers 1–4. Check Enabled, Opacity, mask, and layer visibility before judging a change.

### Layer 1 (Bottom)

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Enabled | Off | Enables this contribution while retaining its settings when off. | On, Off |
| Pattern | Stripes | Procedural shape; no image file is required. | Stripes, Checker, Grid, Dots, Diamond, Honeycomb, Waves, Cloud |
| Ink Mode | Palette Shift | Palette Shift samples another part of the palette; Solid Color uses Ink Color. | Palette Shift, Solid Color |
| Ink Color | 0, 0, 0 | Shape color in Solid Color mode. | Choose a color, enter #RRGGBB, or R, G, B in 0–1. |
| Palette Shift | 0.5 | Cycle offset for Palette Shift ink; 0.5 samples the opposite half of the cycle. | 0 to 1 |
| Sharpness | 0.5 | Higher values create harder shape boundaries; lower values soften them. | 0 to 1 |
| UV Source | Cycle x Band | Screen aligns to the image; Cycle modes align U to iteration bands and V to angle, screen position, or band direction. | Cycle x Band, Cycle x Angle, Cycle x Screen, Screen |
| Blend Mode | Replace | Multiply darkens through multiplication; Overlay combines contrast; Replace uses layer color according to opacity. | Multiply, Overlay, Replace |
| Opacity | 1 | Strength of the layer; 0 makes its other visual controls ineffective. | 0 to 1 |
| Pattern Period | 0 | Iterations per tile; 0 follows the palette. Positive values give an independent period. | Iterations per tile; 0 follows the palette. |
| Repeat U | 4 | Repetition along U; higher values make features repeat more often. | 0 to 200 |
| Repeat V | 4 | Repetition along V; higher values make features repeat more often. | 0 to 200 |
| Palette Follow | 1 | Share of palette movement inherited by U: 1 follows, 0 stays independent, negative moves oppositely. | -2 to 2 |
| Scroll U | 0 | Signed U scrolling rate; compare different animation times. Zero stops this scroll. | -2 to 2 |
| Scroll V | 0 | Signed V scrolling rate; compare different animation times. Zero stops this scroll. | -2 to 2 |
| Edge Enabled | Off | Adds an independent outline at the shape boundary. | On, Off |
| Edge Color | 1, 1, 1 | Outline color. | Choose a color, enter #RRGGBB, or R, G, B in 0–1. |
| Edge Width | 0.15 | Outline width; negative values can narrow the soft seam. | -0.5 to 1 |
| Edge Opacity | 1 | Outline strength; 0 hides it even when Edge Enabled is on. | 0 to 1 |
| Relative Edge Width | Off | Normalizes outline width to the shape's field range, changing comparisons across pattern types. | On, Off |

## Warp & Stripe

### Domain Warp

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Warp Enabled | Off | Enables this contribution while retaining its settings when off. | On, Off |
| Warp Source | Noise | Selects procedural Noise or a texture image as the displacement source. | Noise, Texture 1, Texture 2, Texture 3, Texture 4 |
| UV Source | Cycle x Band | Screen aligns to the image; Cycle modes align U to iteration bands and V to angle, screen position, or band direction. | Cycle x Band, Cycle x Angle, Cycle x Screen, Screen |
| Warp Amount | 0.25 | Strength of appearance-coordinate displacement; 0 leaves coordinates unchanged. | 0 to 2 |
| Warp Detail | 4 | Layers of noise detail; higher values can add GPU work. Applies to Noise. | 1 to 6 |
| Repeat U | 4 | Repetition along U; higher values make features repeat more often. | 0 to 200 |
| Repeat V | 4 | Repetition along V; higher values make features repeat more often. | 0 to 200 |
| Warp Period | 0 | Iterations per tile; 0 follows the palette. Positive values give an independent period. | Iterations per tile; 0 follows the palette. |
| Palette Follow | 1 | Share of palette movement inherited by U: 1 follows, 0 stays independent, negative moves oppositely. | -2 to 2 |
| Scroll U | 0 | Signed U scrolling rate; compare different animation times. Zero stops this scroll. | -2 to 2 |
| Scroll V | 0 | Signed V scrolling rate; compare different animation times. Zero stops this scroll. | -2 to 2 |

### Stripe

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Stripe Type | None | None disables stripes; other choices select the shape of iteration-based darkening. | None, Single Direction, Smooth, Squared |
| Interval 1 | 10 | Iteration spacing of the two stripe components; smaller values repeat more tightly. | Positive iteration interval. |
| Interval 2 | 50 | Iteration spacing of the two stripe components; smaller values repeat more tightly. | Positive iteration interval. |
| Opacity | 1 | Stripe darkening strength; requires a non-None type. | 0 to 1 |
| Offset | 0 | Signed displacement along the iteration pattern. | Signed stripe offset. |
| Animation Speed | 0 | Signed stripe-phase motion speed; 0 stops it. | Signed speed; 0 stops stripe motion. |

## Finishing

### Color Correction

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Gamma | 1 | 1 is neutral; above 1 raises midtones and below 1 lowers them. | Brightness curve. |
| Exposure | 0 | 0 is neutral; negative darkens and positive brightens. This grading control is not measured in stops. | 0 preserves exposure; negative values darken. |
| Hue | 0 | Hue rotation in turns; 0.25 is a quarter turn and 0 preserves hues. | Hue rotation from -1 to 1 turns; 0 keeps the palette hue. |
| Saturation | 0 | 0 preserves saturation; -1 makes grayscale; positive values increase saturation. | 0 keeps saturation; -1 is grayscale. |
| Brightness | 0 | Adds a channel offset; 0 preserves brightness. | Offset added to every color channel. |
| Contrast | 0 | Positive expands contrast about the midpoint; negative flattens it; 0 preserves it. | -1 to 0.999 |

### Bloom

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Threshold | 0 | Brightness threshold selecting the bloom source; higher values limit it to brighter areas. | 0 to 1 |
| Radius | 0 | Spatial spread of the halo; requires selected highlights and nonzero Intensity. | 0 to 1 |
| Softness | 0 | Mixes sharp thresholded glow toward blurred glow. At 1 the halo is fully blurred; lower values can retain hard brightness contours. | 0 to 1 |
| Intensity | 0 | Halo strength; 0 disables bloom. | Bloom strength; 0 disables the halo. |
| Linear Light | Off | Adds glow in linear light instead of the legacy encoded-color addition. | On, Off |

### Fog

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Radius | 0 | Radius of the broad blur mixed by fog. | 0 to 1 |
| Opacity | 0 | Strength of broad fog; 0 disables that contribution. | 0 to 1 |
| Center Start | 0 | Normalized radius where fog starts; larger values preserve a wider clear center. | 0 to 1 |
| Invert Falloff | Off | Reverses the falloff so the center receives fog and the exterior clears. | On, Off |
| Rim Mask | 0 | Restricts fog toward the rim-light footprint; needs a useful rim signal. | 0 to 1 |
| Rim Mask Boost | 20 | Amplifies the rim mask before saturation. | Mask multiplier, 1 to 100. |
| Rim Blur | 6 | Masked-rim blur radius, in pixels relative to 1280 image width. | Blur radius in pixels relative to 1280 width. |
| Blur Quality | Appearance | Speed caps relevant full-resolution blur at 16 texels; Appearance permits larger radii up to 4096 texels and denser sampling. Chaos Blur sampling changes even below the 16-texel cap. | Speed, Appearance |

### Focus Band

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Focus Amount | 0 | Strength of iteration-depth focus blur; 0 disables it. | 0 to 1 |
| Focus Depth | 0.5 | Sharp-band center as a fraction of maximum iteration count. | 0 to 1 |
| Focus Range | 0.25 | Half-width of the sharp band; larger values keep more depth sharp. | 0.01 to 1 |
| Focus Falloff | 1 | Transition out of the sharp band; larger values retain more sharpness. | 0.1 to 4 |
| Focus Blur | 10 | Maximum defocus blur radius, relative to 1280 image width. | Blur radius in pixels relative to 1280 width. |

### Chaos Blur

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Chaos Amount | 0 | Strength of blur on intricate iteration structure; 0 disables it. | Blurs locally intricate fractal regions. Zero disables it; smooth surfaces stay sharp. |
| Chaos Detail Scale | 1 | Detection scale; larger values include broader structure. | Detail detection radius in pixels at 1280 width. One targets fine structure; larger values include broader structure. |
| Chaos Threshold | 0.35 | Minimum selected irregularity; increasing it protects more smooth areas. | Minimum local irregularity to blur. Increase to protect more of the smooth surface. |
| Chaos Transition | 0.5 | Transition from sharp through partial to full irregularity blur. | Smooth transition from sharp through medium blur to full blur as local irregularity increases. |
| Chaos Feather | 6 | Softens the selected region boundary, relative to 1280 image width. | Softens the detected region boundary, in pixels at 1280 width. Detection follows the current zoom and location. |
| Chaos Blur Radius | 4 | Circular blur radius in the selected regions, relative to 1280 width. | Circular aperture radius in pixels at 1280 width. Scales with output size; Blur Quality controls sampling. |
| Chaos Highlight Detail | 0.2 | Restores some original bright detail over the blur; 0 gives pure blur. | Retains a little of the original bright detail over the lens blur. Zero gives a pure circular blur. |
| Chaos Shade | 0 | Darkens selected intricate/defocused regions. | Darkens intricate defocused regions to separate them from smooth foreground surfaces. |

## Animated Materials

These fields apply separately to Layers 1–4. Check Enabled, Opacity, mask, and layer visibility before judging a change.

### Layer 1 (Bottom)

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Enabled | Off | Enables this contribution while retaining its settings when off. | On, Off |
| Effect Type | Rain | Selects the material effect; controls behave according to its type. | Rain, Flame, Embers, Mist, Heat Haze, Ripples, Flow Light, Aurora |
| Blend | Screen | Normal composite, additive light, screen, or multiplication. | Normal, Add, Screen, Multiply |
| Mask | Surface | Restricts coverage to Surface, Exterior, No Surface, Bands, or Iteration Edges. | Surface, Exterior, No Surface, Bands, Iteration Edges |
| Rain Shape | Raindrops | Raindrops versus Water Streaks; used by Rain. | Raindrops, Water Streaks |
| Drop Size | 1 | Raindrop size in the drop-shaped Rain mode. | 0.1 to 8 |
| Opacity | 0.65 | Strength of the layer; 0 makes its other visual controls ineffective. | 0 to 1 |
| Surface Scale | 1 | Spatial scale of the procedural surface. | 0.1 to 100 |
| Density | 0.45 | Feature density; effect-specific. | 0 to 1 |
| Shape Length | 0.65 | Feature elongation and width; effect-specific. | 0.05 to 2 |
| Shape Width | 1 | Feature elongation and width; effect-specific. | 0.1 to 10 |
| Surface Depth | 0.3 | Depth/distortion of the procedural surface response. | 0 to 4 |
| Glow | 0.25 | Strength of glow where the selected effect uses it. | 0 to 4 |
| Sync Color Animation | Off | Links effect movement to color animation. | On, Off |
| Speed | 1 | Signed movement speed; evaluate at two different times. | -10 to 10 |
| Evolution | 0.4 | Rate/direction of internal pattern evolution. | -10 to 10 |
| Flow Bend | 12 | Bends the procedural flow. | -180 to 180 |
| Seed | 1 | Changes the deterministic arrangement without moving the fractal. | 0 to 65535 |
| Band Period | 80 | Iteration interval for band-related structure/masking. | 1 to 1e+06 |
| Primary Color | 0.64, 0.79, 1, 1 | The two colors used by the selected material effect. | Choose a color, enter #RRGGBB, or R, G, B, A in 0–1. |
| Secondary Color | 1, 0.25, 0.025, 1 | The two colors used by the selected material effect. | Choose a color, enter #RRGGBB, or R, G, B, A in 0–1. |

## Palette

### Color Cycle

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Cycle Length (R) | 64 | Iterations per channel cycle; halving all three doubles repetition in iteration space. | 1 to 1e18 iterations per channel cycle. |
| Cycle Length (G) | 64 | Iterations per channel cycle; halving all three doubles repetition in iteration space. | 1 to 1e18 iterations per channel cycle. |
| Cycle Length (B) | 64 | Iterations per channel cycle; halving all three doubles repetition in iteration space. | 1 to 1e18 iterations per channel cycle. |
| Iteration Coloring | Linear | Curve applied before palette cycling; changes color-band spacing across depth. | Linear, Square root, Cube root, Log, LogLog, Smoothstep, Smootherstep |
| Color Smoothing | Normal | None makes discrete steps; Normal/Reversed choose within-step interpolation direction. | None, Normal, Reversed |
| Color Interpolation | RGB | Blend space between entries: encoded RGB, perceptual OKLab, or Linear RGB. | RGB, OKLab, Linear RGB |
| Start Offset | 0 | Initial phase in cycles; moves colors without moving geometry. | 0 to 1 |
| Cycle Bias | 1 | Redistributes color within a cycle; 1 is uniform. | 0.1 to 4 |
| Cycle Curve | Power | Power uses a power-law bias; Wave uses periodic bias with bounded band-width variation. | Power, Wave |
| Seamless (Mirror) | Off | Mirrors palette repetition to change seam behavior. | On, Off |
| Palette Gloss | Off | Adds cycle-linked highlights, separately from relief-based Gloss. | On, Off |
| Gloss Color | 1, 1, 1, 1 | Color of Palette Gloss; requires that contribution to be enabled. | Choose a color, enter #RRGGBB, or R, G, B, A in 0–1. |
| Mandelbrot Color | 0, 0, 0, 1 | Interior/iteration-limit color; invisible in an entirely exterior view. | Choose a color, enter #RRGGBB, or R, G, B, A in 0–1. |

### Band Line

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Band Lines | Off | Adds lines at regular positions in the palette cycle. | On, Off |
| Lines per Cycle | 16 | Line boundaries per complete palette cycle. | 1 to 256 |
| Line Width | 0.03 | Line thickness as a fraction of one band. | 0 to 1 |
| Line Opacity | 1 | Visibility of band lines; 0 hides them. | 0 to 1 |
| Line Softness | 0 | Edge softness: 0 crisp, 1 fully softened swell. | 0 to 1 |
| Line Color | 0, 0, 0, 1 | Band-line color. | Choose a color, enter #RRGGBB, or R, G, B, A in 0–1. |
| Recessed Grooves | Off | Turns band lines into recessed relief grooves. | On, Off |
| Auto Groove | On | Derives groove shape automatically from the band-line setup. | On, Off |
| Groove Depth | 1.5 | Depth of the recessed groove response. | 0 to 5 |
| Groove Width | 0.05 | Width of the manual recessed groove response. | 0 to 0.5 |
| Spine Amount | 0 | Strength of spine decoration; 0 disables it. | Uses Band Line color, width and opacity. Independent of Studio and materials. |
| Spine Length | 1 | Length of spine decoration. | Uses Band Line color, width and opacity. Independent of Studio and materials. |
| Spine Density | 1 | Repetition density of spine decoration. | Uses Band Line color, width and opacity. Independent of Studio and materials. |
| Branch Amount | 1 | Branching within spine decoration. | Uses Band Line color, width and opacity. Independent of Studio and materials. |
| Ornament Amount | 0 | Amount of ornamental band-line detail; 0 disables it. | Uses Band Line color, width and opacity. Independent of Studio and materials. |
| Ornament Size | 1 | Size of ornamental detail. | Uses Band Line color, width and opacity. Independent of Studio and materials. |
| Ornament Density | 1 | Repetition density of ornaments. | Uses Band Line color, width and opacity. Independent of Studio and materials. |
| Ornament Inset | 0.032 | Placement inset of ornaments relative to the line. | Uses Band Line color, width and opacity. Independent of Studio and materials. |
| Line Glow | 0 | Strength of band-line glow. | 0 to 4 |
| Glow Width | 2 | Spread of band-line glow. | 0.1 to 32 |
| Glow Color | 1, 1, 1, 1 | Color of band-line glow. | Choose a color, enter #RRGGBB, or R, G, B, A in 0–1. |

### Color Stops

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Stop Easing | 1 | Interpolation shape between explicit color stops; see the numeric choices in the panel. | 0 Linear, 1 Smoothstep, 2 Smootherstep. |
| Selected Stop | 0 | Selects an existing stop starting at 1. Apply selection before changing its color/position. | Apply the selection before editing its color or position. |
| Stop Position | 0 | Position in the cycle from 0 up to, but excluding, 1; stops remain ordered. | Ordered position from 0 up to, but excluding, 1. |
| Stop Color | 0, 0, 0, 1 | Color of the selected stop. | #RRGGBB or R, G, B, A in 0–1. |

### Preset Library

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Palette Preset | LongRandom64 [Recommend] | Complete palette recipe; applying it can replace more than the color array. | Palette library; Apply Palette Preset applies the chosen recipe. |
| Seed | 1 | Unsigned seed for repeatable palette generation; same preset and seed regenerate the same recipe. | Whole number from 0 to 4294967295 for repeatable generation. |

### Import Colors

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Color Settings File | (empty) | External RFC, RFSP, KFR, or KFP supplying colors. Read first, then apply. | RFC, RFSP, KFR or KFP. Read first, then apply. |
| Include Color Correction | Off | Also imports Gamma, Exposure, Hue, Saturation, Brightness, and Contrast. | Off, On |

## Render & Preview

### Performance

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Rendering FPS | 60 | Live display limit, independent of video export rate. Lower values can reduce preview GPU load. | Live rendering limit: 1 to 1000 frames per second. Lower values reduce rendering load. Video export has a separate FPS setting. |
| Calculation Threads | 16 | CPU calculation workers, limited to logical cores; applies to the next calculation. | 1 up to this computer's logical core count. Applied to the next calculation. |

### Calculation Preview

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Boundary Trace Fill | Off | Fills qualifying regions through boundary tracing; bypassed in Absolute Iteration Mode. | On, Off |
| Two-Color Preview | Off | Two-color calculation preview; return to ordinary coloring to judge appearance edits. | On, Off |
| Coarse Preview | On | Shows a coarse first result while the full calculation proceeds. | On, Off |

## Explore

### Location

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Real | -0.849999999999999991673 | High-precision center coordinates; preserve all digits for deep locations. | High-precision center coordinate. |
| Imaginary | 0 | High-precision center coordinates; preserve all digits for deep locations. | High-precision center coordinate. |
| Log Zoom (e) | 2 | Base-10 scale in the renderer: +1 means 10 times magnification at fixed canvas size. The current hint incorrectly calls this natural-log. | 0 to 16777216; base-10 scale in the renderer. |
| Rotation | 0 | Calculated view rotation in degrees; also panorama yaw. Requires recalculation. | Degrees. Also controls panorama yaw. |

### Iterations

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Automatic Iterations | On | Derives the Mandelbrot iteration ceiling from period times Auto Iteration Multiplier; period detection stays automatic. | O, X |
| Max Iteration | 300 | Manual escape-iteration ceiling when Automatic Iterations is off. | Used when Automatic Iterations is off. |
| Auto Iteration Multiplier | 150 | Multiplier of detected period; raises the ceiling for difficult pixels. | Used when Automatic Iterations is on. |
| Bailout | 1e+30 | Escape radius, affecting termination and smooth iteration evaluation; separate from MPA tolerance. | Escape radius, 2 to 1e38. |
| Decimalize Iteration | LogLog | Fractional iteration method. The normal quadratic escape-potential path gives the same smoothing for exposed non-None choices. | Linear, Square root, Log, LogLog |
| Absolute Iteration Mode | Off | Absolute iteration reporting; bypasses Boundary Trace Fill. | O, X |

### Reference

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Reuse Reference | Disabled | Disabled creates an independent reference; Current/Centered select reuse where a compatible reference exists. | Current, Centered, Disabled |
| Compression Criteria | 0 | Repeated-reference batch threshold; 0 disables reference compression. | Minimum reference batch; 0 disables compression. |
| Compression Threshold | 0 | Positive N sets relative threshold 10^-N; higher is stricter. Zero gives zero tolerance; explicitly disable with Compression Criteria 0. | 0 to 255; see reference-compression discussion in the main guide. |
| Disable Normalization | Off | Disables reference-compressor normalization; advanced storage tuning. | O, X |

### MP-Approximation

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Min Skip Reference | 4 | Minimum usable MPA skip; above the longest period can leave the table empty. | MPA follows periodic structure; minimum 4. |
| Max Multiplier Between Levels | 2 | Spacing of intermediate MPA period levels; affects table/search structure. | Ratio between adjacent period levels, 1 to 255. |
| Precision Level | -3 | MPA epsilon exponent: -5 means 10^-5. More negative is stricter. | -15 to -3. Lower values favor accuracy. |
| Selection Method | Highest | Searches period levels from the low or high end; performance depends on periodic structure. | Lowest, Highest |
| Compression Method | No compression | MPA table packing; separate from reference compression and disk map compression. | No compression, Little compression, Strongest |

### Formula

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Formula Type | Mandelbrot | Mandelbrot versus Custom; switching normally resets the view. | Mandelbrot, Custom |
| Custom Formula | z^3+c | Expression in Custom mode; uses manual iterations and a much shallower useful zoom range than Mandelbrot. | Examples: z^3+c; conj(z)^2+c. Custom formulas use manual iterations. |

### Projection

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Projection | Planar | Planar, 360-degree equirectangular, or perspective 360-degree camera calculation. | Planar, 360° Equirectangular, 360° Camera |
| Layout | Ground and Sky | Ground and sky versus full-sphere panorama mapping. | Ground and Sky, Full Sphere |
| Pitch | -90 | Vertical aim of the perspective panorama camera. | -90 to 90 degrees. Used by the 360 Camera. |
| Field of View | 100 | Horizontal angle captured by the perspective panorama camera. | 1 to 179 degrees. Used by the 360 Camera. |
| Panorama Range | 3 | Base-10 radius limit relative to the panorama horizon radius. | Furthest radius, log10 scale: 0 to 6. |

## Animation

### Color Motion

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Color Animation Speed | 0 | Palette drift in iterations/second; negative reverses it and 0 stops this component. | Palette iterations per second. Negative reverses direction; 0 stops drift. |
| Animation Mode | Linear | Motion style. Changing mode can also set suitable flow defaults. | Linear, Breathing, Turbulence, Psychedelic |
| Flow Amount | 80 | Nonlinear color displacement in iterations; unused by Linear. | Displacement of the color cycle in iterations; 0 or greater. |
| Flow Scale | 3 | Spatial density of fluid bands in applicable motion modes. | Density of fluid bands, 0 to 12. |
| Flow Speed | 0.5 | Rate and direction of flow evolution. | -2 to 2. Negative values reverse the flow. |
| Swirl | 0.4 | Twist about the center in Psychedelic mode; retained but unused by other modes. | Twist around the center, -2 to 2. Used by Psychedelic motion. |
| Color Smoothing | Normal | Color-step smoothing; animation shape edits enable Normal if previously None. | None, Normal, Reversed |
| Palette Start Offset | 0 | Initial palette phase in cycles. | Starting position in the palette cycle, 0 to 1. |

### Frozen Colors

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Freeze Match Tolerance | 0.02 | Fraction of a cycle matched to frozen colors; larger freezes a wider range. | Fraction of one color cycle, 0 to 1. Lower values freeze a narrower band. |
| Frozen Iteration Values | (empty) | Up to 16 iteration values whose colors are held still; empty clears them. | Up to 16 comma-separated iteration values. Empty removes all frozen colors. |

### Zoom Motion

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Zoom Speed | 1 | Saved keyframes per second; greater values traverse faster. Use holds for pauses. | Keyframes per second; greater than 0. |
| Extra Final Zoom-in | 2 | Extra zoom-in beyond the ordinary final saved stage. | Additional zoom at the end of the video; 0 to 8. |
| Show Zoom Ratio | On | Displays the zoom readout in preview and video. | On, Off |

### Timeline

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Timeline Enabled | On | Applies saved tracks without deleting them when off. | On, Off |
| Estimated Keyframes | 100 | Length estimate before folder loading; export counts actual files. | Preview length before loading keyframes: 1 to 100000. |

### Keyframe Generation

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Zoom Step per Keyframe | 2 | Magnification ratio of neighboring maps; 2 means 2 times. Its log10 becomes the stored log-zoom increment. | Greater than 1; a magnification ratio. |
| Rotation / 360 Padding | Off | Generates larger planar maps for rotation/panorama; requires regenerating keyframes. | On, Off |
| Camera Padding Scale | 4 | Linear padding factor; doubling quadruples source pixels and associated storage. | 2 to 64. Memory and disk use grow with the square of this value. |
| Render from PNG Images | Off | Uses already-colored PNGs; iteration-dependent effects require maps instead. | On, Off |

### Camera

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Rotation Mode | Keyframes | Keyframes follows rotation keys; Constant Period replaces them with continuous rotation, even during zoom holds. | Keyframes, Constant Period |
| Seconds per Turn | 10 | Seconds per turn in Constant Period mode; larger means slower. | Duration of one complete turn, 0.01 to 86400 seconds. Used by Constant Period. |
| Rotation Direction | Clockwise | Direction of continuous rotation. | Clockwise, Counterclockwise |
| Rotation Start Angle | 0 | Angle at video time zero for continuous rotation. | Angle at video time zero, in degrees. Used by Constant Period. |
| Camera Rotation | 0 | Rotates the saved source view; values beyond 360 allow multiple turns. | Degrees, -360000 to 360000. Values beyond 360 allow multiple turns. |
| Camera Projection | Planar | Playback/export projection; needs sufficient saved source coverage. | Planar, 360° Equirectangular, 360° Camera |
| Camera Pitch | -90 | Vertical aim in the perspective video camera. | -90 to 90 degrees. |
| Camera Field of View | 100 | Horizontal field of view of the perspective video camera. | 1 to 179 degrees. |
| Camera Panorama Range | 0 | Log10 radius limit; detail remains limited by saved source coverage. | Log10 radius limit, 0 to 6. Available detail depends on saved padding. |
| Camera Layout | Ground and Sky | Ground and Sky versus Full Sphere video mapping. | Ground and Sky, Full Sphere |

## Zoom Overlay

### Appearance

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Show Zoom Ratio | On | Show the zoom ratio in preview and exported video. | On, Off |
| Decimal Places | 6 | Digits after the decimal point in the zoom ratio: 0 to 9. Default: 6. | See effect/dependency column. |
| Custom Appearance | Off | Off preserves the legacy font and placement. Editing a style enables Custom Appearance. | On, Off |
| Alignment | 0 | Choose an anchor and a position with a two-percent margin. | See effect/dependency column. |
| Position X (%) | 2 | Horizontal anchor position, 0 to 100 percent of the video width. | See effect/dependency column. |
| Position Y (%) | 2 | Vertical anchor position, 0 to 100 percent of the video height. | See effect/dependency column. |
| Font | Segoe UI | Installed font family. Use Choose Font to browse available fonts. | See effect/dependency column. |
| Font Style | 0 | Regular, bold, italic, or bold italic. | See effect/dependency column. |
| Font Size (% of height) | 3 | 0.5 to 20 percent of video height. Independent of window size and monitor DPI. | See effect/dependency column. |
| Text Color | 1, 1, 1, 1 | Choose a color, enter #RRGGBBAA, #RRGGBB, or R, G, B, A in 0–1. | See effect/dependency column. |
| Text Color Opacity (%) | 100 | 0 is transparent; 100 is opaque. | See effect/dependency column. |
| Enable Outline | On | Draw a continuous border around each glyph. | On, Off |
| Outline Width (% of font) | 6 | Outward glyph border, 0 to 25 percent of the font size. | See effect/dependency column. |
| Outline Color | 0, 0, 0, 1 | Choose a color, enter #RRGGBBAA, #RRGGBB, or R, G, B, A in 0–1. | See effect/dependency column. |
| Outline Color Opacity (%) | 100 | 0 is transparent; 100 is opaque. | See effect/dependency column. |
| Enable Shadow | Off | Draw an offset shadow independently of the outline. | On, Off |
| Shadow X (% of font) | 8 | -100 to 100 percent of the font size. | See effect/dependency column. |
| Shadow Y (% of font) | 8 | -100 to 100 percent of the font size. | See effect/dependency column. |
| Shadow Color | 0, 0, 0, 1 | Choose a color, enter #RRGGBBAA, #RRGGBB, or R, G, B, A in 0–1. | See effect/dependency column. |
| Shadow Color Opacity (%) | 100 | 0 is transparent; 100 is opaque. | See effect/dependency column. |

## Surface Effects

Studio enables Studio materials. Base Style selects a recipe; related parameter edits can activate corresponding mixes. Complete Replacement bypasses legacy compositing controls. Hidden contributions in the ordered stack retain saved values.

| Control | Effect |
| --- | --- |
| Studio | Enables the Studio material response. |
| Base Style | Original, Liquid Metal, Cyber Sigilism, PHONK, Black Metal, Deep Sea, or Ukiyo-e; applies a coordinated recipe. |
| Style Application | Legacy Compositing uses existing blends; Complete Replacement replaces the palette with the style. |

### Relief & Lighting

| Setting | Fresh default | Effect | Range |
| --- | --- | --- | --- |
| Shading Depth | 0 | Amplifies the relief gradient. High values can saturate the slope, making further increases look similar. | 0 to 10000 |
| Opacity | 1 | Blends slope shading into the underlying color; 0 removes its ordinary contribution. | 0 to 1 |
| Shadow Floor | 0 | Raises the minimum diffuse illumination on the shadow side. | 0 to 1 |
| Light Zenith | 60 | Changes the main light's elevation in degrees. | 0 up to, but excluding, 360 degrees |
| Light Direction | 135 | Rotates the main light around the relief in degrees. | 0 up to, but excluding, 360 degrees |
| Boundary Reflection Guard | 0 | Suppresses reflection near iteration boundaries. | 0 to 1 |
| Lustre Depth | 1 | Lustre relief depth; also available in Lighting & Relief. | 0 to 16 |
| Normal Smoothing | 0 | Relief normal smoothing; also available in Lighting & Relief. | 0 to 1 |
| AO Radius | 8 | Neighborhood radius for cavity shading. | 1 to 64 |
| Relief Waves | 0 | Relief-wave modulation strength. | 0 to 1 |
| Wave Frequency | 0.5 | Frequency of relief-wave modulation. | 0.03 to 1.5 |

### Material & Reflection

| Setting | Fresh default | Effect | Range |
| --- | --- | --- | --- |
| Roughness | 0.32 | Spreads Studio reflections; needs Studio and visible specular/reflection contribution. | 0.04 to 1 |
| Metalness | 0.6 | Blends toward a metallic Studio response. | 0 to 1 |
| Index of Refraction | 1.5 | Dielectric index of refraction, controlling reflection strength. | 1 to 3 |
| Direct Light | 0.8 | Strength of direct Studio illumination. | 0 to 8 |
| Studio Reflections | 0.7 | Strength of Studio environment reflections. | 0 to 8 |
| Clearcoat | 0.3 | Strength of an additional clear coating. | 0 to 1 |
| Coat Roughness | 0.15 | Roughness of the coating; requires Clearcoat above zero. | 0.04 to 1 |
| Environment Rotation | 0 | Rotates the Studio environment independently. | 0 up to, but excluding, 360 degrees |
| Follow Light Direction | 1 | How much the environment follows Light Direction: 0 independent, 1 full follow. | 0 to 1 |
| Specular Antialiasing | 0 | Reduces harsh specular aliasing by adjusting highlight response. | 0 to 1 |
| Reflection Detail | 1 | Detail/scale of reflection structure. | 0.25 to 4 |
| Reflection Contrast | 1 | Contrast within reflections. | 0.25 to 3 |
| Reflection Brightness | 1 | Reflection brightness, separate from final Color Correction. | 0 to 3 |
| Reflection Curvature | 1 | Curvature/shape of the reflection response. | 0.1 to 3 |

### Color & Palette

| Setting | Fresh default | Effect | Range |
| --- | --- | --- | --- |
| Chrome Strength | 1 | Chrome/surface contribution in legacy compositing. | 0 to 1 |
| Surface Phase | 0 | Phase offset of surface/reflection structure. | 0 to 1 |
| Sigil Background | 4.5 | Brightness of the sigil background. | 0 to 8 |
| Palette Color | 0 | Existing palette color mixed into the relevant style. | 0 to 1 |
| Surface Color Amount | 0 | Strength of Surface Color tint. | 0 to 1 |
| Reflection Color Amount | 0 | Strength of Reflection Color tint. | 0 to 1 |
| Background Color Amount | 0 | Strength of Background Color tint. | 0 to 1 |
| Preserve Palette Ink | 1 | Preserves palette dark ink in sigil styling. | 0 to 1 |
| Surface Color | See color picker | Surface tint, controlled by Surface Color Amount. | Color channels 0–1 |
| Reflection Color | See color picker | Reflection tint, controlled by Reflection Color Amount. | Color channels 0–1 |
| Background Color | See color picker | Background tint, controlled by Background Color Amount. | Color channels 0–1 |
| Detail Rim Color | See color picker | Color of detail/band rim glow. | Color channels 0–1 |

### Thin Film & Glints

| Setting | Fresh default | Effect | Range |
| --- | --- | --- | --- |
| Thin Film Color | 0.65 | Strength of thin-film color in the surface style. | 0 to 1 |
| Iridescence | 0 | Strength of angle-dependent iridescent color. | 0 to 1 |
| Film Thickness (nm) | 400 | Film thickness in nanometers; changes iridescent hues rather than only brightness. | 0 to 2000 |
| Rainbow Width | 1 | Width of rainbow/prismatic regions. | 0.1 to 3 |
| Rainbow Spread | 1 | Spread of prismatic color. | 0 to 1 |
| Film Hue | 0 | Hue shift of thin-film color in cycle units. | 0 to 1 |
| Glint Strength | 1 | Strength of glint highlights. | 0 to 3 |
| Glint Size | 1 | Size of glint highlights. | 0.25 to 4 |

### Flames & Frost

| Setting | Fresh default | Effect | Range |
| --- | --- | --- | --- |
| Shadow Crush | 0.72 | Pushes dark regions deeper into shadow. | 0 to 1 |
| Chrome Flames | 1 | Strength of chrome-flame structure in the relevant surface mix. | 0 to 3 |
| Red / Purple | 0.85 | Balance of red versus purple in PHONK. | 0 to 1 |
| Frost Strength | 1 | Strength of Black Metal frost. | 0 to 3 |
| Frost Threshold | 0.45 | Threshold selecting frost regions. | 0 to 1 |
| PHONK Surface Damage | 0 | PHONK surface damage amount. | 0 to 1 |

### Noise & VHS

| Setting | Fresh default | Effect | Range |
| --- | --- | --- | --- |
| VHS Damage | 0.45 | VHS-like disturbance in the finishing contribution. | 0 to 1 |
| Chromatic Shift | 1.5 | Color-channel displacement in VHS finishing. | 0 to 8 |
| Nearest Mix | 0.2 | Amount of nearest/block-like sampling in the finish. | 0 to 1 |
| Nearest Block Size | 4 | Nearest-sampling block size; requires Nearest Mix. | 1 to 16 |
| Film Grain | 0.3 | Grain in the applicable noise/monochrome finish. | 0 to 1 |
| Grunge Scale | 1 | Spatial scale of grain/grunge. | 0.25 to 4 |
| Black Metal Monochrome | 1 | Black Metal monochrome strength. | 0 to 1 |
| VHS / Nearest Effects | 0 | VHS/nearest finishing contribution. | 0 to 1 |
| Monochrome / Grain Effects | 0 | Monochrome/grain finishing contribution. | 0 to 1 |

### Emission & Detail

| Setting | Fresh default | Effect | Range |
| --- | --- | --- | --- |
| Surface Rim Strength | 0 | Detail/band rim glow strength; also exposed with Band Line. | 0 to 4 |
| Surface Rim Width | 2 | Width of detail/band rim glow. | 0.1 to 32 |
| Detail Light | 1 | Brightness of fine surface detail. | 0 to 2 |
| Dense Detail Suppression | 0 | Suppresses excessively bright dense detail. | 0 to 1 |
| Detail Density Threshold | 0.08 | Density threshold used for detail suppression. | 0.005 to 0.4 |
| Sea Glow | 1.6 | Deep Sea emission strength. | 0 to 4 |
| Sea Glow Threshold | 0.3 | Threshold selecting Deep Sea glow regions. | 0 to 1 |
| Sea Body Light | 0.15 | Light on the body beneath Deep Sea emission. | 0 to 1 |
| Sea Particles | 0.5 | Amount of luminous sea particles. | 0 to 1 |
| Sea Particle Size | 1 | Size of sea particles. | 0.25 to 3 |
| Sea Color Balance | 0.55 | Balance of Deep Sea colors. | 0 to 1 |
| Body Color | See color picker | Deep Sea body color. | Color channels 0–1 |
| Emission Color A | See color picker | The two Deep Sea emission colors. | Color channels 0–1 |
| Emission Color B | See color picker | The two Deep Sea emission colors. | Color channels 0–1 |

### Ink & Quantization

| Setting | Fresh default | Effect | Range |
| --- | --- | --- | --- |
| Print Color Count | 5 | Print-quantization color count, 3 to 6. | 3 to 6 |
| Wave Foam | 0.75 | Amount of pale foam-like print detail. | 0 to 1 |
| Woodcut Grain | 0.15 | Woodcut-like grain amount. | 0 to 1 |
| Print Flatness | 1 | Strength of the flat print/quantized finish. | 0 to 1 |
| Print Tone Balance | 0.55 | Balance of light and dark print tones. | 0 to 1 |
| Color Quantization | 0 | Print color quantization independently of Base Style. | 0 to 1 |
| Ink | See color picker | One of the six print colors; contribution depends on print color count and tone selection. | Color channels 0–1 |
| Indigo | See color picker | One of the six print colors; contribution depends on print color count and tone selection. | Color channels 0–1 |
| Asagi | See color picker | One of the six print colors; contribution depends on print color count and tone selection. | Color channels 0–1 |
| Pale Blue | See color picker | One of the six print colors; contribution depends on print color count and tone selection. | Color channels 0–1 |
| Foam | See color picker | One of the six print colors; contribution depends on print color count and tone selection. | Color channels 0–1 |
| Paper | See color picker | One of the six print colors; contribution depends on print color count and tone selection. | Color channels 0–1 |

### Effect Amounts

| Setting | Fresh default | Effect | Range |
| --- | --- | --- | --- |
| Liquid Metal Mix | 0 | Liquid Metal mixed into the surface. | 0 to 1 |
| Cyber Sigilism Mix | 0 | Cyber Sigilism mixed into the surface. | 0 to 1 |
| PHONK Mix | 0 | PHONK mixed into the surface. | 0 to 1 |
| Black Metal Mix | 0 | Black Metal mixed into the surface. | 0 to 1 |
| Deep Sea Light | 0 | Deep Sea light mixed into the surface. | 0 to 1 |
| Ukiyo-e Wave Mix | 0 | Ukiyo-e waves mixed into the surface. | 0 to 1 |

## Surface editor display names

The Surface inspector deliberately shortens some labels. These are alternate names for the same setting, not extra effects. Band-line glow fields are routed to Palette rather than displayed as independent Surface controls.

| Display name | Field name used above | Meaning / location |
| --- | --- | --- |
| Emission | Sea Glow | Deep Sea emission strength. |
| Detail Brightness | Detail Light | Fine-detail light contribution. |
| Density Threshold | Detail Density Threshold | Selection threshold for dense-detail suppression. |
| Emission Threshold | Sea Glow Threshold | Selection threshold for sea emission. |
| Body Light | Sea Body Light | Light on the sea body. |
| Particle Amount | Sea Particles | Luminous-particle amount. |
| Particle Size | Sea Particle Size | Luminous-particle size. |
| Emission Color Balance | Sea Color Balance | Balance between the emission colors. |
| Iridescence | Thin Film Color | Strength of the style's thin-film color (`filmStrength`). |
| Prism Width | Rainbow Width | Width of the prismatic regions. |
| Prism Spread | Rainbow Spread | Spread of prismatic color. |
| Color Count | Print Color Count | Number of print colors, 3–6. |
| Flatness | Print Flatness | Strength of the flattened print appearance. |
| VHS Noise | VHS Damage | VHS disturbance. |
| Pixel Size | Nearest Block Size | Block size for nearest-sampling finish. |
| Palette Amount | Palette Color | Palette contribution to the surface style. |
| Detail Rim Light / Line Glow | Surface Rim Strength | Shared band-line glow strength; edit Line Glow in Palette. |
| Rim Width / Glow Width | Surface Rim Width | Shared band-line glow width; edit Glow Width in Palette. |
| Glow Color | Detail Rim Color | Shared band-line glow color; edit Glow Color in Palette. |

**Two fields can be called Iridescence:** the Surface inspector renames **Thin Film Color** (`filmStrength`) to Iridescence, while the advanced registry also contains a distinct **Iridescence** (`iridescence`) field associated with Film Thickness. Do not assume the two entries are duplicate controls. Other unchanged names, such as Film Hue and Glint Size, already match the tables above.

## HDR and tone mapping

These controls are available through Shader → HDR / the **HDR & Tone Mapping** output group. Defaults below are the declared defaults for these fields; a loaded file or preset can replace them.

| Setting | Default | Effect | Choices / range |
| --- | --- | --- | --- |
| HDR Rendering | Off | Retains above-white light for the HDR rendering path. Required for HDR video transfer. | On, Off |
| Tone Mapping | ACES (Narkowicz fit) | Chooses the display curve for SDR preview/image output; False Color is diagnostic. | Clip, Reinhard, ACES (Narkowicz fit), Filmic, MFR Shoulder, MFR Log View, MFR Linear Clip, MFR False Color |
| Exposure | 0 | Linear-light exposure in stops; +1 doubles light. Distinct from Color Correction Exposure. | −20 to 20 |
| Highlight Headroom | 2 | Highlight range for applicable conventional tone-mapping modes; ignored by MFR SDR display modes. | 1 to 64 |
| MFR Mastering Peak (nits) | 4000 | MFR display metadata/curve control, with reference white 203 nits; does not change source light. | 100 to 10000 |
| Video Transfer | SDR | Chooses exported signal transfer, independently of the SDR preview curve. | SDR, HDR10 (PQ), HLG |
| HDR Peak Brightness (nits) | 1000 | Peak brightness for PQ export. Separate from MFR Mastering Peak. | 100 to 10000 |

[Curve overview](shader-overview.md#hdr-and-tone-mapping) · [Detailed MFR behavior](../docs/mfr-hdr-preview.md)

The ACES choice is a curve approximation, not a full ACES transform; see the attribution in [NOTICE](../NOTICE).

## Shader layer controls

| Control / action | Default or behavior | What it changes |
| --- | --- | --- |
| Custom Layer Order | Off | On uses the saved ordered stack. Off restores the original rendering path while retaining its custom arrangement. |
| Selected Layer | Band Line in the generic form | Chooses which contribution the ordering actions edit; selection itself does not change its pixels. |
| Open Layer List | Action | Opens the layer list; the workspace lists currently active contributions. |
| Move Up / Move Down | Action | Moves a contribution later/earlier in compositing. Higher displayed layers are applied later. |
| Bring to Front / Send to Back | Action | Moves the contribution to the last/first compositing position. |
| Drag a layer | Action | Reorders that contribution. Moving enables Custom Layer Order. |
| Layer visibility | Initially visible | Hides/shows a contribution without discarding its settings; changing visibility enables custom order. |
| Restore Original Order | Action | Resets the order and visibility and turns Custom Layer Order off. |
| Undo / Redo | Action | Restores recorded layer-order/visibility edits through edit history. |

[All 29 layer slots and their purposes](shader-overview.md#shader-layers). Visibility alone does not activate a disabled effect or overcome zero opacity. Warp and palette-coordinate mapping are not reorderable color layers.

## Palette and layer actions

| Action | Result |
| --- | --- |
| Create 8 Stops (Lossy) | Resamples and replaces the palette representation with eight editable stops. |
| Add Stop / Remove Selected Stop | Inserts a stop or removes the selected one in the stop editor. |
| Equal Spacing | Redistributes the stop positions uniformly. |
| Reverse Colors | Reverses the stop colors. |
| Apply Palette Preset | Applies the selected complete palette recipe and seed. |
| Read Color File / Apply Imported Colors | Reads the chosen color source, then applies it with optional Color Correction. |
| Use Current Zoom | Binds Lustre's Zoom Reference to the current fractal zoom. |
| Reset Layer | Restores the selected Texture, Pattern, or Animated Materials layer to its defaults. |
| Copy to Next Layer | Overwrites the next layer with the current layer's settings. |
| Move Toward Bottom / Move Toward Top | Swaps neighboring layer slots; available directions depend on the selected slot. |
| Save Shader Preset / Load Shader Preset | Saves/restores appearance settings in `.rfsp`; full location/configuration uses `.rfc`. |
| Surface: Reset selected value/color | Restores the selected control's default. |
| Surface: Reset effect | Restores the selected effect category's controls. |
| Clear added effects (keep base style) | Clears additional surface effect contributions while keeping the base style. |
| Reset all appearance (includes palette) | Resets the appearance including palette settings; broader than resetting one surface control. |

## Export and timeline controls

The detailed manual covers the remaining controls in context. The [actual UI gallery](ui-gallery.md#export) also lists every current Export form control and its input guidance.

- [Detailed animation, timeline, audio, and export workflows](animation-and-export.md)
- [Local AI setup, appearance, and zoom exploration](local-ai.md)
- [Workspace panels, comparison, files, and preferences](workspace-and-files.md)
- [Presets and recovery](presets-and-recovery.md)

The illustrated introduction provides worked examples:

- [Resolution and quality](SETTINGS_GUIDE.md#resolution-performance-and-image-quality)
- [Animation, timeline, audio, and export](SETTINGS_GUIDE.md#animation-video-and-the-timeline)
- [HDR and tone mapping](SETTINGS_GUIDE.md#hdr-and-tone-mapping)
- [Exploration and workspace preferences](SETTINGS_GUIDE.md#exploration-tools-and-workspace-preferences)

Repeated timeline parameters have the same meaning as their static control above; their values vary with the track. Output paths, source-folder selections, and workspace arrangement are workflow state rather than universal artwork defaults.

## Sources

[Appearance forms](../src/rff2/ui/workspace/AppearanceForms.cpp) · [Palette workspace](../src/rff2/ui/workspace/PaletteWorkspace.hpp) · [Explore model](../src/rff2/ui/workspace/ExploreModel.hpp) · [Animation model](../src/rff2/ui/workspace/AnimationModel.hpp) · [Surface controls](../src/rff2/ui/workspace/SurfaceParameterRegistry.hpp) · [Surface colors](../src/rff2/ui/workspace/SurfaceColorRegistry.hpp) · [Export workspace](../src/rff2/ui/workspace/ExportWorkspace.hpp) · [Timeline overlay](../src/rff2/ui/workspace/TimelineOverlayForm.hpp)

Modified/created documentation: GPT-6, 2026-09-24.
