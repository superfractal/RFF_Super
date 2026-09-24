<!-- Modified by GPT-6 on 2026-09-24. -->
# Project file layout

Run build and release commands from the project root, even when reading a document in `documentation/`.

| Location | Contents |
| --- | --- |
| Project root | README, licensing notices, contribution rules, agent instructions, and CMake entry point |
| `documentation/` | [Build instructions](BUILDING.md), [distribution instructions](DISTRIBUTION.md), engineering notes, and provenance reviews |
| `guide/` | [Illustrated settings guide](../guide/SETTINGS_GUIDE.md), user manual, images, and examples |
| `tools/compile.ps1` | Shader compilation, invoked automatically by CMake; resolves sources from the project root |
| `tools/diagnostics/` | Vulkan diagnostic launcher, layer settings, logs, and dated diagnostic dumps |
| `tools/` | Release preparation and local AI launcher scripts |
| `debug/` | Ignored temporary investigations and verification tools |

Start Vulkan diagnostics with `tools\diagnostics\run_with_validation.bat`. Its working directory is the diagnostics folder, so its logs and layer-generated dumps remain together. It launches the existing `bin/RFF_Super.exe`.

## Files that remain at the root

The current executable locates application data relative to the parent of its `bin/` directory. Keep `preferences.rfp`, `timeline-layout.txt`, `workspace-favorites.txt`, `local-ai.json`, `local-ai-server.json`, and `local-ai-system-prompt.md` at the root. The application also writes `crash-report.log` there. Moving these files requires application changes and a compatible migration for existing settings, not just moving files in Explorer.

Keep `bin/`, `shaders/`, `models/`, and `recovery/` in their current locations. Existing executables, configuration paths, or recovery logic depend on them. Build directories also contain cached absolute paths and should be regenerated instead of moved.

CMake uses `tools/compile.ps1`; rerun CMake configuration if an old generated build still points to the previous script location. The release staging tool preserves the `documentation/` paths for build and distribution documents.

## Public source and binary files

Git excludes local preferences, timeline/workspace state, local AI connection/server configuration, logs, dumps, backups, and build products. These files remain on the developer's machine. Use [the server configuration example](../example/local-ai-server.example.json) when setting up another machine.

`bin/` remains excluded from source control. The cleaned local folder contains `RFF_Super.exe` and its 31 required DLLs; FFmpeg and developer tools are stored in the backup rather than included. Video/audio export requires a separately installed FFmpeg on PATH. Vulkan uses the installed GPU driver's loader.

The bin cleanup is not a release build: the current executable differs from the executable in the earlier release inventory. Follow [the distribution instructions](DISTRIBUTION.md) to rebuild from a committed tag and prepare the matching source, shader, dependency, and notice bundle before publishing binaries. Removing files from Git tracking does not remove them from existing commits.
