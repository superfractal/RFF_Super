<!-- Modified by GPT-6 on 2026-10-07. -->

# Export format review — reconciled

The full 937-line `palette/core/exportFormats.ts` was read in three consecutive, untruncated chunks. Saving the main report and verifying the inventory hash could not proceed because shell process creation failed with `CreateProcessWithLogonW: 1909`. Reconciled on 2026-10-07: the source SHA-256 matches the inventory, findings are integrated into GMT_to_RFF_Shader_Adoption_EN.md, and coverage.json now records this file as read-in-full.

## RFF adoption

Use a capability-based exporter that distinguishes continuous gradients, discrete swatches, single files and collections. This transfers directly to RFF's 2D palettes and can also exchange environment gradients with 3D tools. It remains lower priority than improvements to RFF's own environment reflection and palette filtering.

| GMT export family | Actual representation | RFF requirement |
| --- | --- | --- |
| MAP, hex, arrays, CSV, color-list JSON, GPL | Rounded RGB values, generally 256 samples | Declare display encoding; retain recipes separately |
| GGR/CPT | 255 linear segments with decimal coordinates | Evaluate coordinate rounding and special-color/domain semantics |
| CSS/SVG/GRD/AI/IDML/UGR | Reduced stops with target-specific coordinates | Measure error after serialization and destination interpolation |
| ASE | Discrete colors with no original stop positions | Do not describe it as an equivalent continuous gradient |
| Tokens/CSS variables/Tailwind-shaped fragment | Eleven evenly sampled colors for ramp exports | Disclose loss of narrow color bands |
| Paint.NET text | Ninety-six evenly sampled colors | Report sampling loss despite having no stop-budget control |
| Blender/Cinema 4D scripts | Reduced stops passed to script builders | Validate destination limits and color conversion |

## Reduction and diagnostics

The reducer splits at the largest encoded-byte RGB deviation from a line between rounded endpoints. It always retains samples 0 and 255, starts at tolerance 1.5 and multiplies tolerance by 1.35 until the requested budget is met or forty retries expire. This is adaptive simplification, not global minimum-stop fitting or a perceptual-error guarantee. A large budget still begins with simplification. Validate finite bounded 256-sample input and budgets of at least two.

The warning metric measures maximum RGB Euclidean error only on the original sample grid. Its threshold 24 is an application heuristic, not OKLab Delta E or a universal visibility threshold. CSS/SVG then round positions to 0.1 percent, GRD to integer 0..4096, and UGR to integer 0..399; GGR/CPT also round coordinates. RFF should serialize, decode and compare source samples plus edge neighborhoods and the cyclic seam.

ASE single-gradient export accepts a budget override, but ASE collection export uses the fixed ASE_MAX value. The collection warning can therefore evaluate a different conversion from the file actually written. Fixed eleven-/ninety-six-sample exports also lose information without a stop-reduction warning. RFF should derive preview, diagnostics and output from one conversion result. stopBudgetOf does not enforce finite overrides or destination hard caps; validate those in the writer itself.

## Precision, names and interoperability

GRD writes opaque transparency stops and expands byte RGB values by 257 into sixteen-bit fields, without restoring original precision. It truncates a UTF-8 Pascal name at a character boundary, but its encoding compatibility still requires destination validation. ASE stores RGB floats after byte rounding, writes UTF-16BE names, and lacks a local bound preventing its sixteen-bit name-unit count from overflowing.

AI uses an elementary RGB-to-CMYK fallback rather than ICC-managed print conversion. Name escaping handles parentheses and backslashes but not all control characters in line-oriented records/comments. AI/ASE duplicate-name suffixing can collide with an existing suffixed name. UGR sanitizes and truncates names again inside the block builder, potentially removing a generated uniqueness suffix. Allocate unique names after final truncation/encoding.

UGR retains endpoints at 0 and 399 and packs red into the least-significant byte. This avoids a broad unspecified wrap interval, but unequal endpoint colors still produce a cyclic seam. A ring representation does not automatically create continuity. Preserve explicit clamped/repeated semantics and assess target-grid edge reconstruction.

Token/CSS-variable exports keep a display name separate from the sanitized identifier but omit source positions, bias, interpolation, alpha and the original color-space contract. Eleven colors named 50..950 are not necessarily monotonic in lightness or accessible contrast. The JSON exporter writes a colors array, not the exact gradient-document form. Several exported extensions are not accepted by the inspected import suite, so export coverage is not symmetric import support.

Compatibility claims and historical measurements in source comments were not rerun. Do not treat application stop-budget defaults as universal format limits. An independent RFF implementation needs original format specifications, provenance/license checks and destination fixtures. No external application, export test harness or rendered comparison was run in this batch.

