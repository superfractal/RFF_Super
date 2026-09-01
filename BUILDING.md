# Building RFF Super

Build on Windows with the **MSYS2 MINGW64** toolchain (GCC). Five steps:

1. Install MSYS2 and the packages
2. Add MINGW64 to `Path`
3. Supply GMP
4. Build
5. Supply FFmpeg (needed only to export video)

> Always use the **`mingw64.exe`** shell. The MSYS, UCRT64, and CLANG64 shells use
> different prefixes and will not match this project.

## 1. Install the packages

Install [MSYS2](https://www.msys2.org/), open **`mingw64.exe`**, and update:

```bash
pacman -Syu
```

If the shell closes to finish updating, reopen `mingw64.exe`. Then install everything:

```bash
pacman -S \
  mingw-w64-x86_64-gcc \
  mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-ninja \
  mingw-w64-x86_64-opencv \
  mingw-w64-x86_64-glm \
  mingw-w64-x86_64-zstd \
  mingw-w64-x86_64-vulkan-devel \
  mingw-w64-x86_64-shaderc
```

That is everything the build itself needs. `pacman` installs OpenCV, glm, zstd, Vulkan, and
`glslc` into `C:\msys64\mingw64\` automatically — there is **nothing to place by hand**, and no
manual downloads from opencv.org or LunarG. CMake finds them there.

(`shaderc` provides `glslc`, which compiles the GLSL shaders to SPIR-V during the build.
`zstd` is the entropy coder of the compressed map format.)

If you already have the LunarG Vulkan SDK installed for other work, leave it alone - you do not
need it, but `find_package(Vulkan)` and `compile.ps1` will use whichever install they find first,
so the Vulkan headers and `glslc` may come from the SDK instead of MSYS2. Both are the same
Apache-2.0 upstream sources and the build is the same either way.

CMake must be **4.1.2 or newer** (`cmake --version`); the MSYS2 package is. An older CMake
stops at the `cmake_minimum_required` line.

## 2. Add MINGW64 to Path

So Windows can find the toolchain and runtime DLLs, add to your account's environment
variables:

| Variable | Value |
| --- | --- |
| `MSYS2_ROOT` | your install path, e.g. `C:\msys64` |
| `Path` (append) | `%MSYS2_ROOT%\mingw64\bin` |

Open a new terminal afterward so the change applies.

## 3. Supply GMP

GMP handles the high-precision math. Pick one:

- **Quick:** `pacman -S mingw-w64-x86_64-gmp`
- **Recommended:** build GMP yourself (tuned for your CPU) and drop the result into the
  repo's `include/` and `lib/` folders, which CMake searches first:

  ```text
  include/gmp.h      <- your built header
  lib/libgmp.a       <- your built static library
  ```

These two folders are gitignored — they hold only your local GMP. GMP is the *only*
dependency ever placed by hand at build time.

## 4. Build

From the project root, in the `mingw64.exe` shell:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=g++ \
  -DCMAKE_C_COMPILER=gcc

cmake --build build
```

The result is `bin/RFF_Super.exe`. Shaders compile automatically (the `spirv` target runs
[compile.ps1](compile.ps1)) — no separate step.

## 5. Supply FFmpeg

Video export does not encode in-process: the program pipes raw frames to an **`ffmpeg.exe`**
it starts itself. Nothing else needs it — the build, still image export, and everything on
screen work without it — but **Video → export produces no file** if it is missing.

Install it the same way as the rest:

```bash
pacman -S mingw-w64-x86_64-ffmpeg
```

The program looks for `ffmpeg.exe` **next to `RFF_Super.exe` first, then on `Path`**. The
`pacman` build lands in `%MSYS2_ROOT%\mingw64\bin`, which step 2 already put on `Path`, so
there is nothing more to do. To pin one specific build instead, copy its `ffmpeg.exe` into
`bin/` beside the executable and it wins over `Path`.

Any recent build works as long as it carries the encoders the export modes use:

| Export mode | Encoder required |
| --- | --- |
| Normal | `libx264` (yuv420p) |
| Lossless | `libx264rgb` (RGB 4:4:4, `-qp 0`) |
| HDR (PQ / HLG) | `libx265` with 10-bit support (yuv420p10le) |

The MSYS2 package and the usual gpl Windows builds include all three. Each export writes
`<output>.mp4.log` next to the video containing the exact command line and ffmpeg's own
output — read that first when an export fails.

## Troubleshooting

**Program builds but won't start** — almost always a missing DLL:

- Check `%MSYS2_ROOT%\mingw64\bin` is on `Path`, then open a fresh terminal.
- Or just launch from the `mingw64.exe` shell, where the DLLs always resolve.

**Video export finishes instantly and leaves no `.mp4`** — `ffmpeg.exe` was not found. The
`.mp4.log` stops right after the `[RFF] launching:` line. See step 5.

**Build errors:**

| Message | Fix |
| --- | --- |
| `CMake 4.1.2 or higher is required` | Update MSYS2 (`pacman -Syu`), or install `...-cmake`. |
| `gmp.h: No such file or directory` | Install `...-gmp`, or put your `gmp.h` in `include/`. |
| `undefined reference to __gmp...` | Install `...-gmp`, or put `libgmp.a` in `lib/`. |
| `zstd.h: No such file or directory` | Install `...-zstd`. |
| `opencv2/...` or `glm/...` not found | Install `...-opencv` / `...-glm`; configure from the MINGW64 shell. |
| CMake cannot locate Vulkan | Install `...-vulkan-devel` and reconfigure. |
| `glslc.exe not found!` | Install `...-shaderc`. |
