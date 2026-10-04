<!-- Modified by GPT-6 on 2026-10-04. -->
<!-- Modified by Opus 5.5 on 2026-10-04. -->
# ObjectType and palette license archive investigation

This investigation uses Internet Archive captures, official source APIs and the previously verified Git history to separate publication, permission and actual acquisition. The missing original-author MIT grant for Quilez's cosine palette shader is now established. ObjectType's acquisition history is clearer, but a separate MIT grant is still unestablished. The rainbow table appears in an earlier archived website; its first creator and RFF donor remain unknown. These are provenance findings, not three confirmed infringements.

The owner's new explanation of `0.428` is recorded separately below. No application, shader, numerical or runtime tests run, and no binary or release is rebuilt. The completed 930-file historical audit is not rerun.

## Results

| Question | New evidence | Current conclusion |
| --- | --- | --- |
| ObjectType | Lol4t0 directly answers Niels Lohmann in January 2015; historical profiles have no separate grant in the inspected samples. | The technique's acquisition is directly documented. Separate MIT permission for any protected answer expression remains unestablished. |
| Rainbow | PaletteMaker's October 1, 2023 capture contains all seven stops, preceding the October 6 MIT repository addition. | The MIT counterpart exists, but its repository cannot be assumed to be the table's first source or RFF's donor. |
| Cosine palettes | A November 11, 2019 archived Shadertoy API response contains the author's complete MIT header, function and seven examples. | The original-author permission question is resolved for this identified shader code, subject to preserving its notice. Actual RFF acquisition is still unestablished. |

## ObjectType acquisition and permission

