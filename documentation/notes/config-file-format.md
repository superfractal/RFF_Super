# Config File Format (ConfigIO)

## Timeline Reverse (2026-11-15)

Modified by Opus 5.5 on 2026-11-15. The timeline gains Reverse (default off), which runs the video
from the deepest keyframe out to the first; keys and holds keep their depths. RFC appends TRV1
(uint32 0x31565254) after CSH1 and before BDB1, and the timeline file (RFVT) appends it after the
overlay timing block; both are followed by 1 as uint32. TRV1 is written only while Reverse is on,
so every other file is byte-identical to before. The reader looks at the next marker and leaves
anything other than TRV1 unread; a missing block loads Reverse off. A partial block or a value
above 1 rejects the load. Readers without TRV1 open such a timeline file running forward; an RFC
reader that also reads BDB1 rejects the file. Versions and existing offsets are unchanged.

## Cast Shadow (2026-11-14)

Modified by Opus 5.5 on 2026-11-14. Slope gains Shadow Strength (0 to 1, default 0 = off), Shadow
Length (10 to 500, default 140), Shadow Softness (0.05 to 2, default 0.6), Shadow Depth (-8 to 8,
default 0) and Shadow Quality (0 = Fast, 8 taps; 1 = Standard, 16 taps, the default; 2 = Fine, 32
taps). RFC and RFSP append CSH1 (uint32 0x31485343) after CAB1, and in RFC before BDB1, followed by
strength, length, softness and depth as float32 and the quality as uint32. CSH1 is written only when
any value is off its default, so every other file is byte-identical to before. The reader looks at
the next marker and leaves anything other than CSH1 unread; a missing block loads the defaults. A
partial block or a value out of range rejects the load. Readers without CSH1 open such an RFSP
without the shadow; an RFC reader that also reads BDB1 rejects the file. Versions and existing
offsets are unchanged.

## Chromatic Aberration (2026-11-08)

Modified by Opus 5.5 on 2026-11-08. Color gains Chromatic Aberration (0 to 32, default 0 = off) and
Chromatic Aberration Start (0 to 0.95, default 0.25). RFC and RFSP append CAB1 (uint32 0x31424143)
after BDK1, and in RFC before BDB1, followed by the amount and the start as float32. CAB1 is written
only when either value is off its default, so every other file is byte-identical to before. The
reader looks at the next marker and leaves anything other than CAB1 unread; a missing block loads
the defaults. A partial block or a value out of range rejects the load. Readers without CAB1 open
such an RFSP without the fringe; an RFC reader that also reads BDB1 rejects the file. Versions and
existing offsets are unchanged.

## Boundary Darkness (2026-11-07)

Modified by Opus 5.5 on 2026-11-07. Slope gains Boundary Darkness (0 to 1, default 0) and
Boundary Width (1 to 100, default 12). RFC and RFSP append BDK1 (uint32 0x314B4442) after RSR1,
and in RFC before BDB1, followed by darkness and width as float32. BDK1 is written only when either
value is off its default, so every other file is byte-identical to before. The reader looks at the
next marker and leaves anything other than BDK1 unread; a missing block loads the defaults. A
partial block or a value out of range rejects the load. Readers without BDK1 open such an RFSP
without the darkening; an RFC reader that also reads BDB1 rejects the file. Versions and existing
offsets are unchanged.

## Relief Source (2026-11-07)

Modified by Opus 5.5 on 2026-11-07. Slope gains Relief Source (0 = Iterations, the default,
1 = dz/dc). RFC and RFSP append RSR1 (uint32 0x31525352) after RCP1, and in RFC before BDB1,
followed by the source as uint32. RSR1 is written only when the source is not Iterations, so every
other file is byte-identical to before. The reader looks at the next marker and leaves anything
other than RSR1 unread; a missing block loads Iterations. A partial block or a value above 1
rejects the load. Readers without RSR1 open such an RFSP with Iterations; an RFC reader that also
reads BDB1 rejects the file. Versions and existing offsets are unchanged.

## Relief Cap (2026-11-05)

Modified by Opus 5.5 on 2026-11-05. Slope gains Relief Cap (0 to 8 periods, default 0 = off). RFC
and RFSP append RCP1 (uint32 0x31504352) after ANI1, and in RFC before BDB1, followed by the value
as float32. RCP1 is written only when the value is not 0, so every other file is byte-identical to
before. The reader looks at the next marker and leaves anything other than RCP1 unread; a missing
block loads 0. A partial block or a value outside 0 to 8 rejects the load. Readers without RCP1 open
such an RFSP without the cap; an RFC reader that also reads BDB1 rejects the file. Versions and
existing offsets are unchanged.

## Anisotropy Direction (2026-11-04)

Modified by Opus 5.5 on 2026-11-04. Slope gains Anisotropy Direction (0 = Fixed Angle, the
default, 1 = Along Bands, 2 = Across Bands). RFC and RFSP append ANI1 (uint32 0x31494E41) after
OPB1, and in RFC before BDB1, followed by the direction as uint32. ANI1 is written only when the
direction is not Fixed Angle, so every other file is byte-identical to before. The reader looks at
the next marker and leaves anything other than ANI1 unread; a missing block loads Fixed Angle. A
partial block or a value above 2 rejects the load. Readers without ANI1 open such an RFSP with a
fixed angle; an RFC reader that also reads BDB1 rejects the file. Versions and existing offsets
are unchanged.

## OpenPBR Surface (2026-11-04)

Modified by Opus 5.5 on 2026-11-04. Studio gains Material Model (0 = Studio GGX, the default, 1 =
OpenPBR Surface), Coat IOR (1 to 3, default 1.6), Coat Darkening (0 to 1, default 1), Coat Color
and Edge Tint (r, g, b each 0 to 1, default white), Fuzz (0 to 1, default 0), Fuzz Roughness (0 to
1, default 0.5), Fuzz Color (default white) and Film IOR (1 to 3, default 1.4). RFC and RFSP append
OPB1 (uint32 0x3142504F) after VIG1, and in RFC before BDB1, followed by the model as uint32, then
float32 coat IOR, coat darkening, coat color r, g, b, edge tint r, g, b, fuzz, fuzz roughness, fuzz
color r, g, b and film IOR, in that order. OPB1 is written only when the model or one of the values
differs from its default, so every other file is byte-identical to before. The reader looks at the
next marker and leaves anything other than OPB1 unread; a missing block loads the defaults,
including reused load destinations. A partial block or a value outside its range rejects the load.
Readers without OPB1 open such an RFSP as Studio GGX; an RFC reader that also reads BDB1 rejects the
file, because it reads OPB1 as BDB1. Versions and existing offsets are unchanged.

## Vignette (2026-11-04)

Modified by Opus 5.5 on 2026-11-04. Color gains Vignette (0 to 1, default 0 = off), Vignette Radius
(0 to 0.95, default 0.5), Vignette Richness (0 to 2, default 0.5) and Vignette Shape (0 = Ellipse,
the default, 1 = Frame). RFC and RFSP append VIG1 (uint32 0x31474956) after OTX1, and in RFC before
BDB1, followed by the three values as float32 in that order and the shape as uint32. VIG1 is written
only when one of the four differs from its default, so every other file is byte-identical to before.
The reader looks at the next marker and leaves anything other than VIG1 unread; a missing block loads
the four defaults, including reused load destinations. A partial
block or a value outside its range rejects the load. Readers without VIG1 open such an RFSP without
the vignette; an RFC reader that also reads BDB1 rejects the file, because it reads VIG1 as BDB1.
Versions and existing offsets are unchanged.

## Orbit Trap Shapes (2026-10-29)

Modified by Opus 5.5 on 2026-10-29. Orbit Trap Shape gains Grid Points = 5, Grid Lines = 6, Rings = 7,
Polygon = 8, Star = 9 and Spiral = 10, and Orbit Trap Sides (3 to 64, default 5) for Polygon and Star.
OTS1 stores these shapes as the nearest one an older reader can draw: Grid Points as Point (1), Grid
Lines as Cross (3), the rest as Circle (2). RFC and RFSP append OTX1 (uint32 0x3158544F) after OTR1,
and in RFC before BDB1, followed by the shape as uint32 (5 to 10) and the sides as uint32 (3 to 64).
OTX1 is written only for a shape past Line, so every other file is byte-identical to before. The
reader looks at the next marker and leaves anything other than OTX1 unread; a missing block keeps
OTS1's shape and loads sides 5. A partial block or a value out of range rejects the load. Readers
without OTX1 open such an RFSP with the stand-in shape; an RFC reader that also reads BDB1 rejects
the file, because it reads OTX1 as BDB1.

Keyframe traps (.rftz) keep version 2 for the new shapes, with their kind in the shape byte, which
older readers reject. Polygon and Star write version 3: version 2 with a trailing uint32 sides, so
planes measured with another number of sides are not taken for the current ones.

## Orbit Trap Region (2026-10-29)

Modified by Opus 5.5 on 2026-10-29. Orbit Trap Inside Set becomes Apply To, a three-way choice:
Inside and Outside, Outside Only or Inside Only. OTI1 keeps its 0/1 flag, written 0 for Outside Only
and 1 otherwise. RFC and RFSP append OTR1 (uint32 0x3152544F) after OTM1, and in RFC before BDB1,
followed by the region as uint32 (0 = Outside Only, 1 = Inside and Outside, 2 = Inside Only). OTR1 is
written only for Inside Only, so every other file is byte-identical to before. The reader looks at the
next marker and leaves anything other than OTR1 unread for the block that follows. A missing block
keeps what OTI1 gave. A partial block or a region above two rejects the load. Readers without OTR1
draw an Inside Only RFSP as Inside and Outside. An RFC reader that also reads BDB1 rejects an Inside
Only file, because it reads OTR1 as BDB1. Versions and existing offsets are unchanged.

## Buddhabrot (2026-10-26)

