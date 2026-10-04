<!-- Modified by GPT-6 on 2026-10-03. -->
# Palette source and permission follow-up — 2026-10-03

B02 and B04 remain unresolved, with stronger published source and license evidence. This pass compares five complete current palette generators, including both LongRainbow7 callbacks, with six complete public function bodies. It does not complete the whole palette preset file or establish the actual donor. The repository audit still has 15 grouped unresolved questions and zero confirmed violations.

## Cosine palettes: B02

[Erkaman/glsl-cos-palette](https://github.com/Erkaman/glsl-cos-palette/tree/2183c2352891f08afe6b7f27e29119a6e9ca78ec) publishes the short vector cosine function and an actual [MIT grant](https://github.com/Erkaman/glsl-cos-palette/blob/2183c2352891f08afe6b7f27e29119a6e9ca78ec/LICENSE). Its five retained commits lead back to the May4 2016 initial commit, 572a42e09c565f29f8ba769914d2b7681a64f84e. That initial revision already has the same function and MIT text. The retained grant begins with an MIT declaration and contains permission, notice-preservation and warranty terms, but does not name a copyright holder. The README credits Inigo Quilez and links his palette article. This repository's statement is not evidence of an express grant from Quilez for every preceding source or preset tuple.

RandomSmooth uses the same mathematical construction, expanded into scalar RGB with local random frequencies and phases, 200 samples, clamping and attribute settings. RandomSmoothShort delegates to it and changes the interval. GlossySunset uses the same cosine form with custom phases and channel adjustments; its retained former-tuple comment differs from active parameters. None of these complete current generator bodies matches the published five-parameter, single-return function as a whole.

The exact RandomSmooth signature first appears at the current path in retained all-ref Git history at 7e5af23e745cf5f7f5414726ff099bc847928f0f, July5 2026. The full introduced file, diff and search output are retained. This establishes a local introduction, not a donor, earlier rename history or independent derivation. Quilez's article retrieval was blocked by robots, and the selected Shadertoy retrieval failed with402; no original author grant was obtained from either. Secondary copies and guessed author accounts are not permission evidence.

## Seven rainbow stops: B04

The ordered rounded colors are E81416, FFA500, FAEB36, 79C314, 487DE7, 4B369D and 70369D. Both current generators and both complete original RFF generators contain them. [Original RFF at cc0fe08](https://github.com/Merutilm/RFF-2.0/blob/cc0fe08c63236af48f4e761f7f87d8a77963c4be/src/rff2/preset/shader/palette/ShdPalettePresets.cpp) also contains LongRainbow7's two expansions and random modulation, and Rainbow's interval/offset/smoothing settings. Its actual GPLv3 license is retained. That project license does not independently establish third-party selection history.

The exact first float appears in retained all-ref history at the current path in c58986a40ac4d8a89cb1fa2b98b8a7e5966dc6cd, July5 2026. The introduced file and diff contain the seven-value sequence. It was present before the recent fog changes; finding a source question during this audit does not show fog introduced the rainbow material.

[TerminalTextEffects](https://github.com/ChrisBuilds/terminaltexteffects/blob/14d34ec1a4ca0273bd2c30ad86c2e5562af6cc44/terminaltexteffects/effects/effect_bubbles.py) contains the same ordered seven HEX stops inside the complete BubblesIterator initializer. Its April5 2026 file snapshot predates that selected local introduction. The current October2 revision also retains them. Both revisions carry the same actual [MIT license](https://github.com/ChrisBuilds/terminaltexteffects/blob/14d34ec1a4ca0273bd2c30ad86c2e5562af6cc44/LICENSE), naming ChrisBuilds and2023 with brackets in the original notice. Its terminal state, five-step gradient and build call differ from RFF's generators. This establishes a licensed published counterpart, not the earliest seven-color author, first TTE addition, or the RFF donor.

[PaletteMaker's rainbow page](https://palettemaker.com/rainbow-colors) repeats the sequence and displays a rights-reserved footer. No express table/code reuse grant was established from the retained page. Its [About page](https://palettemaker.com/about) identifies Tanya K and app creation in2022; it does not identify who selected these seven values or when. Free worksheet downloads are not a blanket implementation license. Raw HTML and derived text are retained without executing scripts or downloading images.

## Scope, preservation and next work

The public MIT grants strengthen the evidence for both questions, but the audit must retain the uncertainty about acquisition and earlier rights. No infringement or independent-origin conclusion follows solely from a mathematical match, shared constants, or a failed finite search. B02/B04 each remain one grouped question; A03 remains informational and excluded from the15.

The saved review contains exact source fragments, hashes, parser ranges, actual grants, publication metadata and selected Git introductions. All928 inventoried source hashes, NOTICE and LICENSE remain unchanged. Saved recipes retain their IDs, generation and RNG order. No application or public implementation was run, and no behavior/numerical tests were performed. The historical695/928 and373/5597 counters are unchanged;110 files without any comparison entry and additional partial scopes remain. This pass supplies no defensible overall completion percentage.

Continue distinctive-source/acquisition questions across the other open entries, then compare all remaining implementations and partial scopes. A final audit may report unresolved permissions; it must still complete the requested implementation comparison coverage.
