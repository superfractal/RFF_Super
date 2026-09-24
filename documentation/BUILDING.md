# Building RFF_Super

Modified by GPT-6 on 2026-09-24.

This guide targets Windows x64 with the project's MSYS2 MINGW64/GCC toolchain. Run all project commands from the directory containing `CMakeLists.txt`, **not** from `documentation/`.

A public source checkout does not need the developer's `include/`, `lib/`, `bin/`, local settings, or backup folders. Install dependencies through MSYS2. CMake builds the executable and shaders; it does **not** collect runtime DLLs or produce a complete release package.

For publication, use the distribution build below, then follow [DISTRIBUTION.md](DISTRIBUTION.md). A local Release build uses `-march=native` and is intended for the machine that builds it.

## 1. Install the packages

Install [MSYS2](https://www.msys2.org/) and open its **MINGW64** shell (`mingw64.exe`). The commands in this section are Bash commands for that shell.

This project still uses `/mingw64` dependencies and release inventory names. MSYS2 now lists MINGW64 as deprecated and recommends UCRT64 for new projects; changing this project to UCRT64 requires a separate toolchain/dependency migration. Do not mix MINGW64 and UCRT64 libraries or reuse a build cache from another toolchain. See [MSYS2 environments](https://www.msys2.org/docs/environments/).

Update the installation:

```bash
pacman -Syu
```

If the update asks you to close the terminal, do so, reopen MINGW64, and run `pacman -Syu` again to finish. MSYS2 supports full system upgrades; see [its update instructions](https://www.msys2.org/docs/updating/).

Install the build dependencies, including GMP:

```bash
pacman -S --needed \
  mingw-w64-x86_64-gcc \
  mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-ninja \
  mingw-w64-x86_64-gmp \
  mingw-w64-x86_64-opencv \
  mingw-w64-x86_64-glm \
  mingw-w64-x86_64-zstd \
  mingw-w64-x86_64-vulkan-headers \
  mingw-w64-x86_64-vulkan-loader \
  mingw-w64-x86_64-shaderc
```

The [Vulkan headers](https://packages.msys2.org/packages/mingw-w64-x86_64-vulkan-headers) and [loader package](https://packages.msys2.org/packages/mingw-w64-x86_64-vulkan-loader) supply the development headers and import library. `shaderc` supplies `glslc`, the shader compiler. Install a suitable GPU driver separately to run the application; development packages do not replace the GPU driver.

Check the tools before configuring:

```bash
echo "$MSYSTEM"
which gcc g++ cmake ninja glslc
cmake --version
powershell.exe -NoProfile -Command '$PSVersionTable.PSVersion'
```

`MSYSTEM` should be `MINGW64`; the compiler and build tools should come from `/mingw64/bin`. CMake must be **4.1.2 or newer**. The shader build invokes Windows PowerShell as `powershell`, so it must be available on PATH.

A separate LunarG Vulkan SDK is optional when the MSYS2 development packages are installed. If both are present, inspect CMake's selected Vulkan paths; `tools/compile.ps1` chooses `glslc` from PATH before its Vulkan SDK fallback.

## 2. Add MINGW64 to Path

The MINGW64 shell sets its own PATH. You can build and run from that shell without changing your account-wide environment.

For PowerShell commands, use a session-local setting (replace the prefix if MSYS2 is installed elsewhere):

```powershell
$env:Path = "C:\msys64\mingw64\bin;" + $env:Path
```

For launching a locally built executable from Explorer, either add `C:\msys64\mingw64\bin` to your account's Path and start a new session, or prepare the required runtime DLLs alongside the executable using the release workflow. `MSYS2_ROOT` is not read by CMake; setting that variable alone does not configure dependency paths.

Do not copy the entire MSYS2 `bin` folder into the project. It contains compilers, unrelated programs, and hundreds of libraries. Old DLLs already beside the executable can take precedence over updated dependencies on PATH; keep runtime files consistent with the libraries used for the build.

## 3. Supply GMP

The [official GMP package](https://packages.msys2.org/packages/mingw-w64-x86_64-gmp), installed in step 1, supplies both the header and libraries. **No project-local `include/` or `lib/` folder is required for a fresh checkout.** The old `include/opencv2.unused-4.11/` directory is not a build dependency and should not be copied into a release.

Advanced local builds may supply a matching custom `include/gmp.h` and `lib/libgmp.a`. Ordinary builds search these local directories, so a stale header or library there can affect the result. Keep custom builds separate from the package-based release workflow.

Distribution mode explicitly selects `libgmp.a` from `RFF_GMP_PREFIX` and uses headers from that prefix. The default is `C:/msys64/mingw64`. The release inventory verifies the static library against MSYS2 package records; an independently built GMP requires a separately documented and reviewed source/build path.

## 4. Build

Choose one configuration. Use a new build directory when changing toolchains or dependency installations. Keep distribution build directories **inside the project**: the shader script validates that its output stays within the project and ends in a dedicated `shaders` directory.

### Distribution build

From the project root in the **MINGW64 shell**:

```bash
cmake -S . -B build/distribution -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=g++ \
  -DCMAKE_C_COMPILER=gcc \
  -DRFF_DISTRIBUTION=ON \
  -DRFF_ASAN=OFF

cmake --build build/distribution --parallel 2
```

If MSYS2 is elsewhere, add `-DRFF_DEPENDENCY_PREFIX=C:/your-msys2/mingw64` to the first configuration of a new build directory. `RFF_GMP_PREFIX` defaults to that prefix; specify it explicitly if GMP comes from a different matching package installation. Existing CMake cache values do not automatically follow a changed prefix.

Outputs:

```text
build/distribution/runtime/
  bin/RFF_Super.exe
  shaders/*.spv
```

This selects a generic x86-64 CPU target, static GMP, and shared zstd/OpenCV. The application still needs the shared runtime dependencies. To try it on the build machine, run `./build/distribution/runtime/bin/RFF_Super.exe` from MINGW64. For other machines, use the dependency collection and source/notice packaging steps in [DISTRIBUTION.md](DISTRIBUTION.md); the build output alone is not that package.

### Local Release build

For use on the build machine, from the **MINGW64 shell**:

```bash
cmake -S . -B build/local -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=g++ \
  -DCMAKE_C_COMPILER=gcc \
  -DRFF_DISTRIBUTION=OFF \
  -DRFF_ASAN=OFF

cmake --build build/local --parallel 2
./bin/RFF_Super.exe
```

This writes `bin/RFF_Super.exe` and `shaders/*.spv`, replacing the corresponding local outputs. It links GMP by library name, so the selected local/package library determines whether an additional GMP DLL is needed. The DLL list from a distribution build must not be assumed sufficient for every local build.

For Debug, use another directory with `-DCMAKE_BUILD_TYPE=Debug -DRFF_DISTRIBUTION=OFF`. The executable is `bin/RFF_Super_Debug.exe`. `RFF_ASAN=ON` probes AddressSanitizer support; if it cannot link, CMake warns and continues without it. Distribution mode requires Release.

Both builds compile shaders automatically through [tools/compile.ps1](../tools/compile.ps1). Keep `bin/` and `shaders/` as siblings: the shader loader resolves paths relative to the executable. Lower `--parallel` if compiler processes run out of memory.

## 5. Supply FFmpeg

FFmpeg is optional for building and still-image rendering, and required for video/audio export workflows. It is not included in the cleaned application `bin` folder.

Install it in the **MINGW64 shell**:

```bash
pacman -S --needed mingw-w64-x86_64-ffmpeg
ffmpeg -version
ffmpeg -encoders | grep -E 'libx264|libx264rgb|libx265'
```

The video exporter checks for `ffmpeg.exe` beside `RFF_Super.exe` first, then searches PATH. Keeping the installed MSYS2 FFmpeg on PATH avoids copying its additional runtime dependencies. Copying only the MSYS2 `ffmpeg.exe` to another machine is insufficient.

| Export mode | Required encoder |
| --- | --- |
| Normal | `libx264` |
| Lossless RGB | `libx264rgb` |
| HDR (PQ / HLG) | `libx265` with 10-bit support |

The current video exporter redirects FFmpeg diagnostics to `NUL` and removes a stale output `.log`; it does **not** promise a per-export `.mp4.log`. If export fails, first check that FFmpeg runs in the environment used to launch RFF_Super and provides the required encoder.

## Optional tools and publication

Python is not needed to build or run the application. The release inventory, source-download, and staging scripts require Python 3.10 or newer, Windows `tar.exe`, and MSYS2 `gpgv`/`msys2-keyring`. Install their packages in MINGW64:

```bash
pacman -S --needed mingw-w64-x86_64-python gnupg msys2-keyring
```

Run the release commands in [DISTRIBUTION.md](DISTRIBUTION.md) from **PowerShell**, with the MINGW64 bin folder first on PATH. Its `tar.exe` invocation expects Windows tar, not an MSYS tar executable. The reviewed dependency versions are recorded in `tools/release-license-review.json`; package upgrades can require a new review before staging succeeds.

Local AI is optional and needs its own server/model configuration. See [the Local AI guide](../guide/local-ai.md). Personal configuration and saved workspace state are excluded from source control.

Publish binaries with matching shaders, runtime dependencies, notices, and corresponding source materials as described in the distribution guide. Create source archives from a committed release tag, not by zipping the working folder. See [project file layout](project-layout.md) for which directories are local-only.

## Troubleshooting

| Symptom | Check |
| --- | --- |
| CMake version is too old | Run `cmake --version` and check which executable PATH selects; the minimum is 4.1.2. |
| Compiler or CMake generator changed | Configure a new build directory instead of reusing a cache from another toolchain. |
| `gmp.h` missing or GMP link failure | Install the GMP package; check matching headers/libraries and stale local `include/` or `lib/` files. |
| OpenCV, GLM, or zstd missing | Install the corresponding MINGW64 packages and configure from the matching environment. |
| Vulkan not found | Check the Vulkan headers and loader development packages, or the selected Vulkan SDK paths. |
| `glslc.exe not found` or PowerShell missing | Check `glslc` and `powershell.exe` on PATH; shader compilation requires both. |
| Build still refers to the old root `compile.ps1` | Rerun CMake configuration; the script is now `tools/compile.ps1`. |
| Shader output path is rejected | Use a distribution build directory inside the project; retain the generated `runtime/shaders` layout. |
| Missing DLL or entry-point error at startup | Use matching MINGW64 runtime DLLs; check for stale copies beside the executable. |
| Vulkan initialization fails | Check the GPU driver and application error; do not copy an arbitrary loader DLL into `bin`. |
| Shaders cannot be opened | Complete the shader build and keep the sibling `bin`/`shaders` layout. |
| Video export fails | Run `ffmpeg -version` and check encoders using the same PATH as the application. |

Verification on 2026-09-24: fresh CMake build directories configured successfully for both distribution and local Release modes using the installed MINGW64 GCC 16.1.0/OpenCV 4.13.0 environment. The distribution shader target compiled all 16 SPIR-V outputs with MSYS2 glslc. This machine used installed LunarG Vulkan headers because its MSYS2 Vulkan headers were absent. A fresh MSYS2 installation, a complete application rebuild, and runtime rendering were not tested in this review.
