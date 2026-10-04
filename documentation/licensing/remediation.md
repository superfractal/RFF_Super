<!-- Modified by GPT-6 on 2026-10-03, 2026-10-04. -->
<!-- Modified by Opus 5.5 on 2026-10-04. -->
# RFF_Super licensing remediation

The available source and notice repairs are applied to **eight code files**, with **five full license texts** added to both source and distribution notice folders. MFR-HDR, Lustre and the owner's commissioned AI material remain unchanged. This report supersedes the earlier audit's status for these changes; it does not claim that all historical permissions or distribution questions are cleared.

## Applied repairs

| Area | Result |
| --- | --- |
| JSON detection, B09 | The detector core is adapted from the official Boost.TypeTraits 1.88 source under BSL-1.0, with Glen Joseph Fernandes's copyright and the full grant retained. Existing detection aliases remain. |
| JSON ADL/endian, B10 | The unused Stack Overflow-derived ADL tag-probe is removed. Ordinary standard/ADL range lookup remains. C++20 `std::bit_cast` replaces the endian byte-pointer helper and preserves its supplied-integer interface. The ObjectType language technique remains, with its author/editor, source link and historical limitation recorded. |
| RFC explanatory prose, B11 | Both copied explanatory-prose extracts are removed. Binary16 decoder code, existing source links and the IETF BSD grant remain. |
| IGN, A02 | All three implementations are adapted anew from Alan Wolfe's MIT-licensed IGNLDS expression. Source credits and the full MIT notice are installed. GLSL `fract`, the existing seed, coefficients and operation order remain. Jimenez's origin credit remains distinct from Wolfe's license. |
| Hable curve, B01 | The implementation is adapted anew from Sascha Willems's MIT-licensed Uncharted2Tonemap. Source-file and repository copyright notices and the full grant are installed. Coefficients, operation order and RFF_Super headroom normalization remain. |
| Cosine palettes, B02 | All three primary generators identify Eric Arnebäck's MIT-licensed equation counterpart. Its full upstream grant is retained. Quilez's origin credit remains. Generator code, random draws and recipe ids are unchanged. |
| Rainbow stops, B04 | Both primary generators identify the inherited RFF GPL table and ChrisBuilds's matching MIT-licensed seven-stop table. The exact upstream notice is retained. Local floating-point stops and recipe behavior are unchanged. |
| Half-offset sampling, A01 | Source and NOTICE explain the midpoint in squared radius of equal-area annuli. Vogel's angular arrangement is credited separately; the half-offset is not falsely assigned to the 1979 paper. Sampling code is unchanged. |
| Norm approximation, B03 | Both implementations record their inherited original RFF GPL provenance. The 2026-10-04 follow-up identifies Edgar Bonet's 2014 mathematical publication and follows the original RFF addition to May 2025. The actual earlier acquisition chain remains unestablished; the coefficient is preserved. |
| Vulkan memory selection, B08 | The primary implementation records the conventional reference and distinguishes the tutorial's CC0 code listings from its CC BY-SA prose. The inherited loop is unchanged. |
| Project images, B12 | Both current icons match the pinned original GPL repository byte for byte. On 2026-10-04 the owner confirms that they created the README showcase image themselves. The original GPL source and owner authorship statements address the recorded creator/source concern. |

The five added grants are in `extern/licenses/` and `third-party/licenses/bundled/`: `boost-type-traits-BSL.txt`, `alan-wolfe-ign-MIT.txt`, `sascha-willems-tonemapping-MIT.txt`, `erkaman-cos-palette-MIT.txt` and `terminaltexteffects-rainbow-MIT.txt`. Their hashes are appended to `third-party/SHA256SUMS.json`; all 287 earlier entries remain unchanged. The existing release collector copies each full grant from `extern/licenses/` into the binary notice folder. Canonical source/author/license mappings are in [NOTICE](../../NOTICE).

## Owner clarification and exclusions

On 2026-10-03 the owner states that the original project author created the `res` assets and that guide/README images outside `res` are AI-generated. The owner requested excluding MFR-HDR and their commissioned AI material from repair. This clarification is recorded as an owner statement; it is not recast as an invented third-party grant.

