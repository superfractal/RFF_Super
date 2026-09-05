# Update History

## 2026/09/

* **3.0.0**

## 2026/09/06

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
