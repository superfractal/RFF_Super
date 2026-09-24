<!-- Created by GPT-6 on 2026-09-24. -->
<!-- Modified by GPT-6 on 2026-09-25. -->
# Feature coverage map

[Manual](user-manual.md) · [Shader coverage audit](shader-coverage.md) · [UI gallery](ui-gallery.md)

This checklist maps current menu actions and workspace-only features to the manual. It is a documentation inventory, not a claim that every action was executed. Dynamic preset entries and conditional Debug commands are identified separately. Source: [SettingsMenu.cpp](../src/rff2/ui/SettingsMenu.cpp) and [Application.cpp](../src/rff2/ui/Application.cpp).

## File

| Menu action | Description |
| --- | --- |
| New Document | [Detailed workflow](workspace-and-files.md) |
| Save Settings | [Detailed workflow](workspace-and-files.md) |
| Save Map | [Detailed workflow](workspace-and-files.md) |
| Save Image | [Detailed workflow](workspace-and-files.md) |
| Save Location / Settings | [Detailed workflow](workspace-and-files.md) |
| Load Map | [Detailed workflow](workspace-and-files.md) |
| Load Image | [Detailed workflow](workspace-and-files.md) |
| Load Location / Settings | [Detailed workflow](workspace-and-files.md) |

## Fractal

| Menu action | Description |
| --- | --- |
| Reference | [Detailed workflow](exploration-and-calculation.md) |
| Iterations | [Detailed workflow](exploration-and-calculation.md) |
| MP-Approximation | [Detailed workflow](exploration-and-calculation.md) |
| Automatic Iterations | [Detailed workflow](exploration-and-calculation.md) |
| Absolute Iteration Mode | [Detailed workflow](exploration-and-calculation.md) |
| Formula | [Detailed workflow](exploration-and-calculation.md) |
| Projection | [Detailed workflow](exploration-and-calculation.md) |

## Render

| Menu action | Description |
| --- | --- |
| Render Properties | [Detailed workflow](exploration-and-calculation.md) |
| Rendering FPS | [Detailed workflow](exploration-and-calculation.md) |
| Linear Interpolation | [Detailed workflow](exploration-and-calculation.md) |
| Boundary Trace Fill | [Detailed workflow](exploration-and-calculation.md) |
| 2-Color Preview Mode | [Detailed workflow](exploration-and-calculation.md) |
| Coarse Preview | [Detailed workflow](exploration-and-calculation.md) |
| Smooth Zooming | [Detailed workflow](exploration-and-calculation.md) |
| Dither | [Detailed workflow](exploration-and-calculation.md) |

## Shader

| Menu action | Description |
| --- | --- |
| Palette | [Detailed workflow](shader-overview.md) |
| Texture | [Detailed workflow](shader-overview.md) |
| Pattern | [Detailed workflow](shader-overview.md) |
| Warp | [Detailed workflow](shader-overview.md) |
| Stripe | [Detailed workflow](shader-overview.md) |
| Slope | [Detailed workflow](shader-overview.md) |
| Color | [Detailed workflow](shader-overview.md) |
| Effects | [Detailed workflow](shader-overview.md) |
| Fog | [Detailed workflow](shader-overview.md) |
| Bloom | [Detailed workflow](shader-overview.md) |
| HDR | [Detailed workflow](shader-overview.md) |
| Shader Layers | [Detailed workflow](shader-overview.md) |
| Load KFR Color | [Detailed workflow](shader-overview.md) |
| Import Color | [Detailed workflow](shader-overview.md) |
| Local AI appearance | [Detailed workflow](local-ai.md) |
| Save Shader Preset | [Detailed workflow](shader-overview.md) |
| Load Shader Preset | [Detailed workflow](shader-overview.md) |

## Preset

| Menu action | Description |
| --- | --- |
| Calculation presets | [Detailed workflow](presets-and-recovery.md) |
| Render presets | [Detailed workflow](presets-and-recovery.md) |
| Resolution presets | [Detailed workflow](presets-and-recovery.md) |
| Shader: Palette, Stripe, Slope, Color, Fog, Bloom | [Detailed workflow](presets-and-recovery.md) |
| Shader: Example files available on this installation | [Detailed workflow](presets-and-recovery.md) |

## Video

