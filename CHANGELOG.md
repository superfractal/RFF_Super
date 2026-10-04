<!-- Modified by GPT-6 on 2026-09-25, 2026-09-26, 2026-09-27, 2026-09-29, 2026-09-30, 2026-10-01, 2026-10-02. -->
<!-- Modified by Opus 5.5 on 2026-09-29, 2026-09-30, 2026-10-02, 2026-10-03, 2026-10-04. -->
# Update History

## 2026/10/04

* **3.0.0.1**
  * Rendering FPS 0 pauses the preview: RFF_Super keeps the last frame on the canvas while settings and calculations still apply, and any value from 1 resumes drawing.
  * Keyframe generation, camera padding and Save Map now accept up to 200,000,000 pixels, so 3840 x 2160 at Supersampling 4 no longer stops with "Keyframes require 1 to 100,000,000 pixels". A keyframe map at that size uses about 1 GB of memory.
  * Pause Preview During Export now also frees the main preview's VRAM for the video export, and RFF_Super puts the finished picture back without recalculating it when the export ends. Save Map waits until the export ends while the preview is set aside.

* **3.0.0**
  * Added Match Planar Framing (Video Camera): RFF_Super matches the planar scale at the playhead using the loaded keyframe aspect ratio, a downward-facing 360 camera and Ground layout, while retaining rotation. Undo restores all matching settings together.
  * Added Zoom Holds (Timeline Settings): RFF_Super offers zoom pauses at keyframe positions, with editable durations and Undo. Color animation, constant-period rotation and audio continue during each pause, and settings and timeline files retain the pauses.
  * Added Display Start (s) and Display End (s) (Timeline Editor, Zoom Overlay and Max Iterations Display): RFF_Super controls each readout independently in preview and video export. Start 0 means the video start and End 0 means the video end; dedicated display tracks are no longer offered.
    * Settings and timeline files retain the display intervals; older files keep unrestricted display.
  * Added Save Appearance Settings and Load Appearance Settings (File menu): RFF_Super saves and loads `.rfsp` appearance presets.
  * Added Open Guide (? menu): RFF_Super opens the online user manual in the default browser after confirming the link; Cancel leaves the browser closed.
  * RFF_Super keeps its settings files in a config folder beside bin, and moves the ones an earlier version left beside bin into it on first use. A crash report is written to the recovery folder.
    * Local AI now finds its connection settings whatever folder RFF_Super is started from.
    * The Shader menu reads its examples from example/shader, and moves a shader_example folder from an earlier version there on first use.
* **Settings Choices**
  * Settings panels show On/Off settings as checkboxes.
    * A checkbox sits the same distance below its name as a text box does, and its row is only as tall as the box, so the next setting no longer starts after a wider gap.
  * Long settings lists are split into groups by a thin line and some space: Iteration Coloring, Start Offset and Palette Gloss in Color Cycle; Recessed Grooves, Spine Amount and Line Glow in Band Line; Independent Specular Light, Relief Waves and Iridescence in Lighting & Relief; Palette Follow in Textures and Patterns, plus Edge Enabled in Patterns; Rim Mask in Fog; Sync Color Animation and Primary Color in Animated Materials.
  * A setting with two to four short choices shows them as buttons side by side, so one click picks a choice; longer lists stay drop-down lists.
  * Alignment (Timeline Settings) is picked on a 3 x 3 grid, with the chosen position named beside it, and Font Style is set with separate B and I buttons.
  * Interpolation shows the curve each choice draws between keys, and Iteration Coloring (Palette) shows each choice as a tile with its curve, three to a row so every name fits. Smootherstep is no longer offered there; files that use it still render with it.
  * Settings windows: radio choices whose labels fit sit side by side in one row, and the Texture and Pattern layer numbers read as tabs. Color Smoothing and Color Interpolation keep one row per choice with their previews.
* **GPU Work Split**
  * Added GPU Work Split (Video settings, Export workspace): RFF_Super can send each video frame to the GPU in smaller parts, so a long export at a high resolution or Supersampling is not cut off by VK_ERROR_DEVICE_LOST when Windows resets a GPU that stays busy with one piece of work for about 2 seconds.
    * 0 sends each frame in one part, as before; 1 sends each shader pass on its own; 2 to 64 also divide the fractal pass into that many horizontal bands. The exported video is the same at every value, and higher values export slightly slower.
    * Settings files from earlier versions load with GPU Work Split at 0.
  * During a video export, the console shows about every 2 seconds how many parts each frame is sent in, how long the longest part takes on the GPU and in which pass, and how close that comes to the Windows limit, with a summary when the export ends.
* **Timeline Editor**
  * With no keyframe folder loaded, the preview reads No keyframes loaded and offers Select keyframe folder... in its middle; the transport no longer repeats the request.
  * The transport readout of the keyframes' zoom is named Magnification, and the zoom control under the tracks is named Timeline zoom, so the two are no longer both called Zoom.
  * Play and Pause are one button, which shows Pause while playback runs.
  * The ruler marks the end of the video with its distance and total time, and the tick labels sit to the right of their ticks, clear of the playhead.
  * Track rows keep a compact height instead of stretching to fill the track area, and the divider between the preview and the transport shows a grip for resizing it.
  * The timeline scrollbar is slimmer and appears only while the timeline is zoomed in.
  * Timeline Settings shows On/Off settings as checkboxes. While Custom Appearance is Off, the Zoom Display rows it governs are dimmed, and the panel notes that editing one of them turns it on.
  * RFF_Super creates new tracks with one keyframe at the start and holds its value through the end, without automatically adding an ending keyframe.
* **Appearance Workspace**
  * The section list is headed Surface Effects, the same name as the module chooser above it, and its header button reads Hide Sections or Show Sections on every page.
  * Shader Layers numbers each layer in the order it is applied, 1 first, and the list is only as tall as its layers, with Move Up, Move Down and Reset directly below.
  * The Basic and Detail tabs are drawn like the workspace tabs, with the selected tab in the accent color and underlined.
  * The strips for dragging the section list and the settings panel are half as tall and show a grip; their names appear as tooltips.
  * Secondary text in the settings panels is easier to read, in both the dark and the light theme.
* **Menu Visibility**
  * RFF_Super reads `menu-visibility.json` from the folder that holds `local-ai.json` and hides menu-bar items set to false; setting a whole menu or submenu to false hides its contents.
    * When the file is missing, RFF_Super writes a template using the original English captions in every interface language; changes take effect at the next start.
  * Local AI appearance (Shader menu) and the AI Edit button in the Timeline Editor are hidden by default and appear when `menu-visibility.json` sets them to true.
* **Local LLM**
  * Automatic video accepts 0 for Max changes to apply the initial appearance proposal and continue without re-evaluation or refinement.
  * RFF_Super offers Adjust appearance before Auto Zoom for automatic videos. The saved option adjusts the overview and starting view before exploration; the default retains adjustment after exploration.
