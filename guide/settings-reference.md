<!-- Created by GPT-6 on 2026-09-24. -->
<!-- Modified by GPT-6 on 2026-09-25, 2026-09-30, 2026-10-01. -->
<!-- Modified by Opus 5.5 on 2026-10-03, 2026-10-04, 2026-10-06, 2026-10-08, 2026-10-09, 2026-10-10, 2026-10-18, 2026-10-20, 2026-10-21, 2026-10-22, 2026-10-23, 2026-10-25, 2026-10-27, 2026-10-28, 2026-10-29, 2026-11-01, 2026-11-04, 2026-11-05, 2026-11-06. -->
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
- [Orbit Trap](#orbit-trap)
- [Other](#other)
- [Render & Preview](#render--preview)
- [Explore](#explore)
- [Animation](#animation)
- [Zoom Overlay](#zoom-overlay)
- [Max Iterations Display](#max-iterations-display)
- [Surface Effects](#surface-effects)
- [Export and timeline controls](#export-and-timeline-controls)
- [Supersampling filter](#supersampling-filter)
- [Video export load](#video-export-load)
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
| Anisotropy Direction | Fixed Angle | Studio only. Fixed Angle stretches every highlight the same way on screen; Along Bands stretches it along the color bands so it flows around spirals and tendrils, and Across Bands across them. With either, Anisotropy Angle twists the stretch away from the bands. Needs Studio and Specular Anisotropy above 0. | Fixed Angle / Along Bands / Across Bands |

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
| Rim Power | 3 | Concentrates the rim highlight as it increases; below 1 the rim spreads further across the surface. | 0.1 to 64 |
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
| Relief Cap | 0 | Stops the Lustre relief rising this many periods above the view's lowest escaped iteration, so detail too dense for the pixels lies flat and matte instead of glittering while the broad bands keep their gloss; colors are unaffected. The cap follows the view, including through a video zoom. 0 is off. | 0 to 8 |
| Relief Waves | 0 | Adds wave modulation to relief; 0 disables it. | 0 to 1 |
| Wave Frequency | 0.5 | Frequency of relief waves; requires Relief Waves above zero. | 0.03 to 1.5 |
| Waves on Clear Coat Only | Off | Relief Waves shape only the Studio clear-coat highlights, leaving the shading free of their lines and dark bands. | Off / On |
| Zoom Reference | -1 | Reference zoom for compensation. Use Current Zoom binds it; -1 is unbound. | Saved reference zoom: 0 to 16777216, or -1 for an unbound reference. |

### Material Presets

| Action | Result |
| --- | --- |
| Bronze / Pearl / Obsidian / Ceramic | Turns Studio on and sets Roughness, Metalness, Clearcoat, Coat Roughness, Specular Intensity, Specular Anisotropy, Iridescence, and Film Thickness (nm) to that material's values in one undoable edit. |

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
| Vignette | 0 | Darkens the picture toward the frame edges, deepens the colors just inside the dark rim, and leaves the center at its graded color. The value is how dark the edge becomes; 0 is off and draws exactly as before. Applied after the grade and before Fog and Bloom. | 0 to 1 |
| Vignette Shape | Ellipse | Ellipse darkens toward the corners in a rounded shape fitted to the frame. Frame runs a band of the same width along all four edges, slightly deeper in the corners. Requires Vignette above zero. | Ellipse, Frame |
| Vignette Radius | 0.5 | How far the untouched center reaches. With Ellipse, a fraction of the way from the center to the middle of each edge; with Frame, each edge band is (1 − Radius) of half the frame height wide. Requires Vignette above zero. | 0 to 0.95 |
| Vignette Richness | 0.5 | Saturation added where the vignette begins, so colors deepen before they darken; 0 only darkens. Requires Vignette above zero. | 0 to 2 |

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
| Pick Focus Depth | - | Press, then click the fractal: Focus Depth moves to that pixel's iteration count. The black interior leaves it unchanged. | Button (classic Fog window); Finishing workspace action |
| Focus Range | 0.25 | Half-width of the sharp band; larger values keep more depth sharp. | 0.01 to 1 |
| Focus Falloff | 1 | Transition out of the sharp band; larger values retain more sharpness. | 0.1 to 4 |
| Focus Blur | 10 | Maximum defocus blur radius, relative to 1280 image width. | Blur radius in pixels relative to 1280 width. |
| Smooth Focus Across Keyframes | On | Video only: the band's maximum-iteration reference glides between the two keyframes instead of jumping where Auto Max Iteration changes Max Iterations. Off keeps the stepped reference. Older files open with it off. | On / Off |

### Chaos Blur

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Chaos Amount | 0 | Strength of blur on intricate iteration structure; 0 disables it. | Blurs locally intricate fractal regions. Zero disables it; smooth surfaces stay sharp. |
| Chaos Detail Scale | 1 | Detection scale; larger values include broader structure. | Detail detection radius in pixels at 1280 width. One targets fine structure; larger values include broader structure. |
| Chaos Threshold | 0.35 | Minimum selected irregularity; increasing it protects more smooth areas. | Minimum local irregularity to blur. Increase to protect more of the smooth surface. |
| Chaos Transition | 0.5 | Transition from sharp through partial to full irregularity blur. | Smooth transition from sharp through medium blur to full blur as local irregularity increases. |
| Chaos Feather | 6 | Softens the selected region boundary, relative to 1280 image width. | Softens the detected region boundary, in pixels at 1280 width. Detection follows the current zoom and location. |
| Chaos Blur Radius | 4 | Circular blur radius in the selected regions, relative to 1280 width. | Circular aperture radius in pixels at 1280 width. Scales with output size; Blur Quality controls sampling. |
| Chaos Shade | 0 | Darkens selected intricate/defocused regions by the selection alone, up to black at 1 whatever Chaos Amount is. | Darkens intricate defocused regions to separate them from smooth foreground surfaces. At 1, fully selected regions turn black whatever Chaos Amount is. |
| Chaos Interior | 0 | Selection strength for areas that reach Max Iterations, such as the black interior; 0 keeps them sharp. Areas where interior and exterior mix are selected regardless. | Blurs areas that reach Max Iterations, such as the black interior of the Mandelbrot set. Zero leaves them sharp; higher values select them more strongly. |
| Chaos Edge Protection | On (older files: Off) | Keeps smooth surfaces next to blurred regions out of the blur, so their outlines stay crisp; Off blurs every neighbouring pixel into the region as earlier versions did. Costs about 20% more frame time at 2560 x 1440 with Chaos Blur on. | On, Off |
| Chaos Bokeh Shape | Circle | Outline of the Chaos Blur aperture; bright specks in blurred regions take this shape. Polygons stand on a flat edge at 0 degrees. Circle at aspect 1 is the original round blur. | Circle, Triangle, Square, Pentagon, Hexagon, Heptagon, Octagon |
| Chaos Bokeh Rotation | 0 | Turns the outline, in degrees; with Circle it turns the stretch set by Chaos Bokeh Aspect. | 0 to 360 |
| Chaos Bokeh Aspect | 1 | Height over width of the outline at the same area; above 1 stretches it vertically, below 1 horizontally. | 0.25 to 4 |
| Chaos Blur Averaging | Average in Gamma Space | Where the circular blur averages its samples. Gamma Space keeps dark gaps between bright details dark; Linear Space lets bright detail spread and brightens blurred regions. | Average in Gamma Space, Average in Linear Space |

## Animated Materials

These fields apply separately to Layers 1–4. Check Enabled, Opacity, mask, and layer visibility before judging a change.

### Layer 1 (Bottom)

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Enabled | Off | Enables this contribution while retaining its settings when off. | On, Off |
| Effect Type | Rain (Raindrops) | Selects the material effect and, in parentheses, its Rain Shape or style; controls behave according to its type. Classic is each type's original look. Flame (Turbulent) rises from each band's base with smoke above; Flame (Burning Edge) moves a burning front that leaves char. Embers (Rising Sparks) rise and cool from white to the Secondary color; Embers (Spark Bursts) fly outward from a point. Mist (Drifting) covers the surface in the Primary color with two moving cloud layers; Mist (Pooled) settles at each band's base. Ripples (Interference) overlaps waves from four drifting sources; Ripples (Caustics) lights the palette with a moving net of Primary light. Flow Light (Band Chase) lights the bands one after another with an afterglow. Aurora (Ray Curtain) has a sharp Primary hem fading into Secondary with fine rays; Aurora (Ribbons) draws three soft ribbons. | Rain (Raindrops), Rain (Water Streaks), Flame (Classic), Flame (Turbulent), Flame (Burning Edge), Embers (Classic), Embers (Rising Sparks), Embers (Spark Bursts), Mist (Classic), Mist (Drifting), Mist (Pooled), Ripples (Classic), Ripples (Interference), Ripples (Caustics), Flow Light (Classic), Flow Light (Band Chase), Aurora (Classic), Aurora (Ray Curtain), Aurora (Ribbons) |
| Blend | Screen | Normal composite, additive light, screen, or multiplication. | Normal, Add, Screen, Multiply |
| Mask | Surface | Restricts coverage to Surface, Exterior, No Surface, Bands, or Iteration Edges. | Surface, Exterior, No Surface, Bands, Iteration Edges |
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
| Band Period | 80 | Iteration interval for band-related structure/masking. | 1 to 1e+09 |
| Primary Color | 0.64, 0.79, 1, 1 | The two colors used by the selected material effect. | Choose a color, enter #RRGGBB, or R, G, B, A in 0–1. |
| Secondary Color | 1, 0.25, 0.025, 1 | The two colors used by the selected material effect. | Choose a color, enter #RRGGBB, or R, G, B, A in 0–1. |

## Palette

### Color Cycle

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Link G & B to R | On | Palette workspace only. While On, changing Cycle Length (R) sets G and B to the same value. | On, Off |
| Cycle Length (R) | 64 | Iterations per channel cycle; halving all three doubles repetition in iteration space. | 1 to 1e18 iterations per channel cycle. |
| Cycle Length (G) | 64 | Iterations per channel cycle; halving all three doubles repetition in iteration space. | 1 to 1e18 iterations per channel cycle. |
| Cycle Length (B) | 64 | Iterations per channel cycle; halving all three doubles repetition in iteration space. | 1 to 1e18 iterations per channel cycle. |
| Iteration Coloring | Linear | Curve applied before palette cycling; changes color-band spacing across depth. | Linear, Square root, Cube root, Log, LogLog, Smoothstep. The legacy Palette window also offers Smootherstep; files that use it still render with it. |
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
| Rounded Bands | On (older files: Off) | Swells each band between two lines into a rounded cord, so the lines become the creases between cords instead of carved grooves. Groove Depth sets how far the bands swell; Auto Groove, Smooth Grooves and Groove Width apply only with it off. Off carves grooves. | On, Off |
| Crease Blur | 6 (older files: 0) | With Rounded Bands, softens the crease between two bands over this many pixels (at 720 lines) so it does not read as a thin line; the bands keep their full swell. 0 leaves the crease sharp. | 0 to 24 |
| Auto Groove | On | Derives groove shape automatically from the band-line setup. | On, Off |
| Smooth Grooves | On (older files: Off) | Symmetric walls, a softer and hue-keeping shadow, and smoothed narrow grooves, so grooves do not read as thin black lines. Off draws the original grooves. | On, Off |
| Groove Depth | 1.5 | Depth of the recessed groove response; above 5 the shading range widens too. | 0 to 20 |
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
| Stop Easing | Smoothstep | How colors blend between neighboring stops. | Linear, Smoothstep, Smootherstep |
| Selected Stop | 0 | Selects an existing stop starting at 1. Apply selection before changing its color/position. | Apply the selection before editing its color or position. |
| Stop Position | 0 | Position in the cycle from 0 up to, but excluding, 1; stops remain ordered. | Ordered position from 0 up to, but excluding, 1. |
| Stop Color | 0, 0, 0, 1 | Color of the selected stop. | #RRGGBB or R, G, B, A in 0–1. |

### Preset Library

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Palette Preset | LongRandom64 [Recommend] | Complete palette recipe; applying it can replace more than the color array. | Palette library; Apply Palette Preset applies the chosen recipe. |
| Seed | 1 | Unsigned seed for repeatable palette generation; same preset and seed regenerate the same recipe. | Whole number from 0 to 4294967295 for repeatable generation. |

### KFR Color

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Loaded KFR File | (empty) | Names the KFR file Load KFR File last read and shows its colors before they are applied. | Display only; Load KFR File fills it. |

### Colors from Settings

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Loaded Settings File | (empty) | Names the RFC or RFSP that Load Settings File last read and shows its colors before they are applied. | Display only; Load Settings File fills it. |
| Palette Preview | (display only) | Shows the current palette drawn with each Color Smoothing and each Color Interpolation choice, before color correction, so the choices can be compared without applying them. | Not saved; nothing to edit. |

## Orbit Trap

Opened by **Shader > Orbit Trap**. Every setting except Coloring is greyed out while Coloring is None. Iterations that MPA skips are not sampled, so the trap can differ slightly from the exact orbit; Ignore MPA on GPU gives the exact one on the GPU. A view opened from a map file draws as None until it is recalculated. Keyframes generated with Coloring on save their traps beside each map as a numbered `.rftz` file, which the Timeline Editor preview and video export read.

### Trap

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Coloring | None | What the palette follows besides the iteration count. Cross Glow: nearest approach to the real and imaginary axes (Pickover stalks). Cellular: nearest and second nearest approach to the origin, each cell colored by the iteration of its nearest approach and darkened toward its border. Shape: nearest approach to the trap set below. | None, Cross Glow, Cellular, Shape |
| Shape | Point | Shape coloring only. Circle measures the distance to its rim. Grid Points and Grid Lines measure to the nearest point or line of a square lattice, Rings to the nearest of evenly spaced circles around the center, Polygon and Star to their outline, and Spiral along the radius to a spiral. Changing it recalculates the view. | Point, Circle, Cross, Line, Grid Points, Grid Lines, Rings, Polygon, Star, Spiral |
| Center Real | 0 | Shape coloring only: real part of the trap's center. Recalculates. | −4 to 4 |
| Center Imaginary | 0 | Shape coloring only: imaginary part of the trap's center. Recalculates. | −4 to 4 |
| Radius | 0.5 | The size of Circle, Polygon and Star, the spacing of Grid Points, Grid Lines and Rings, and the step per turn of Spiral; 0 measures as Point (Grid Lines as Cross). Point, Cross and Line do not use it. Recalculates. | 0 to 4 |
| Angle | 0 | Degrees the trap turns about its center; Point, Circle and Rings do not use it. At 0 a Polygon or Star has a corner on the positive real axis. Recalculates. | −180 to 180 |
| Sides | 5 | Polygon and Star only: the polygon's sides or the star's points; a star's inner corners sit at half its size. Recalculates. | 3 to 64 |

### Palette

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Color Mapping | Distance | Shape coloring only. Distance runs the palette along the distance to the trap. Angle turns it with the direction the orbit came from at its nearest approach, Arms times per turn. Bands lays rings of equal distance, a quarter cycle per halving times Density, with a dark seam between them. No recalculation is needed. | Distance, Angle, Bands |
| Arms | 3 | Angle color mapping only: palette turns per revolution. | 1 to 64 |
| Density | 1 | Palette cycles across the trap; unused under Angle color mapping. | −16 to 16 |
| Offset | 0 | Shifts the trap's colors along the palette, in cycles. | −1 to 1 |
| Animation Speed | 0 | Runs the trap's colors on their own, in palette cycles per second; 0 holds still. | −4 to 4 |
| Hold in Color Animation | Off | Keeps the trap's colors still while the palette animates, so only the iteration bands move; Animation Speed still runs. | On, Off |

### Glow

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Width | −5 | log2 of the distance where the glow halves; lower it at deep zooms. With Relief, the tube's radius. | −100000 to 8 |
| Glow Sharpness | 1 | Higher draws thinner lines. Unused with Relief. | 0.1 to 8 |
| Glow Color | 1, 1, 1 | Color of the light where the orbit nearly hits the trap; with Relief, the tube's highlight. | Choose a color, enter #RRGGBB, or R, G, B in 0–1. |
| Glow Strength | 0.6 | Brightness of that light. | 0 to 4 |
| Relief | Off | Cross Glow and Shape only: draws the trap as a round, glossy tube lit by the Lighting & Relief light direction; outside the tube the picture takes only its shadow. | On, Off |

### Blending

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Amount | 0.8 | Share of the trap coloring over the iteration palette. | 0 to 1 |
| Blend Mode | Mix | How the trap is laid over the palette; Screen only brightens, so the palette keeps its colors under the glow. | Mix, Multiply, Screen, Add |
| Apply To | Inside and Outside | Which pixels the trap colors. Inside means the pixels that reach Max Iterations. Outside Only leaves them in Mandelbrot Color; Inside Only leaves the escaping pixels on the iteration palette. | Inside and Outside, Outside Only, Inside Only |

### Cell Border

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Border Width | 1 | Cellular only: how far the dark border reaches into each cell. | 0.1 to 8 |
| Border Darkness | 0.85 | Cellular only: how dark the border gets. | 0 to 1 |

Settings files and shader presets retain these values; older files open with None, Point at 0, 0, Radius 0.5, Angle 0, Distance, Arms 3, Relief Off and Hold in Color Animation Off.

## Other

Opened by **Shader > Other**.

### Escape Angle

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Escape Angle Coloring | None | Shifts the palette by the angle at which each orbit escaped. Binary Decomposition moves the half of every band that escaped below the real axis by Escape Angle Amount cycles, drawing a checkerboard along the external rays. Angle Twist moves every pixel by Escape Angle Amount cycles per full turn; whole-number amounts leave no seam. Live view and image export only: a view opened from a map file, and video export, draw as None. | None, Binary Decomposition, Angle Twist |
| Escape Angle Amount | 0.5 | Palette cycles; greyed out while Escape Angle Coloring is None. | −4 to 4 |

## Render & Preview

### Performance

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Rendering FPS | 60 | Live display limit, independent of video export rate. Lower values can reduce preview GPU load; 0 pauses the preview, keeping the last frame while settings and calculations still apply. | Live rendering limit: 0 to 1000 frames per second; 0 pauses the preview. Lower values reduce rendering load. Video export has a separate FPS setting. |
| Calculation Threads | Logical core count | CPU calculation workers, limited to logical cores; applies to the next calculation. | 1 up to this computer's logical core count. Applied to the next calculation. |

### Calculation Preview

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Boundary Trace Fill | Off | Fills qualifying regions through boundary tracing; bypassed in Absolute Iteration Mode. | On, Off |
| Two-Color Preview | Off | Two-color calculation preview; return to ordinary coloring to judge appearance edits. | On, Off |
| Coarse Preview | On | Shows a coarse first result while the full calculation proceeds. | On, Off |

### GPU Perturbation

Set in the **GPU Perturbation** group of the Render & Preview panel, opened by **Render > Render & Preview**; **Render > Use GPU Perturbation** toggles the first row. Render presets keep these values.

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Use GPU Perturbation | Off | Iterates on the GPU in double precision down to zoom 1e290. Deeper zooms, custom formulas, compressed references and GPUs without 64-bit shader integers stay on the CPU. Boundary Trace Fill, Two-Color Preview and Coarse Preview apply only to CPU calculation. | On, Off |
| Single Precision | On | Iterates in float down to zoom 1e30, as RFF 2.3.0 did: on GeForce GPUs often more than 10 times faster, but iteration values drift from the double-precision result, by hundreds of iterations in sensitive views. Deeper zooms, and MPA tables beyond float range, use double. | On, Off |
| Batch Duration (s) | 0.1 | Target time of each GPU batch; the batch length doubles or halves toward it. | 0.01 to 1 second |
| Allowed Unfinished Pixels | 0 | Ends the GPU calculation with up to this many pixels still iterating; they keep the iteration reached. | 0 or more; 0 finishes every pixel |
| Ignore MPA on GPU | Off | Iterates every step without MP-Approximation skips. | On, Off |
| Stop MPA After Batches | 0 | After this many batches, the remaining pixels iterate without MP-Approximation. | 0 or more; 0 keeps MPA |
| Interpolate Isolated Pixels | Off | An unfinished pixel with four finished neighbors takes their mean; faster, but approximated. | On, Off |

## Explore

### Location

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Real | -0.849999999999999991673 | High-precision center coordinates; preserve all digits for deep locations. | High-precision center coordinate. |
| Imaginary | 0 | High-precision center coordinates; preserve all digits for deep locations. | High-precision center coordinate. |
| Log Zoom (10) | 2 | Base-10 scale: +1 means 10 times magnification at fixed canvas size. Older captures show the incorrect label Log Zoom (e). | 0 to 16777216; base-10 scale. |
| Rotation | 0 | Calculated view rotation in degrees; also panorama yaw. Requires recalculation. | Degrees. Also controls panorama yaw. |

### Iterations

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Automatic Iterations | On | Derives the Mandelbrot iteration ceiling from period times Auto Iteration Multiplier; period detection stays automatic. | On, Off |
| Max Iteration | 300 | Manual escape-iteration ceiling when Automatic Iterations is off. | Used when Automatic Iterations is off. |
| Auto Iteration Multiplier | 150 | Multiplier of detected period; raises the ceiling for difficult pixels. | Used when Automatic Iterations is on. |
| Bailout | 1e+30 | Escape radius, affecting termination and smooth iteration evaluation; separate from MPA tolerance. | Escape radius, 2 to 1e38. |
| Decimalize Iteration | LogLog | Fractional iteration method. The normal quadratic escape-potential path gives the same smoothing for exposed non-None choices. | Linear, Square root, Log, LogLog |
| Absolute Iteration Mode | Off | Absolute iteration reporting; bypasses Boundary Trace Fill. | On, Off |

### Reference

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Reuse Reference | Disabled | Disabled creates an independent reference; Current Reference/Centered Reference select reuse where a compatible reference exists. | Current Reference, Centered Reference, Disabled |
| Compression Criteria | 0 | Repeated-reference batch threshold; 0 disables reference compression. | Minimum reference batch; 0 disables compression. |
| Compression Threshold | 0 | Positive N sets relative threshold 10^-N; higher is stricter. Zero gives zero tolerance; explicitly disable with Compression Criteria 0. | 0 to 255; see reference-compression discussion in the main guide. |
| Disable Normalization | Off | Disables reference-compressor normalization; advanced storage tuning. | On, Off |

### MP-Approximation

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Min Skip Reference | 4 | Minimum usable MPA skip; above the longest period can leave the table empty. | MPA follows periodic structure; minimum 4. |
| Max Multiplier Between Levels | 2 | Spacing of intermediate MPA period levels; affects table/search structure. | Ratio between adjacent period levels, 2 to 255. |
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

### Color Animation

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
| Frozen Colors | (none) | Up to 16 frozen bands, shown as swatches. Pick Color to Freeze adds one; click a swatch to remove it; Clear Frozen Colors removes all. | Swatches; ? marks a band whose picked screen color is unavailable in this session. |

### Zoom Motion

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Zoom Speed | 1 | Saved keyframes per second; greater values traverse faster. Use holds for pauses. | Keyframes per second; greater than 0. |
| Extra Final Zoom-in | 2 | Extra zoom-in beyond the ordinary final saved stage. | Additional zoom at the end of the video; 0 to 8. |
| Show Zoom Ratio | On | Displays the zoom readout in preview and video. | On, Off |

### Keyframes

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Zoom Step per Keyframe | 2 | Magnification ratio of neighboring maps; 2 means 2 times. Its log10 becomes the stored log-zoom increment. | Greater than 1; a magnification ratio. |
| Rotation / 360 Padding | Off | Generates larger planar maps for rotation/panorama; requires regenerating keyframes. A 360 live view is also calculated once over the padded area, so turning the camera or changing Pitch, Field of View, Panorama Range or Layout redraws without recalculating; the picture is resampled from that area. Keyframe generation and video export always calculate directly. | On, Off |
| Camera Padding Scale | 4 | Linear padding factor; doubling quadruples source pixels and associated storage. | 2 to 64. Memory and disk use grow with the square of this value. |
| Render from PNG Images | Off | Uses already-colored PNGs; iteration-dependent effects require maps instead. | On, Off |

### Video Camera

Rotation Mode, Seconds per Turn, Rotation Direction, Rotation Start Angle and the camera's Rotation, Projection, Pitch, Field of View, Panorama Range and Layout are set in the Timeline Editor (Camera / Rotation / 360° panel and camera tracks), not in the Animation workspace.

**Match Planar Framing** (in the Timeline Editor and on a camera track's right-click menu) writes keys at the playhead for 360 Camera projection, Pitch −90, a Field of View calculated from the loaded keyframes' aspect ratio, Ground and Sky layout, and a Panorama Range that covers the frame, keeping rotation. It needs keyframes loaded in the Timeline Editor that were generated with Rotation / 360 Padding. Undo restores all of them together.

## Zoom Overlay

### Appearance

| Setting | Fresh default | What changing it does / dependency | Choices or input note |
| --- | --- | --- | --- |
| Show Zoom Ratio | On | Show the zoom ratio in preview and exported video. | On, Off |
| Display Start (s) | 0 | First visible time, including zoom holds. 0 means the video start. | 0–604800 seconds. |
| Display End (s) | 0 | First hidden time. 0 means the video end; otherwise it must exceed Display Start. | 0–604800 seconds. |
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

## Max Iterations Display

Open **Max Iterations Display** in the Timeline Editor. This is a separate overlay from Zoom Display, with independent visibility, placement, font, text color, outline, and shadow. Shared styling fields use the ranges listed under [Zoom Overlay](#zoom-overlay); it has no Decimal Places or Custom Appearance switch.

| Setting | Fresh default | What changing it does / dependency |
| --- | --- | --- |
| Show Max Iterations | Off | Shows the source-map iteration limit in preview and exported video. It does not show the hovered pixel's iteration value. |
| Display Start (s) | 0 | First visible time, including zoom holds; 0 means the video start. Range: 0–604800 seconds. |
| Display End (s) | 0 | First hidden time; 0 means the video end. A nonzero end must exceed Display Start. Range: 0–604800 seconds. |
| Interpolate per Frame | Off | Gradually changes the displayed count between source maps. It does not change calculation or rendering limits. |

See [overlay workflow and source limitations](animation-and-export.md#zoom-overlay-and-preview-guides) for position editing and PNG behavior.

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
| Relief Cap | 0 | Flattens Lustre relief above the view's lowest escaped iteration plus this many periods; also available in Lighting & Relief. | 0 to 8 |
| Lustre Depth | 1 | Lustre relief depth; also available in Lighting & Relief. | 0 to 16 |
| Normal Smoothing | 0 | Relief normal smoothing; also available in Lighting & Relief. | 0 to 1 |
| AO Radius | 8 | Neighborhood radius for cavity shading. | 1 to 64 |
| Relief Waves | 0 | Relief-wave modulation strength. | 0 to 1 |
| Wave Frequency | 0.5 | Frequency of relief-wave modulation. | 0.03 to 1.5 |

### Material & Reflection

| Setting | Fresh default | Effect | Range |
| --- | --- | --- | --- |
| Material Model | Studio GGX | How Studio shades its material. OpenPBR Surface layers it as an approximation of the OpenPBR Surface specification (not a conforming implementation): reflections keep their energy at every roughness, metals take an Edge Tint, the coat has its own IOR, color and darkening, and Fuzz adds a sheen; Specular Intensity becomes its specular weight. Choosing OpenPBR Surface turns Studio on. | Studio GGX / OpenPBR Surface |
| Roughness | 0.32 | Spreads Studio reflections; needs Studio and visible specular/reflection contribution. | 0.04 to 1 |
| Metalness | 0.6 | Blends toward a metallic Studio response. | 0 to 1 |
| Index of Refraction | 1.5 | Dielectric index of refraction, controlling reflection strength. | 1 to 3 |
| Direct Light | 0.8 | Strength of direct Studio illumination. | 0 to 8 |
| Studio Reflections | 0.7 | Strength of Studio environment reflections. | 0 to 8 |
| Clearcoat | 0.3 | Strength of an additional clear coating. | 0 to 1 |
| Coat Roughness | 0.15 | Roughness of the coating; requires Clearcoat above zero. With Coat Softbox on, lower values give the softbox reflections sharper edges down to 0.04. | 0.04 to 1 |
| Environment Source | Two Colors | What the base and coat reflect: the original sky/ground light, a four-stop Gradient, or Texture Layer 1-4 read as a 360-degree equirectangular panorama (the layer need not be on). | Two Colors / Gradient / Texture Layer 1-4 |
| Environment Brightness | 6 | How bright a white gradient stop or image pixel reflects, multiplied by Studio Reflections; above 1 shines like a lit softbox. Requires a source other than Two Colors. | 0 to 32 |
| Gradient Stop 1-4 Position | 0 / 0.55 / 0.68 / 0.82 | Where each stop sits along the reflected direction: 0 straight down, 0.5 level, 1 straight up. Requires Gradient. | 0 to 1 |
| Gradient Stop 1-4 Color | dark / slate / white / blue | Color reflected at each stop. Requires Gradient. | Color |
| Coat Softbox | On | The coating reflects the key and fill lights as small rectangular softboxes, crisp window shapes that bend with the relief; Off keeps round highlights. Older files open with it off. Requires Clearcoat above zero. | Off / On |
| Coat IOR | 1.6 | OpenPBR Surface: index of refraction of the coat; the base's own IOR is measured against it. Requires Clearcoat above zero. | 1 to 3 |
| Coat Darkening | 1 | OpenPBR Surface: how much the base darkens and deepens under the coat; 0 keeps it as bright as without the coat. Requires Clearcoat above zero. | 0 to 1 |
| Coat Color | White | OpenPBR Surface: tint absorbed by everything seen through the coat, stronger on slopes seen at an angle. Requires Clearcoat above zero. | Color |
| Edge Tint | White | OpenPBR Surface: color a metal turns toward near its grazing edges (F82 tint). Requires Metalness above zero. | Color |
| Fuzz | 0 | OpenPBR Surface: a soft velvet- or dust-like sheen over everything, brightest on steep slopes. | 0 to 1 |
| Fuzz Roughness | 0.5 | OpenPBR Surface: low values gather the fuzz into a thin rim; high values spread it. Requires Fuzz above zero. | 0 to 1 |
| Fuzz Color | White | OpenPBR Surface: color of the fuzz. Requires Fuzz above zero. | Color |
| Film IOR | 1.4 | OpenPBR Surface: index of refraction of the thin film that Iridescence draws as real interference. Requires Iridescence above zero. | 1 to 3 |
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

These controls are available through Shader → HDR & Tone Mapping, which opens the **HDR & Tone Mapping** output group. Defaults below are the declared defaults for these fields; a loaded file or preset can replace them.

| Setting | Default | Effect | Choices / range |
| --- | --- | --- | --- |
| HDR Rendering | Off | Retains above-white light for the HDR rendering path. Required for HDR video transfer. | On, Off |
| Tone Mapping | ACES (Narkowicz fit) | Chooses the display curve for SDR preview/image output; False Color is diagnostic. | Clip, Reinhard, ACES (Narkowicz fit), Filmic, MFR Shoulder, MFR Log View, MFR Linear Clip, MFR False Color |
| Exposure | 0 | Linear-light exposure in stops; +1 doubles light. Distinct from Color Correction Exposure. | −20 to 20 |
| Highlight Headroom | 2 | Highlight range for applicable conventional tone-mapping modes; ignored by MFR SDR display modes. | 1 to 64 |
| MFR Mastering Peak (nits) | 4000 | MFR display metadata/curve control, with reference white 203 nits; does not change source light. | 100 to 10000 |
| Video Transfer | SDR | Chooses exported signal transfer, independently of the SDR preview curve. | SDR, HDR10 (PQ), HLG |
| HDR Peak Brightness (nits) | 1000 | Peak brightness for PQ export. Separate from MFR Mastering Peak. | 100 to 10000 |

[Curve overview](shader-overview.md#hdr-and-tone-mapping) · [Detailed MFR behavior](../documentation/notes/mfr-hdr-preview.md)

The ACES choice is a curve approximation, not a full ACES transform; see the attribution in [NOTICE](../NOTICE).

## Video output size

These controls are in Export Settings (Video Output), which the Timeline Editor opens too. Output Width and Output Height are also in the Export workspace. Every frame is still rendered at the keyframe size, shading, stripes and palette included; only the finished frame is resized, with a Lanczos filter, when the file is encoded.

| Setting | Default | Effect | Choices / range |
| --- | --- | --- | --- |
| Output Width | 0 | Width of the video file. 0 keeps the keyframe width, or follows the aspect ratio when only Output Height is set. | 0 to 16384 |
| Output Height | 0 | Height of the video file. 0 keeps the keyframe height, or follows the aspect ratio when only Output Width is set. Both set to another aspect ratio stretches the picture. Lossy and HDR video round each side down to an even number. | 0 to 16384 |

## Video export load

**GPU Work Split** is in Export > Keyframe Output and in Export Settings, which the Timeline Editor opens too. It applies to video export only, not to keyframe generation, image export or the live view.

| Setting | Fresh default | Effect | Choices / range |
| --- | --- | --- | --- |
| GPU Work Split | 16 | Sends each video frame to the GPU in smaller parts, so a long export at a high resolution or Supersampling is not stopped when Windows resets a GPU that stays busy with one piece of work for about 2 seconds. 0 sends each frame as one part, 1 sends each shader pass separately, 2 to 64 also cut the fractal pass into that many horizontal bands. The video is identical at every value; higher values export slightly slower. A settings file keeps its saved value; files from before the setting existed open with 0. | 0 to 64 |

## Supersampling filter

**SSAA Filter** sits beside Supersampling in Export > Resolution & Quality (the Render Properties window with Use Legacy Settings UI). It applies to the live view, image export and video export, SDR and HDR alike; with Supersampling 1 nothing changes.

| Setting | Fresh default | Effect | Choices |
| --- | --- | --- | --- |
| SSAA Filter | Mitchell (Linear) | Mitchell (Linear) combines the samples with a Mitchell-Netravali filter in linear light, so thin dark filaments keep their true brightness and edges alias less. Box (Gamma) takes the plain mean of the stored values, which darkens fine detail. Settings files saved before this setting existed open with Box (Gamma); render presets leave it as it is. | Mitchell (Linear), Box (Gamma) |

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
| Restore Original Order | Action | Resets the order and visibility and turns Custom Layer Order off. The layer panel's **Reset** button does the same. |
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
| Load KFR File / Apply KFR Colors | Load KFR File chooses a `.kfr` file and reads its colors at once; Apply KFR Colors puts them on the palette. Undo restores the previous palette. |
| Load Settings File / Apply Colors / Apply Colors and Color Correction | Load Settings File chooses an `.rfc` or `.rfsp` and reads its palette at once; Apply Colors puts the palette on, and Apply Colors and Color Correction also sets Gamma, Exposure, Hue, Saturation, Brightness, and Contrast. |
| Use Current Zoom | Binds Lustre's Zoom Reference to the current fractal zoom. |
| Clear Image | Textures only: removes that layer's image and turns the layer off, leaving its other settings. Choosing an image turns the layer on. |
| Reset Layer | Restores the selected Texture, Pattern, or Animated Materials layer to its defaults. |
| Copy to Next Layer | Overwrites the next layer with the current layer's settings. |
| Move Toward Bottom / Move Toward Top | Swaps neighboring layer slots; available directions depend on the selected slot. |
| Save Appearance Settings / Load File | Saves appearance settings in `.rfsp`; Load File restores them from an `.rfsp`. Full location/configuration uses `.rfc`. |
| Surface: Reset selected value/color | Restores the selected control's default. |
| Surface: Reset effect | Restores the selected effect category's controls. |
| Clear added effects (keep base style) | Clears additional surface effect contributions while keeping the base style. |
| Reset all appearance (includes palette) | Resets the appearance including palette settings; broader than resetting one surface control. |

## Export and timeline controls

The detailed manual covers the remaining controls in context. The [actual UI gallery](ui-gallery.md#export) also lists the captured Export form controls and their input guidance.

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

## Overlay display timing

Zoom Overlay and Max Iterations Display each offer a Show switch, Display Start (s), and Display End (s). Both times default to 0 and accept 0–604800 seconds. Start 0 means the video start; End 0 means the video end. Either time can remain 0, and both 0 shows the enabled text throughout. A nonzero end must exceed start. Apply the settings to display text from start inclusive to end exclusive, including zoom holds. No separate Limit Display Time switch or visibility track is required. Open these settings from the footer buttons or the track context menu's Parameters submenu.

## Zoom holds

Timeline Settings → Zoom Holds offers Add Zoom Hold at Playhead, Zoom Hold selection, Hold Keyframe, Hold Duration (s), and Remove Selected Hold. New holds use 2 seconds. Duration accepts 0–604800 seconds, with 0 disabling the hold. Keyframe position uses the loaded source range. Apply commits edits; Undo/Redo and `.rfc`, `.rfvt`, and timeline JSON preserve the hold data.
