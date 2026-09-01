# Project Overview

**RFF-2.0** is a fast Mandelbrot set rendering application, built with C++20, the Vulkan API, and GMP.

- **Fast-period-guessing (FPG):** Resolves the current view's maximal period automatically (fixed, not user-editable).
- **Multilevel Periodic Approximation (MPA):** Skips directly to the periodic point. **This is NOT BLA** — MPA derives its skip levels from the reference orbit's *periodic structure* (see `MPAPeriod`), not from per-iteration reference magnitude. Skip lengths are variable and tied to period levels. Do not refer to it as BLA.
- **Reference Compression:** Compresses the approximation tables, heavily reducing RAM usage and skipping table-creation work.
- **Perturbation Theory:** Light and Deep perturbators compute pixel values around a high-precision reference orbit.

## Project Structure

**Core (`src/rff2/`):**

- `formula/` - Fractal computation math (`DeepMB2Perturbator`).
- `mrthy/` - Multilevel Periodic Approximation logic (`MPAPeriod`, `LightPAGenerator`).
- `vulkan/` - Graphics/Compute pipelines (`GPC*` graphics, `CPC*` compute): palettes, blurs, blooms, fractals.
- `ui/` - Win32 UI, window callbacks, and `RenderScene`.
- `io/` - Maps, videos, and location saving/loading.
- `locator/` - Minibrot finding algorithm.

**Vulkan framework (`src/vulkan_helper/`):** object-oriented wrapper around Vulkan (devices, swapchains, pipelines, buffers, command execution).

**Shaders (`shdsrc/`):** GLSL shader files.

## Guidelines

- **Precision:** Perturbation equations are highly sensitive to floating-point drift. *Never* trade precision for speed.
- **Memory:** Array compressors are heavily used — storing billons of iterations in memory is prohibitive. Verify rendering stays crisp, free of pixelation or visual glitches.
- **Vulkan:** Descriptor sets, push constants, and pipeline layouts must match exactly between the C++ pipeline configurator and the GLSL shaders.
- **Comments:** Keep explanatory comments to a SINGLE line. This does NOT apply to the `// Modified by ...` modification tag. Do NOT modify comments that already exist — this rule applies only to comments you add or write yourself.

## Attribution, Licensing, and Low-Risk Provenance

`NOTICE` is the canonical record for third-party components and named algorithms. Keep it in sync
with source comments whenever shader, palette, noise, hashing, dithering, tone-mapping, or color-space
code is added or changed.

- Find the earliest practical original source and its actual license before adapting a distinctive
  formula or constant set. Do not infer the license from a hosting site's default terms.
- Put a one-line source/author/license comment beside recognizable implementations. Add a matching
  `NOTICE` acknowledgement when the technique has a named author or publication.
- Generic textbook operations normally need no attribution. Still record a source if code or
  distinctive constants were actually copied.
- Mathematical or very short idioms can be low copyright risk without being provenance-free. Record
  the known origin when practical, and replace an uncertain implementation with a clearly licensed one
  when that is cheap.
- When third-party-derived material is duplicated between implementations or documentation, keep an
  attribution at each primary implementation or add an explicit pointer to `NOTICE`.
- Use technical names only to identify implemented functionality, distinguish approximations from full
  standards, and never imply certification or trademark approval.
- Implement file-format compatibility independently; a public format name or documented field does not
  grant permission to copy another program's source.

For GPL source releases, every modified work must carry a modification notice and a relevant date.
If the historical date is unavailable, state that uncertainty and add the current modification or
release date. Build source archives only from a committed tag; never ZIP the working directory because
ignored folders can contain third-party binaries, headers, and build products.

## File-Format Stability (file couples to code — read before editing)

- **Palette recipes** save as `{presetId, seed}` and regenerate on load: freeze `genPalette()`
  RNG/order, never reuse/renumber recipe ids. See [docs/palette-recipe-stability.md](docs/palette-recipe-stability.md).
- **ConfigIO** is a flat ordered binary stream: add settings append-only at EOF in fixed order,
  guard each trailing field on load with `peek() != eof`, bump `VERSION` only for non-additive
  changes. See [docs/config-file-format.md](docs/config-file-format.md).
- **Compressed maps** (`.rfmz`) carry a fixed versioned header naming the stream mode: never
  renumber a mode, never make one lossy (video keyframes read this path), and keep `streamSize` in
  step with `preprocess`. See [docs/compressed-map-format.md](docs/compressed-map-format.md).

## Changelog

[CHANGELOG.md](CHANGELOG.md) is written to one fixed style: present tense, about the program rather
than the work, fixes phrased as the result and never as their internal cause, and the program always
called RFF_Super. Read [docs/changelog-style.md](docs/changelog-style.md) before adding an entry.

## Debug / Test Programs

Any throwaway debugging or test program (CPU verification scripts, scratch tools, one-off
experiments, etc.) MUST be created inside the `debug/` folder, which is gitignored

## Code Modification Tracking

Whenever you (the AI) modify or add code, insert a dated modification tag (e.g. `// Modified by Sonnet 4.6 on 2026-08-23`):

- Place it BELOW any existing leading comments (header/copyright block), never above them.
- If the file has no leading comment, place it at the top of the file.
- If a tag for the same model and version already exists in the file, do NOT add another one.
- Append every later change date to the existing tag as an individual ISO `YYYY-MM-DD` date.
- Never abbreviate dates with a range or ellipsis such as `..` or `...`; list every date explicitly, separated by commas.
- For OpenAI models, use only the GPT version name (for example, `GPT-5`) in modification tags; do not use variants such as `GPT-5.x` or `Codex GPT-5`.

```cpp
// Created by Fractal
// Modified by Sonnet 4.6 on 2026-08-23
#pragma once
```