* **Rendering**
  * Palette coloring in the preview and in video rendering is faster when Cycle Length (R), (G) and (B) share one value — up to about twice as fast with Log, Log-Log, Cube Root or frozen colors — and every pixel keeps exactly the color it had.
  * Rendering FPS accepts 0, which removes the limit on live redraws. Video Frame Rate still starts at 1.
  * Chaos Blur (Fog settings) no longer washes intricate regions out to a light grey: blurred areas keep the dark color between bright details and stay close to the original brightness.
  * Chaos Blur no longer offers Chaos Highlight Detail. Smooth surfaces next to blurred regions keep their white lines as sharp as with Chaos Blur off, and blurred regions no longer carry scattered bright specks.
    * Settings files, shader presets and timelines from earlier versions still open; their Chaos Highlight Detail value and track are ignored. Settings files and shader presets saved by this version cannot be opened by earlier versions of RFF_Super.
  * Added Chaos Blur Averaging (Fog settings): Average in Gamma Space keeps the dark gaps between bright details dark, and Average in Linear Space lets bright detail spread and brightens the blurred regions. Settings files and shader presets without it open with Average in Gamma Space.
  * Chaos Shade now goes up to 1 and darkens by the detected region alone, so at 1 the fully selected regions turn black whatever Chaos Amount is. Files that use Chaos Shade with Chaos Amount below 1 now render darker than before.
  * Chaos Blur (Fog settings) keeps blurred holes and edges round instead of squared off, and its blur no longer shows faint repeated copies of bright detail. The detected region now fades out evenly in every direction.
* **Fixes**
  * Keyframe generation now preserves the selected fractal rotation and 360 projection without replacing the Video Camera settings; 360 projections retain their framing without planar padding.
  * Video camera: planar and panorama projections now keep the same rotation direction when switching projection modes.
  * RFF_Super displays setting colors as clickable swatches that open the color picker, with borders that match the interface theme.
  * RFF_Super clears canceled input errors with Escape, waits for Band Lines and reflection selections to be confirmed before editing dependent values, and enables manual iteration limits only when Automatic Iterations is Off.
  * RFF_Super now retains escape colors for custom formulas such as sin(z^3+c) at large bailout values instead of showing black when the orbit overflows.
  * RFF_Super shows the screen color captured by Pick Color in frozen-color swatches for the current session, and marks unavailable samples with a question mark.
  * RFF_Super waits for related selections to be confirmed before editing dependent values, keeping color-stop targets and RGB linking consistent with the selected state.
  * RFF_Super preserves legacy arrow increments for relief depth, repeats, projection settings, MFR mastering brightness and iteration periods, and disables unavailable color-stop and Lustre controls.
  * RFF_Super keeps the original Surface sliders and spacing while appearance forms use numeric fields and arrow keys without additional sliders.
  * RFF_Super displays frozen colors as individually removable color swatches instead of an iteration-value input, and preserves unrelated pending edits while applying one field.
  * RFF_Super adjusts HDR peak brightness by 100 nits with the arrow keys (1000 with Shift) and disables video encoding controls when the selected output does not use them.
  * RFF_Super offers palette comparison and import previews, and Bronze, Pearl, Obsidian and Ceramic material presets in appearance workspaces.
  * Workspace numeric fields now retain fractional arrow adjustments where supported, and Escape discards the current field's pending input.
  * Texture selection now enables the layer, Clear Image removes only its image and enabled state, and inactive appearance controls now appear disabled.
  * Palette workspace: Link G & B to R is now available and starts On; changing Cycle Length (R) also updates G and B.
  * Workspace numeric fields now support Up and Down arrows, with Shift for larger adjustments.
  * RFF_Super keeps image evaluation responses in the required format during corrections, instead of repeatedly rejecting missing score and satisfied fields.
  * Automatic video now retains AI failure details and stops at Error limit instead of repeatedly returning to concept attempt 1. Theme generation shows each request attempt and its failure reason.
  * Automatic video now keeps Color Animation Mode at Linear, including when loading saved modes, while Color Animation Speed remains editable.
  * Smooth Zoom now shows the low-resolution draft and the full-resolution pass as one calculation: the percentage and elapsed time continue into the full pass, and Done appears only when that pass finishes. The elapsed time covers both calculations and excludes the zoom animation between them.
  * Timeline Editor: the Keyframes folder box and the file buttons now stay fully visible in medium-width windows instead of being covered by the preview.
  * Timeline Editor: the Zoom Overlay and Max Iterations Display buttons now leave space above and below them in the footer.
  * Appearance presets now retain the current camera position and orientation when loaded.
  * Presets containing the 64000000-color LongRandom64 2 palette now reopen successfully.
  * An unreadable example preset now leaves the current appearance intact and reports the loading failure.
  * Band Line now accepts very small widths without failing during palette preparation.
  * Video export: Band Line decorations now retain the preview's repeat positions when the repeat count includes a fractional part.
  * Stripe now leaves colors unchanged when either interval is zero, in previews and video rendering.
  * Band Period now accepts values up to 1000000000, and Undo and Redo restore values above 1000000.
  * Palette previews now handle large built-in palettes at high display scaling without invalid color selections.
  * Local LLM now retains complete characters when shortening multilingual retry and concept history, so later requests remain readable.
  * Locate Minibrot now reports an unavailable reference after reference-generation cancellation instead of closing RFF_Super.
  * Find Center and Locate Minibrot now return a failure when the current reference cannot provide a center correction, instead of terminating RFF_Super.
  * Reference settings now retain the current value of the other coordinate when only one coordinate is edited.
  * Reference and Explore now describe Log Zoom (10) as a base-10 value in English and Japanese.
  * Dragging from A/B Comparison now continues panning after the comparison view closes.
  * The custom language menu now marks the selected language.
  * Settings windows: keyboard increments now move logarithmic sliders above zero when their smallest positive value is below 1.
  * Settings windows now refresh their background colors when the high-contrast palette changes.
  * GPU timing now counts each completed frame once during smooth navigation.
  * GPU timing now includes ordered-layer rendering and presentation in preview totals.
  * Video GPU timing now keeps measurements under the correct effect names when enabled layers change.
  * Rendering progress now uses a valid refresh interval at zero and very small positive zoom values.
  * Rendering now starts normally for images with heights near 65535 instead of hanging during preparation.
  * Max Multiplier Between Levels now requires at least 2, preventing unbounded preparation with a value of 1.
  * Reference compression now preserves access to repeated reference sections instead of failing when those sections are used.
  * JPEG loading now preserves the expected color interpolation at the right edge.
  * Interlaced RGB PNG images now load correctly with an alpha channel.
  * BMP loading now preserves pixel positions and channel colors in images with custom color masks.
  * BMP image information now reports positive dimensions and the decoded color-channel count.
  * RGB PSD images without an alpha channel now load as opaque in both compressed and uncompressed files.
  * GIF loading now preserves the intended transparency when an image uses its own color palette.
  * TGA loading now preserves horizontal orientation, 16-bit truecolor and palettes whose first color index is nonzero.
  * Short HDR images now return their image information successfully.
  * Shader Layers: the panel's scroll bar is now dark in the dark theme instead of white, and it no longer appears when the layers, buttons and caption already fit.
  * Zoom Step per Keyframe (Animation, Keyframes, and Video > Data Settings) now describes its value as the magnification between neighboring keyframes, where 2 means each keyframe is 2x deeper, instead of as a logarithmic step.