[Lol4t0's answer](https://stackoverflow.com/a/9860911) is posted March 25, 2012. The retained official answer/revision records assign CC BY-SA 3.0 and identify the editor strcat. Its complete demonstration combines a template-template parameter, a trailing type parameter pack, a recursive `SomeType`, `std::unordered_map` and `std::unique_ptr`.

The [official answer-comment API](https://api.stackexchange.com/2.3/answers/9860911/comments?site=stackoverflow&filter=withbody&pagesize=100) supplies all four currently returned comments, with `has_more: false`. On January 25, 2015 at 11:36:44 UTC, Niels Lohmann asks whether the declaration can default to `std::vector`. At 20:15:58 UTC, Lol4t0 replies with the corresponding declaration and explains the difference from an associative container. Six days later, [commit befd90d](https://github.com/nlohmann/json/commit/befd90dead2f9ce42b5c7fdd7cf2e0e0db2dd226), January 31, 2015, introduces the cited ObjectType declaration in the inspected JSON ancestry. The direct conversation and explicit commit credit establish more than a coincidental code match.

On March 26, 2024, another commenter links JSON's ObjectType definition and asks whether it uses this technique. Lohmann confirms the connection on March 29, 2024. His comment concerns compiler/language behavior; it does not grant a license on Lol4t0's behalf. Neither the January 2015 help nor the 2024 confirmation contains an express MIT grant.

The archive index returns 89 distinct successful profile captures in the selected query. The inspected [April 13, 2012 profile](https://web.archive.org/web/20120413050601/http://stackoverflow.com/users/774651/lol4t0) and [March 28, 2015 profile](https://web.archive.org/web/20150328010417/http://stackoverflow.com/users/774651/lol4t0) have empty author biography sections. Their contribution license links point to CC BY-SA 3.0. The [February 20, 2026 profile](https://web.archive.org/web/20260220171522/https://stackoverflow.com/users/774651/lol4t0) supplies no inspected author-specific MIT grant either. These are three sampled captures, not a claim that all 89 biographies were reviewed.

The current official Stack Exchange profile supplies no broader permission statement. The author's [Habr profile](https://qna.habr.com/user/Lol4t0) explicitly cross-links this Stack Overflow identity and GitHub/Lol4t0; identity is not inferred solely from matching handles. The current linked GitHub profile has no biography, and the profile README lookup returns 404. Two targeted upstream issue searches, naming Lol4t0 and checking `involves:Lol4t0`, each return zero results. Earlier exact-answer and licensing searches remain part of the retained audit. No private correspondence is inspected and no author is contacted.

JSON uses its own `basic_json` declaration and map/comparator/allocator arrangement; the complete smart-pointer demonstration is absent. The shared syntax expresses a [standard C++ template mechanism](https://eel.is/c++draft/temp.arg.template). The remaining question must therefore be scoped to any protected expression actually retained, rather than assuming the whole JSON library or every use of this language mechanism needs a separate license. This investigation establishes neither blanket MIT clearance of the answer nor a confirmed infringement. Direct permission covering the relevant answer and any protected editor contribution would settle the grant question; similarity, helpful comments and unrelated MIT repositories cannot substitute for that evidence.

## Rainbow publication and actual donor

The seven stops, in order, are `E81416`, `FFA500`, `FAEB36`, `79C314`, `487DE7`, `4B369D` and `70369D`.

| Date in UTC | Record | What it proves |
| --- | --- | --- |
| October 1, 2023 at 02:21:53 | [Archived PaletteMaker rainbow page](https://web.archive.org/web/20231001022153/https://palettemaker.com/rainbow-colors) | All seven exact stops are already publicly present in this order. |
| October 6, 2023 at 03:50:05 | [TerminalTextEffects addition](https://github.com/ChrisBuilds/terminaltexteffects/commit/b3b3d9f9dc81e57decfb97c197e38f99fcbbf5d3) | The same stops enter the inspected Python file with a repository MIT grant. |
| May 27, 2025 | [Original RFF palette addition](https://github.com/Merutilm/RFF-2.0/commit/c0633b288c64d1dec0cee8343d79474f96450f24) | RFF already has the rounded floating-point table before later renames and local imports. |

The archive query returns 20 distinct successful PaletteMaker captures; October 1 is the earliest returned capture for this URL. The actual replay contains the ordered color boxes and hex table, not just a search snippet. Its footer reserves rights, and no MIT grant is present in the inspected page. A footer alone does not determine whether the numeric stops are protected, who first selected them, or which uses require permission. Conversely, a copy button or free worksheet description does not establish a code redistribution license.

The [actual TerminalTextEffects license at its October 6 commit](https://github.com/ChrisBuilds/terminaltexteffects/blob/b3b3d9f9dc81e57decfb97c197e38f99fcbbf5d3/LICENSE) is MIT. Its seven assignments have no donor citation in the inspected introduction. The earlier PaletteMaker capture means this repository is a licensed matching publication, not proven original authorship of the table. The chronology also does not prove that TerminalTextEffects copied PaletteMaker. The retained 63-commit path history and original RFF addition likewise do not identify RFF's acquisition route.

Targeted exact-value searches produce additional later copies, including ColorsWall and a Stack Overflow example. Those matches cannot establish the first creator or RFF donor; they are not treated as permission evidence. Search date filters also return later pages, so their ranking/date labels are not used to assert an earlier origin.

The practical unresolved fact is where the original RFF author obtained or selected these particular values. A contemporaneous reference or author statement can answer that; archives show public availability, not private acquisition. Original RFF's GPL inheritance and the retained MIT counterpart remain documented without inventing a donor chain. Both local generators keep their exact truncated floats, RNG order and recipe ids.

## Quilez cosine palette permission

The earliest returned successful article capture is [July 5, 2015](https://web.archive.org/web/20150705144704/http://iquilezles.org:80/www/articles/palettes/palettes.htm). The article contains the cosine function and seven parameter examples, and embeds Shadertoy shader `ll2GD3`. The May 4, 2016 article replay confirms this content and link. The inspected article snapshots do not themselves state MIT terms. Their footer year is not treated as an article publication date.

The archived shader view and embed HTML load the actual shader through a separate response. An October 2019 view contains MIT headers for the CodeMirror editor; those headers do **not** license the shader. The decisive record is the [November 11, 2019 archived public Shadertoy API response](https://web.archive.org/web/20191111072843id_/https://www.shadertoy.com/api/v1/shaders/ll2GD3?key=NtrtMH). It identifies shader `ll2GD3`, title `Palettes`, username `iq`, and the same article URL. The article embeds this exact shader; Quilez's later article page also links the `iq` account. These links and the code's named copyright connect the grant to the author, rather than to a third-party mirror.

The response reports June 2, 2015 at 08:57:06 UTC as the shader's date. The source begins with an MIT declaration, `Copyright © 2015 Inigo Quilez`, and the complete permission, notice-preservation and warranty text. The inspected primary archive proves that this grant is present by November 11, 2019. The shader's stated 2015 date and copyright year do not independently prove that the header was present in its first 2015 revision.

The grant covers the captured cosine function and its seven example calls. The permission text allows copying, modification, distribution and sublicensing, conditional on retaining the copyright and permission notice. Its existence resolves the previously missing original-author grant for this identified implementation. The local scalar/randomized generators do not become exact copies of the full shader by sharing the equation, and their actual acquisition route remains unknown. No broader grant is inferred for all article prose, other shaders or the external texture used by the demonstration.

The [May 4, 2016 Erkaman MIT counterpart](https://github.com/Erkaman/glsl-cos-palette/tree/572a42e09c565f29f8ba769914d2b7681a64f84e) and its existing grant are retained. A third-party CodePen reproduction carrying Quilez's header was only a lead; it is not the authority used to close this question. The full recovered original-author grant is installed as `extern/licenses/iquilez-palettes-MIT.txt` and an identical bundled copy, with its hash added to the dependency manifest. NOTICE and dated implementation clarifications now point to this primary evidence.

## The reported independent 0.428 discovery

On October 4, 2026, the project owner reports that the original author says `0.428` was found coincidentally. This is an owner-relayed original-author explanation of independent discovery. It supersedes treating the coefficient's earlier selection as wholly unexplained, while remaining distinct from an independently inspected contemporary notebook or author message.

Bonet's earlier matching mathematical publication remains a comparison reference. It is not evidence that RFF copied his code or prose, nor a basis by itself for requiring permission from him. The original GPL source inheritance is unchanged. The coefficient is therefore not counted as a demonstrated third-party copying or permission problem on the evidence available here. Arithmetic and operation order remain unchanged.

## Evidence preservation and limits

Raw archive responses, CDX indices, decoded HTML/JSON, source and license extraction, official API comments, targeted search results and before-edit backups are retained in `debug/license-archive-investigation-20261004/`. The review records twelve selected raw captures: three articles, two shader views, two embeds, one shader API response, three author profiles and one rainbow page. All twelve match their selected CDX SHA-1 digest, using the original compressed bytes where the archive digest describes compressed content. Raw and decoded SHA-256 hashes are recorded separately. The decisive shader API and rainbow replay both match the indexed payload directly.

These checks establish evidence integrity and the license extraction; they execute no application implementation. All 292 existing dependency-manifest entries remain unchanged, and the new grant adds one entry. The two installed grant copies contain identical extracted notice text. Source differences in this pass contain only dated provenance comments and the palette file's modification date; palette and norm implementation tokens are unchanged. The report supplies the new current conclusions while older dated audit reports remain historical records. No release artifact is produced or legal determination of every possible jurisdiction made.

## Subsequent ObjectType code mitigation

At the owner's request, the ObjectType replacement changes both active parameter declarations to a single standard type pack and records their design basis. The before-edit and replacement headers pass the same focused compatibility check. The historical permission findings above remain records of what was and was not established; the mitigation supplies a documented current implementation without inventing an author grant or erasing attribution.

The owner subsequently [sets aside Rainbow and historical ObjectType permission and requests the original upstream JSON header](owner-disposition.md). The replacement is reversed, and `json.hpp` is byte-identical to upstream v3.12.0. The two questions are closed for further action within this task by the owner's decision; the archive evidence above remains intact.