Modified by Opus 5.5 on 2026-10-26. Formula Type gains Buddhabrot = 2, so the load check on Formula
Type accepts 0 to 2; a file saved with Buddhabrot selected is rejected by older readers. RFC appends
BDB1 (uint32 0x31424442) at EOF after OTM1, followed by Samples per Pixel as uint32 (1 to 4096) and
Min Iteration as uint64. A missing block loads 16 and 0. An invalid marker, a partial block or a
sample count out of range rejects the load. Versions and existing offsets are unchanged.

Until `BUDDHABROT_AVAILABLE` (FractalAttribute.h) is turned on, Formula Type accepts only 0 to 1
and BDB1 is read but never written, so saved files stay exactly as they were before Buddhabrot.

## Orbit Trap Mapping and Relief (2026-10-24)

Modified by Opus 5.5 on 2026-10-24. RFC and RFSP append OTM1 (uint32 0x314D544F) at EOF after GCB1.
Three uint32 follow: Orbit Trap Mapping (0 = Distance, 1 = Angle, 2 = Bands), Orbit Trap Arms (1 to
64) and Orbit Trap Relief (0 or 1). New documents and a missing block, including reused load
destinations, load Distance, 3 and 0, which draw exactly as before. An invalid marker, a partial block
or a value out of range rejects the load. Versions and existing offsets are unchanged; older readers
ignore and discard the tail on resave.

## Crease Blur (2026-10-23)

Modified by Opus 5.5 on 2026-10-23. RFC and RFSP append GCB1 (uint32 0x31424347) at EOF after GRB1.
One float32 follows: Crease Blur, 0 to 24, in pixels at 720 lines. New documents start with 6; a
missing block loads 0, including reused load destinations, so older files keep a sharp crease. An
invalid marker, a partial block or a width that is not finite or outside [0,24] rejects the load.
Versions and existing offsets are unchanged; older readers ignore and discard the tail on resave.

## Rounded Bands (2026-10-23)

Modified by Opus 5.5 on 2026-10-23. RFC and RFSP append GRB1 (uint32 0x31425247) at EOF after GSM1.
One uint32 follows: Rounded Bands, 0 or 1. New documents start with 1; a missing block loads 0,
including reused load destinations, so older files keep carving grooves. An invalid marker, a
partial block or a flag above 1 rejects the load. Versions and existing offsets are unchanged;
older readers ignore and discard the tail on resave.

## Smooth Grooves (2026-10-23)

Modified by Opus 5.5 on 2026-10-23. RFC and RFSP append GSM1 (uint32 0x314D5347) at EOF after OTH1.
One uint32 follows: Smooth Grooves, 0 or 1. New documents start with 1; a missing block loads 0,
including reused load destinations, so older files draw their grooves as before. An invalid marker,
a partial block or a flag above 1 rejects the load. Versions and existing offsets are unchanged;
older readers ignore and discard the tail on resave.

## Orbit Trap hold (2026-10-22)

Modified by Opus 5.5 on 2026-10-22. RFC and RFSP append OTH1 (uint32 0x3148544F) at EOF after EFS1.
One uint32 follows: Orbit Trap Holds in Color Animation, 0 or 1. A missing block loads 0, including
reused load destinations; an invalid marker, a partial block or a flag above 1 rejects the load. The
same change widens Groove Depth to [0,20]; an older reader rejects a file whose depth exceeds 5.
Versions and existing offsets are unchanged; older readers ignore and discard the tail on resave.

## Effect Styles (2026-10-20)

Modified by Opus 5.5 on 2026-10-20, 2026-10-21. RFC and RFSP append EFS1 (uint32 0x31534645) at EOF after OTS1.
For each of the four effect layers, six uint32 follow in this order: Flame Style (0 = Classic,
1 = Turbulent Flames, 2 = Burning Edge), Embers Style (0 = Classic, 1 = Rising Sparks, 2 = Spark
Bursts), Mist Style (0 = Classic, 1 = Drifting Mist, 2 = Pooled Mist), Ripples Style (0 = Classic,
1 = Interference, 2 = Caustics), Flow Light Style (0 = Classic, 1 = Band Chase) and Aurora Style
(0 = Classic, 1 = Ray Curtain, 2 = Ribbons). A missing block loads Classic everywhere, including
reused load destinations; an invalid marker, a partial block or a style past its last value rejects
the load. Heat Haze is gone: effect type 4 stays reserved, a stored layer of type 4 loads switched
off as Mist (type 3) with its other values kept, and validation rejects type 4 after loading.
Versions and existing offsets are unchanged; older readers ignore and discard the tail on resave.
The Effect Type menu shows type and style as one entry; the file still stores them separately.

## Orbit Trap Shape (2026-10-16)

Modified by Opus 5.5 on 2026-10-16. RFC and RFSP append OTS1 (uint32 0x3153544F) at EOF after OTL2.
A uint32 shape (1 = Point, 2 = Circle, 3 = Cross, 4 = Line) and float64 Center Real and Center
Imaginary in [-4,4], Radius in [0,4] and Angle in [-180,180] degrees follow. OTP1's mode now also
accepts 3 = Shape. A missing block loads Point, 0, 0, 0.5 and 0, including reused load destinations;
an invalid marker, a partial block, a shape outside 1 to 4 or a value out of range rejects the load.
Versions and existing offsets are unchanged; older readers ignore and discard the tail on resave, and
an older reader rejects a file whose OTP1 mode is 3.

## Orbit Trap look (2026-10-12)

Modified by Opus 5.5 on 2026-10-12. RFC and RFSP append OTL2 (uint32 0x324C544F) at EOF after OTI1.
Then float32 Offset in [-1,1] cycles, float32 Animation Speed in [-4,4] cycles per second, four
float32 Glow Color, float32 Glow Strength in [0,4], float32 Glow Sharpness in [0.1,8], uint32 Blend
(0 = Mix, 1 = Multiply, 2 = Screen, 3 = Add), float32 Cell Border Width in [0.1,8] and float32 Cell
Border Darkness in [0,1]. A missing block loads 0, 0, white, 0.6, 1, Mix, 1 and 0.85, including
reused load destinations, which draws as the trap did before these settings existed; an invalid
marker, a partial block or a value out of range rejects the load. OTL1 (uint32 0x314C544F), written
briefly on 2026-10-12, is the same block with a trailing float32 Inside Amount; it still loads, and
that value is discarded.
Versions and existing offsets are unchanged; older readers ignore and discard the tail on resave.

## Orbit Trap Inside Set (2026-10-12)

Modified by Opus 5.5 on 2026-10-12. RFC and RFSP append OTI1 (uint32 0x3149544F) at EOF after OTP1.
A uint32 flag follows: 1 = the trap also colors pixels that reach Max Iterations, 0 = they keep
Mandelbrot Color. A missing block loads 1, including reused load destinations, which draws as the
trap did before the switch existed; an invalid marker, a partial block or a flag above one rejects the
load. Versions and existing offsets are unchanged; older readers ignore and discard the tail on resave.

## Orbit Trap Coloring (2026-10-12)

Modified by Opus 5.5 on 2026-10-12. RFC appends OTP1 (uint32 0x3150544F) at EOF after VOS1; RFSP
appends it after FSK1. A uint32 mode (0 = None, 1 = Cross Glow, 2 = Cellular) and float32 Orbit
Trap Amount in [0,1], Orbit Trap Width in [-100000,8] and Orbit Trap Density in [-16,16] follow. A
missing block resets all four to None, 0.8, -5 and 1, including reused load destinations, which draws
exactly as before; an invalid marker, a partial block, a mode past 2 or a value out of range rejects
the load. Versions and existing offsets are unchanged; older readers ignore and discard the tail on
resave. Map files (.rfm) do not store orbit traps.

## Video Output Size (2026-10-10)

Modified by Opus 5.5 on 2026-10-10. RFC appends VOS1 (uint32 0x31534F56) at EOF after FSK1. Three
uint32 follow: outputWidth and outputHeight in [0,16384], where 0 keeps the keyframe axis, then
upscaleFilter (0 = Lanczos, 1 = Bicubic, 2 = Nearest). A missing block loads 0, 0, Lanczos, which
exports exactly as before, including reused load destinations; an invalid marker, a partial block or
an out-of-range value rejects the load. Versions and existing offsets are unchanged; older readers
ignore and discard the tail on resave. RFSP does not include these export settings.

## Smooth Focus Across Keyframes (2026-10-09)

Modified by Opus 5.5 on 2026-10-09. RFC and RFSP append FSK1 (uint32 0x314B5346) at EOF after
CBK1. A uint32 flag follows: 0 = in a video the Focus Band is measured against the larger of the two
keyframes' Max Iterations, 1 = against a value interpolated geometrically between them by the
frame's position in the interval. A missing block loads 0, which draws exactly as before, including
reused load destinations; a new document starts with 1. An invalid marker, a partial block or a flag
above one rejects the load. Versions and existing offsets are unchanged; older readers ignore and
discard the tail on resave.

## Chaos Bokeh (2026-10-09)

Modified by Opus 5.5 on 2026-10-09. RFC and RFSP append CBK1 (uint32 0x314B4243) at EOF after
CEP1. A uint32 shape follows (0 = Circle, 1-6 = Triangle to Octagon), then float32 rotation in
degrees in [0,360] and float32 aspect (height over width) in [0.25,4]. A missing block loads Circle,
0 and 1, which draws exactly as before, including reused load destinations. An invalid marker, a
partial block or a value out of range rejects the load. Versions and existing offsets are unchanged;
older readers ignore and discard the tail on resave.

## Chaos Edge Protection (2026-10-09)

Modified by Opus 5.5 on 2026-10-09. RFC and RFSP append CEP1 (uint32 0x31504543) at EOF after
SEN1. A uint32 flag follows: 0 = Chaos Blur gathers every neighbouring pixel, 1 = each tap counts
only as far as its own defocus reaches. A missing block loads 0, which draws exactly as before,
including reused load destinations; a new document starts with 1. An invalid marker, a partial
block or a flag above one rejects the load. Versions and existing offsets are unchanged; older
readers ignore and discard the tail on resave.