MFR-HDR (B06), the commissioned Lustre/material inputs (B07), and the AI guide/README imagery covered by B13 remain unchanged. The commissioned artistic/noise policy portions of B05 are also left unchanged; existing third-party hash/noise acknowledgements remain. The clarification does not establish every possible earlier generation input. The historical guide source-map inputs are preserved, and no permission is fabricated for them.

On 2026-10-04 the owner confirms that they created `res/readme-showcase.jpg` themselves. This supersedes their earlier tentative statement about copying it for the README and resolves this image's unidentified third-party creator/source concern on the owner's statement. The image remains unchanged, and no separate third-party license is invented or requested for it. The other `res` assets retain the previously recorded original-author provenance.

## Verification and limits

The focused C++20 compile-only check passes for the replacement detection aliases, valid/invalid type substitution, standard containers, arrays, ADL ranges and JSON binary-format interfaces. It produces no executable and runs no implementation. This is an interface compatibility check for the source repair; it is separate from the completed licensing comparison audit.

Source checks establish that palette, norm and memory-loop tokens are unchanged; after substituting renamed local coefficients, the complete shader code tokens are unchanged. Descriptor declarations and unrelated shader code remain identical. In JSON, executable-source changes are confined to the detector, ADL macro and endian helper, plus two required standard includes. RFC prose removal and additional notices do not change decoder operations. All five grants match the retained original license bytes and their two installed copies and manifest hashes. Nine retained source/license files match the pinned Git tree blobs. Both icons match the original Git blobs and current asset bytes. All 179 present historical assets remain unchanged, as do the checked MFR/Lustre sources and guide documents.

Evidence is retained under `debug/license-remediation-20261003/`, including the original local source backups, `verified-source-pairs.json`, `source-remediation-record.json`, `grant-installation-record.json`, `icon-correspondence.json` and `remediation-verification.json`. This new repair snapshot supplements the historical 930-file audit; that audit is not rerun or represented as a hash check of the subsequently changed source.

Historical acquisition, earlier author permission and first-creator questions are not all solved by adding a licensed counterpart or mathematical explanation. In particular, the retained ObjectType technique and earlier palette/coefficient history retain explicit limits. The showcase-image creator concern is resolved by the owner's 2026-10-04 clarification. No blanket rights-clearance conclusion follows from these edits. The earlier count of 15 is a historical count of grouped questions, not a count of confirmed violations or a new completion percentage.

No numerical, rendering, UI or application/runtime tests run for this repair. No EXE, shader binary, release bundle or source archive is rebuilt. Existing source/binary correspondence and static GMP source questions remain release findings. Source archives must be built from a committed tag, as required by the project instructions.

The [2026-10-04 history and rebuild follow-up](history-followup.md) adds the earlier coefficient publication, the October 2023 MIT rainbow table, original RFF rename/introduction records and a current GMP hash check. The prepared distribution GMP library matches the recorded official package; the installed default library does not. These findings qualify the remaining history questions and identify a concrete release build path.

The later [2026-10-04 archive investigation](archive-investigation.md) establishes Quilez's own full MIT grant for shader ll2GD3 and installs it as a sixth grant, `iquilez-palettes-MIT.txt`, in both license locations with one new manifest entry. The original five-grant verification above remains the historical repair record. Dated comment clarifications preserve existing comments and implementation tokens. ObjectType's separate MIT permission and rainbow's first creator/actual donor remain unestablished. The owner's independent-discovery explanation for `0.428` is recorded; the earlier Bonet match does not demonstrate copying.

The subsequently authorized ObjectType replacement changes the forward declaration and definition macro to a single standard type pack. Both headers pass the same focused C++20 compile check. Historical attribution is preserved alongside the replacement's dated standard-language basis; no new author grant or blanket historical clearance is asserted.

The owner subsequently directs [restoring the complete original upstream JSON header](owner-disposition.md) and setting aside Rainbow and historical ObjectType permission. The header is now byte-identical to upstream v3.12.0. All local JSON repairs, annotations, the ObjectType replacement and older functional patches are reversed; the records above describe historical work. Other repairs remain. Those two historical questions receive no further action within this task by the owner's decision.