## 2026/09/26

* **3.0.0-beta4 - Iteration Overlay**
  * Added RandomSmooth [10-20] (Shader, Palette): RFF_Super generates RandomSmooth colors with a palette cycle length between 10 and 20.
  * Local LLM: RFF_Super offers saved switches for AI appearance adjustment, palette-only AI changes, and a fresh RandomSmooth palette for each automatic video. AI can adjust shading and effects when the palette-only limit is off; color animation is retained in every mode.
  * Automatic video enables AI appearance adjustment by default and retains explicitly saved on/off choices.
  * Added Automatic video (Local LLM): RFF_Super proposes abstract themes, explores, generates RandomSmooth palette colors, generates keyframes and exports videos for a chosen count or until cancelled. Failed stages restart with a new concept within Error limit.
  * Local LLM defaults Error limit to 5 while retaining the 1–100 range and existing saved values.
  * Automatic video offers a saved Log Zoom ceiling, exploration settings, and Locate Minibrot with optional zoom-decrease retries.
  * Added Color Animation Speed and Color Animation Mode (Local LLM, Automatic video, Appearance): RFF_Super retains the selected animation while generating new palette colors.
  * Automatic video restores the starting position and appearance for every new concept and video while retaining output and video settings.
  * Local LLM continues with a fresh chat when history reaches the context limit, preserving the current goal, settings, images and progress. A request that still exceeds capacity stops with an explanation.
  * Added Show Max Iterations (Timeline Editor, Max Iterations Display): RFF_Super displays the larger iteration limit of the two source maps in previews and exported videos, independently of Show Zoom Ratio.
  * Zoom Display and Max Iterations Display offer separate positions, fonts, colors, outlines and shadows. Preview Guide has its own settings page.
  * Added Interpolate per Frame (Timeline Editor, Max Iterations Display): the displayed count gradually changes between adjacent source maps. Rendering keeps its existing iteration limits.
  * PNG keyframes now save their maximum iteration count in `.rfsm` metadata. Earlier keyframes remain readable and show N/A when the count is unavailable.
  * Settings and timelines retain both overlays and Interpolate per Frame. Earlier files remain readable, with interpolation off and the saved maximum-iteration visibility preserved.
  * Keyframe generation saves each frame's location and settings in a matching numbered `.rfc` file, including PNG, uncompressed and compressed map keyframes. Saved settings retain the chosen effects and camera view rather than temporary source-generation adjustments.
  * Timeline Editor now labels Text Overlays as Zoom Overlay and offers Max Iterations Display in place of Controls. Each button opens its corresponding settings page, and F1 opens the controls guide.
  * Timeline Editor now lists Max Iterations Display directly above Panel Layout in the settings selector.
* **Fixes**
  * Local LLM now appears in the OBS window capture selection list.
  * Local LLM: RFF_Super now retries interrupted AI generation and, if retries fail during AI zoom or automatic video, restores the starting view and shader, clears AI history and progress, and starts again from the beginning until cancelled.
  * Automatic video now skips AI appearance evaluation when AI appearance adjustment is disabled and retains the current palette when RandomSmooth is also disabled.
  * Local LLM now distinguishes four-number palette.colors entries from five-number palette.stops entries in its appearance instructions.
  * Automatic video now identifies each video and concept attempt.
  * Automatic video now groups settings into Output, Zoom and Appearance pages, with aligned input fields and a larger log that stays visible alongside Start automatic video and Cancel.
  * Timeline overlays now align each text line to the selected left, center or right alignment.
  * RFF_Super now keeps timeline overlay labels steady as digits change within the same digit count, with evenly spaced numbers in previews and exported videos.
  * RFF_Super now vertically centers the module selector, such as Surface Effects, within the workspace header when the header fits on one row.
  * Shader Layers: the help text now stays visible without flickering while resizing the Effects panel.
  * Status bar I now updates after each keyframe calculation, even with the pointer stationary or keyframe preview paused.
  * Timeline Editor: Click to add audio now sits in the center of the empty audio lane, and Audio labels use the same neutral text color as other track names.

* **3.0.0-beta3 - Deep Zoom**
* **Fixes**
  * Status bar messages now align to the left within their existing fields.
  * Status bar P now accommodates three counts of 10000000000 when space allows, while calculation-time and progress fields share the remaining space equally.
  * Status bar I and P values now use commas every three digits, including 1,000 and the values in parentheses.
  * RFF_Super now zooms beyond 1e290 with the mouse wheel and no longer jumps back to 1e290 when zooming out from deeper views.

* **3.0.0-beta2 - Video and Rendering**
  * Timeline time readouts now include hours and days for long videos. Playback, seeking and export frame times now retain fine timing at multi-day positions.
  * Audio preview now starts promptly at distant timeline positions instead of processing the entire preceding silent interval.
  * Timeline playback now previews enabled audio clips with source trimming, volume and fades, including pause, seeking and looping.
  * Add Audio File now applies valid pending edits before opening the file chooser, so another clip can be added without manually applying the previous selection.
  * Audio now shares the normal track list and vertical scrolling, starts below the parameter tracks, and supports row dragging, Alt+Up/Down and Undo/Redo for row order.
  * Timeline Editor now shows an Audio row with named clips. Drag a clip to move its playback position, or drag either edge to adjust its source interval; Escape cancels a drag and Undo restores a completed edit.
  * Added Add Audio File (Timeline Editor, Audio): RFF_Super offers an audio clip selector, source-file selection, start time, source trimming, clip volume, mute and fades in seconds, with clip removal and Undo. Parameters in the right-click menu also opens Audio settings.
  * Added Open Encoder Log (Export workspace): RFF_Super keeps the FFmpeg version, export settings, diagnostics and exit code in a log beside the video output. Export failure dialogs also offer to open the log.