## Environment Source (2026-10-08)

Modified by Opus 5.5 on 2026-10-08. RFC and RFSP append SEN1 (uint32 0x314E4553) at EOF after
CSB1. A uint32 source follows (0 = Two Colors, 1 = Gradient, 2-5 = Texture Layer 1-4), then a
float32 Environment Brightness in [0,32] and four stops of float32 position, r, g, b, each in
[0,1]. A missing block loads Two Colors with the default brightness and stops, which draws as
before, including reused load destinations. An invalid marker, a partial block or a value out of
range rejects the load. Versions and existing offsets are unchanged; older readers ignore and
discard the tail on resave. The texture layer's image path stays in its own layer block.

## Coat Softbox (2026-10-08)

Modified by Opus 5.5 on 2026-10-08. RFC appends CSB1 (uint32 0x31425343) at EOF after WCO1; RFSP
appends it after WCO1 as well. A uint32 flag follows: 0 = the clear coat reflects key and fill as
point lights, 1 = as rectangular softboxes. A missing block loads 0, which draws exactly as before,
including reused load destinations; a new document starts with 1. An invalid marker, a partial
block or a flag above one rejects the load. Versions and existing offsets are unchanged; older
readers ignore and discard the tail on resave.

## Waves on Clear Coat Only (2026-10-08)

Modified by Opus 5.5 on 2026-10-08. RFC appends WCO1 (uint32 0x314F4357) at EOF after SSF1; RFSP
appends it after EAC1. A uint32 flag follows: 0 = Relief Waves shape the whole relief, 1 = only the
Studio clear coat. A missing block loads 0, which draws exactly as before, including reused load
destinations; an invalid marker, a partial block or a flag above one rejects the load. Versions and
existing offsets are unchanged; older readers ignore and discard the tail on resave.

## SSAA Filter (2026-10-07)

Modified by Opus 5.5 on 2026-10-07. RFC appends SSF1 (uint32 0x31465353) at EOF after EAC1,
followed by a uint32 SSAA Filter: 0 = Box (Gamma), 1 = Mitchell (Linear). A missing block loads
Box (Gamma), the plain mean every earlier build drew, including reused load destinations; a new
document starts on Mitchell (Linear). An invalid marker, a partial block or a value above one
rejects the load. Render presets leave it as it is. Versions and existing offsets are unchanged;
older readers ignore and discard the tail on resave. RFSP does not include this render setting.

## Escape Angle Coloring (2026-10-07)

Modified by Opus 5.5 on 2026-10-07. RFC appends EAC1 (uint32 0x31434145) at EOF after ASA1; RFSP
appends it after CHI1. A uint32 mode (0 = None, 1 = Binary Decomposition, 2 = Angle Twist) and a
float32 Escape Angle Amount in palette cycles follow. A missing block resets both to their defaults
(None, 0.50), including reused load destinations; an invalid marker, a partial block, a mode above
two or an amount outside [-4,4] rejects the load. Versions and existing offsets are unchanged; older
readers ignore and discard the tail on resave. Map files (.rfm) do not store escape angles.

## Adaptive SSAA (2026-10-07)

Modified by Opus 5.5 on 2026-10-07. RFC appends ASA1 (uint32 0x31415341) at EOF after GPU1,
followed by uint32 preview, uint32 video and float32 equivalentSsaa. A missing block resets every
field to its default (off, off, 0 = three quarters of Supersampling), including reused load
destinations; an invalid marker, a partial block, a flag above one or an equivalentSsaa outside
[0,8] rejects the load. Versions and existing offsets are unchanged; older readers ignore and
discard the tail on resave. RFSP does not include these render settings.

## GPU Perturbation (2026-10-06)

Modified by Opus 5.5 on 2026-10-06. RFC appends GPU1 (uint32 0x31555047) at EOF after CHI1,
followed by uint32 use, float32 preferredBatchDuration, uint32 allowedGlitchPixelCount, uint32
completelyIgnoreMpa, uint32 automaticAcceptMpaBatches and uint32 interpolateIsolated. A missing
block resets every field to its default (off, 0.1, 0, off, 0, off, single precision on), including reused load
destinations; an invalid marker, a partial block, a flag above one or a batch duration outside
[0.01,1] rejects the load. Versions and existing offsets are unchanged; older readers ignore and
discard the tail on resave. RFSP does not include these render settings. An optional uint32
singlePrecision follows interpolateIsolated; a GPU1 block without it loads with single precision
on, and a value above one rejects the load.

## Chaos Interior (2026-10-06)

Modified by Opus 5.5 on 2026-10-06. RFC appends CHI1 (uint32 0x31494843) at EOF after the bare
gpuSubmitSplit; RFSP appends it after CBA1. A float32 Chaos Interior in [0,1] follows. A missing
block or value resets to 0, which renders as before, including reused load destinations; an
invalid marker, a partial scalar or a value outside [0,1] rejects the load. Versions and existing
offsets are unchanged; older readers ignore and discard the tail on resave. Timeline track ID 520
animates the value.

## GPU Work Split (2026-10-04)

Modified by Opus 5.5 on 2026-10-04. RFC appends a bare uint32 gpuSubmitSplit at EOF, after CBA1.
0 = one submission per video frame, 1 = one per pass, 2..64 = also that many fractal-pass bands.
A missing value resets to 0, including reused load destinations; values above 64 reject the load
and a partial scalar fails the stream check. Versions and existing offsets are unchanged; older
readers ignore the tail and discard it on resave. RFSP does not include this export setting.

## Chaos Blur Averaging and Shade range (2026-10-03)

Modified by Opus 5.5 on 2026-10-03. RFC and RFSP append CBA1 (uint32 0x31414243) at EOF, after
every block listed below (RFC: after the overlay timing block; RFSP: after CHB1), followed by a
uint32 Chaos Blur Averaging: 0 = Average in Gamma Space, 1 = Average in Linear Space. A missing
block or value resets to Gamma Space, including reused load destinations; an invalid marker, a
partial scalar or a value above one rejects the load. Versions and existing offsets are unchanged,
and older readers of RFC 8 and RFSP 5 ignore and discard the tail on resave. Chaos Shade now
accepts [0,1] instead of [0,0.5], in CHB1 and in timeline track 519 alike. Its darkening follows the
selection strength alone rather than the selection times Chaos Amount, so files that use Shade with
Amount below 1 render darker than before; the stored value itself is unchanged.

## Chaos Highlight Detail removal (2026-10-03)

Modified by Opus 5.5 on 2026-10-03. New saves use RFC 8 and RFSP 5, and CHB1 carries seven
float32 values: Chaos Amount, Detail Scale, Threshold, Transition, Feather, Blur Radius and Shade.
RFC 3-7 and RFSP 1-4 remain readable: their retired Highlight Detail float sits between Blur Radius
and Shade, and the reader consumes it into a temporary only to keep Shade and every later block in
place. It is still range-checked to [0,1] so a corrupt legacy file is rejected as before. No
runtime attribute, shader uniform or UI control remains, and older applications reject RFC 8 and
RFSP 5. Timeline track ID 518 belonged to Chaos Highlight Detail; it is consumed and discarded on
load, as the retired Softbox IDs 1202-1221 are, and must never be reused. RFVT is unchanged.

## Overlay display timing (2026-09-30)

Modified by GPT-6 on 2026-09-30. RFC and RFVT append VOT1 (uint32
0x31544f56) after MIO1. The payload writes zoom overlay then iteration overlay,
each as uint32 limitDisplayTime, float64 displayStart, float64 displayEnd.
Times are finite seconds in [0, 604800]; a nonzero end must exceed start.
The enabled interval includes start and excludes end; end 0 means the video end.
The Show switch remains the master visibility switch. Times include zoom holds.

Versions and prior offsets remain unchanged. Each trailing scalar is EOF guarded;
missing timing fields default to false/0/0, including reused destinations.
Partial scalars, invalid markers, nonboolean flags, and invalid times reject loading.
Old readers ignore the tail and discard timing when resaving. JSON version 1
adds the same three optional fields to both overlays with those defaults.


## Maximum iteration overlay and PNG metadata (2026-09-26)

Independent appearances append MIO1 (uint32 0x314f494d) after ZMI1, followed
by a ZOV1 appearance block for maxIterationOverlay and uint32
interpolateMaxIteration (0 or 1). Missing MIO1 inherits the zoom appearance,
places the iteration text 1.5 font sizes lower (clamped to the frame), enables
its custom appearance, and uses the ZMI1 visibility. Missing interpolation
resets to false. Each scalar is EOF guarded; partial scalars, invalid markers
and invalid values reject the load. ZMI1 mirrors the independent overlay's
visibility for older readers. Old readers discard MIO1 on resave.

Timeline JSON version 1 adds optional maxIterationOverlay (the same appearance
fields, without decimalPlaces/showMaxIteration) and interpolateMaxIteration.
Absent fields migrate as above. Per-frame interpolation affects only the
displayed integer: at depth n the count is map n's value; between n and n+1 it
is linearly interpolated and rounded. Extra zoom beyond map 1 keeps map 1's
count. Missing counts remain unknown. Both integer display modes travel with
each frame, so a paused preview updates when the mode changes.

Modified by GPT-6 on 2026-09-26. RFC and RFVT append ZMI1 (uint32
0x31494d5a) after ZDP1, followed by uint32 showMaxIteration (0 or 1).
Existing offsets and versions remain unchanged. Absent fields reset to false,
including reused destinations. Each trailing scalar is EOF guarded; partial
scalars, invalid markers and values above one reject loading. Older readers
ignore the tail and discard it on resave. Timeline JSON version 1 accepts an
optional zoomOverlay.showMaxIteration boolean, defaulting to false.

