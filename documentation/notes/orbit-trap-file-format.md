<!-- Modified by Opus 5.5 on 2026-10-13, 2026-10-16. -->
# Keyframe Orbit Trap Format (`.rftz`)

`RFFOrbitTrapBinary::write` / `read` store a keyframe's per-pixel orbit traps beside its map, under
the same number (`0036.rfmz` + `0036.rftz`). Keyframe generation writes one only while Orbit Trap
Coloring is on, and removes a stale file under the number it is writing otherwise. The Timeline
Editor preview and video export read it through `VideoFrameSource` and `VideoWindow`; a missing or
mismatched file leaves that keyframe without traps, which the merge shader draws as with the trap off.

The payload is lossless: the reopened planes equal the saved floats bit for bit.

## Layout

Version 1 carries the four planes every trap coloring uses. Version 2 (2026-10-16) is written instead
while Orbit Trap Coloring is Shape and the view holds the shape planes; see the end of this file.

All fields little-endian, in this order:

| Field | Type | Notes |
| --- | --- | --- |
| magic | `char[4]` | `RFTZ`. |
| version | `uint16` | `RFFOrbitTrapBinary::VERSION` (1). A reader rejects anything else. |
| width, height | `uint16` x2 | Must equal the map's size, or the file is ignored. |
| planeCount | `uint32` | `OrbitTrap::PLANE_COUNT` (4). |
| rawSize | `uint64` | `width * height * planeCount * 4`. |
| compressedSize | `uint64` | Length of the frame that follows; must fit in the bytes left. |
| payload | `char[compressedSize]` | One zstd frame (`RFFMapCompression::compress`). |

The decompressed stream is the planes `crossLog2`, `pointLog2`, `secondLog2`, `iteration` one after
another, each laid out as the map's own table (row-major, the map's row order), with the four bytes
of every float split into four byte planes of equal significance. `iteration` is one plus the
wrapped iteration of the nearest origin approach; 0 marks a pixel without trap data.

Size: about 30 MB per 2160 x 3840 keyframe for a shallow test zoom, against roughly 133 MB raw.

## Version 2 (Shape)

Same as version 1 with `version` 2 and `planeCount` `OrbitTrap::SHAPE_PLANE_COUNT` (8), and these
fields between `planeCount` and `rawSize` (82 header bytes in all):

| Field | Type | Notes |
| --- | --- | --- |
| shapeKind | `uint8` | `ShdPalOrbitTrapShape`: 1 Point, 2 Circle, 3 Cross, 4 Line. Anything else rejects the file. |
| accuracy | `uint8` | Reserved, written 0 (sampling as MPA leaves it). |
| statistic | `uint8` | Reserved, written 0 (the minimum distance). |
| observation | `uint8` | Reserved, written 0 (no transform). |
| centerReal, centerImag | `float64` x2 | The trap's center c. |
| radius | `float64` | Circle's radius; 0 for the other shapes. |
| angleDegrees | `float64` | Cross's and Line's angle; 0 for the other shapes. |
| captureStart, captureEnd | `uint64` x2 | Reserved, written 1 and 0 (every iteration). |

After the version 1 planes come `shapeLog2` (log2 distance to the shape, as `crossLog2` is stored),
`shapeIteration` (one plus the wrapped iteration of that approach), `shapeAngle` (arg(z - c) there, in
radians) and `shapeStatus` (0 = no data, 1 = valid). The header values are the normalized
`TrapShapeParams`, so a radius or angle the shape ignores is 0.

A reader passes the shape it is drawing. When every header field matches it, the eight planes are
returned; otherwise the file is treated as version 1 and only the first four are returned, which draws
Shape as with the trap off while Cross Glow and Cellular still work. Keyframe generation writes
version 1, byte for byte as before, whenever Shape is not selected.

## Stability

- Never change the meaning of version 1 or 2; a different layout takes a new version number.
- The reserved version 2 fields belong to capture window, accuracy, statistic and observation settings
  not built yet; when they arrive they must keep 0, 0, 0, 1, 0 meaning today's sampling.
- Keep the plane order in step with `OrbitTrap` and with `vk_2_map_iter_stripe.comp`'s trap buffers.
