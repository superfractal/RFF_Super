<!-- Created by GPT-6 on 2026-09-24. -->
<!-- Modified by GPT-6 on 2026-09-25, 2026-09-26. -->
# Guide validation and measurement notes

[Back to the guide](SETTINGS_GUIDE.md)

## Image method

The method below applies to the 110 fractal PNGs in `images/`: 54 before/after pairs and two supplied-location views. The six explanatory PNGs in `diagrams/` are separately described under [Workflow diagrams](#workflow-diagrams).

- Inputs: the two supplied RFC files, copied unchanged into `examples/source-1.rfc` and `source-2.rfc`.
- Production C++ calculation objects and freshly compiled Vulkan shaders from the current source tree; no Computer Use, clicks, screenshots of applications, or synthetic fractal artwork.
- Actual Mandelbrot perturbators generate the iteration maps. Auto iteration converges to period × multiplier. Comparisons use Precision Level −7.
- 800 × 450 output, Clarity 1, 2× SSAA in each dimension: a 1600 × 900 internal map. Palette phase is held at time zero except for the explicitly timed animation example.
- The video offscreen renderer receives both the correctly calculated wider map and the detailed map at the configured adjacent zoom ratio. This avoids false boundary fallback colors.
- The shader baseline is neutral color correction, stopped linear animation, disabled textures/patterns/material effects/warp/bloom/fog/stripe, Original surface with Studio off, Slope Opacity 1, Shading Depth 0.02, Slope Brightness/Gamma 1, and specular/rim/cavity/tint/fill/gloss disabled. Other values are retained from source 2; the complete state is in `baseline.rfc`.
- Each displayed comparison writes PNGs and complete before/after RFCs. The PNG byte hashes and image-difference metrics are recorded in [validation-data.json](validation-data.json).
- The 54 comparisons all have nonzero maximum pixel difference. Repeating the baseline after each image batch gives a maximum 8-bit channel difference of **0**. This verifies repeatability, not every setting in the application.
- All comparison images are inspected in contact sheets; right-edge crops are inspected separately. The original source files are not modified.

## Workflow diagrams

Six 1200 × 650 PNG diagrams illustrate rotation keys, interpolation, zoom speed and holds, audio placement, camera coverage, and export rates. They are drawn from geometry and worked examples, not captured application UI. All six are visually reviewed for clipping and legibility. Their labels explicitly distinguish them from screenshots. No additional end-to-end playback, audio export, or performance measurement is claimed for these examples.

- Key controls and editing actions are checked against [TimelineInspector.cpp](../src/rff2/ui/TimelineInspector.cpp) and [TimelineWindow.cpp](../src/rff2/ui/TimelineWindow.cpp).
- Interpolation and decreasing-depth progression are checked against [TimelineEvaluator.cpp](../src/rff2/video/TimelineEvaluator.cpp). Smooth uses `u²(3 − 2u)`; Cubic depends on neighboring values. The guide uses the UI names **Smooth** and **Cubic**.
- The constant-speed example uses the depth span divided by speed, plus hold duration, as in [TimelineSchedule.cpp](../src/rff2/video/TimelineSchedule.cpp): `10 / 1 = 10 s`, `10 / 2 = 5 s`, and `10 / 1 + 2 = 12 s`.
- Clip duration, non-overlap, and fade limits are checked against [AudioTimeline.cpp](../src/rff2/video/AudioTimeline.cpp): source `20 − 12 = 8 s`, placed at `3–11 s` in the video.
- Camera coverage is a schematic, not a rendered before/after test. Doubling both padded dimensions gives four times the pixel count.
- Export examples calculate frame count and target payload for ten seconds. Actual encoder throughput, output size, audio overhead, and quality are not measured by these diagrams.

## Shader documentation audit

The [shader overview](shader-overview.md) and [coverage checklist](shader-coverage.md) expand the earlier abbreviated shader summaries. The audit checks all 17 Shader menu entries, all 29 layer slots, 319 field/alias inventory rows, 157 labels read directly from current AppearanceForms source, and 27 Surface inspector groups. Counts include repeated labels/aliases; they are not counts of distinct settings or rendered tests. The seven appearance/palette forms contain 200 field definitions when repeated layers are counted once.

The documentation audit adds previously missing display-name mappings, HDR field rows, and palette/layer/reset actions. The later image batch below expands the visual examples without changing production rendering code.

## Additional shader image batch

The [additional gallery](shader-comparisons.md) adds 24 actual render pairs and 48 downloadable RFC files. It uses the same correctly paired inner/outer maps and rendering method as the original suite. The new baseline matches the original baseline pixel for pixel, and the repeat at the end of this batch also has maximum channel difference 0.

All 24 pairs are inspected in contact sheets, with separate enlarged right-edge inspection. Ornament and prism examples are also inspected at full size. Their small average differences are real: ornaments cover a small portion of the frame, and Prism Spread changes selected colored reflection regions. Captions identify these localized/subtle effects. Every pair has a nonzero pixel difference; no production fix or workaround is needed for this batch.

Studio/material comparisons hold their prerequisites fixed within each pair, and captions identify recipe-dependent setups. Intentional Nearest Mix pixelation and MFR False Color diagnostic colors are explained as selected effects. HDR examples are SDR PNG previews of HDR rendering, not HDR-encoded images. Mist is sampled at 2 seconds on both sides. The PNGs are compressed losslessly; their hashes, differences, and additional-batch repeatability are recorded in [validation-data.json](validation-data.json).

## Corrections during preparation

1. The first comparison harness supplied one map for two zoom levels, creating a narrow erroneous right-edge strip when the renderer fell back to the wider map. The harness now computes the actual wider map. All final images were regenerated, without cropping away the border.
2. The first Roughness test had Specular Intensity 0. Both images were identical. With Studio On and Specular Intensity 0.8 on both sides, Roughness 0.1 and 0.8 produce visibly different reflections.

No production-code fix was needed for these harness/setup issues. The field descriptions also distinguish currently misleading hints from actual behavior: Log Zoom uses base 10, Zoom Step per Keyframe is a magnification ratio, and zero Compression Threshold means zero tolerance rather than the explicit reference-compression off switch.

## Reproducing and scope

Load a before/after RFC from the gallery, wait for the calculation, and compare at the same size and animation time. These images come from the production offscreen video pipeline; interactive still sampling and display scaling can differ slightly. No external textures/audio are needed for these examples.

The field reference covers the current static forms and surface controls, with export, timeline, audio, and optional tool settings explained in the main guide. Visual tests cover the 54 illustrated cases, not every combination, custom formula, projection, HDR encoder, or optional AI workflow. No speed numbers are claimed for unmeasured effects.

## GitHub check

Fractal renders live in `guide/images/`, explanatory PNGs in `guide/diagrams/`, and actual UI images in `guide/ui/`. All use relative Markdown links. Downloadable settings are in `guide/examples/`. Link targets, heading anchors, file hashes, image dimensions, nonzero comparison differences, baseline repeatability, and unchanged source RFCs are checked before delivery. There are no references to local debug files in rendered image URLs. The expanded manual also uses six GitHub-compatible Mermaid workflow diagrams.

## Documentation accuracy review

The September 24, 2026 review compares the guide with current form definitions, command handlers, timeline evaluation, shader code, preset values, file handling, and Local AI request/state handling. It corrects three inaccurate workflow descriptions and clarifies two rendering details:

| Topic | Corrected or clarified behavior | Implementation checked |
| --- | --- | --- |
| Timeline parameter editing | The Parameters submenu opens the inspector's parameter catalog. Ordinary Shader-panel edits do not automatically record keys through the current UI; add/select tracks and edit keys in Selection. | [TimelineWindow](../src/rff2/ui/TimelineWindow.cpp), [TimelineInspector](../src/rff2/ui/TimelineInspector.cpp) |
| RGB cycle linking | Linking is inferred from matching track data. The current inspector has no dedicated unlink switch. | [TimelineWindow](../src/rff2/ui/TimelineWindow.cpp) |
| AI Edit image sheets | Each source image retains a 512 × 320 tile and a 32-pixel label strip. Changing from 2 × 2 to 3 × 3 enlarges the page from 1024 × 704 to 1536 × 1056; it does not shrink saved tiles. | [TimelineAiExchange](../src/rff2/ui/TimelineAiExchange.cpp) |
| Color-key interpolation | Color tracks interpolate in OKLab, with alpha handled separately, rather than directly interpolating sRGB channel values. | [TimelineEvaluator](../src/rff2/video/TimelineEvaluator.cpp) |
| Blur Quality | Appearance changes sampling budgets as well as the radius limit. Chaos Blur uses denser sampling even below Speed's 16-texel radius cap; large radii still have a 4096-texel safety limit. | [Fog shader](../shdsrc/vk_fog.frag) |

These are documentation changes. The existing image evidence remains unchanged. Automated checks verify links, field/menu coverage, image integrity, and recorded comparison data; they do not prove every behavioral sentence or replace end-to-end Local AI, audio, or HDR video testing.

## Expanded manual and actual UI images

The [user manual](user-manual.md) adds detailed workspace/file, exploration/calculation, Local AI, animation/export, and preset/recovery chapters. The [feature map](feature-coverage.md) inventories 58 statically named menu actions, the language choices, preset families, and workspace-only tools. The field explanations and existing shader audit remain part of the manual rather than being replaced by a short overview.

The [UI gallery](ui-gallery.md) adds **62 PNGs**: 59 nonempty form groups from 15 production form definitions, two Local AI tabs, and the actual timeline editor before loading a source folder. Its [inventory](ui-inventory.json) contains 478 field rows, counting repeated layer fields separately. These are documentation counts, not counts of distinct features or independent behavior tests. [UI image metadata](ui-validation.json) records dimensions and SHA-256 hashes.

The capture program is a throwaway native fixture kept under ignored `debug/`. It links the current production form, scene, and timeline objects, creates its own controls outside the visible desktop, and asks those controls to draw into DIB image buffers. Native EDIT controls require a visible ancestor style to draw their text, so the fixture uses an offscreen host rather than capturing the desktop. There is no Computer Use, input injection, mouse/keyboard operation, or capture of the user's application session.

Local AI uses a scratch copy of its production UI construction with connection configuration reads, the initial metadata request, and the timer disabled. The fixture substitutes an explicit connection-not-requested message. It does not send a prompt, query a server, produce a model result, or edit the user's AI configuration. The UI appearance and zoom controls themselves use the production layout/drawing code. The timeline image deliberately shows its own “no keyframes loaded” sample state, not a fabricated preview.

All 62 images are inspected in six contact sheets; Location, Local AI, and Timeline are also inspected at full size. The initial blank native edit values were corrected in the capture harness and the images regenerated. Long standalone forms are made taller to show their fields; application-wired actions omitted from those standalone models are explained in the workflow text. Existing UI ellipses and untranslated headings are retained and described rather than painted over.

Source review also corrects earlier ambiguity about audio and holds: the current Audio inspector exposes export enable and master gain, while per-clip edits and hold entries use timeline JSON. The manual includes the exact audio microsecond convention and an example clip. No new end-to-end inference, locator success-rate, audio playback, or video-encoding measurements are claimed by this documentation expansion. The existing 54 render comparisons and source-file preservation checks are rerun without changing those images.


## Audio export validation, 2026-09-25

The production `FFmpegPipe` implementation is exercised in an isolated Windows test with synthetic 64 × 64 video frames at 30 FPS for four seconds, using FFmpeg 8.1.1. The test links the production audio validator and audio filter builder. It does not exercise the interactive export controls, GPU rendering or long-duration reliability.

The known stereo WAV source contains 440 Hz during source seconds 0–1, 1000 Hz during 1–3, and 2000 Hz during 3–4, with amplitude 0.5. The clip selects source seconds 1–3, starts at video second 1, uses 0.5-second fades, and applies clip gain 0.5 and Master Volume 0.5. The source filename includes Japanese characters and a space.

| Check | Measured result |
| --- | --- |
| Standard SDR, PQ and HLG MP4 | One AAC audio stream; 120 video frames; 4.000-second duration. |
| Lossless SDR MKV | One FLAC audio stream; 4.000-second duration. |
| Source trim | Decoded steady region measures 1000 Hz. |
| Gain | Expected RMS 0.088388; measured AAC 0.088390 and FLAC 0.088389. |
| Fades | RMS in video seconds 1.1–1.2 and 2.8–2.9 is approximately 0.027, consistent with the selected fades. |
| Placement and silence | Measured samples at video seconds 0.2–0.8 and 3.2–3.8 are silent. |
| Two clips | Clips at seconds 0.5 and 2.5 retain a silent gap; the second clip has twice the measured amplitude when its clip gain is doubled. |
| Export Audio off or all clips muted | Video completes without an audio stream, even with missing inactive sources. |
| Missing active source | Export preparation fails and the log identifies the missing source. |
| Corrupt source | Export fails and the log retains the encoder's invalid-input diagnostic. |
| Cancellation | Export reports failure and the log records cancellation. |

Successful runs retain the command, encoder version and zero exit code, without non-monotonic timestamp warnings. Temporary audio filter scripts are removed. During development, decoded-media checks exposed invalid timestamps after silence padding; the final filter assigns timestamps from the audio sample count, and the above checks pass with that correction.

Local test sources, generated media, encoder logs and machine-readable measurements are in the gitignored `debug/audio-export-20260925/` folder. The complete RFF_Super build also passes after these changes.


## Audio inspector validation, 2026-09-25

The Audio inspector now exposes clip addition, removal, source selection, timeline placement, source trimming, clip gain, mute and fades. Parameters → Audio in the track context menu opens this inspector section, including for PNG sources. These controls supersede the earlier JSON-only per-clip workflow described in the historical validation notes above.

A focused C++ test uses the production form bindings and audio validator. It checks Japanese WAV filenames, automatic four-second duration detection, corrupt and missing input rejection, microsecond-preserving seconds conversion, gain and mute edits, and rejection of overlapping clips, excessive fades and out-of-range trims without changing the previous settings. The test sources are under `debug/audio-ui-20260925/`. The Windows build passes. Interactive visual checks and native dialog operation are not exercised by this test.


## Audio timeline editing validation, 2026-09-25

A focused test exercises the production clip-edit helper with two audio files. Moving preserves the source interval; left-edge trimming changes placement and source In together; right-edge trimming changes source Out. Negative placement, overlap and source overflow leave the previous settings intact. Shortened clips retain fades that fit their new duration. The second clip remains unchanged. The test is in `debug/audio-ui-20260925/clip_drag_test.cpp`. Native mouse interaction and visual layout are not covered by this helper test.


## Shared Audio row ordering validation, 2026-09-25

Audio now participates in the regular row layout, clipping, vertical scrolling, mouse reordering and keyboard focus. It starts at the bottom. A model harness executes the production reordering method for Audio-only moves, parameter moves across Audio, mixed selections and no-op drops. Linked RGB rows stay together and hidden parameter tracks remain present. These tests pass alongside the audio clip-edit regression test. The Audio row position is tracked in the editor's Undo/Redo history and is not written into the timeline format. Native mouse and visual checks remain unperformed.


## Audio preview validation, 2026-09-26

A Windows test runs the production FFmpeg preview decoder and waveOut playback device with two clips at low master volume. Instrumentation verifies nonzero PCM submissions, including after seeking into the second clip, no further submissions after stopping, disabled-audio silence, missing-source rejection and repeated immediate start/stop. The test uses the same trim, placement, gain and fade filters as export. It does not measure acoustic output or end-to-end GPU/video synchronization. Sources are in `debug/audio-preview-20260926/`. Add Audio File now permits pending edits and applies them before adding the next source; invalid drafts remain subject to the existing validation.


## Multi-day timing validation, 2026-09-26

The tests in `debug/long-timeline-20260926/` check day/hour formatting and carry at 59.96 seconds, millisecond progression at 48 hours and six days, adjacent export frame times at 1000 FPS, long schedules and holds, and identical audio filter plans when a clip and the seek origin are shifted by multiple days. All pass.

A production AudioPreview test places the first sound 0.5 seconds after a seek to 172800 seconds and 518400 seconds. The first nonzero PCM block reaches the Windows waveOut API at 484 ms and 500 ms respectively; this records submission timing, excluding device buffering and acoustic latency. The existing two-clip playback, seeking, stopping and failure tests also pass. No continuous multi-day rendering or GPU/audio end-to-end synchronization is claimed.

## GPU and audio timing follow-up, 2026-09-26

The follow-up harness in `debug/long-timeline-20260926/av_test.cpp` links the current production VideoRenderScene and uses AudioPreview, TimelineTime and the FFmpegPipe definition extracted from VideoWindow.cpp. It renders a black/white marker through Vulkan and reads the image back while audio plays through the Windows waveOut device. Three trials each start at 0, 172800 and 518400 seconds. The first nonzero audio block reaches waveOut 0–16 ms before the corresponding GPU marker readback; the multi-day starts show the same range as the zero-time control.

A separate 60-second run starts at six days, renders 7,623 frames and checks six audio/visual cues at 0.5, 10.5, 20.5, 30.5, 40.5 and 50.5 seconds after the start. Submission/readback differences are 0, 0, 0, 0, 0 and +16 ms respectively. No increasing drift, playback error or render failure occurs during this run. Measurements use GetTickCount64 and have approximately one system tick of timing granularity.

Three four-second, 60 FPS GPU-rendered test windows also pass through the production FFmpegPipe and are decoded again. Each contains 240 video frames and 48 kHz AAC audio. White frames occupy exactly frames 60–89; the tone starts at 1.000 seconds and ends at 1.500 seconds at the measurement's 1 ms resolution. FFmpeg reports success without non-monotonic timestamp warnings. For these short files, clip times are rebased to the window start; the harness first verifies that the production AudioExport offset filter for each absolute position is identical to the local export filter. These are sampled windows, not full multi-day export files.

The harness supplies the marker directly and does not automate the TimelineWindow UI or measure monitor presentation, audio-device buffering or acoustic latency. The one-minute test does not establish 48-hour continuous rendering reliability. The earlier precision/formatting tests and two-clip playback, seek, stop and error regressions pass again. Measurements are saved as `av-results.csv`, `steady-preview.csv`, `decoded-av-results.json` and `av-run.log` beside the harness.