RFSM keeps its original float32 logZoom, uint32 width, uint32 height header
and optionally appends uint64 maxIteration using the existing scalar encoding.
Absent metadata means unknown, not zero. Partial iteration values reject the
map. Old readers ignore the tail. Newly generated PNG keyframes store the
completed map's iteration limit, without copying its pixel matrix. Preview and
export use the larger limit of the two source maps; if either PNG header lacks
the limit, the overlay shows N/A. The frame queue carries the exact uint64 value
alongside its image, including during export subsampling.

## Explorer removal (2026-09-25)

Modified by GPT-6 on 2026-09-25. New saves use RFC 7 and omit the four
Bio Explorer floats between Effect Sync and Chrome. RFC 3-6 remain readable:
the reader consumes retired explorer fields only to preserve subsequent offsets.
RFC 3-5 contain six floats and a boolean in this location; RFC 6 contains four
floats. No explorer settings or runtime state remain. New files are 16 bytes
smaller than RFC 6, and older applications reject RFC 7. RFSP and RFVT are unchanged.

## Compact formats (2026-09-24)

Modified by GPT-6 on 2026-09-24. This section supersedes earlier version and
retained-slot statements below. New saves use RFC 6, RFSP 4 and RFVT 3.
Readers retain RFC 3–5, RFSP 1–3 and RFVT 1–2 support. Older applications reject
the new versions. Existing files are converted only when explicitly saved.

RFC/RFSP omit the two retired slope-shadow floats, the Line Depth placeholder,
Studio softboxWidth, 32 Softbox lighting floats, and 16 material-decoration
floats (including three styleEdgeColor channels). RFC additionally omits
stimulusGain, brainDurationMs and freeExploration. These fields have no runtime
attribute members. Only legacy readers consume their old bytes into temporary
variables; byte order and all remaining field order are unchanged.

The timeline body omits its unused one-byte color-space placeholder per track.
RFC 6 selects the compact embedded body; RFVT 3 selects the compact standalone
body. Legacy readers consume the placeholder. Retired Softbox tracks with IDs
1202–1221 are consumed and discarded; these IDs must never be reused. All other
track IDs, key interpolation, audio and overlay payloads retain their layouts.

Line Glow's styleRimStrength, styleRimWidth and styleRimColor remain active and
remain serialized. Palette recipes, palette band decorations, surface style
IDs and compressed-map layouts are unchanged. GPU buffer slots are internal
and compacted together with their shader declarations, independently of files.

For one timeline track, RFC is 221 bytes smaller, RFSP 208 bytes smaller and
RFVT one byte smaller. Removing old Softbox tracks can save additional bytes.

## Timeline audio payload (2026-09-19)

Modified by GPT-6 on 2026-09-19. RFC appends an optional `AUD1` block at EOF,
after `ZOV1`; the embedded timeline body and existing RFC version/offsets are
unchanged. An absent block resets audio to an empty lane with master gain 1 and
export enabled, including reused load destinations. A present but incomplete or
invalid block rejects the entire load. Older readers ignore and discard the
audio tail on resave.

Standalone RFVT now writes version 2, with a required `AUD1` following the full
version-1 timeline/rotation/overlay body. The reader still accepts version 1 and
initializes empty audio. Version-1 readers reject version 2 instead of silently
discarding music. This supersedes the version statement in the older overlay
section below.

`AUD1` uses explicit little-endian fields: uint32 marker `0x31445541`, uint32
payload version 1, uint32 payload byte length, then the payload. Its payload is
uint32 exportEnabled, IEEE float32 master gain, uint32 clip count, then each clip:
uint64 nonzero ID; uint32 UTF-8 path length and path bytes; six int64 microsecond
values in the order sourceDuration/start/in/out/fadeIn/fadeOut; float32 clip gain;
and uint32 muted. Flags accept only 0/1, gains must be finite in [0,4], and IDs
must be unique. Paths must be nonempty, contain no NUL and use at most 32768
bytes. At most 1024 clips and 33636368 payload bytes are permitted.

Audio timestamps use 1000000 ticks per second. Source duration and clip endpoint
are bounded to seven days; start/in/fades are nonnegative, in < out <= source
duration, and fadeIn + fadeOut <= out - in. Clips cannot overlap, including muted
clips. Extra bytes inside the declared payload are rejected. Stream failure
never commits a partial audio model. No audio samples or waveform data are saved.

Save resolves relative media paths against the current working directory and
writes paths relative to the destination document directory when both paths
share a root. Different-drive paths remain absolute. Load resolves stored
relative paths against the document directory without requiring media to exist.
This allows missing files to remain available for subsequent relinking.

## Zoom ratio overlay (2026-09-18)

RFC appends `ZOV1` (uint32 `0x31564F5A`) after Studio Lighting (`SBL1`).
Standalone RFVT appends the same extension after `ROT1`. The embedded
timeline block has no overlay bytes, preserving every existing RFC offset.
RFC version 5 and RFVT version 1 are unchanged.

The field order is uint32 visible/custom/anchor; float32 X/Y/size; uint32
UTF-8 font-name byte length followed by those bytes; uint32 font style;
float32 text RGBA; uint32 outline enabled, float32 outline width/RGBA;
uint32 shadow enabled, float32 shadow X/Y/RGBA. Anchor IDs 0–8 are row-major
from top-left to bottom-right. Style IDs 0–3 are Regular, Bold, Italic,
Bold Italic. Coordinates are normalized, size is a fraction of frame height,
and outline/shadow dimensions are fractions of font size. Colors are sRGB.

Flags accept only 0/1. X/Y and RGBA accept [0,1], size [0.005,0.20], outline
width [0,0.25], and shadow offsets [-1,1]; all floats must be finite. Font
names contain 1–256 valid UTF-8 bytes without NUL. Each trailing scalar has
an independent stream-buffer EOF guard; missing fields retain explicit
defaults, while partial scalars, invalid values/markers and incomplete
length-prefixed strings reject the load transactionally.

Old RFC files reset styling to Legacy and take visibility from the historical
animation.showText field. New saves mirror canonical overlay visibility into
that original field. Old RFVT files reset styling to Legacy while retaining
the destination visibility. New RFVT files carry visibility explicitly.
Older readers ignore the suffix and lose custom styling when saving again.
The extension is absent from shader presets and map files.

## Studio softbox controls (2026-09-18)

The `SBL1` extension (uint32 marker `0x53424C31`) follows Show Video Export
Preview at the end of ConfigIO, and follows Layer Order at the end of shader
presets. Existing blocks and version numbers retain their offsets and values.
The float order is environment rotation in degrees, light-direction follow,
then aspect multiplier, tilt in degrees, brightness multiplier and color mix
for softboxes 1 through 4, then RGBA colors for softboxes 1 through 4.
Every scalar has its own stream-buffer EOF guard. Missing fields use the new
defaults; partial scalar values and invalid markers are rejected. Missing
extensions reset all these controls to their defaults, even when loading into
an already edited destination. Rotation and tilt accept -360000 to 360000
degrees for timeline turns; aspect accepts 0.125 to 8, brightness 0 to 8,
and follow, color mix and RGBA accept 0 to 1. Non-finite values are rejected.

Defaults are rotation/tilt 0, follow 1, aspect/brightness 1, color mix 0 and
white RGBA. They retain the original environment and its inherited light
colors. Timeline target IDs 1200 through 1221 identify these controls and
must not be renumbered. No shader layer IDs are introduced: the controls
belong to the existing surface shading and its clearcoat.

## Constant-period rotation (2026-09-15)

The current writer appends a `ROT1` block after `writeChrome`: uint32 rotation mode
(0 = keyframes, 1 = constant period), float seconds per turn (0.01–86400), uint32 direction
(0 = clockwise, 1 = counterclockwise), and float start angle in degrees (-360000–360000).
An absent block keeps the default keyframe mode. Each trailing field is guarded independently:
an omitted field keeps its default, while a partial field or invalid value is rejected.
Earlier readers ignore the new suffix. The embedded timeline block keeps its
original layout so all existing configuration fields retain their offsets.

Standalone timeline files append the same block after the estimated keyframe count, keeping
version 1. Earlier timeline files load in keyframe mode and earlier readers ignore the suffix.

`ConfigIO` reads/writes a flat, ordered binary stream. To add a setting **without breaking old
builds**, append-only with a `peek()` guard instead of bumping `VERSION`:

- **Append new fields at the very END of the file, in a fixed order.** Old builds stop after the
  fields they know and ignore trailing bytes (forward compat). Never insert mid-stream, reorder, or
  change a field's type — that shifts every following offset and corrupts the read.
- **On load, guard each trailing field with the `hasMore()` helper (`in.rdbuf()->sgetc() != eof`),
  NOT with `in.peek()`.** Absent field → keep the default already copied from the live `Attribute`
  (`Attribute t = out;`). `peek()` at EOF sets `eofbit`, and a *second* chained `peek()` then sees
  `good() == false` and sets `failbit` via its sentry — which trips the `in.fail()` corruption check
  even though every real read succeeded (a file missing the last two optional fields hits this).
  `sgetc()` reads the streambuf directly without mutating the istream state flags, so chained guards
  stay safe while genuine truncation still leaves `failbit` set for the final check.
- **save and load must append/read in the identical order.** A mismatch reads e.g. a `bool` as a
  `float`. Chained optional fields are read in sequence, each behind its own `peek()`.
- **The appended block must stay last in the file.** If you later add a whole new section, it must
  go after the existing trailing fields — otherwise an older file (missing the trailing field) lets
  `peek()` grab the new section's bytes and misreads them.
- **Bump `VERSION` only for changes old builds cannot safely skip** (mid-stream insert, reorder,
  type change). Raising `VERSION` makes `version > VERSION` reject the file in older builds, so it
  breaks forward compat by design — keep it unchanged for purely additive fields.

