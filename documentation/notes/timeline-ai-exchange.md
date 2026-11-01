<!-- Modified by Opus 5.5 on 2026-11-01. -->
# Exchanging the Timeline with a Generative AI

Updated: 2026-09-24 (GPT-6)

Open it from the **AI Edit button to the right of Export** at the top of the timeline. The menu has three items: 2 x 2, 3 x 3, and save to folder.

## Saving everything to a folder

Choose **Image grid: 2 x 2** (4 images) or **Image grid: 3 x 3** (9 images), then choose **Save prompt + JSON and all images to folder...**. After you pick a destination, a dated `timeline-ai-...` folder is created inside it. If a folder with the same date and time exists, a sequence number is appended; nothing is overwritten.

- `prompt.txt`: the combined prompt of the system prompt, a description of the current state, and the editable JSON.
- `page_000001.png` onward: every image from the first page to the last. It does not depend on the page currently selected.
- `backup/timeline.json`: a backup for reading back in. It matches the JSON inside `prompt.txt`.
- `backup/images.json`: which image file belongs to which keyframe numbers, the number of pages saved, and the completion state. Image file names are relative to the top of the saved folder.

Attach `prompt.txt` from the top of the saved folder and the images you need to the generative AI. The `backup` folder does not need to be attached. Saving every page needs no "Next" step. Audio source paths are saved as absolute paths so they still work when the AI's answer is loaded from the clipboard. The audio sources themselves are not copied.

The images are generated in order with the edit values and appearance at the moment saving starts. While saving, the page count is shown, and **Cancel image generation** in the same menu stops it. Switching folders or closing also interrupts saving. Files written so far remain, and the state in `backup/images.json` becomes `cancelled` or `failed`. It becomes `complete` only when every page has been written.

## Loading the AI's answer

Save only the complete JSON the AI returned to a .json file, and load it with the timeline's **Load**. Do not include Markdown fences or explanatory text. **Save** can write either JSON or the older .rfvt format.

## Scope and behavior

- The JSON replaces the whole timeline, including speed, color and shader animation keys, hold sections, rotation, zoom display and audio clips. Loading supports undo and redo. It does not load the base shader settings, the keyframe image folder, output settings or the source files themselves.
- The images are preview renders of the current folder's RFM/RFMZ or RFSM/PNG with the current timeline and appearance. They contain no zoom text or editing guides. 2 x 2 is 1024 x 704 pixels and 3 x 3 is 1536 x 1056 pixels, keeping the aspect ratio. Saving to a folder covers every page.
- Saving needs a keyframe image folder that can be previewed. Without one selected, saving to a folder is disabled.
- The system prompt's appearance description comes from the bundled `local-ai-system-prompt.md`, and target IDs, value ranges and enumerated values come from the actual timeline definitions. If the file cannot be found, the reason is shown.
- Because PNG images have their colors baked in, tracks whose catalog entry has `affectsPNG` set to `false` do not affect the render.
- Before loading, the format, version, value ranges, duplicate keys, audio times and so on are validated. On an error, the current edit is kept. The JSON may be up to 16 MiB. When a folder is open, `estimateKeyframes` must match the current number of keyframes.
- There is no automatic sending or connection to an AI service. Attach the saved prompt and images to the generative AI you use by hand.
