# RFF_Super Distribution Preparation

Modified by GPT-6 on 2026-09-24, 2026-09-25, 2026-10-01.
Modified by Opus 5.5 on 2026-10-18, 2026-10-28, 2026-11-01.

## Materials published in the GitHub repository

The third-party license texts and the corresponding sources and patches have been moved from the
Git-ignored `build/` to [third-party/](../third-party/README.md). Commit and push this folder together
with the program's `LICENSE` and `NOTICE`.
It holds 23 source packages; the five large ones are split into parts of 24 MiB or less.
Because every part and the restore script are published, anyone can restore them without an external download.
All 23 have been restored this time and matched against their original SHA-256.

This prepares the third-party materials for publication. Establishing the correspondence between the
current program EXE and the program source is separate work.
The candidate-building steps inside `build/` below can be used to recollect and recheck when dependencies are updated.

## Current configuration

RFF_Super remains GPLv3. Commercial use and sale are also permitted.
Switching to static linking does not remove the need for license notices or for supplying the corresponding source.

| Component | Distribution form | Basis / handling |
| --- | --- | --- |
| RFF_Super | EXE | GPLv3. Supply the source and build instructions of the tag that was built, together |
| GMP 6.3.0 | Static link | GPLv3 chosen from the GPLv2-or-later option. Supply the same version's source, MSYS2 patches and build conditions. GMP's own upstream alternatives remain |
| OpenCV core/imgproc/imgcodecs | DLL | Apache-2.0 and the terms of the bundled parts. Image-format support is kept |
| zstd, image codecs, numerical libraries | DLL | Only the DLLs the EXE needs recursively are selected. Versions, hashes and original texts are recorded |
| GCC runtime, libgfortran | DLL | GPLv3-or-later with the GCC Runtime Library Exception. Redistribution obligations for the DLLs themselves were also checked |
| libquadmath | DLL | LGPLv2.1-or-later. Not lumped in with the GCC exception |
| MinGW runtime, GLM, stb_image, nlohmann/json | Static / header | Each copyright notice and its terms are retained |
| Vulkan loader/driver | The user's GPU driver | Not copied from the working bin |
| FFmpeg, local AI server and models | Supplied separately by the user | Not included in this distribution candidate |
| FlyWire, whole-brain models, PyArrow | Removed | Source, UI, installer tools and the dedicated model folder were removed |

The checked configuration is the program plus 31 required DLLs. The many development-tool and
unrelated DLLs in the working `bin/` are not distributed. To keep OpenCV's image loading, its codec
dependencies were not simply removed just to reduce the DLL count.
Exact versions are recorded in `tools/release-license-review.json`, and individual files in the generated
`DEPENDENCIES.json`. The packaging tool stops if a dependency version changes.

## Points rechecked against the original texts

* The MSYS2 metadata for JBIG-KIT abbreviates its license to `GPL-2.0`, but the obtained
  2.1 `libjbig/jbig.c` states "version 2 ... or ... any later version".
  It was confirmed that this is not GPLv2-only and that GPLv3 can be chosen.
* The XZ package as a whole also lists GPL/LGPL, but `COPYING` describes the
  distributed liblzma as 0BSD. The xz command-line tools are not bundled.
* For OpenCV's main license and GMP's COPYING files, the installed license folder
  alone is not enough. The original texts from the corresponding source archives are bundled too.
* JPEG notice for documentation: This software is based in part on the work of the Independent JPEG Group.
* OpenStax's CC BY-NC-SA 4.0 covers textbook content. The shaders concerned are independent
  implementations of physical formulas; no textbook prose, figures or sample code were found in them.
  The source remains cited in NOTICE. The owner's statement that the original Lustre HTML was AI-generated at their request is recorded.

The check has two stages. First, the actual files are matched against the hashes of the installed packages,
and the official same-version sources, patches and licenses are confirmed. Next, DLL dependencies are
cross-checked with an independent `objdump`, and the hashes of all files after packaging are recorded.
This is a technical audit record for the versions and files checked.

## Running the distribution build in one step

The distribution build is done with `tools/release_build.ps1`. Tag the release commit first, then
run it from the project root.

```powershell
git tag v3.1.0
powershell -ExecutionPolicy Bypass -File tools/release_build.ps1 -Ref v3.1.0
```

The script performs the following in order and stops as soon as any step fails.

1. Matches the SHA-256 of the GMP static library against the recorded value for MSYS2 GMP 6.3.0-2. It uses `build/release-deps/mingw64` if present,
   otherwise `C:/msys64/mingw64` (changeable with `-GmpPrefix`)
2. Checks out the tagged commit cleanly into `build/release/<tag>/source` (git worktree)
3. Builds the distribution (EXE and shaders) from that checkout. CMake rechecks the GMP hash as well
4. Confirms after the build that the checkout was not modified
5. Builds the candidate with the matching, fetching and packaging tools contained in that tag
6. Stores the program source, `git archive`d from the tag, as `candidate/sources/RFF_Super-source.tar.gz`
7. Removes `NOT-FOR-DISTRIBUTION.txt` and regenerates `SHA256SUMS.json`
8. Records the commit, tool versions, and the GMP, EXE and source hashes in `build-record.json`

