<!-- Modified by GPT-6 on 2026-10-04. -->
# Owner disposition and original JSON restoration

On October 4, 2026, the owner instructs setting aside the Rainbow and historical ObjectType permission questions and restoring the complete original `extern/nlohmann/json.hpp`. Those two questions are closed for further action within this task by the owner's decision. The original-source and permission findings remain historical records; no new author grant is claimed.

## Restored vendor file

The header is restored byte-for-byte to [nlohmann/json v3.12.0](https://github.com/nlohmann/json/blob/v3.12.0/single_include/nlohmann/json.hpp), commit `55f93686c01528224f448c19128836e7df245f72`, file `single_include/nlohmann/json.hpp`. Its calculated Git blob hash is `82d69f7c5d044c9887c96b90c97f5639083ecd14`, matching the retained tagged repository. Its SHA-256 is `aaf127c04cb31c406e5b04a63f1ae89369fccde6d8fa7cdda1ed4f32dfc5de63`.

This restores the entire upstream file, including the original ObjectType declarations, detector, ADL helpers, endian helper, documentation and copyright notices. Earlier local functional patches are also removed. It is not merely an undo of the latest ObjectType change. No local modification tag or attribution comment is inserted into the restored vendor file, following the owner's explicit request for the original. The restoration date and authorship of this action are recorded in NOTICE and this report.

The previous modified header is preserved under `debug/json-upstream-restoration-20261004/json-before-restoration.hpp`. The earlier ObjectType replacement and JSON repair reports describe work that has now been reversed. NOTICE includes the current disposition so those historical sections are not mistaken for the active header state.

## Scope and verification

The restored file's bytes and Git blob hash are verified against the retained original source. The existing focused C++20 check also passes with the restored header across six container forms and representative JSON object, parsing, serialization and conversion interfaces. It produces no executable and runs no application implementation. This is a focused compatibility check, not a validation of every behavior affected by removing the earlier patches.

Other application, shader and palette repairs and their license files are unchanged. Existing acknowledgements outside the vendor header remain. The earlier Boost detector license is retained as part of the repair history; this restoration does not claim that detector is present in the current header. No historical audit evidence is erased or rewritten.

The restoration does not rebuild the executable, shader binaries, release bundle or source archive. A recorded release rebuild and use of the verified GMP library with its matching source remain separate distribution work. Evidence, the backup and restoration hashes are retained under `debug/json-upstream-restoration-20261004/`.