`RecoveryIO` writes this same format, once per compute, as the running session's snapshot
(`recovery/session-<pid>.rfc`, offered back as `recovery/crashed.rfc`, and held as
`recovery/interrupted.rfc` when the run was closed mid-compute rather than lost). Two things follow:
keep `save()` cheap enough to sit in front of every recompute, and remember that a snapshot is read
by the build that comes after the crash — which, after an update, is a *newer* build reading an
*older* file, exactly the case the append-only rule above protects.

## Current trailing block

Nine compatibility fields for the removed Adaptive Material prototype follow Texture Size. They
remain in their original order and are ignored on load so settings written while that prototype was
available still align correctly.

The Universal Light block follows those compatibility fields and is the current end of the stream.
Its fixed order is Universal Light, Light Strength, Relief and Detail Scale. Every field is guarded
independently while loading, so a file ending before the block keeps the values already present in
the destination attribute.

## Sheen block

The sheen is the current end of the stream, and unlike every block above it, it sits behind a marker
word — `ShaderPresetIO::SHEEN_MAGIC`, `"SHEN"` — rather than being appended bare. Its order is
Use, Amount, Lightness Lock, Base Lightness, Ridges, Sharpness, Turbulence and Turbulence Scale;
each field is guarded independently, so a file ending inside the block keeps the defaults for the
rest. A file whose first four trailing bytes are not the marker is treated as carrying no sheen at
all, the destination attribute is left alone, and nothing further is read.

The marker is there because the append-only rule above only holds while every build that ever wrote
the file agreed on where the last field ended, and that has already failed once in practice: a
config saved on 2026-08-28 by a build carrying the trailing block described above ends in 31 bytes
this source writes nothing for, and a bare block would have read the first of them as its own. The
same reasoning put `TRAILER_MAGIC` and the timeline's `CONFIG_BLOCK_MAGIC` where they are.

**The two sections above are stale as of 2026-08-29.** No source in this tree writes the Adaptive
Material compatibility fields, the Universal Light block, or the sheen. Files written by the builds
that did carry them are still readable — the guard is what makes that true — but neither section
describes a stream this build produces.

## Gloss block

The slope's gloss is written after Texture Size, and it sits behind a
marker word of its own — `ShaderPresetIO::GLOSS_MAGIC`, `"GLSS"` — rather than being appended bare.
Its order is Intensity, Source, Bands, Sharpness, Phase and the three channels of its color; each
field is guarded independently, so a file ending inside the block keeps the defaults for the rest.
A file whose first four trailing bytes are not the marker is treated as carrying no gloss at all,
the destination attribute is left alone, and nothing further is read — which is the picture that
file was written under, since the gloss is off in every build that does not write this block.

The marker is not a precaution here, it is a requirement: the bare append-only rule holds only while
every build that ever wrote the file agreed on where the last field ended, and by now two builds
have broken that. A config saved by the Line Depth-era build ends in 31 trailing bytes this source
writes nothing for, and one saved by the sheen build ends in 33 beginning with `"SHEN"`. Appended
bare, the gloss would have read the first of those as its Intensity — a legal float, with nothing
downstream able to tell. The same reasoning put `TRAILER_MAGIC` and the timeline's
`CONFIG_BLOCK_MAGIC` where they are.

Anything appended after this must go after the gloss block, and behind a marker of its own for the
same reason.

## Key light block

The slope key light's strength is written after the gloss block and
behind a marker word of its own — `ShaderPresetIO::KEY_LIGHT_MAGIC`, `"KEYL"`. The block is a single
float. A file whose next four bytes are not the marker is treated as carrying no key-light block at
all, the destination attribute is left alone, and nothing further is read — which is the picture that
file was written under, since the key light is at unity in every build that does not write it.

The marker is required for the reason the gloss block's is, and for one more that is live: this block
is appended during the same release the gloss block was, so a settings file saved partway through
that work ends in the gloss block alone, and its last field must not be read as this block's
strength — any float is a legal one, with nothing downstream able to tell. Anything appended after
this must go after the key light block, and behind a marker of its own.

## Bloom color balance block

Bloom Color Balance is written after the earlier marked blocks and
behind `ShaderPresetIO::BLOOM_COLOR_MAGIC`, `"BLCL"`. The block contains one boolean. A file with no
matching marker leaves the destination setting alone, so shader presets from earlier versions load
with the default off while settings files keep the live session's value.

The marker keeps files ending in a different experimental block from being mistaken for this boolean.

## Warp stability block

Warp stability is the current end of the stream, written after Bloom Color Balance and behind
`ShaderPresetIO::WARP_STABILITY_MAGIC`, `"WSTB"`. Its fixed order is Stable Band, Band Follow and
Lightness Lock. Each field is guarded independently, so a file ending inside the block keeps the
destination value for every remaining field. A file with no matching marker leaves all three values
alone; fresh shader presets therefore use the original Warp behavior, while settings files retain the
live session's values.

Anything appended after Warp stability must follow this block and use a marker of its own.

## Lightness lock block

The palette's lightness lock is two floats behind `ShaderPresetIO::LIGHTNESS_LOCK_MAGIC`, `"LOCK"`
- the lock amount and the lightness it locks to, in that order, each guarded on its own so a file
written between them still loads. In this source it is written directly after the gloss block, which is the last block any
code here writes; the three sections above describe blocks no source in this tree produces, so a
file written by this build carries the gloss block and then this one. A file whose next four bytes
are not the marker is treated as carrying no lock at all, the destination attribute is left alone,
and nothing further is read - which is the picture that file was written under, since the lock is
off in every build that does not write this block.

The marker earns its place twice over. The bare append-only rule holds only while every build that
ever wrote the file agreed on where the last field ended, and by now several have not: a config from
the Line Depth-era build ends in 31 trailing bytes this source writes nothing for, one from the sheen
build ends in 33 beginning with `"SHEN"`, and one from the abandoned Lightness Lock prototype ends in
blocks marked `"BLCL"` and `"WSTB"`. Appended bare, this block would have read the first bytes of any
of those as its amount - a legal float between 0 and 1 is easy to land on, and nothing downstream
could tell. Behind the marker each of those files simply loads with the lock off.

Anything appended after this must go after the lightness lock block, and behind a marker of its own
for the same reason.

## Warp color block

**The section above is stale as of 2026-08-30.** No source in this tree defines
`LIGHTNESS_LOCK_MAGIC` or writes a lightness lock block; the gloss block is the last one the
sections above describe that this build actually produces.

The color the warp carries into the picture is five floats behind
`ShaderPresetIO::WARP_COLOR_MAGIC`, `"WCLR"`, written directly after the gloss block, which makes it
the current end of the stream. Its fixed order is Color Amount, Color Spread, the three
channels of Warp Color, and Color Source. Each field is guarded independently, so a file ending inside the block keeps
the destination value for every remaining field, and the color's alpha is restored to 1 rather than
stored. A file whose next four bytes are not the marker is treated as carrying no warp color, the
destination attribute is left alone, and nothing further is read - which is the picture that file
was written under, since the warp bends the bands and lays in no color of its own in every build
that does not write this block.

The marker is a requirement rather than a precaution, for the reason the gloss block's is and for
one that is live in files already on disk: a config saved by the bloom color balance build ends in
`"BLCL"` and one further byte, and a bare float here would have read that byte and the three past
the end of the file as its own Color Amount. Behind the marker such a file simply loads with no
warp color at all.

Nothing about the palette is stored with it, and nothing is measured off one on load. The color is
named outright and laid over whatever the palette produced, so the block means the same thing on any
palette - including one that holds nothing like that color anywhere, which is the case it exists for.
Color Amount is the amount before the shader scales it by the brightness already at the pixel, so the
same stored number lays in less over a dark coloring than over a light one.

Anything appended after this must go after the warp color block, and behind a marker of its own for
the same reason.

## Orbit trap blocks

**The two sections above are stale as of 2026-08-30.** No source in this tree defines
`WARP_COLOR_MAGIC` or `LIGHTNESS_LOCK_MAGIC`, or writes either block. The gloss block is the last
one this build produced before the two described here.

The orbit trap is stored as two runs, both written after the gloss block and read in this order.

The calculation half is six bare fields — Shape and Measure as `int32`, Center Real, Center Imag and
Radius as `double`, and Skip Iterations as `uint32`. Each is guarded on bytes remaining, and a value
naming a shape or a measure this build no longer has falls back to the trap being off and to the
closest approach respectively. Bare rather than behind a marker because the gloss block that
precedes it carries a marker of its own: a file that ends before this run ends at a boundary the
gloss block already established, so there is no bare trailing field of an older build for these to
be read out of.

The look of it follows behind `ShaderPresetIO::ORBIT_TRAP_MAGIC`, `"OTRP"`, in the fixed order
Blend, Opacity, Distance Span, Offset, Contrast, Invert, the three channels of Tint Color, and
Palette Shift. Each field is guarded independently, and the tint's alpha is neither stored nor read.
A file whose next four bytes are not the marker is treated as carrying no trap look at all, the
destination attribute is left alone, and nothing further is read — which is the picture that file
was written under, since the trap is off in every build that does not write these blocks. The same
block, written by the same pair of functions, is the last thing in a shader preset.

The two halves are kept apart because they are answered by different work: the calculation half
recomputes the map, and the look does not. That is also why the look travels in shader presets while
the shape does not — a preset that carried the shape would recompute the picture it was loaded onto.

Anything appended after this must go after the orbit trap look block, and behind a marker of its own
for the same reason.

## Palette iteration coloring block

**The two sections above are stale as of 2026-08-31.** No source in this tree defines
`ORBIT_TRAP_MAGIC` or writes either orbit trap run. The gloss block is still the last one this build
writes before the block described here, and the projection block below follows it.