| Menu action | Description |
| --- | --- |
| Data Settings | [Detailed workflow](animation-and-export.md) |
| Animation Settings | [Detailed workflow](animation-and-export.md) |
| Timeline Editor | [Detailed workflow](animation-and-export.md) |
| Export Settings | [Detailed workflow](animation-and-export.md) |
| Generate Video Keyframe | [Detailed workflow](animation-and-export.md) |
| Export Zooming Video | [Detailed workflow](animation-and-export.md) |

## View

| Menu action | Description |
| --- | --- |
| Language / 言語 (restart required): English / Japanese | [Detailed workflow](workspace-and-files.md) |
| Show Setting Descriptions | [Detailed workflow](workspace-and-files.md) |
| Use Legacy Settings UI | [Detailed workflow](workspace-and-files.md) |
| Dark Mode | [Detailed workflow](workspace-and-files.md) |

## Explore

| Menu action | Description |
| --- | --- |
| Recompute | [Detailed workflow](exploration-and-calculation.md) |
| Cancel | [Detailed workflow](exploration-and-calculation.md) |
| Reset | [Detailed workflow](exploration-and-calculation.md) |
| Find Center | [Detailed workflow](exploration-and-calculation.md) |
| Locate Minibrot | [Detailed workflow](exploration-and-calculation.md) |

## Debug

| Menu action | Description |
| --- | --- |
| Dump Scene State | [Detailed workflow](presets-and-recovery.md) |
| Measure GPU Pass Times | [Detailed workflow](presets-and-recovery.md) |
| Show GPU Pass Times | [Detailed workflow](presets-and-recovery.md) |

## Help

| Menu action | Description |
| --- | --- |
| Version | [Detailed workflow](presets-and-recovery.md) |

## Workspace and editor features

| Feature | Documentation |
| --- | --- |
| Search, keyboard focus, pending edits, validation, Undo/Redo | [Workspace](workspace-and-files.md#workspaces-sections-and-pending-changes) |
| Surface Basic/Detail, All/Used/Favorites, group reset, layer visibility and ordering | [Workspace](workspace-and-files.md#surface-navigation-and-shader-layers), [all shader groups](shader-overview.md) |
| Docking, extra panels, dimensions, restore layout | [Panel layout](workspace-and-files.md#panel-layout) |
| A/B, before-last-edit, split, fixed comparison time | [Comparison](workspace-and-files.md#compare-appearances-fairly) |
| Folder browsing, numbered map jumps, image viewer | [File browsing](workspace-and-files.md#browse-maps-and-images) |
| Local model setup, appearance proposals, refinement, undo, AI zoom and locator retries | [Local AI](local-ai.md) |
| Color motion, freeze picking, preview transport | [Color animation](animation-and-export.md#color-motion-and-frozen-colors) |
| Source generation, compression, padding, PNG compatibility | [Keyframes](animation-and-export.md#generate-source-keyframes) |
| Timeline tracks, keys, interpolation, speed, holds, camera | [Timeline editing](animation-and-export.md#add-and-edit-parameter-tracks) |
| Audio enable/gain; JSON clip trim, placement, fades, mute | [Audio](animation-and-export.md#audio) |
| Zoom text, font, colors, outline, shadow, position, Shorts guides | [Overlay](animation-and-export.md#zoom-overlay-and-preview-guides) |
| Timeline RAM preload, layout, keyboard navigation, JSON/RFVT save/load | [Timeline](animation-and-export.md) |
| AI Edit bundle, 2x2/3x3 sheets, cancellation, JSON import | [Timeline AI Edit](animation-and-export.md#timeline-ai-edit) |
| Image/video paths, resolution, encoding, antialiasing, HDR, progress and cancellation | [Export](animation-and-export.md#still-image-export) |
| Recovery restore/generate, restore/wait, location-only, ignore | [Recovery](presets-and-recovery.md#session-recovery) |

## Scope and evidence

The UI inventory contains 15 production form definitions, 59 nonempty form-group images, 478 field rows (including each repeated texture/pattern/material layer), two Local AI tabs, and one timeline editor image. Some application-attached actions are described in the workflow chapters rather than captured in the standalone forms. The [existing shader audit](shader-coverage.md) additionally covers all 27 Surface inspector groups and all 29 compositing slots.

The [field reference](settings-reference.md) explains individual values and dependencies; the [UI inventory](ui-inventory.json) records displayed field IDs, labels, choices, and sample values. No claim is made that every slider has its own before/after render. There are 54 measured render comparisons, and the other workflows are documented from current code.
