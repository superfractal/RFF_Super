<!-- Modified by Opus 5.5 on 2026-10-04. -->
# CHANGELOG Style

Rules for writing [CHANGELOG.md](../../CHANGELOG.md). Every entry, old and new, is meant to read as
one voice, so follow these rather than the phrasing of whichever entry you happened to look at.

Entries written before this file existed do not all follow it. Several older fixes are phrased as a
past symptom rather than as a result, and a few state their cause. **They are not a model.** Copying
the shape of a neighbouring line is the most common way these rules get broken, and it is worth
being most suspicious of when the neighbouring line looks like the one being written.

## Structure

```md
### 2026/08/14

* **2.0.6.5 - Panels and Exports**
  * A change, stated as what the program now does.
    * A detail of the change above.
* **Fixes**
  * One fix, one line.
```

- Newest first. A new release opens a `### YYYY/MM/DD` section at the top of the file, under
  `# Update History`. Several releases can share one date section; separate them with a blank line.
- One `* **<version> - <Title>**` block per release. The title is a short noun phrase (2-4 words)
  naming the release's headline change — `Texture Layers`, `Lossless Video Export`, `Sliders`. Not
  a sentence, not a summary of everything in the release.
- A release with no headline feature is titled after what it does cover; a run of unrelated small
  changes still gets one title, chosen from the largest of them.
- Feature bullets come first, `* **Fixes**` last. Sub-bullets (4-space indent) carry details of the
  bullet above them; do not nest a third level.
- A block of related changes may take its own `* **<Area>**` heading (e.g. `* **Fog**`) between the
  version block and Fixes.

## Voice

- Write about the program, in the present tense: what it now does, offers, or keeps. Not about the
  work: never "fixed", "changed", "refactored", "added support for".
- A new setting or feature is introduced as `Added <Name> (<Menu> menu): ...` on first mention.
- A fix is written as a **result**, in the `now ...` / `no longer ...` form:
  - `The rows below a section no longer flash as it is folded or unfolded.`
  - `Video export: the progress window now opens in front of the main window, instead of behind it.`
  - The symptom the user saw belongs in the line, as the "instead of" half or as a following
    sentence. What is fixed must be recognizable to someone who hit it.
  - Shortening a fix cuts its explanation, never its form. `Slope: thin dark dashes ran along the
    color bands.` is short and wrong — it states the symptom and leaves the result unsaid. `Slope no
    longer draws thin dark dashes along the color bands at depth.` is the short form.
- **A fix never states its internal cause.** No threads, z-order, pointers, race conditions, window
  styles, buffers, or call ordering. "The dialog had no owner window, so the main window stayed
  clickable" is wrong; "a Load or Save dialog now holds RFF_Super until it is answered" is right.
- Features may explain *why* they behave as they do, when that changes how the reader would use
  them ("Bitrate is ignored while it is on"). They still never explain *how* they are built.
- Never name a source file, class, function, or constant. The reader does not have the source.

## Wording

- The program is **RFF_Super**. Never "RFF", never "the app", never "we" or "you"-as-developer.
- Name controls exactly as they appear in the program, with their own capitalization: `Macro
  Radius`, `Pause keyframe preview`, `Repeat U / V`. Do not quote or bold them.
- Prefix a fix with the area it belongs to when the line alone would not place it: `Slope: `,
  `Video export: `, `Settings windows: `. Drop the prefix when the subject already names the area.
- Every bullet is a complete sentence and ends with a period.
- Em dash — spaced, as here — for an aside. No parentheses for asides; parentheses are for naming a
  menu or a default.
- Plain digits, no thousands separators: `1000000`, `65535`.
- American spelling: `color`, not `colour`.
- Backticks only for file names and extensions (`.log`, `.mkv`). Never for control names.
- No bold or italics inside a bullet; the only bold is the version and section headings.

## Check it before you write it

A line is a claim about the program and is read as one. Check each against the code that produces
it, not against what the change was meant to do. The two part company most often exactly where the
line sounds most reassuring, so the sentences below are the ones to distrust first.

- **"Looks the same", "is unchanged", "matches the old behaviour".** Follow the path end to end
  before writing any of these. A new control sitting at its default is not proof: a curve, a
  normalisation or an encode/decode round trip on the same path can still move the result while the
  control itself does nothing. If the claim only holds over part of the range, say which part.
- **What an older file loads with.** Check both loaders, because they do not agree. A shader preset
  is read into a freshly default-initialised attribute, so a field the file lacks comes up at its
  default. A settings file is read into a copy of the live session, so a field the file lacks keeps
  whatever the session already had — default on a fresh start, and whatever was last set otherwise.
  "Files from earlier versions load with X" is therefore true of presets and only conditionally true
  of settings. Write the two halves separately whenever they differ.
- **A default that is meant to reproduce earlier behaviour.** Trace it to the point where the old
  and the new path rejoin, not merely to the branch that picks between them. A default is only
  worth stating as compatibility once the whole path has been compared.
- **A control's reach.** Name what it actually touches. A control that shapes one part of the
  picture in its name but is applied to the whole of it needs that said, however it is named.

## What earns a line

- Anything visible: a new control, a changed default, a changed range, a different look, a fix a
  user could have noticed.
- File-format compatibility, whenever a release changes what is written: say plainly whether files
  from earlier versions still load, and what happens to what they lack.
- Not: build files, internal restructuring, comments, or a fix for a bug that only ever existed in
  an unreleased build.