The palette's Iteration Coloring mode is one `int32` behind
`ShaderPresetIO::PALETTE_COLORING_MAGIC`, `"PCLR"`, written directly after the gloss block. The
field is guarded on bytes remaining, and a value naming a curve this build no longer has falls back
to Linear — the straight iteration count, which is what every build without this block drew. A file
whose next four bytes are not the marker is treated as carrying no such block at all, the
destination attribute is left alone, and nothing further is read.

The marker is a requirement rather than a precaution, for the reason the gloss block's is: several
builds ended the stream in bytes this source writes nothing for — the Line Depth build in 31 of
them, the sheen build in 33 beginning with `"SHEN"` — and each of the abandoned prototypes the
sections above describe would have ended it in a marked block of its own. A bare `int32` here would
have read the first word of any of those as its mode, and 0 through 6 are all legal ones with
nothing downstream able to tell. Behind the marker each of those files simply loads on Linear.

Nothing about the picture's depth is stored with the mode, and nothing is measured off one on load.
The curve is taken on the iteration count already divided by the cycle length, so the stored mode
means the same thing at any zoom and under any Cycle Length: each curve reaches the end of its first
cycle at exactly one Cycle Length, and only the cycles past the first are re-spaced.

The projection block described below is written directly after it, behind a marker of its own
for the same reason.

## Noise reduction block (removed)

The Render menu's Noise Reduction is gone, and this build neither writes it nor reads it. A config
saved by a build that had it carries one `bool` for the switch, one `uint32` for Similar Neighbours
and one `float` for Difference Threshold behind the marker `"NRDC"`, directly after the palette
iteration coloring block. This source no longer knows that marker, so such a config is treated as
ending at the palette iteration coloring block: the blocks written behind the removed one are not
reached, and the projection and warp target attributes keep the defaults they would take from a file
that carries neither — Planar and Palette, which is what a release build draws in any case.

A config this build writes has no such block, so the projection block sits directly after the
palette iteration coloring one instead, and a config this build re-saves carries no trace of it.

## Projection block

The Fractal menu's Projection — one `int32` for the projection method, one `float` each for
Panorama Range, Pitch and Field of View, then one `int32` for the layout — sits behind
`ConfigIO::PROJECTION_MAGIC`, `"PRJC"`, written directly after the palette iteration coloring block
and is the current end of the stream. Each field is guarded on bytes remaining, so a config written partway
through this block still loads, and one written before a later field existed loads on that field's
own default. A file whose next four bytes are not the
marker is treated as carrying no such block at all, the destination attribute is left alone, and
nothing further is read — which is the picture that file was written under, since the projection is
Planar in every build that does not write this block.

Both builds write this block and read it back, so one config stream serves them alike, but only a
debug build acts on what it carries: a release build has no Projection panel and draws every view
Planar, including one a debug build saved on a 360 projection. The values still survive the round
trip, so the same file opens back on that projection in a debug build.

The projection is not positional with the Fractal fields it belongs to, which end at the custom
formula string near the head of the stream. Appending it there would have moved every field written
after it, so it goes at the tail behind a marker of its own, for the reason the gloss, palette
coloring blocks each carry one.

Panorama Range is stored as the number the panel shows and is not resolved against the canvas on
load. It is a cap: the radius the picture actually folds at is the smaller of it and what one pixel
of the current canvas can tell apart, which is measured at render time and never written down. So a
config carries the limit its author chose rather than the resolution they chose it on, and the same
file folds a pixel's width away on any canvas it is opened at.

Nothing else about the canvas size is stored with the projection, and nothing else is measured off
one on load. Both 360 modes are laid out over whatever canvas they are drawn on — the equirectangular
layout's full width is one turn of longitude and its full height is pole to pole, and the camera's
Field of View is measured across the full width — so the stored values mean the same thing at any
resolution, and a 2:1 output is a matter of proportion rather than of the format.

The heading the 360 modes face is the Fractal menu's own Rotation, which is positional near the head
of the stream and is not repeated here: under a 360 projection that one angle is read as the yaw
rather than as the tilt of a flat image.

The warp target block described below is written directly after it, behind a marker of its own for
the same reason.

## Warp target block

Which side of Iteration Coloring's curve the domain warp is added on is one `int32` behind
`ShaderPresetIO::WARP_TARGET_MAGIC`, `"WTGT"`, written directly after the projection block, and is
the current end of the stream. The field is guarded on bytes remaining, and a value naming a side
this build no longer has falls back to Palette. A file whose next four bytes are not the marker is
treated as carrying no such block at all, the destination attribute is left alone, and nothing
further is read.

The field carries a marker and a place of its own rather than riding inside the warp block it
belongs to. That block sits mid-stream with the slope lighting fields directly behind it, so a
field appended there would move every offset after it; the config stream has the same problem one
block further on, since the projection block sits behind the palette iteration coloring one.

Palette is the fallback because it is the expression every build before the choice used, so a file
without the block is drawn from the same arithmetic it was saved from — bit for bit, not merely to
within a tolerance. The other side could not stand in for it: the two agree only under Linear with
Cycle Bias at 1.00, and Cycle Bias parts them even under Linear, because `biased_ratio` is taken on
the iteration term alone. Bands sends the displacement through the bias while Palette adds it
behind, so at a bias of 2.00 the two stand about a quarter of a cycle apart on the same pixel. A
file that names Bands is one saved since the choice existed.

Anything appended after this must go after the warp target block, and behind a marker of its own for
the same reason.

## Video camera block (2026-09-08)

Release builds now honor both 360 projection modes; the earlier debug-only
description is historical. After Gloss Relief, the current stream appends marker
`0x5643414D`, followed by Camera Padding (`bool`), Camera Scale (`uint32`), Rotation
(`float`), Projection (`int32`), Pitch, Field of View and Panorama Range (three
`float`s), and Layout (`int32`), in this order. Each field has its own `hasMore()`
guard. Older files retain the destination's defaults for missing fields; older
builds ignore the appended block. Config VERSION is unchanged.

The effective source scale is folder metadata, not a config setting. Padded
keyframe folders contain `camera-source.txt`: `RFF_CAMERA_1 <integer scale>`,
where scale is 2 through 64. Keep this file with all the keyframes. An absent file
means legacy unpadded coverage; an invalid file is an error. Map and compressed
map headers and their lossless payloads are unchanged.

Camera timeline target IDs 1100 through 1105 identify Rotation, Projection, Pitch,
Field of View, Panorama Range and Layout. Projection and Layout use held values;
Rotation interpolates the supplied angle without wrapping, allowing multiple turns.

## Linear RGB palette interpolation (2026-09-10)

The existing palette interpolation int32 in the Oklab modes section now accepts
`2 = Linear RGB`; `0 = RGB` and `1 = OKLab` retain their meanings. No fields move,
no new bytes are appended, and neither RFC nor RFSP changes its core version.
Earlier readers fall back to RGB for value 2; opening and saving with an earlier
build therefore loses the Linear RGB selection. Recipe IDs, seeds, raw palette
values and frozen iteration doubles retain their existing storage formats.

The GPU mode word is not the serialized enum. Bit 8 still selects OKLab and the
previously unused bit 14 selects Linear RGB; cycle curve and iteration coloring
retain bits 9 and 10–13. Both uniform and specialization paths use this mapping.

## Studio material block (2026-09-10)

RFC appends Studio after the video camera block; RFSP appends it after Gloss
Relief. Marker `0x53544758` is followed by a uint32 model (`0 = legacy`,
`1 = Studio GGX`) and eight float32 values in this fixed order: roughness,
metalness, IOR, direct intensity, environment intensity, softbox width,
clearcoat strength, clearcoat roughness. The complete block is 40 bytes.
Core versions and previous fields are unchanged. Future blocks belong after this.

An absent block explicitly resets Studio to its disabled defaults, including when
loading into a previously enabled session. Each complete trailing field is optional;
EOF retains defaults, while partial fields and unsupported model IDs are rejected.
Ranges and finiteness are validated. Older builds ignore the block and lose it if
they save again. Palette recipes and compressed map formats are unaffected.

## Surface and authored-stop block (2026-09-11)

After the complete Studio block, RFC and RFSP append marker `0x53524631`.
The fixed sequence is iridescence, film thickness, specular AA (three float32),
Lustre Relief (uint32 boolean), Lustre depth, normal smoothing, AO radius,
relief waves, wave frequency (five float32), invert relief (uint32 boolean),
boundary guard (float32), then a uint32 stop count. Counts are zero or 2–32.
Each stop has position followed by RGBA, all float32. A final uint32 stores
easing: 0 linear, 1 smoothstep, 2 smootherstep. New fields belong after this.

Missing fields retain the extension defaults, explicitly disabling new effects
and clearing stop metadata when loading older files into a reused session.
Every scalar trailer read is guarded; once a nonzero stop count is read its
entire array is required. Reject partial values, invalid booleans/counts/easing,
nonfinite values and positions that are not strictly increasing in [0,1).
Authored palettes save ordinary baked raw colors with recipe ID -1; the recipe
algorithm and all previous field offsets/versions stay unchanged.

## Lustre zoom reference (2026-09-11)

After stop easing, RFC and RFSP append Auto Zoom Compensation (uint32 boolean)
and the reference log10 zoom (float32), each guarded individually on read.
The flag defaults to true. Reference -1 means unset; other references must be
finite and nonnegative. Invalid booleans and partial scalars are rejected.
Older RFC files use their saved camera zoom as the reference, preserving the
loaded view. Older RFSP files start with an unset reference, resolved at the
current view when applied. The reference persists in new files; old readers
ignore the extra bytes. Core versions and earlier offsets do not change.

## Effects layers (2026-09-11)

After the complete Lustre zoom reference, RFC and RFSP append marker
`0x45465831`, followed by four layers in bottom-to-top order. Each layer is
96 bytes: four uint32 values (enabled, type, blend, mask), twelve float32
values (opacity, scale, speed, evolution, density, length, width, direction,
distortion, glow, seed, period), primary RGBA and secondary RGBA (eight float32).
The full extension is 388 bytes. Existing versions and offsets stay fixed.

