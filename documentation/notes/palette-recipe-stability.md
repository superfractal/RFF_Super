# Palette Recipe Stability

Recipe-eligible palettes (`getPaletteRecipeId() >= 0`: `LongRandom64`=1, `LongRandom64_2`=2,
`LongRainbow7`=3) save as `{presetId, seed}` and regenerate on load via
`genPalette()` — so the file couples to that code.

- **Freeze recipe presets' `genPalette()`** (RNG draw count/order, base count, expansion, blend math).
  Trailing param assignments are safe (stored separately).
- Id `4` was the retired KFR Banded palette: it is off the Palette menu, its colors are still drawn by
  `retiredKFRBandedColors()` so files saved with it load unchanged, and the id stays taken.
- **Never reuse/renumber a recipe id**; new recipe presets get the next unused number + a `case` in `regenerateRecipeColors`.
- Adding non-recipe presets is free (default id `-1` = raw storage).
- Keep `rff_math::random_*` as fixed `gen()` arithmetic (not `std::uniform_*_distribution`).