* **Fixes**
  * Video export now includes enabled audio clips with source trimming, timeline placement, Master Volume, clip volume, mute and fades. Audio ends with the video; gaps remain silent.
  * Main view and video preview now present successive frames reliably, including after window resizing.
  * Keyframe generation now rejects invalid Zoom Step per Keyframe values and steps too small to advance at the current depth, instead of repeatedly saving the same zoom position.
  * PNG keyframe generation now restores Stripe, Slope, Fog and Bloom settings on completion, cancellation or failure, including before automatic video creation.
  * Debug: Dump Scene State now stops calculation before reading the scene and remains unavailable while background operations are running.
  * Video export no longer remains at Writing video after the final queued frame has been written.
  * PNG video export now retains the selected PQ or HLG output format and rejects frames whose pixel format does not match the export.
  * PNG video timelines now apply constant Color tracks even when their values match the original scene settings.
  * Map saving and keyframe generation now enforce the same 100000000-pixel limit as map loading, including enlarged camera-padding keyframes.
  * Settings and timeline loading now reject invalid on/off values instead of accepting damaged true/false fields.
  * Video export now weights spatial and temporal antialiasing samples evenly at keyframe boundaries. Some combinations require more rendering samples.
  * Timeline Editor now continues updating the Zoom readout throughout Extra Final Zoom-in and matches the exported zoom.
  * MFR Shoulder, MFR Log and MFR Linear SDR output now applies VHS, Mono and Print Finish with layer ordering disabled; MFR False Color retains its diagnostic appearance.
  * Texture and Pattern cycle coordinates now remain continuous across palette-cycle boundaries with fractional Repeat U values.
  * Workspace dropdown selection fields now retain consistent backgrounds, text positions and focus styling during selection and focus changes.
  * Workspace dropdown lists no longer clear to an empty background before their options are redrawn.
  * Workspace dropdown lists, such as Effects in the Dark theme, now open in one step instead of briefly showing white strips as they slide open.
  * Settings windows now move partially visible input fields to their correct positions when scrolling or folding sections.

## 2026/09/25