When someone else runs it, every MSYS2 package must exactly match the version recorded in
`tools/release-license-review.json`. A different version stops at the packaging stage. Because MSYS2 is
updated continually, when releasing with newer versions, recheck the licenses before updating the versions in this file.

The distributable output is `build/release/<tag>/candidate`. To try it without a tag, use
`-Ref HEAD -Candidate`. In that case the marker is kept and the output is not for distribution.
The working `bin/RFF_Super.exe` is built with `-march=native` and an unverified GMP, so it is not distributed.

## Building the candidate folder (manual)

Requirements: Windows x64, MSYS2 MINGW64, Python 3.10 or later, Windows `tar.exe`,
and MSYS2's `gpgv` and `msys2-keyring`. Python is for distribution preparation only and is not needed to run the application.
The basic build steps are in [BUILDING.md](BUILDING.md).

In PowerShell, add the MINGW64 bin to Path and use a separate build directory for distribution.
GMP uses the static library from the official MSYS2 package. CMake stops at configuration if its SHA-256 differs from the recorded value.
`C:/msys64/mingw64/lib/libgmp.a` was restored to the official package on 2026-10-28 and matches the recorded value.
Before using it, confirm with `pacman -Qkk mingw-w64-x86_64-gmp` that it is unmodified. When using a custom GMP,
also record its source, patches and build conditions separately, and update the current matching tool.

```powershell
$env:Path = "C:\msys64\mingw64\bin;" + $env:Path
cmake -S . -B build/distribution -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc -DRFF_DISTRIBUTION=ON -DRFF_ASAN=OFF -DRFF_GMP_PREFIX=build/release-deps/mingw64
cmake --build build/distribution --parallel 6
python tools/release_inventory.py --exe build/distribution/runtime/bin/RFF_Super.exe --static-gmp build/release-deps/mingw64/lib/libgmp.a --output build/release-inventory.json
python tools/release_sources.py --inventory build/release-inventory.json --output build/release-sources
python tools/release_stage.py --inventory build/release-inventory.json --sources build/release-sources --shaders build/distribution/runtime/shaders --output build/release-candidate
```

Run the commands from the project root, where `CMakeLists.txt` is, not from inside `documentation`.
If MSYS2 is installed elsewhere, adjust Path and `-DRFF_DEPENDENCY_PREFIX=...` to match,
and change the GMP location too. The build directory stays inside the project for shader generation.
Use the MINGW64 Python above and the Windows `tar.exe`.
When a dependency update changes a version, recheck rather than reusing the previous audit list as is.

A non-standard GMP location is given with `-DRFF_GMP_PREFIX=...`. The audit used the
headers and static library extracted from the official GMP package in the MSYS2 cache into
`build/release-deps/mingw64`, and confirmed their hashes against the package record.
In that case, pass the same actual file to the inventory's `--static-gmp` as well.

The distribution build generates its shaders in `build/distribution/runtime/shaders`.
The regular `shaders` folder is not updated, so it never mixes with the executable of another build.

The candidate folder contains the EXE/DLLs, shaders, copyright notices, 23 third-party source packages
(including build recipes and patches), and signature-verification records.
Use a new, empty name for the output. An existing folder is never overwritten.
Already fetched sources can be reused. `SHA256SUMS.json` is for verifying the whole candidate.

## Final steps before actual distribution

The candidate carries `NOT-FOR-DISTRIBUTION.txt`. The program source in the working tree is never
zipped. Prepare a release tag with the changes committed, rebuild from a clean checkout of that tag,
and run the matching and packaging above.
The source is created only from that tag.

```powershell
git status --short
git archive --format=tar.gz --prefix=RFF_Super-source/ --output=build/RFF_Super-source.tar.gz <release-tag>
```

Confirm that the tag points to the commit that was built and that no tracked file has uncommitted changes.
Supply the program source, third-party sources, build instructions and license notices together with the binary,
and for download distribution make the corresponding source available from the same place.
Remove the candidate marker after this check. If FFmpeg or AI models are ever bundled,
check those actual files and their build configuration anew.

## Primary sources

* [GPLv3 text](https://www.gnu.org/licenses/gpl-3.0.html)
* [GMP's selectable licenses](https://gmplib.org/manual/Copying)
* [GCC Runtime Library Exception](https://gcc.gnu.org/onlinedocs/libstdc++/manual/license.html)
* [JBIG-KIT official source](https://www.cl.cam.ac.uk/~mgk25/jbigkit/)
* [MSYS2 official source packages](https://repo.msys2.org/mingw/sources/)
* [Scope of MSYS2 license information](https://www.msys2.org/license/)
* [CC BY-NC-SA 4.0 legal code](https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.en)
