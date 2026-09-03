# Update History

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