* **3.0.0 beta1**
  * RFF_Super accepts 1–1000 FPS for preview and video, 0–8 extra final zoom-in, and consistent color, shading and blur input ranges across settings panels and timeline tracks. Legacy integer inputs reject negative and overflowing values; out-of-range numeric timeline files require adjustment before loading.
  * RFF_Super offers lightweight exploration without FlyWire models or setup downloads.
  * RFF_Super saves compact settings, shader presets and timelines while continuing to read older files and retaining active appearance controls.
  * Added AI Edit (Timeline Editor): RFF_Super offers a button beside Export with 2 x 2 and 3 x 3 image grid choices and folder saving. Timeline JSON loads through Load, validates before replacing edits and supports Undo.
    * Save prompt + JSON and all images to folder saves a combined prompt and every image page in a new dated folder, with progress and cancellation. The backup subfolder keeps the timeline JSON and image index separate from the files to attach to AI.
  * Local LLM: RFF_Super supplies iteration-based candidates and image crops for AI zoom exploration. If a zoom loses detectable structure, exploration restores the previous location, lowers the per-step zoom factor and tries another candidate within Error limit.
  * Local LLM: RFF_Super separates appearance settings and AI zoom exploration, with independent prompts, limits and logs. Exploration can end with Locate Minibrot, retry failed location at the same center with lower log zoom, and begin another exploration after success.
  * Added Start AI zoom (Local LLM): RFF_Super asks a local vision model to choose visible destinations, centers and zooms with an adjustable per-step factor, and waits for each render before continuing. Max changes limits navigation, Cancel retains the current location, and Locate Minibrot is available after exploration stops.
  * Added Automatically apply and refine and Max changes (Local LLM): RFF_Super can start automatic appearance application and image evaluation from Generate settings, with a limit of 1 to 10 applied changes selected before generation.
  * Added Apply and refine (Local LLM): RFF_Super sends the rendered image to a local vision model, applies appearance changes and evaluates the result repeatedly up to a configured limit. Cancellation and Undo remain available, and RFF_Super retains the highest model-scored appearance when refinement ends without intervening edits.
  * Added Error limit and New chat (Local LLM): RFF_Super remembers a limit of 1 to 100 invalid responses and can clear the current chat after cancelling pending generation while keeping the applied appearance.
  * RFF_Super closes the Local LLM window without a crash, cancels pending generation and stops the matching dedicated local llama.cpp server.
  * RFF_Super displays Local LLM context usage, available tokens and generation speed during streaming; estimates are marked with ~, and updating logs preserve text selection and reading position.
  * RFF_Super offers Local AI appearance in the Shader menu: a written description produces editable appearance proposals through a locally configured API, with application, Undo and RFC saving while position and zoom remain unchanged. Invalid proposals return their validation errors to the model up to the configured error limit; model selection and inference options remain in an external configuration file.
  * Added Show Guide Descriptions (Timeline Editor, Zoom Overlay): RFF_Super can display explanatory labels outside the preview or show only the guide margins and dashed frame. Descriptions start off and the choice applies to the current editor session.
  * Added YouTube Shorts Guide (Timeline Editor, Zoom Overlay): RFF_Super previews adjustable subtle shaded margins for interface, title, description and control areas. The guide keeps labels outside the video, serves as an approximate placement aid and never appears in exported video.
  * Added Decimal Places (Timeline Editor, Zoom Overlay): RFF_Super displays 0 to 9 decimal places in preview and exported video. Settings and timelines retain the choice; older files use 6.
  * Added Chaos Blur (Fog settings, Finishing workspace): RFF_Super detects intricate fractal regions at the current location and zoom, gradually softens them with a circular blur and retains smooth surfaces. Detection scale, threshold, transition, feathering, highlight detail and shading are adjustable. Settings, shader presets and timeline tracks retain these controls; older files keep Chaos Blur off.
  * Added Style Application (Effects settings): RFF_Super offers Complete Replacement without the original palette color or Legacy Compositing with the existing blend controls. Settings and shader presets retain the choice; older files use Legacy Compositing.
  * Exploration: RFF_Super removes Minibrot Hunt and Bio Explorer, including the /fly command. Existing settings files still load; new saves omit retired exploration settings.
  * Studio: RFF_Super removes Softbox controls and rectangular reflections from the base and clearcoat. Direct lighting and hemisphere environment light remain available. Existing settings and presets still load; saved Softbox values no longer affect rendering.
  * Base Style: RFF_Super applies coordinated material, finishing and Band Line settings when a style is selected. PHONK includes VHS, Black Metal includes monochrome and white branches, Cyber Sigilism includes spines and ornaments, and Ukiyo-e includes quantization and outlines. Each effect remains independently editable, and workspace Undo restores the entire style change.
  * Added Show Setting Descriptions (View menu): RFF_Super can hide inline help in workspace and Timeline Editor settings, closing the space below each input. The preference persists between launches; validation errors remain visible.
  * Appearance: RFF_Super groups shading and relief controls in Lighting & Relief, gives Band Line its own glow, and applies VHS, monochrome, grain and color quantization only through their independent effect amounts. Materials no longer activate these finishing effects. Existing files retain their values; former material rim light follows Band Line visibility.
  * Band Line: RFF_Super draws spines, branches and margin ornaments with the line color, width and opacity independently of Studio, shading and material effects. Settings and shader presets retain these controls; older files keep decorations off.
  * Effects: RFF_Super controls repeating outlines, spines, branches and margin ornaments through Band Line. Materials no longer draw separate sigil decorations, frost branches or print outlines, and their duplicate controls are absent. Existing settings and presets still load; former material decorations are absent.
  * Timeline Editor: RFF_Super offers a draggable divider for resizing the video preview and track editor, with saved placement and a double-click reset.
  * Added Zoom Overlay (Timeline Editor): RFF_Super offers position presets, drag placement, font selection, size, text color, opacity, outline and shadow settings for preview and exported video. Timelines and settings retain the appearance; older files keep the legacy style.
  * Zoom Overlay: the editing window now stays in place when changing fonts, colors, outlines and position presets.
  * Render: RFF_Super removes Fast mode and its adaptive pixel sampling option.
  * Added Smooth Zooming (Render menu): RFF_Super smoothly zooms and pans with reduced-resolution, SSAA 1 calculation previews, then restores the configured quality after navigation stops. This setting is off by default and applies to the current session.
  * GPU shader preparation: RFF_Super now shows the current pipeline and elapsed time in a preparation window, including in the Timeline Editor. Remaining time is estimated from previous measurements; first-time preparation and estimates that run over are clearly labeled.
  * GPU shader preparation: RFF_Super now spends less time preparing shaders on first use, including Studio materials and ordered layers.
  * Added Studio Softbox Controls (Shader menu, Appearance workspace): RFF_Super offers independent reflection rotation and per-softbox aspect ratio, tilt, brightness and color mixing for the base material and clearcoat. Settings, shader presets, Undo, resets and timeline tracks retain these controls; older files keep their original lighting.
  * Added Show Video Export Preview (Video settings, Export workspace): RFF_Super can omit the video preview during export while keeping progress and cancellation available.
  * Added Find setting (legacy settings windows): RFF_Super searches item names and descriptions in English and the selected language with Ctrl+F, reveals matching sections and moves to the next match with F3.
  * Added Use Legacy Settings UI (View menu): RFF_Super opens the original settings windows from its menus and remembers the selection at startup. Earlier preferences files retain the workspace UI.
  * Added Shader Layers (Shader menu, Appearance workspace): RFF_Super keeps the layer stack below the current appearance settings, with drag reordering, visibility checkboxes and Undo for 29 visual effects, including Band Line, textures, patterns, materials, fog, bloom and tone mapping.
    * Up and Down move the selected layer, Shift with an arrow selects another layer, and Space toggles visibility. Compact checkboxes and a single row of Move Up, Move Down and Reset controls keep the stack clear.
    * Custom Layer Order applies higher layers later and saves order and visibility with settings and shader presets for still images and video. Disabling it restores the original rendering; Reset restores the original order and makes every layer visible. Older files load with their original rendering and every layer visible.
  * Added Panel Layout (Appearance workspace): RFF_Super places the main settings and layer stack on either side and opens up to four additional appearance panels on the left, right or bottom. Removing a panel retains its settings and edit history; panel positions and dimensions persist between sessions.
    * Navigation Panel Position independently places the section list on the left or right across all workspaces, allowing the two sides to swap or share one side. Earlier workspace preferences keep the section list on the left.
    * The section list and main settings panel move by dragging their top bars to either edge of the workspace. Highlighted drop targets show the destination; Escape or releasing outside a target cancels the move, and Left and Right arrow keys move the focused bar's panel.
    * Dropping a panel at an edge places it outside the other main panel on that side, so panels on the right or left can change their horizontal order. Section List Order also selects this arrangement, which persists between sessions.
  * Added MFR Display Modes (Export, HDR): RFF_Super offers MFR Shoulder, MFR Log View, MFR Linear Clip and MFR False Color for SDR preview and output, with separate exposure and a 203-nit virtual reference white.
    * MFR Mastering Peak defaults to 4000 nits and saves with settings and shader presets. Earlier files retain their original appearance. These display modes do not add MFRV file support.
  * RFF_Super now starts with its main window maximized.
  * Added Remove added effects (Appearance workspace, Surface Effects): RFF_Super removes the current category's added materials while retaining the base style and control values. Undo restores the effects.
  * Timeline Editor and video export: Black interior regions now blend smoothly across keyframes without flashing palette colors.
  * Timeline Editor: Color Animation Speed now follows its edited track during preview playback. Paused edits and seeking now show the updated animation, and edits remain responsive while keyframes preload into RAM.
  * Added RAM Preload (Timeline Editor): RFF_Super automatically preloads reduced keyframes when opening a folder and shows the frame count and estimated memory use. Playback uses the cached previews after loading completes.
    * Stop cancels preloading. Insufficient free RAM or a loading failure returns to on-demand loading with a visible status message. Video export retains its original source quality.
  * Added Constant Period (Timeline Editor, Camera): RFF_Super offers Seconds per Turn, Rotation Direction and Rotation Start Angle for continuous rotation, including during zoom holds. Settings and timeline files retain the rotation mode.
  * Timeline Editor: RFF_Super now previews keyframes at half the output width and height, capped at 1280 by 720 with SSAA at 1, while video export retains its configured resolution and sampling quality.
  * Added Language / 言語 (View menu): RFF_Super offers English and Japanese menus, workspace settings, descriptions, validation messages and Timeline Editor controls, with the selected language applied after restarting RFF_Super.
    * Settings search accepts Japanese and English names. Language is saved separately from artwork settings, and earlier preferences files continue to open in English.
  * Added Before Last Edit (Appearance workspace, A/B Comparison): RFF_Super compares current appearance with the start of the latest edit, independently of Fixed A.
    * Reference Source selects the comparison basis. Sliders retain their starting appearance throughout a drag, and cancel restores the previous comparison reference.
  * Added More resets (Appearance workspace, Surface Effects): RFF_Super resets one selected value, the current effect, added effects or all appearance, with one Undo step for each reset.
    * Reset all appearance includes the palette and retains color motion, camera, view and Rendering FPS. Clear added effects retains the base style and chosen values.
  * Added Changed and Recent Filters (Appearance workspace, Surface Effects): RFF_Super lists categories with values that differ from reset defaults or the last five opened categories, alongside All, Used and Favorites.
    * Changed includes numeric and color controls and follows reset and Undo. Recent retains category order and saves visits in workspace preferences separately from artwork files.
  * Added Detail Groups (Appearance workspace, Surface Effects): RFF_Super organizes all 105 surface controls into 30 expandable groups and provides six common controls per Basic page.
    * Detail shows how many advanced controls differ from reset defaults. Each group shows its changed-control count even while collapsed, and search opens the matching group.
    * Group expansion follows mouse and keyboard input and retains its state while switching categories. Invalid numeric input remains open when a group cannot close.
  * Added Minibrot Hunt (Game) (Explore menu): RFF_Super offers three image-search challenges with warmer/colder guidance, three narrowing hint regions and discovery scores based on hint use.
    * Discovery requires matching the target position and magnification. Ending a game restores the fractal view from before the game.
  * Added Effects Across All Styles (Slope menu): editing any Surface Style detail automatically applies its effect to the selected base style, including Original. RFF_Super offers independent material, emission, color quantization, VHS and monochrome amounts.
    * Clear Layers removes the extra effects while retaining the base style and chosen values. Settings, shader presets and Color Animation retain combined effects.
  * Added Deep Sea Bioluminescence and Ukiyo-e Fractal (Slope menu, Surface Style): RFF_Super offers dark blue surfaces with luminous cyan-green detail and particles, or 3–6 flat ink colors with thick outlines and pale wave crests.
    * Deep Sea Bioluminescence provides emission, threshold, body light, particle and color-balance controls with three editable colors. Ukiyo-e Fractal provides color count, outline width, wave foam, woodcut grain, flatness and tone balance with six editable inks.
    * Print Flatness preserves a limited palette before antialiasing and export resizing; lower values allow soft shading. Print Ink can replace black outlines with gold or another chosen color.
    * Settings and shader presets retain the new controls. Earlier files load their defaults, existing styles retain their appearance, and the new materials follow Color Animation.
  * Added Surface Color & Blend and Surface Light & Detail (Slope menu): RFF_Super offers palette color transfer, Mix, Add, Multiply and Screen composition, and separate material, reflection, contour, background and rim colors across all four Surface Styles.
    * Detail Light controls bright fine structure, while Dense Detail Suppression and Detail Density Threshold reduce light in crowded regions. Contour strength, rim width, Liquid Metal glint size and Cyber Sigilism ornament density and inset are independent controls.
    * PHONK Surface Damage starts at zero for the earlier smooth PHONK appearance; worn red scars and heavy tracking blocks remain adjustable. Black Metal Monochrome allows colored frost when reduced below one.
    * Settings and shader presets retain the new controls. Earlier files load default color and detail adjustments in both loaders, and Color Animation follows the customized material.
  * Added PHONK / Drift Phonk and Black Metal Album Art (Slope menu, Surface Style): RFF_Super offers dark red-purple chrome with white flame detail, VHS damage, chromatic separation and partial Nearest sampling, or monochrome frost and branching white contours on black surfaces.
    * Frost Branches extends irregular hanging tendrils and fine cracks around the contours. PHONK Surface Damage combines worn red surfaces with broken tracking blocks, and bright PHONK reflections emphasize edges and isolated glints.
    * PHONK / Black Metal provides 12 independent controls and Reset Dark Detail. Material motion follows Color Animation; film grain and VHS damage remain stationary.
    * Settings and shader presets retain the new controls. Earlier files retain their styles and load default dark-style controls; earlier builds reject the two new style IDs.
  * Added Surface Detail (Slope menu): RFF_Super offers 16 independent controls for chrome reflections, rainbow width and spread, film hue, glints, contour width, spine length and density, branches, margin ornaments and background brightness.
    * Reset Detail restores the detail controls. Surface Style changes retain custom values, and Color Animation follows the adjusted material.
    * Settings and shader presets retain Surface Detail. Earlier files use the previous appearance defaults; older builds ignore the detail controls and discard them when saving again.
  * Added Surface Style (Slope menu): RFF_Super offers Y2K Chrome / Liquid Metal and Cyber Sigilism with Chrome Strength, Thin Film Color and Sigil Edges, following Color Animation.
    * Liquid Metal carries vivid rainbow reflections and bright glints across silver surfaces. Cyber Sigilism combines long chrome-lined spines, branching contours, and fine margin ornaments.
    * Settings and shader presets retain these controls. Earlier files load with Original; older builds ignore the controls and discard them when saving again.
  * Added Free Exploration (No Attraction) (Explore menu): RFF_Super uses neural activity for movement and zoom without favoring boundaries or the screen center.
  * Added Boundary Attraction and Center Preference (Explore menu): RFF_Super offers adjustable boundary bonuses and screen-center preference during exploration.
  * Fly Monitor shows a brain schematic with the 21 stimulated neuron IDs, individual spike counts and the MN9 readout.
  * Added Fly Monitor (Explore menu): RFF_Super displays an animated fly in a separate window with stimulus, neural response and exploration status.
  * Added Start Bio Explorer and Stop Bio Explorer (Explore menu): RFF_Super feeds completed views to a locally installed FlyWire brain model and uses its neural response to control approach and zoom. Escape or manual camera input stops exploration.
    * The sugar-sensing circuit receives detail and visual-change cues; its MN9 response adjusts camera movement toward image-selected features. Brain state continues between views, with fixed connections and no reward learning.
    * Model data download separately with the setup script and are not included in distributions. Start Lightweight Explorer retains the original controller without a model download.
  * Added Bio Explorer Settings and Bio Explorer Status (Explore menu): Zoom Speed, Observation Pause, Stimulus Gain and Brain Response control each exploration step; status reports stimulus, neural firing and completed views.
    * Settings files retain the four controls. Earlier files use normal zoom speed, no observation pause, normal stimulus gain and a 200 ms brain response. Older builds ignore these controls and discard them when saving again.
  * Added Groove Mode, Groove Depth, Groove Width and Auto Groove (Palette menu, Band Line): RFF_Super shades softly recessed bands in their original colors, follows palette animation and warp, and automatically adjusts wide and narrow grooves as the view changes.
  * Added Effects (Shader menu): RFF_Super applies animated Rain, Flame, Embers, Mist, Heat Haze, Ripples, Flow Light and Aurora to the fractal surface, with four independently editable material layers, colors, motion and iteration masks.
    * Materials follow iteration bands and slope direction. Rain forms wet micro-relief and smoother Studio reflections; Heat Haze and Ripples animate surface normals, and luminous materials enter Bloom. Effects require map data and leave the interior untouched.
    * Rain Shape switches between rounded Raindrops and Water Streaks. Drop Size changes droplet radius while the local slope shapes each bead; earlier Effects settings retain Water Streaks.
    * Surface Scale sizes every material. Shape Length and Shape Width adjust flame tongues, ember particles, mist and heat clouds, ripple rings, light segments and aurora curtains.
    * Sync Color Animation makes each material follow the palette animation rate, including pause and reverse, with Speed and Evolution as multipliers. Surface Scale reaches 100.
    * Settings and shader presets retain the four layers. Earlier files load with Effects disabled; older builds ignore and discard these settings when saving again.
  * Added Auto Zoom Compensation and Use Current Zoom (Slope menu): Lustre Relief adjusts its depth around a reference zoom retained in settings and shader presets.
  * Added Iridescence, Film Thickness, Specular Antialiasing and material presets (Slope menu): RFF_Super offers artistic thin-film colors, smoother highlights, and Bronze, Pearl, Obsidian and Ceramic materials.
  * Added Lustre Relief (Slope menu): independent depth, normal smoothing, AO radius, height waves and relief inversion shape the surface.
  * Added Boundary Reflection Guard (Slope menu): reflections soften beside sampled interior pixels.
  * Added Color Stops (Palette menu): an explicit palette conversion opens movable stops, HEX colors, easing, spacing and reversal, with the authored stops retained in settings and shader presets.
  * Added Studio GGX (Slope menu): roughness, metalness and Material IOR shape the surface highlights, with adjustable studio reflections and an independent Clearcoat layer.
    * Studio Material offers Direct Light, Studio Reflections, Softbox Width and Coat Roughness. Settings and shader presets retain these choices; earlier files use the existing shading mode.
  * Added Import Color (Shader menu): RFF_Super previews palettes from settings and shader presets, applies the palette with optional color correction, and offers Undo while the import panel remains open.
  * Added Linear RGB (Palette menu): palette colors blend in linear light in the preview and video output.
    * Existing RGB and OKLab choices retain their values. Earlier builds display palettes saved with Linear RGB using RGB instead.
  * RFF_Super offers Projection in release builds, including 360° Camera and 360° Equirectangular.
  * Added Camera tracks (Timeline Editor): Rotation, Projection, Pitch, Field of View, Panorama Range and Layout animate both map and PNG videos.
  * Added Rotation / 360 padding and Camera padding scale (Video menu): keyframe generation reserves coverage for rotation at the original pixel density, while video output keeps its original size.
    * Rotation and 360° editing require newly generated padded keyframes. Panorama Range is limited by their saved coverage, and larger padding uses more memory and disk space.
  * Settings files from earlier versions still load; missing camera settings retain the session defaults.
