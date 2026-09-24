<!-- Created by GPT-6 on 2026-09-24. -->
# Shader documentation coverage

[Shader overview](shader-overview.md) · [Main guide](SETTINGS_GUIDE.md) · [Field reference](settings-reference.md)

Checked against the workspace source on **September 24, 2026**. This is a documentation coverage check, not a claim that every parameter has been visually regression-tested. The image suite contains **54 before/after comparisons**, including [24 additional shader examples](shader-comparisons.md). Reused style pictures demonstrate complete recipes.

## Scope

- All **17 Shader menu entries** have a destination and explanation in the overview.
- All **29 shader layer slots** are named and described, with shared Texture/Pattern/Effect slots grouped as 1–4.
- The seven appearance/palette forms are checked against their captured field inventory; current AppearanceForms source labels are also checked directly.
- Surface numeric/color registries and display-name overrides are checked against both the overview and the field reference. Identical names and repeated layer fields are not separate feature claims.
- The **27 Surface inspector groups** are mapped below, including groups whose individual controls are routed to Palette.
- HDR controls, palette/layer actions, shader presets, and links to Animation and Local AI appearance supplement the field inventory.

The earlier guide had short summaries and omitted some displayed-name mappings and layer actions. The expanded overview and reference now separate those features explicitly. This checklist covers user-facing shader features in these sources, not every private uniform, implementation variable, or historical dialog alias.

## Appearance and palette form groups

The count is the number of field definitions in that group, not the number of effects or rendered tests. Layers 2–4 repeat the same controls with independent values. Action-only groups can have zero fields.

