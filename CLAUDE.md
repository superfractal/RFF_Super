# Project Instructions

Read and follow [AGENTS.md](AGENTS.md) before doing anything in this repository. All project guidelines, structure, and conventions are defined there.

## Modification Notices (GPL)

This project is under the GPL, which requires each modified file to carry a prominent notice stating **that it was changed and the date of the change**. When you (the AI) add or modify code, the modification tag MUST include the change date in ISO form, e.g.:

```cpp
// Modified by Opus 4.8 on 2026-07-05
```

Follow the placement rules in AGENTS.md ("Code Modification Tracking"): put the tag below any existing leading comment block, and do not add a second tag for the same model/version already present in the file. If that same model edits the file again on a later date, **append the new date to the existing tag — never overwrite the earlier date** (overwriting would erase the record of the earlier change, which GPL requires you to preserve). Keep a single tag line and list the dates:

```cpp
// Modified by Opus 4.8 on 2026-07-05, 2026-09-01
```

Never abbreviate modification dates with a range or ellipsis such as `..` or `...`, even when the list grows long. Write every relevant change date individually in ISO `YYYY-MM-DD` form, separated by commas.
