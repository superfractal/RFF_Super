# Update History

### 2026/09/03

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