| Form / group | Field definitions | Documentation |
| --- | --- | --- |
| Lighting & Relief / Slope Shading | 8 | [Overview](shader-overview.md#lighting-and-relief) · [Fields/actions](settings-reference.md#slope-shading) |
| Lighting & Relief / Light Direction | 5 | [Overview](shader-overview.md#lighting-and-relief) · [Fields/actions](settings-reference.md#light-direction) |
| Lighting & Relief / Specular | 9 | [Overview](shader-overview.md#lighting-and-relief) · [Fields/actions](settings-reference.md#specular) |
| Lighting & Relief / Relief Detail | 3 | [Overview](shader-overview.md#lighting-and-relief) · [Fields/actions](settings-reference.md#relief-detail) |
| Lighting & Relief / Lit / Shadow Tint | 6 | [Overview](shader-overview.md#lighting-and-relief) · [Fields/actions](settings-reference.md#lit--shadow-tint) |
| Lighting & Relief / Tone Mapping | 4 | [Overview](shader-overview.md#lighting-and-relief) · [Fields/actions](settings-reference.md#tone-mapping) |
| Lighting & Relief / Rim Light | 3 | [Overview](shader-overview.md#lighting-and-relief) · [Fields/actions](settings-reference.md#rim-light) |
| Lighting & Relief / Gloss | 7 | [Overview](shader-overview.md#lighting-and-relief) · [Fields/actions](settings-reference.md#gloss) |
| Lighting & Relief / Lustre Relief | 10 | [Overview](shader-overview.md#lighting-and-relief) · [Fields/actions](settings-reference.md#lustre-relief) |
| Textures / Layers 1–4 (shared controls) | 13 | [Overview](shader-overview.md#textures) · [Fields/actions](settings-reference.md#textures) |
| Patterns / Layers 1–4 (shared controls) | 20 | [Overview](shader-overview.md#patterns) · [Fields/actions](settings-reference.md#patterns) |
| Warp & Stripe / Domain Warp | 11 | [Overview](shader-overview.md#domain-warp) · [Fields/actions](settings-reference.md#domain-warp) |
| Warp & Stripe / Stripe | 6 | [Overview](shader-overview.md#stripe) · [Fields/actions](settings-reference.md#stripe) |
| Finishing / Color Correction | 6 | [Overview](shader-overview.md#color-correction) · [Fields/actions](settings-reference.md#color-correction) |
| Finishing / Bloom | 5 | [Overview](shader-overview.md#bloom) · [Fields/actions](settings-reference.md#bloom) |
| Finishing / Fog | 8 | [Overview](shader-overview.md#fog-and-selective-blur) · [Fields/actions](settings-reference.md#fog) |
| Finishing / Focus Band | 5 | [Overview](shader-overview.md#fog-and-selective-blur) · [Fields/actions](settings-reference.md#focus-band) |
| Finishing / Chaos Blur | 8 | [Overview](shader-overview.md#fog-and-selective-blur) · [Fields/actions](settings-reference.md#chaos-blur) |
| Animated Materials / Layers 1–4 (shared controls) | 21 | [Overview](shader-overview.md#animated-materials) · [Fields/actions](settings-reference.md#animated-materials) |
| Palette / Color Cycle | 13 | [Overview](shader-overview.md#palette-and-band-lines) · [Fields/actions](settings-reference.md#color-cycle) |
| Palette / Band Line | 21 | [Overview](shader-overview.md#palette-and-band-lines) · [Fields/actions](settings-reference.md#band-line) |
| Palette / Color Stops | 4 | [Overview](shader-overview.md#palette-and-band-lines) · [Fields/actions](settings-reference.md#color-stops) |
| Palette / Arrange Stops | 0 | [Overview](shader-overview.md#palette-and-band-lines) · [Fields/actions](settings-reference.md#palette-and-layer-actions) |
| Palette / Preset Library | 2 | [Overview](shader-overview.md#palette-and-band-lines) · [Fields/actions](settings-reference.md#preset-library) |
| Palette / Import Colors | 2 | [Overview](shader-overview.md#imports-and-presets) · [Fields/actions](settings-reference.md#import-colors) |

Total with repeated layers counted once: **200 field definitions**. The Surface registries, aliases, HDR/output fields, and action controls are additional sources below.

## Surface inspector groups

| Source group | Overview |
| --- | --- |
| Palette & surface | [Description](shader-overview.md#material-reflection-and-color) |
| Reflections & rim | [Description](shader-overview.md#material-reflection-and-color) |
| Background | [Description](shader-overview.md#material-reflection-and-color) |
| Material | [Description](shader-overview.md#material-reflection-and-color) |
| Studio lighting | [Description](shader-overview.md#material-reflection-and-color) |
| Coating | [Description](shader-overview.md#material-reflection-and-color) |
| Reflection shaping | [Description](shader-overview.md#material-reflection-and-color) |
| Thin film | [Description](shader-overview.md#film-prism-and-glints) |
| Prism | [Description](shader-overview.md#film-prism-and-glints) |
| Glints | [Description](shader-overview.md#film-prism-and-glints) |
| Sigil ink | [Description](shader-overview.md#material-reflection-and-color) |
| Glow & color | [Description](shader-overview.md#emission-and-dense-detail) |
| Dense detail | [Description](shader-overview.md#emission-and-dense-detail) |
| Body & rim | [Description](shader-overview.md#emission-and-dense-detail) |
| Particles | [Description](shader-overview.md#emission-and-dense-detail) |
| Chrome flames | [Description](shader-overview.md#chrome-flames-frost-and-noise) |
| Frost | [Description](shader-overview.md#chrome-flames-frost-and-noise) |
| Print & outlines | [Description](shader-overview.md#print-and-color-quantization) |
| Waves & grain | [Description](shader-overview.md#print-and-color-quantization) |
| Print colors | [Description](shader-overview.md#print-and-color-quantization) |
| VHS & color | [Description](shader-overview.md#chrome-flames-frost-and-noise) |
| Pixelation | [Description](shader-overview.md#chrome-flames-frost-and-noise) |
| Grain & monochrome | [Description](shader-overview.md#chrome-flames-frost-and-noise) |
| Relief | [Description](shader-overview.md#lighting-and-relief) |
| Lighting | [Description](shader-overview.md#lighting-and-relief) |
| Waves | [Description](shader-overview.md#lighting-and-relief) |
| Surface layers | [Description](shader-overview.md#studio-and-surface-styles) |

## Additional control families

| Family | Documentation |
| --- | --- |
| Studio, Base Style, Style Application, and six style mixes | [Studio and surface styles](shader-overview.md#studio-and-surface-styles) |
| Surface inspector aliases and the two Iridescence fields | [Display-name mapping](settings-reference.md#surface-editor-display-names) |
| HDR Rendering, eight tone curves, exposure, headroom, peaks, and transfer | [HDR reference](settings-reference.md#hdr-and-tone-mapping) |
| Ordering, visibility, selection, and restoration | [Shader layer controls](settings-reference.md#shader-layer-controls) |
| Palette stop editing, recipe/import actions, and local layer actions | [Actions](settings-reference.md#palette-and-layer-actions) |
| Shader file loading/saving, preset families, and Local AI appearance | [Imports and presets](shader-overview.md#imports-and-presets) |
| Palette animation, frozen colors, scroll, effect clocks, and timeline | [Movement](shader-overview.md#movement) |

## Source files checked

- [SettingsMenu.cpp](../src/rff2/ui/SettingsMenu.cpp)
- [AppearanceForms.cpp](../src/rff2/ui/workspace/AppearanceForms.cpp)
- [PaletteWorkspace.hpp](../src/rff2/ui/workspace/PaletteWorkspace.hpp)
- [SurfaceParameterRegistry.hpp](../src/rff2/ui/workspace/SurfaceParameterRegistry.hpp)
- [SurfaceColorRegistry.hpp](../src/rff2/ui/workspace/SurfaceColorRegistry.hpp)
- [SurfacePresentation.hpp](../src/rff2/ui/workspace/SurfacePresentation.hpp)
- [SurfaceInspectorContent.hpp](../src/rff2/ui/workspace/SurfaceInspectorContent.hpp)
- [ShaderLayerModel.hpp](../src/rff2/ui/workspace/ShaderLayerModel.hpp)
- [ShaderLayerWorkspace.hpp](../src/rff2/ui/workspace/ShaderLayerWorkspace.hpp)
- [OrderedShaderLayers.hpp](../src/rff2/vulkan/OrderedShaderLayers.hpp)
- [ExportWorkspace.hpp](../src/rff2/ui/workspace/ExportWorkspace.hpp)
- [ShdHdrAttribute.h](../src/rff2/attr/ShdHdrAttribute.h)
- [ShdToneMapMethod.h](../src/rff2/attr/ShdToneMapMethod.h)