* **Fixes**
  * Lighting & Base Material: RFF_Super now renders ordered base materials consistently without intermittent block noise.
  * Surface Effects values now retain trailing zeros shown in the panel when selected for editing, including 1.60.
  * Color Cycle numeric fields now place the insertion point after the value when focused.
  * Smooth Zooming: RFF_Super now continues rendering after repeated zoom input and restores the configured quality when navigation stops.
  * Main window: RFF_Super now moves and resizes normally while the custom menu is closed, and resumes its layout after a long-running operation finishes.
  * Saving an image no longer closes RFF_Super when a very small canvas at low Clarity is saved with Supersampling (SSAA) above 1 — the image is now saved.
  * Loading a settings file whose Clarity and Supersampling (SSAA) exceed the GPU render-size limit now keeps the current settings and explains why, instead of applying a render size the GPU cannot hold.
  * Timeline Editor previews and video export now keep animated Palette, Stripe, Texture, Pattern and Warp positions consistent after seeking and at low frame rates.
  * Fog and Bloom now retain thin highlights when a large image is strongly downsampled for blur.
  * Lustre Relief Waves now keep their phase on imported maps with very large iteration values.
  * Rain and Embers now keep particles continuous across procedural cell boundaries, including wider drops and streaks.
  * Cube Root and LOG palette coloring now retain the intended colors for large frozen values and small positive Pattern Period values.
  * Studio materials now retain defined shading at boundaries between exterior and interior pixels.
  * RFF_Super now stays responsive when mouse-wheel zoom cancels an earlier render.
  * Closing RFF_Super or its Timeline Editor now cancels active video exports started there.
  * Moving animated Texture, Pattern and Effect layers now retains each layer's animation position, including through Undo and Redo.
  * Fog and Bloom now share their blur images with ordered read and write operations.
  * Video packing and main-view blur now receive completed graphics results before computation.
  * Timeline STEP changes and slow speed curves now keep the intended video duration and frame count.
  * Oversized video keyframes and texture images now stop before unsupported GPU resources are created.
  * Canvas sizing now rejects the storage-buffer boundary that exceeds a device limit by one byte.
  * Transparent still-image and palette colors now blend over a black canvas without red contamination or repeated darkening.
  * Image and video exports now make completed GPU readbacks available to the CPU before saving.
  * RFF_Super removes tiled image export from the File menu and Export workspace.
  * Startup and video export: RFF_Super now prepares materials and SDR/HDR video shaders together at startup with a shorter initial wait.
  * RFF_Super now displays fully drawn menus and replaces open menus in place when moving between menu-bar items. Keyboard navigation, submenus and checked settings remain available.
  * Workspace settings now scroll when the mouse wheel is over a closed dropdown without changing its selection. Open dropdowns retain wheel navigation.
  * Workspace dropdown lists now use the current theme color for their background, including the space below their options.
  * Band Line now retains its light and dark contrast on Surface Style materials, including translucent black lines.
  * Timeline Editor now opens in a separate, resizable window from the Video menu or Animation workspace and retains edits and Undo history when reopened.
  * Surface Effects now keeps selection outlines inside their rows and tabs, with complete bottom edges and separate favorite controls.
  * Surface Effects labels and status text now retain spacing before the right-hand controls.
  * Surface Effects now redraws the complete inspector when scrolling, including at either end of the list.
  * RFF_Super now keeps settings input and panel repainting responsive between demanding preview renders.
  * Surface Effects sliders now update the inspector without repainting unrelated panels during each movement.
  * The workspace header now has clearer spacing between its two rows, and text fields now keep vertically centered text across display scales.
  * Settings fields now scroll fully into view when reached by keyboard in short windows, including long lists.
  * Settings fields now keep matching selector and numeric-input heights at different display scales.
  * Slope: the settings panel now opens without crashing as the number of controls grows.
  * Lustre Relief: Relief Waves now retain their reference appearance more closely when zooming out after Use Current Zoom, including their white highlights.
  * Effects: buttons and option labels now fit within the panel without clipping at the right edge.
  * Studio GGX: RFF_Super no longer crashes when the material is enabled or disabled, and the current map and animation continue through the switch.
  * Save dialogs now confirm replacement when RFF_Super adds a file extension, and recognize uppercase extensions already entered.
  * Video export now stops after an encoder failure or cancellation, including during boundary antialiasing.
  * Cancelling while FFmpeg finishes now leaves the selected output file unchanged and reports cancellation.
  * Failed image uploads and FFmpeg startup now release the resources acquired before the failure.
  * Texture layers now repair their sampler bindings after a later layer fails to upload and the settings are retried.
  * Video export now follows the Timeline Editor's duration at low positive Zoom Speed values.
  * Unpadded keyframe generation now clears leftover camera padding metadata, preserving the intended video size.
  * Keyframe generation now stops when an image or map cannot be saved and keeps static image and map numbers paired.
  * Video export now rejects a missing or mismatched first keyframe image instead of writing a black or incorrectly framed video.
  * Numeric settings now reject trailing text and keep the original value when rounded display text is confirmed unchanged. Video Frame Rate also rejects nonfinite values.
  * Save Map now writes the map currently loaded on the canvas, and a failed Load Map leaves browsing at its previous position.
  * Timeline metadata now follows the map actually selected when a damaged compressed keyframe falls back to an uncompressed one.
  * A newly started render now observes its own cancellation state from its first instruction.
  * RFF_Super now keeps the previous shaders available if compilation fails and rejects empty or malformed shader files before use.
  * RFF_Super now preserves the previous graphics cache if a new save is interrupted and starts without loading an oversized cache.
  * Shader settings can now retry after a temporary pipeline creation failure.
  * RFF_Super can rebuild the main and Timeline preview renderers after a failed format change.
  * Legacy settings panels and the recovery menu now use the main window of the current RFF_Super instance.
  * RFF_Super waits for the next frame or window message while idle, reducing CPU use between frames.
  * The Timeline Editor now wakes its idle preview worker reliably when closing or changing keyframe folders.
  * Confirming unchanged Timeline Time now preserves the selected keyframe depth.
  * The Timeline Editor refreshes the preview when capture ends during a key or scrub drag.
  * Layer dragging now moves the layer grabbed at the start even if keyboard selection changes before release.
  * Loading a KFR palette now clears authored stops from the previous palette.
  * Preview animation and video progress timing now remain stable when the system clock changes.
  * Restarting GPU pass timing now measures only frames recorded after the restart.
  * Adding a Timeline shader key now preserves the displayed value of a cubic curve at the insertion point.
  * Whole-turn Hue adjustments now preserve the original colors even at large accepted values.
  * Palette colors now meet continuously at the cycle seam near a wrapped boundary.
  * Palette animation and angle-based decoration now define a stable color at the exact canvas center.
  * Band-aligned decoration now keeps the field direction at image edges.
  * Pattern and Lighting color fields now show RGB inputs, matching the colors their effects use.
  * Lustre highlights now keep a flat horizontal slope on one-pixel-wide images.
  * Saving an image near the normal path-length limit now keeps its temporary file path within the usable length.
  * Dynamic video stripes now retain the selected positive period below 0.0001.
  * Deep video stripes now keep their color multiplier within range at large finite iteration values.
  * Rain and Embers now retain stable particle positions after long synchronized animation.
  * Studio materials now define a stable angle on flat exterior regions.
  * Frozen palette colors now match the intended band with large accepted values under Reversed smoothing.
  * Video export now renders a missing preview frame offscreen and recovers its presentation surface before the next sample.
  * Boundary antialiasing now uses the selected number of color-animation time positions.
  * Video color animation now samples the same pixel centers as still-image coloring.
  * Display colors now retain their intended brightness on an sRGB swapchain fallback.
  * Hiding Band Line now preserves the original color corners of imported palettes.
  * On short windows, additional Appearance panels now display one selected bottom panel at a usable height.
  * OKLab palette previews now use the same color transfer as rendered output.
  * Cycle-based texture and pattern layers now follow warped nonlinear palette bands.
  * Dynamic video now retains map-edge values near keyframe transitions.
  * Static video now blends between keyframe images smoothly at their overlap edge.
  * Low Clarity now keeps at least one internal pixel on each axis.
  * Thin images now retain a valid blur height in the Timeline Editor and video renderer.
  * Video export now accepts filenames beginning with a hyphen.