Types 0–7 are Rain, Flame, Embers, Mist, Heat Haze, Ripples, Flow Light and
Aurora. Blends 0–3 are Normal, Add, Screen and Multiply. Masks 0–4 are All,
Exterior, Interior, Bands and Iteration Edges. These IDs must not be renumbered.

Effects are surface materials: their coordinates follow iteration bands and
slope aspect. `distortion` stores the surface micro-relief strength exposed as
Surface Depth. The Interior mask is retained as a reserved no-surface value;
effects never paint the interior. The All mask covers the exterior surface.

Loading without this extension resets all effect layers to disabled, including
when reusing a live session. Each scalar is guarded with the stream-buffer EOF
check; an absent complete scalar retains its default, and a partial scalar is
rejected. Invalid IDs, booleans, nonfinite values and out-of-range settings are
rejected. Older readers ignore the extension and discard it when saving again.

### Rain shape and size

After the complete Effects block, marker `0x45465832` introduces four pairs of
uint32 rain shape and float32 drop size (36 bytes including the marker).
Shape 0 is Water Streaks and 1 is Raindrops. Size is finite in [0.1,8].
Every trailing scalar is guarded individually; partial scalars and unknown IDs
are rejected. An older Effects block without this extension retains Water
Streaks and size 1. New layers default to Raindrops and size 1. Existing
Effects offsets and IDs are unchanged.

### Band Line grooves (2026-09-11)

After the complete rain shape extension, RFC and RFSP append four scalars:
uint32 Groove Mode, float32 Groove Depth, float32 Groove Width, and uint32
Auto Groove. The extension is 16 bytes; versions and earlier offsets stay fixed.
Defaults are 0, 1.5, 0.05, 1. Depth is finite in [0,5], width in [0,0.5],
and flags are 0 or 1. Each scalar is guarded individually; missing fields retain
defaults, while partial scalars and invalid values are rejected. Loading an
older file resets Groove Mode to off, preserving its painted Band Line appearance.
Old readers discard the extension when saving again. Width is a fraction of
band spacing; automatic adjustment caps screen width and softens narrow grooves.

### Effects color animation synchronization (2026-09-11)

After Band Line grooves, four uint32 flags retain Sync Color Animation in layer
order (16 bytes). Missing flags default to false; partial scalars and values
other than 0 or 1 are rejected. Existing offsets and versions stay unchanged.
When enabled, Speed and Evolution multiply the palette color animation rate.
Surface Scale accepts finite values in [0.1,100]; older readers can reject values
above their former upper limit of 10.

### Bio Explorer controls (2026-09-11)

RFC alone appends four float32 scalars after Effects color animation synchronization:
Zoom Speed, Observation Pause in seconds, Stimulus Gain, Brain Response in ms.
Defaults are 1, 0, 1, 200. Valid finite ranges are [0,10], [0,10], [0,2],
[50,1000]. Each scalar is guarded individually by the stream-buffer EOF check;
missing fields use explicit defaults, partial scalars and invalid values reject
the load. This is 16 bytes with no version change and no shift of earlier fields.
RFSP shader presets do not contain these controls. Model files, neural state,
and the running/stopped flag are not saved. Older readers ignore this trailer
and discard it when saving again.

### Bio Explorer attraction controls (2026-09-11)

RFC appends float32 Boundary Attraction and float32 Center Preference after Brain Response. Both default to 1 and accept finite values in [0,2]. These 8 bytes are append-only; each scalar has its own EOF guard. Missing fields reset to defaults and partial or invalid fields reject the load. RFSP is unchanged.

### Free exploration (2026-09-11)

RFC appends a uint32 Free Exploration flag after Center Preference, default 0. Only 0 and 1 are valid; missing fields default to disabled and partial scalars reject the load. Existing offsets and RFSP are unchanged.

### Surface Style (2026-09-12)

RFC appends Surface Style after Free Exploration. RFSP appends it after Effects
color animation synchronization. Each trailer is uint32 style (0 Original,
1 Y2K Chrome / Liquid Metal, 2 Cyber Sigilism), then float32 Chrome Strength,
Thin Film Color, and Sigil Edges, in that order. The three strengths are finite
in [0,1] and default to 1, 0.65, and 0.7. Missing fields reset to these defaults
and Original even when loading into an existing session. Each scalar has its
own EOF guard; partial scalars, invalid IDs and nonfinite/out-of-range strengths
reject the load. Earlier offsets and versions are unchanged. Older readers
ignore the trailer and discard it when saving.

### Surface Detail (2026-09-12)

RFC and RFSP append 16 float32 fields after Sigil Edges, in the following fixed
order (64 bytes). No existing version, offset, or style ID changes.

| Field | Default | Finite range |
| --- | ---: | --- |
| Reflection Detail | 1 | [0.25,4] |
| Reflection Contrast | 1 | [0.25,3] |
| Reflection Brightness | 1 | [0,3] |
| Reflection Curvature | 1 | [0.1,3] |
| Surface Phase | 0 | [0,1] |
| Rainbow Width | 1 | [0.1,3] |
| Rainbow Spread | 1 | [0,1] |
| Film Hue | 0 | [0,1] |
| Glint Strength | 1 | [0,3] |
| Contour Width | 1 | [0,3] |
| Spine Length | 1 | [0,3] |
| Spine Density | 1 | [0.25,3] |
| Branch Strength | 1 | [0,1] |
| Margin Ornaments | 1 | [0,1] |
| Ornament Size | 1 | [0.25,3] |
| Sigil Background | 4.5 | [0,8] |

Every scalar has its own stream-buffer EOF guard. Missing fields explicitly
reset to the listed defaults even when loading into a reused destination.
Partial scalars and invalid values reject the load. Old readers ignore this
extension and discard it when saving. The defaults reproduce the earlier
formulas; GPU rounding can differ by one 8-bit output code value.

### PHONK / Black Metal (2026-09-13)

The existing Surface Style uint32 additionally accepts 3 = PHONK / Drift Phonk
and 4 = Black Metal Album Art. IDs 0–2 are unchanged. Earlier readers reject
IDs 3–4; settings using IDs 0–2 remain readable by those readers.

After Sigil Background, RFC and RFSP append these 12 float32 fields (48 bytes):

| Field | Default | Finite range |
| --- | ---: | --- |
| Shadow Crush | 0.72 | [0,1] |
| Chrome Flames | 1 | [0,3] |
| Red / Purple | 0.85 | [0,1] |
| VHS Damage | 0.45 | [0,1] |
| Chromatic Shift | 1.5 | [0,8] |
| Nearest Mix | 0.2 | [0,1] |
| Nearest Block Size | 4 | [1,16] |
| Film Grain | 0.3 | [0,1] |
| Frost Strength | 1 | [0,3] |
| Frost Branches | 1 | [0,3] |
| Frost Threshold | 0.45 | [0,1] |
| Grunge Scale | 1 | [0.25,4] |

Every scalar is guarded independently with the stream-buffer EOF check.
Missing fields reset to defaults in reused destinations; partial scalars and
nonfinite/out-of-range values reject the load. Existing offsets and core
versions are unchanged. Earlier readers discard the extra fields on save.

### Surface Color & Blend / Surface Light & Detail (2026-09-13)

RFC and RFSP append 132 bytes after Grunge Scale: the following 17 float32
fields in order, one uint32 blend ID, then five RGB float32 triples.

| Field | Default | Finite range |
| --- | ---: | --- |
| Palette Color | 0 | [0,1] |
| Surface Color Amount | 0 | [0,1] |
| Reflection Color Amount | 0 | [0,1] |
| Contour Color Amount | 0 | [0,1] |
| Background Color Amount | 0 | [0,1] |
| Surface Rim Strength | 0 | [0,4] |
| Surface Rim Width | 2 | [0.1,32] |
| Preserve Palette Ink | 1 | [0,1] |
| Black Metal Monochrome | 1 | [0,1] |
| Glint Size | 1 | [0.25,4] |
| Ornament Density | 1 | [0.25,2] |
| Ornament Inset | 0.032 | [0,0.25] |
| Contour Strength | 1 | [0,1] |
| Detail Light | 1 | [0,2] |
| Dense Detail Suppression | 0 | [0,1] |
| Detail Density Threshold | 0.08 | [0.005,0.4] |
| PHONK Surface Damage | 0 | [0,1] |

Surface Blend IDs are 0 = Mix, 1 = Add, 2 = Multiply, 3 = Screen; default 0.
Never renumber or reuse these IDs. Following the blend ID are Surface Color,
Reflection Color, Contour Color, Background Color and Surface Rim Color, each
stored in R, G, B order as finite [0,1] encoded display colors. Defaults are
white except Background Color, which is black. Alpha is not stored and resets
to one. These RGB channels are individual scalars, not padded vec4 values.

Each scalar is independently EOF guarded. Missing values explicitly reset in
both loaders, including reused destinations. An incomplete scalar, invalid enum,
or nonfinite/out-of-range value rejects the load. Earlier readers ignore and
discard this tail when saving; no earlier offsets or core versions change.
Default color amounts, rim and density suppression are zero. Detail Light and
Contour Strength default to one. PHONK Surface Damage defaults to zero and
selects the earlier smooth PHONK reflections and tracking appearance, including
when loading files from the intervening worn-surface implementation. To retain
that worn look, raise PHONK Surface Damage explicitly. Default Black Metal
Monochrome is one; reduce it to retain chosen colors after output grading.

### Deep Sea Bioluminescence / Ukiyo-e Fractal (2026-09-13)

Surface Style adds 5 = Deep Sea Bioluminescence and 6 = Ukiyo-e Fractal.
IDs 0–4 retain their meanings. Earlier readers reject the new style IDs.
RFC and RFSP append 156 bytes after Surface Rim Color B: twelve float32
controls, then nine RGB float32 triples, in the following order.

