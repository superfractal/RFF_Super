# Compressed Map Format (`.rfmz`)

`RFFDynamicMapBinary::exportCompressedFile` / `readCompressed` write and read the compressed form of
a dynamic map. A `.rfm` is the iteration table stored as raw `double`s — 8 bytes per pixel, 15.8 MB
at 1920x1080 — and this format is the same table put through a reversible rearrangement and then
zstd. **It is lossless in both directions**: the reopened table equals the saved one bit for bit,
which is what lets keyframes be written in this form without changing a single frame of a video.

The header is fixed-size and versioned; unlike `ConfigIO` there is no append-only rule, because the
payload's meaning depends on the whole header being understood.

## Layout

All fields little-endian, in this order, written by `exportCompressedFile`:

| Field | Type | Notes |
| --- | --- | --- |
| magic | `char[4]` | `RFMZ`. What `isCompressedFile` tests, so the loader never depends on the extension. |
| version | `uint16` | `COMPRESSED_VERSION`. A reader rejects anything else outright. |
| mode | `uint8` | Stream layout: `RFFMapCompression::CURRENT_MODE` is written, `MODE_STREAM_SPLIT` is still read. Any other value is rejected. |
| width, height | `uint16` x2 | |
| logZoom | `float` | |
| period, maxIteration | `uint64` x2 | |
| rawSize | `uint64` | Length of the preprocessed stream, i.e. what the frame decompresses to. |
| compressedSize | `uint64` | Length of the frame that follows. |
| payload | `char[compressedSize]` | One zstd frame. |

Before anything is allocated, `rawSize` must equal `RFFMapCompression::streamSize(width * height)`
exactly, and `compressedSize` must fit in the bytes left in the file. A corrupt header must not be
able to ask for a buffer the file could never fill.

## The stream

Both modes are exactly `count * 8` bytes and end in the same byte split: byte *k* of every value
goes into plane *k*, so bytes of equal significance sit together instead of interleaving all eight
per pixel. What differs is what is split.

- **`MODE_STREAM_SPLIT`** (the first form, still read) splits the values themselves. The high planes
  are nearly constant across neighbouring pixels and pack away, but the low mantissa planes measure
  7.99 bits per byte — the coder gets nothing out of them. Ratio 1.5x to 2.7x.
- **`MODE_LORENZO_SPLIT`** (`CURRENT_MODE`) subtracts a prediction from each value first: `left + up
  - upleft` over the raw bit patterns, the plane through the three neighbours already seen. An
  iteration table is close to a smooth surface, so the prediction lands within a few ulp and the
  wrapping difference is a small number, zigzagged so that a negative one stays small. Ratio 2.0x to
  3.9x — roughly 35% smaller files than the split alone, on the same maps.

The prediction is taken over bit patterns rather than values because for numbers of one sign the
pattern rises with the value; iterations are never negative. Reversal is exact regardless: the
difference wraps in unsigned arithmetic and unwraps the same way, and every neighbour the decoder
predicts from has already been reconstructed on an earlier step of the same raster scan.

Plain zstd over the raw table, with no rearrangement at all, reaches only 1.1x to 1.9x.
`RFFMapCompression::DEFAULT_LEVEL` is the zstd level used everywhere — about 50 ms to write a
1920x1080 map and 13 ms to read one, and within two percent of the ratio level 19 reaches, which
costs fifteen to thirty times as long.

## Keyframes

`exportAsKeyframe(dir, compressed)` writes whichever form Compress keyframes (Export settings) asks
for, and `readByID` reads whichever form it finds. A folder can hold both — that setting toggled
between runs, keyframes written before this format existed, or a run resumed into an older folder —
so numbering must never be taken over one extension alone:

- `keyframeCount` counts `n` while `{n}.rfmz` **or** `{n}.rfm` exists, and is what the video export
  uses for its keyframe total.
- A new keyframe takes `keyframeCount(dir) + 1`. Using `IOUtilities::generateFileName` here would
  count only `.rfmz` and hand back a number an `.rfm` already holds.

## Changing it

- **Never renumber a mode, and never change what an existing mode's stream means.** A reader picks
  its decoder from the stored number alone, and files are already written.
- **Add a mode by taking the next number**, point `CURRENT_MODE` at it, and leave the older modes
  decodable. Teach `preprocess` / `postprocess` / `streamSize` about it together — `streamSize` is
  what the reader validates `rawSize` against, so a mode whose stream is a different length will
  otherwise be rejected as corrupt.
- **Any new mode must stay lossless.** Video export reads keyframes through this path, and a mode
  that rounded the values would silently change every frame rendered from a folder.
- **Bump `COMPRESSED_VERSION` only for a header change**, since that is what an older build cannot
  skip past. A new mode alone does not need it: an older build already rejects a mode number it
  does not know.
- zstd is a link dependency (`zstd` in `CMakeLists.txt`, `libzstd.dll` beside the executable). The
  level is not stored, because it does not change what the frame decodes to.