## 2026/09/06

* **2.2.1 - Faster Coloring**
  * Video frames and the preview are colored faster, with the picture unchanged down to the last bit. A frame of a video export takes about a fifth less time than before, and a frame with frozen colors about a sixth less; the preview pass is about a tenth faster.
    * The coloring is now built for the palette, stripe and animation settings in use, and rebuilt when one of them changes. A change to those settings therefore pauses the picture for a moment while the next one is built.

* **2.2.0.3 - Timeline Export Settings**
  * Export Settings opened from the Timeline Editor now offers only the settings its own Export Video reads. Compress keyframes, Auto-create video after keyframes and Pause keyframe preview stand greyed out there, since the editor exports from keyframes that already exist.
    * The same panel opened from the Video menu keeps every setting editable, and the values themselves are shared either way.

* **Image Viewer**
  * A picture put up by Load Image is now taken off by the mouse: clicking it, or turning the wheel over it, puts the fractal back on the canvas, the way a loaded map is left the moment the view is worked on. Escape still does the same.
    * That first press only hands the canvas back - it does not also pan or zoom. The one after it lands on the fractal and works as it always does.
    * Nothing is recomputed by this: the fractal was never taken down, only covered, and the canvas presents it again on the next frame. The strip the picture gave back is left to that present instead of being erased first, so it does not blink black on the way out.
  * Fixed: stepping through a folder with the arrow keys blacked the canvas out for one frame, about one step in three. The canvas went on presenting the fractal underneath every frame although the picture covered it whole, so the two were writing the same pixels; the presenting is now held for as long as a picture is up and taken up again the moment it comes off. The picture is also no longer re-inserted at the top of the window order on every step, which it never needed.