| Control | Default | Finite range |
| --- | ---: | --- |
| Sea Glow | 1.6 | [0,4] |
| Sea Glow Threshold | 0.3 | [0,1] |
| Sea Body Light | 0.15 | [0,1] |
| Sea Particles | 0.5 | [0,1] |
| Sea Particle Size | 1 | [0.25,3] |
| Sea Color Balance | 0.55 | [0,1] |
| Print Color Count | 5 | [3,6] |
| Print Outline Width | 2 | [0,6] |
| Wave Foam | 0.75 | [0,1] |
| Woodcut Grain | 0.15 | [0,1] |
| Print Flatness | 1 | [0,1] |
| Print Tone Balance | 0.55 | [0,1] |

| RGB color | Default R, G, B |
| --- | --- |
| Sea Body Color | 0.02, 0.16, 0.38 |
| Sea Glow Color A | 0, 0.8, 1 |
| Sea Glow Color B | 0.55, 1, 0.16 |
| Print Ink | 0.008, 0.014, 0.025 |
| Print Indigo | 0.025, 0.12, 0.27 |
| Print Asagi | 0.13, 0.48, 0.58 |
| Print Pale Blue | 0.5, 0.72, 0.76 |
| Print Foam | 0.82, 0.89, 0.85 |
| Print Paper | 0.95, 0.91, 0.8 |

Each color is an encoded display RGB triple with finite components in [0,1].
Alpha is not serialized and resets to one. Every scalar has an independent
EOF guard, including individual RGB components. Missing fields reset to these
defaults in both loaders, even when the destination is reused. Partial scalars,
nonfinite/out-of-range values and unknown styles reject the load. Existing
offsets and core versions are unchanged. Old readers ignore this extension
for supported older styles and discard it when saving again.

Print Color Count rounds to an integer for rendering. Active colors are Ink,
the first N-2 intermediate colors (Indigo, Asagi, Pale Blue, Foam), and Paper.
Print Flatness one and full Chrome Strength/Opacity enforce those colors at
the final graphics pass; antialiasing and export downsampling can add mixtures
at edges. Lower flatness or partial material blending deliberately allows
intermediate colors. Configured RGB values are encoded for HDR exports using
the existing output transfer path.


### Cross-style effect amounts (2026-09-13)

Append ten float32 scalars (40 bytes) immediately after printPaper.b in this order:
layerMetal, layerSigil, layerPhonk, layerFrost, layerSea, layerPrint,
layerQuantize, layerVhs, layerMono, layerCommon. Each defaults to zero and accepts
only finite values in [0,1]. Each scalar has its own EOF guard; missing fields
reset on reused destinations, and incomplete scalars reject the file. Core
versions and existing offsets remain unchanged. Older readers ignore this tail
and discard it on resave. layerCommon records activation of common adjustments
on Original; the other nine amounts are editable under Effects Across All Styles.
UI edits activate the required family without rewriting defaults in old files.

### MFR display experiment (2026-09-16)

Tone-map IDs 0–3 retain their meanings. IDs 4, 5, 6, 7 select MFR Shoulder,
MFR Log View, MFR Linear Clip and MFR False Color, respectively. Earlier readers
fall back to Clip for these IDs. RFC appends float32 hdr.mfrPeakNits after the
ROT1 rotation block; RFSP appends the same field after layerCommon. The field
defaults to 4000 on every load if absent, accepts finite [100,10000], and is
individually EOF-guarded. A partial field or invalid value rejects the load.
Existing versions and byte offsets are unchanged; older readers ignore the new
trailing field and discard it on resave. MFR display metadata does not change
scene-light buffers or the existing PQ/HLG mastering-peak control.

### Shader layer order (2026-09-17)

Both RFC and RFSP append a 128-byte LAY1 block after hdr.mfrPeakNits:
uint32 marker 0x4C415931, uint32 enabled (0 or 1), uint32 count (29), then
29 uint32 layer IDs in first-applied to last-applied order. IDs 1 through 29
are fixed in ShdLayerOrder.h and must occur exactly once. Missing blocks reset
to disabled and the canonical order, including on reused load destinations.
The block starts only when bytes remain; partial blocks, invalid flags/counts,
unknown IDs and duplicates reject the load without replacing the destination.
Existing versions and offsets are unchanged. Earlier readers ignore this tail.

An optional 8-byte VIS1 block follows LAY1: uint32 marker 0x56495331 and uint32
hiddenMask. Bit (layer ID - 1) hides that layer; bits 29–31 must be zero.
An absent VIS1 resets every layer to visible, including on reused destinations.
Partial blocks, a wrong marker or unsupported bits reject the whole load.
Readers that understand LAY1 alone ignore visibility and show every layer.

Custom compositing is opt-in and uses linear-light working buffers. Moving or
hiding a layer enables it; disabling retains the order and visibility but uses
legacy rendering. Visibility leaves all effect parameters intact.
Reset restores the disabled flag, canonical order and visibility. Palette/warp source
coordinates and final display encoding/resampling remain fixed dependencies.

### Video export preview (2026-09-18)

RFC appends uint32 showExportPreview (0 or 1) after the LAY1/VIS1 blocks.
An absent field resets to true, including reused destinations. The field is
individually EOF guarded; partial fields and values above one reject the load.
Existing offsets and versions remain unchanged; older readers ignore this tail.
RFSP does not include this export setting.

## Independent Band Line decorations (2026-09-19)

Modified by GPT-6 on 2026-09-19. RFC appends BDS1 (uint32 0x42445331)
after AUD1; RFSP appends it after SBL1. Versions and earlier offsets are unchanged.
Eight float32 fields follow: spine amount, length, density, branch amount,
ornament amount, size, density, inset. Defaults: 0, 1, 1, 1, 0, 1, 1, 0.032.
Ranges: [0,1], [0,3], [0.25,3], [0,1], [0,1], [0.25,3], [0.25,2], [0,0.25].
All values must be finite. Each trailing scalar is independently EOF-guarded;
absent fields reset to defaults even in reused destinations. Partial scalars
and invalid markers reject the load. Older readers discard the extension on
resave. Existing material controls are neither copied nor enabled by this block.

The retired material sigil, frost-branch and print-outline fields retain their
serialized positions and values for compatibility, but no longer produce line
geometry. Loading them does not enable Band Line or migrate decorations.

The existing styleRimStrength, styleRimWidth and styleRimColor slots now store
Band Line glow. No slots move and no extension is added. Their loaded values
are retained, but glow requires Band Line. Existing layerVhs, layerMono and
layerQuantize amounts control standalone finishing independently of Studio,
shading depth and material strength; base styles no longer imply these layers.

## Setting-description preference (2026-09-19)

PreferencesIO appends one bool after legacySettingsUI in RFFP version 1.
True shows inline setting descriptions; false removes their layout height.
Files without the trailing byte use true. Artwork settings and shader presets
do not contain this UI preference.

Softbox removal (2026-09-19): the legacy softboxWidth and SBL1 Softbox
shape, brightness and color slots remain readable and writable at their
existing offsets, but no longer affect rendering or appear as controls.
Environment rotation/follow still orient the hemispheric environment light.

## Retired surface style IDs (2026-09-20)

Modified by GPT-6 on 2026-09-20. Serialized style IDs 7 and 8 are retired
and reserved against reuse. RFC and RFSP load them as Original (0); later
fields retain their existing offsets. IDs 0 through 6 remain unchanged.

## Surface style application (2026-09-20)

Modified by GPT-6 on 2026-09-20. RFC and RFSP append SRP1 (uint32
0x53525031) after BDS1, followed by uint32 replaceSurfaceStyle (0 or 1).
Missing fields reset to false, including reused load destinations. Each
trailing scalar is EOF guarded; partial scalars, invalid markers and values
above one reject the load. Existing versions and offsets remain unchanged.
Older readers ignore the tail and discard it on resave. True replaces a
non-Original base style without palette tint, palette-derived ink masks,
Surface Blend, Chrome Strength or Slope Opacity. Original and disabled
Studio retain their previous rendering behavior.

## Chaos blur (2026-09-20)

Modified by GPT-6 on 2026-09-20. RFC and RFSP append CHB1 (uint32
0x43484231) after SRP1, then eight float32 values in this fixed order:

| Field | Default | Finite range |
| --- | ---: | --- |
| Chaos Amount | 0 | [0,1] |
| Chaos Detail Scale | 1 | [0.5,4] |
| Chaos Threshold | 0.35 | [0,1] |
| Chaos Transition | 0.5 | [0.01,1] |
| Chaos Feather | 6 | [0,32] |
| Chaos Blur Radius | 4 | [0,32] |
| Chaos Highlight Detail | 0.2 | [0,1] |
| Chaos Shade | 0 | [0,0.5] |

Detection, feather and blur radii are measured at 1280 pixels wide. The mask
uses local iteration variation, without a fixed screen direction or an absolute
iteration-count cutoff. PNG video sources have no iteration structure and
disable these controls, as they already disable Focus Band. Every scalar is independently EOF
guarded. Missing values reset to defaults, including reused destinations;
partial scalars, invalid markers, nonfinite values and out-of-range values
reject the load without committing changes. Versions and existing offsets
remain unchanged. Older readers ignore this tail and discard it on resave.
Timeline IDs 512 through 519 follow this same order and remain fixed.

## Zoom overlay precision (2026-09-20)

Modified by GPT-6 on 2026-09-20. RFC appends ZDP1 (uint32 0x3150445a)
after CHB1; RFVT appends it after AUD1. A uint32 decimalPlaces follows,
accepting 0 through 9. Missing fields reset to 6, including reused load
destinations. Each trailing scalar is EOF guarded; partial scalars, invalid
markers and out-of-range values reject loading. Existing versions and offsets
remain unchanged. Older readers ignore this tail and discard it on resave.
