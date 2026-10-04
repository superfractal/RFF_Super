<!-- Modified by GPT-6 on 2026-10-04. -->
<!-- Modified by Opus 5.5 on 2026-10-04. -->
# Remaining source history and rebuild findings

This requested follow-up establishes an earlier published source and derivation for `0.428`, an earlier MIT-licensed rainbow table, the original RFF introduction/rename history, and the exact GMP library that can be used for a recorded rebuild. It does not establish private acquisition records or declare every historical permission cleared. No implementation, application or numerical test runs, and no release is built.

## ObjectType

[Lol4t0's answer 9860911](https://stackoverflow.com/a/9860911), posted March 25, 2012, demonstrates a template-template parameter with two named type parameters and a trailing parameter pack. Its complete example uses a recursively defined `SomeType`, `std::unordered_map` and `std::unique_ptr`. The retained official API records identify CC BY-SA 3.0 and the editor strcat.

[Niels Lohmann's January 31, 2015 commit befd90d](https://github.com/nlohmann/json/commit/befd90dead2f9ce42b5c7fdd7cf2e0e0db2dd226) introduces the cited ObjectType declaration in the selected JSON release ancestry. The commit explicitly credits the answer. JSON uses its own `basic_json` type and `std::map`, with its own later comparator/allocator arguments; it does not contain the answer's complete smart-pointer example. The shared template-parameter syntax implements a [standard C++ language mechanism](https://eel.is/c++draft/temp.arg.template).

The new exact-answer-ID upstream issue search returns one issue, [#94](https://github.com/nlohmann/json/issues/94), concerning static-analysis warnings. Its historical copied header is already recorded; it does not establish separate author permission. The search is finite and not evidence that private permission never existed. No author-specific MIT grant is established in the inspected records. The existing source credit and NOTICE remain applicable. This is a narrow language-technique/acquisition question, not evidence that the entire JSON library was copied under an incompatible license. No blanket relicensing or infringement conclusion is made.

## Cosine palettes

The previously verified [Erkaman/glsl-cos-palette initial revision](https://github.com/Erkaman/glsl-cos-palette/tree/572a42e09c565f29f8ba769914d2b7681a64f84e), May 4, 2016, already contains the cosine equation and its actual MIT grant. The README credits Iñigo Quilez. That supports a licensed mathematical counterpart, not an express license for all content in Quilez's article.

The retained local history places RandomSmooth at its current path in July 2026. The original RFF palette source inspected in this pass does not contain RandomSmooth. The local generators expand the equation into randomized channel settings and additional project policy; they do not reproduce the complete published GLSL function. The five-generator comparison and recipe-preservation record remain in [the earlier palette follow-up](palette-origin.md).

No new original-author grant is obtained. The current primary article URL fails retrieval with HTTP 404 in this pass; web search is also blocked by the site's robots policy. Neither result proves that no grant exists. The actual local donor and earlier article permission remain unestablished. Existing MIT counterpart notices are retained, and palette generation is unchanged.

## Seven rainbow stops

The ordered table is E81416, FFA500, FAEB36, 79C314, 487DE7, 4B369D and 70369D.

The exact table appears in [ChrisBuilds's October 6, 2023 addition of effect_bubbles.py](https://github.com/ChrisBuilds/terminaltexteffects/blob/b3b3d9f9dc81e57decfb97c197e38f99fcbbf5d3/terminaltexteffects/effects/effect_bubbles.py). The actual [LICENSE at that same commit](https://github.com/ChrisBuilds/terminaltexteffects/blob/b3b3d9f9dc81e57decfb97c197e38f99fcbbf5d3/LICENSE) is MIT, with the same `Copyright (c) [2023] [ChrisBuilds]` text as the already installed grant. This is earlier evidence than the April 2026 counterpart retained in the previous audit.

The original RFF palette file is added in [commit c0633b2, May 27, 2025 UTC](https://github.com/Merutilm/RFF-2.0/commit/c0633b288c64d1dec0cee8343d79474f96450f24), and already contains the rounded seven-stop floats. Subsequent June/August path changes are renames/refactors, not first creation of the table. Its presence therefore predates the local July 2026 import and the later fog changes. These are Git record dates, not independent verification of when private work began.

The 2023 MIT table predates the original RFF addition, but neither repository proves the table's first creator or the actual RFF acquisition route. The matching licensed table and original GPL inheritance are now documented more precisely. Local floats, saved recipe ids and RNG order remain unchanged.

## The 0.428 coefficient

[Edgar Bonet's answer 26607206](https://stackoverflow.com/questions/3506404/fast-hypotenuse-algorithm-for-embedded-processor/26607206#26607206), October 28, 2014, already publishes the quadratic approximation with coefficient `0.428`. A May 25, 2017 revision explains scaling by the longer side and manually tuning a quadratic approximation to reduce maximum error. Both revisions retain the coefficient; the official API assigns CC BY-SA 3.0. This establishes a known mathematical publication and author, not an MIT grant or the project's actual donor.

[Original RFF commit 959b66b, May 12, 2025 UTC](https://github.com/Merutilm/RFF-2.0/commit/959b66b049d06638553a16a40e229c102ca8fcef) adds `src/merutilm/rff/approx/ApproxMath.h` with the exact expression now inherited locally: `max + 0.428 * min / max * min`. Its leading creation comment says May 9, 2025; the Git addition date is distinct from that stated creation date. The inspected path history follows renames through `approx_math.h`, `rff_math.h`, and the later `src/rff2/` path. The coefficient is already present in that initial selected file addition, with no donor reference in its comment or commit message.

The expression is quadratic in the ratio of the shorter to longer side; it is not the different linear approximation `max + 0.428 * min`. Bonet's form is algebraically equivalent, but the local multiplication/division order differs and is preserved. No approximation-accuracy or application-behavior claim is tested here. The former absence of a known matching publication is resolved; the original author's actual acquisition/permission chain remains unestablished. Source comments and NOTICE now record that distinction.

## What rebuilding resolves

**A recorded clean rebuild can resolve source/binary correspondence for the newly produced binaries.** Build the intended source commit from a clean checkout with a release tag, rebuild the shaders from that same source, record compiler/options/dependency paths and output hashes, and provide the matching application source, dependency source/patches, build scripts and notices. The project's instructions require creating the source archive from the committed tag. A normal rebuild of a dirty working directory does not provide that release record. A new build also does not establish correspondence for older EXEs that continue to be distributed.

**Static GMP need not necessarily be rebuilt itself.** The project can use an exact verified official binary package with its corresponding source and packaging recipe/patches. Alternatively, build GMP from recorded source and retain its exact patches, configuration and build record. Rebuilding only RFF_Super while reusing an unidentified GMP library leaves the static-library question open. GMP expressly offers GPLv2-or-later/LGPLv3-or-later alternatives; this project's distribution policy selects GPLv3. [Official GMP copying conditions](https://gmplib.org/manual/Copying). Corresponding-source requirements are recorded in [GPLv3 sections 1 and 6](https://www.gnu.org/licenses/gpl-3.0.html).

The read-only library check on 2026-10-04 finds:

| File | SHA-256 | Correspondence |
| --- | --- | --- |
| `build/release-deps/mingw64/lib/libgmp.a` | `cea4c8ae34263d44e982ff5f7a3a6a38095d539045c0e50e9ee462c5d83784f5` | Matches the installed official MSYS2 GMP 6.3.0-2 package's recorded file hash. |
| `C:/msys64/mingw64/lib/libgmp.a` | `c27fba1b2193787b3c9b2da965faf20bde11bf15ad3b58711f03006b2b8f480b` | Does not match that recorded package file. Its differing source/build is not established here. |

The existing `build/distribution/CMakeCache.txt` explicitly selects the verified `build/release-deps/mingw64` GMP prefix and static library. The ordinary `build/` cache has distribution mode disabled. A fresh release build must explicitly select the verified prefix and matching header, rather than assume the installed default library is the reviewed artifact. The matching MSYS2 GMP source package, upstream source, packaging recipe and patches are already retained under `third-party/sources/`; their earlier source/archive integrity review is not repeated here. Library hash correspondence does not prove which GMP was linked into an old EXE.

This follow-up does not overwrite either GMP library, rebuild the program, create a release tag, or publish an artifact. It identifies the concrete rebuild path and preserves the remaining distinction between new build correspondence and historical provenance.

## Evidence

New responses, pinned source/license files, revision bodies, hash records and the GMP package check are retained in `debug/license-history-followup-20261004/`. Earlier audit evidence remains intact. The ObjectType answer/revisions, cosine MIT initial revision, and matching GMP source archive records are reused from the completed audit. No contact with upstream authors is made.

## Later archive findings and owner clarification

The [archive investigation](archive-investigation.md) establishes an original-author MIT grant in Quilez's archived palette shader, superseding the unestablished-grant status above for that specific function and its seven examples. It also documents Lol4t0's direct January 2015 exchange with Lohmann and an October 1, 2023 rainbow table capture before the MIT counterpart. Actual palette acquisition remains unestablished. The owner's report that the original RFF author discovered `0.428` independently qualifies the earlier unexplained-selection wording; Bonet's match is a comparison reference, not donor proof. The rebuild findings above are unchanged.