* **Bailout Range**
  * Bailout (Fractal menu) now accepts anything from 2 up to 1e38, where it stopped at 1,000,000 before. Settings files carrying such a value load as they are.
    * A fresh session now starts at 1e30 instead of 1,000,000. Saved settings keep the bailout they were written with.
    * The field writes a large value in exponent form (`1.00e+06`), the notation the Cycle Length fields use, instead of spelling out all thirty digits.
    * Raising the bailout does not reshape the colouring: the smoothed iteration divides by the log of the bailout, so every value moves by one constant and the band widths stay as they were. 1e6 to 1e30 is a shift of 2.32 iterations, which reads as a palette offset, and costs about that many extra steps per escaping pixel.

* **Slope**
  * RFF_Super now shades steep relief more gently while retaining gentle slopes and fine image detail.

## 2026/09/03

* **2.2.0.2 - Image Viewer**
  * Added Load Image (File menu): a saved `.png` is put on the canvas of the main window, and the arrow keys walk the rest of the pictures in its folder, the way Load Map walks the maps beside the one it opened.
    * Left and Right step one picture, Up and Down step ten, Home and End go to the ends of the folder, and Escape takes the picture off and puts the fractal back.
    * The status bar names the place in the folder. A picture from a keyframe run made under Render from PNG images also names the zoom it stands at, taken from the `.rfsm` of the same number beside it. A picture saved on its own has no such file, and the bar keeps naming the view underneath.
    * The picture is fitted to the canvas and is never blown up past its own size. The fractal is left as it was beneath it, and loading a map or computing again brings it back.
  * The File menu now lists every save first and every load after it, each group in the same order of kind: Save Map, Save Image, Save Location / Settings, then Load Map, Load Image, Load Location / Settings.
  * Dither now sits at the bottom of the Render menu, below Coarse Preview.

* **2.2.0.1 - Fine Shading Gloss**
  * Added Fine Shading (Slope menu, Gloss Source): the gloss bands follow how far the surface faces the light on a relief of the gloss's own, so they ring every form from its crest outward and stay in place at any Shading Depth.
    * Added Gloss Relief (Slope menu): how steeply that relief is read, in doublings from 0 to 16. It is greyed out on the other three sources.
    * Fine Shading is the default source for a fresh gloss. Gloss Bands now starts at 2, Gloss Sharpness at 6 and Gloss Phase at 0.25, so raising Gloss Intensity alone shows the bands.
    * Shader presets and settings files from earlier versions keep the gloss they were saved with. A preset that lacks Gloss Relief loads it at 8; a settings file that lacks it keeps the value the session already had.
* **Fixes**
  * Switching Dark Mode no longer blacks the picture out for a frame.
  * Video export: the preview now opens on its first completed frame instead of showing an empty surface.
  * Recovery now appears before the default view begins rendering.
  * Dark Mode: the status text keeps the same placement as Light Mode and no longer blinks or exposes bright seams while rendering.
  * The picture keeps rendering while a menu is open, which halves the flicker when moving from one menu to the next.
