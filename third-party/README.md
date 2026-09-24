<!-- Modified by GPT-6 on 2026-09-24, 2026-09-25. -->
# Third-party licenses and corresponding source

This folder is tracked in Git for publication. When distributing RFF_Super binaries, also publish this folder and the root `LICENSE` and `NOTICE` files.

## Contents

- `licenses/`: Original license texts and copyright notices for DLLs, statically linked components, and bundled source. Additional original notices collected from upstream source are in `licenses/source-notices/`.
- `sources/`: 23 MSYS2 third-party source packages, including upstream source, packaging recipes, and applied patches.
- `sources/sources.json`: Original package names, official URLs identifying the versions, SHA-256 hashes, and signature-file SHA-256 hashes.
- `sources/archive-parts.json`: Part order and SHA-256 hashes, plus the size and SHA-256 hash of each restored archive.
- `sources/*.sig` and `*.verification.txt`: Original archive signatures and records of earlier signature verification.
- `restore-sources.ps1`: A script that restores split archives and verifies the hashes of all 23 packages.

The five archives larger than 24 MiB are split into `.part001`, `.part002`, etc., each no larger than 24 MiB. Publish every part. The original archives can be restored without Git LFS or external downloads.

## Restore and extract the source

Obtain the complete repository, then run the following in Windows PowerShell from the project root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File third-party/restore-sources.ps1
```

All 23 archives are created in `debug/restored-third-party-sources/`. The published archive parts remain unchanged. Existing output is reused if its hash matches; otherwise, the script stops.

For example, extract the GMP source package using Windows `tar.exe`:

```powershell
New-Item -ItemType Directory -Force debug/gmp-source
tar.exe -xf debug/restored-third-party-sources/mingw-w64-gmp-6.3.0-2.src.tar.zst -C debug/gmp-source
```

The extracted `PKGBUILD` is the MSYS2 build recipe. Refer to it together with the bundled upstream archives and patches. The GCC source package includes the source for libquadmath and the GCC runtimes.

## GMP, JBIG-KIT, and libquadmath

- **GMP 6.3.0**: Licensed under GPLv2 or later / LGPLv3 or later. For RFF_Super's binary distribution, this project selects GPLv3 under the GPLv2-or-later option. The upstream licensing alternatives and original texts are preserved unchanged; the exact linked source and build inputs still require verification.
- **JBIG-KIT 2.1**: The original implementation notices permit GPLv2 or any later version, allowing GPLv3 to be used.
- **libquadmath**: Licensed under LGPLv2.1 or later. It is treated separately from the exceptions for other GCC runtimes; the corresponding GCC source and original license text are included.

The original notices for each component take precedence. References: [GMP](https://gmplib.org/manual/Copying), [JBIG-KIT](https://www.cl.cam.ac.uk/~mgk25/jbigkit/), and [GCC runtimes](https://gcc.gnu.org/onlinedocs/libstdc++/manual/license.html).

## Binary correspondence and updates

On 2026-09-24, the 31 DLLs in the current `bin/` folder were verified to match the SHA-256 hashes from the recorded MSYS2 packages. See `licenses/review.json` for package versions.

Publishing this material does not establish the provenance of RFF_Super.exe itself. Its correspondence to the application source, and the version and build of GMP actually linked statically, must be verified separately. When updating dependencies, update this folder to the corresponding versions as well.

## FFmpeg setup

FFmpeg is supplied separately by the user and is not bundled here. See the [FFmpeg setup guide](../guide/ffmpeg-setup.md) for Windows installation and verification steps.
