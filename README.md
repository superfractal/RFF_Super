# RFF Super

> **IMPORTANT NOTE:**
> **This is an improved and extended version of the original [RFF-2.0](https://github.com/Merutilm/RFF-2.0) by Merutilm — a modified version of it, changed from 2026-07-05 onward. Each changed source file carries its own modification notice and date.**
> This project was developed using vibe coding. As I am not a mathematics major or a highly experienced programmer, there may be imperfections.
>
> The precision of the Mandelbrot set calculations may be lower than that of the original version.

## The Original and This Fork

RFF-2.0 by Merutilm is the original. This repository is a fork of it, pulled in a different
direction, so "better" depends on what you are doing:

| | **RFF-2.0** (Original) | **RFF_Super** (this fork) |
| --- | --- | --- |
| **Priority** | Stability | Features and visual quality |
| **Availability** | Public [Merutilm/RFF-2.0](https://github.com/Merutilm/RFF-2.0) | Public |
| **Best for** | Reliable everyday exploration | Producing the most beautiful stills and videos |
| **Strengths** | Simpler and easier to use; latest, faster, and stable releases; highest calculation precision | Custom formulas, extra shader types, palette settings, free rotation, presets |
| **Trade-offs** || More settings to learn; precision may be lower than the original; more bugs |

## Building from Source

See **[BUILDING.md](BUILDING.md)** for the full compile guide.

## Features of the Improved Version

### Differences from Standard RFF

Based on RFF 2.1.2.3, the following features have been added or improved:

* Added custom formulas (zoomable up to e14)
* Added new Shader types
* Added palette settings
* Boundary tracing
* Freely adjustable fractal Rotation
* Improved User Interface (UI)
* Settings save / load (`.rfc`): store and recall the full configuration — location, Fractal/Render/Resolution/Shader/Video — in one file
* Shader preset save / load (`.rfsp`): store and recall the full Palette/Stripe/Slope/Color/Fog/Bloom setup
* Supersampling
* etc...

## Known Issues & Areas for Improvement

The following issues are currently known:

* The precision may be lower than that of the original.
* Unintended black or white lines may appear on the screen.
  * **Fix:** Lower `Precision Level` until the lines disappear. This is most common cause, but the lines may rarely appear for other reasons too.
* Moving the mouse from one menu to the next can show the opening menu as an empty bordered
  rectangle for a single frame, with the picture visible straight through it.
  * **Cause:** Windows makes a fresh popup window for every menu, shows it, and only paints it
    about 2 ms later — measured with message hooks. A frame composited inside that gap goes out
    with the window on screen and nothing drawn in it. A stock Win32 program with a plain menu
    bar reproduces this exactly, so it is not specific to RFF Super; it is only conspicuous here
    because what sits behind the menu is a picture rather than a still, pale window. Nothing on
    the menu API side changes it — the menu background brush, the fade and animation settings,
    and the DWM window attributes were all measured and made no difference.
  * **Status:** the picture now keeps rendering while a menu is open, which halves the exposure
    from two frames to one. Removing it entirely needs menus that are drawn by this program
    instead of by Windows.
* A long video export at a high resolution can abort part-way with `Failed to submit queue! VK_ERROR_DEVICE_LOST`, killing the process and leaving a truncated `.mp4`. The GPU is lost after tens of minutes of sustained load; where it stops varies from run to run. Validation layers report no errors during a full run, so the cause may lie in the driver or the hardware — but a bug in this application has not been ruled out.
  * **Workaround:** lower `Supersampling (SSAA)` before generating keyframes. Per-frame GPU load is `window client size x Clarity x SSAA`, but the video output size is only `window client size x Clarity` — so lowering SSAA cuts the load without changing the output resolution.

## License

RFF Super is free software under the **GNU General Public License v3**; the full text is in
[LICENSE](LICENSE), which also reproduces the licenses of the components this program links.

Third-party components, the named algorithms implemented in the source, and the license each one
is taken under are recorded in [NOTICE](NOTICE). The libraries other than stb_image are not
redistributed here and are installed separately; see [BUILDING.md](BUILDING.md). Their license
terms still apply to any prebuilt binary that links them, so review NOTICE before distributing
one.
