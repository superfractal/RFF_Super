<!-- Modified by GPT-6 on 2026-10-07, 2026-10-08. -->
<!-- Modified by Opus 5.5 on 2026-11-01. -->

# GMT formula implementation audit and concrete 2D transfers

> **Final review entry point (2026-10-08):** [Improving RFF_Super with GMT as a reference](GMT_to_RFF_Review_EN.md). All 3,727 inventoried GMT files have completed inspection records and unchanged hashes. This companion preserves chronological evidence: earlier progress counts, pending review statements and provisional priorities describe their recording time, not current coverage. Later corrections and the final entry point govern recommendations. Source inspection does not establish runtime correctness or measured GPU benefit.

Date: 2026-10-07. Part of the [ongoing full-code audit](GMT_Fractals_Deep_Code_Audit_EN.md). This is an incremental implementation review, not a completed proof of all 48 native formulas. The [coverage ledger](gmt-audit/coverage.json) records which complete files were read. Formula data, defaults and capabilities were read alongside their GLSL bodies. No GPU shader execution is claimed.

## Batch 3: power, polynomial, quaternion and sine families

### Individual implementation and transfer decisions

The scores below describe a useful RFF extension suggested by each row. They do not score the cost of reproducing the whole 3D renderer. Repeated infrastructure scopes overlap and must not be summed. Plane restrictions assume matching parameter/initial conditions; arbitrarily dropping a coordinate after each iteration creates a different map.

| Formula | Actual mechanism inspected | Concrete 2D transfer and limits | Difficulty / 10 | Effectiveness / 10 |
|---|---|---|---:|---:|
| Mandelbulb | Spherical power with independent polar/azimuth phase, z-dependent twist and optional post-step y clamp | A complex power/phase playground is useful, but a spherical-power slice is not automatically complex exponentiation. The clipping mutation is nonanalytic and needs its own acceleration policy. | 8.2 | 7.2 |
| Mandelbar3D | `(x*x-y*y-z*z, 2*x*y, -2*x*z)`, then scale, constant and offset | With y=0 and matching constants/transforms, the x/z plane is the conjugate-square map (Tricorn). With z=0 the x/y plane is ordinary complex squaring. Offer an explicitly named 2D conjugate mode rather than assuming every slice is Tricorn. | 7.8 | 7.8 |
| Bristorbrot | `(x*x-y*y-z*z, y*(2*x-z), z*(2*x+y))`, preceded by twist and x/z-axis rotations | With the third coordinate and its constant zero and transforms disabled, the x/y plane reduces to ordinary complex squaring. Its distinctive 3D cross terms disappear there. A coupled 3-component field slice would be genuinely different, but remains a different dynamical system. | 8.5 | 5.2 |
| MakinBrot | `(x*x-y*y-z*z, 2*x*y, 2*z*(x-y))`, rotation/twist and affine additions | The same zero-third-coordinate restriction reduces to complex squaring. Do not count this as an additional unique 2D formula unless coupling is deliberately retained. | 8.3 | 5.0 |
| Tetrabrot | Four-component quadratic map with a positive w-squared term in the real component and mixed products; three spatial plane rotations | The subspace z=w=0 with matching constants reduces to complex squaring. Other 2D slices require carrying all four evolving components and defining the sampling plane. A slice explorer is feasible, but does not inherit RFF's Mandelbrot MPA. | 8.7 | 6.1 |
| Quaternion | Quaternion square, 3D/4D rotations, optional inversion and an extrapolation-like iteration adjustment | A 2D view into a four-component state can morph interestingly with hidden coordinates. Preserve hidden state in saved views. Without rotations/inversion/adjustment and with the other imaginary components zero, it again reduces to complex squaring. | 8.8 | 6.8 |
| Buffalo | Selected absolute-value folds before and after a spherical-power step, with shared axis-angle rotation | A separate complex-plane pre/post-fold family can produce useful nonanalytic variants. Define the exact recurrence rather than transplanting spherical coordinates or claiming every toggle matches an existing named 2D formula. | 8.4 | 7.7 |
| BoxBulb | Box fold, sphere fold, scaling, spherical power, then add c | A 2D fold/power sequence is a plausible creative mode. Validate radius ordering and carry a real Jacobian or conservative bound; ordinary complex analytic derivatives do not apply across folds. | 8.8 | 7.0 |
| SineJulia3D | Period-wrapped extended sine with hyperbolic y terms, custom running derivative estimate and inversion framing | With z=0, constant z=0 and framing preserving that plane, the orbit map is exactly `s*sin(x+i*y)+c`. Implement the 2D map independently with a suitable transcendental iteration/termination policy; a polynomial escape-radius rule is not automatically valid. | 8.0 | 7.4 |
| Modular | Empty formula placeholders intercepted by `CoreMathFeature`, with a default Julia repeater pipeline | A restricted 2D formula/field graph remains useful, but this declaration alone is not a graph implementation. GraphCompiler, node contracts and precision support remain separate audit work. | 8.0 | 6.9 |

Sources: [Mandelbulb](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Mandelbulb.ts), [Mandelbar3D](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Mandelbar3D.ts), [Bristorbrot](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Bristorbrot.ts), [MakinBrot](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/MakinBrot.ts), [Tetrabrot](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Tetrabrot.ts), [Quaternion](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Quaternion.ts), [Buffalo](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Buffalo.ts), [BoxBulb](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/BoxBulb.ts), [SineJulia3D](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/SineJulia3D.ts), [Modular](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/formulas/Modular.ts). For each, grep the `formula_` function, `loopInit`, `parameters` and `capabilities` entries.

### F01 — Mandelbulb has an unguarded zero-radius angular expression

`formula_Mandelbulb` computes r=length(z), then z.z/r. At z=0 this is 0/0 before clamping; the subsequent angular/power reconstruction cannot be assumed to recover a finite zero. Buffalo and BoxBulb already floor their corresponding radius, but also retain `atan(0,0)` edge cases worth an explicit policy. Prefer a zero-vector branch defining the limiting power result and derivative/trap behavior, not reliance on NaN handling or arbitrary orientation. Test origin, axes, near-zero radii and powers across the supported range.

This is a source-level singular expression, not a reproduced GPU blank image. **GMT fix: difficulty 3.5, effectiveness 6.8.** The 2D lesson is to handle the exact complex origin algebraically and preserve coordinate precision; never use a visual blur to hide numerical failures.

### F02 — Native four-component formulas do not request the four-dimensional DE capability

Quaternion and Tetrabrot evolve z.w and use length(z) in their derivative tracker, but neither declares `render:de-4d`. `CoreMathFeature.inject` enables the four-dimensional kernel only when that token is present. Thus their standard current declarations do not opt into the 4D radius/bailout path, despite their descriptions and four-component iteration.

This is distinct from S08: S08 concerns inconsistency **inside an enabled 4D kernel**; this finding concerns native formulas not enabling it. Determine whether the xyz-only projection is intentional legacy behavior before changing presets. If it is intentional, describe that approximation; otherwise add the capability with image comparisons and saved-scene compatibility handling. **GMT fix: difficulty 6.2, effectiveness 7.4.** For RFF's potential slice explorer, state dimension, view dimension and bailout dimension must be separate metadata.

Sources: the two formula files above and [core_math.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/core_math.ts) (`render:de-4d`, `builder.enableIs4D`).

### F03 — Several derivative trackers are heuristics rather than complete transformed-map derivatives

Mandelbar3D, MakinBrot and Bristorbrot update `dr=2*r*dr+1` and then multiply the entire result by scale, while their point update is `scale*F(z)+c+offset`. On the positive real one-dimensional restriction, a parameter-dependent c with derivative one requires `scale*2*x*D+1`, not `scale*(2*x*D+1)`. At x=1,D=1,scale=2, those give 5 and 6. This is an explicit restricted-map counterexample to interpreting the tracker as an exact chain rule.

Julia constants require a different derivative convention from parameter-plane constants, and twist/nonconformal maps require more than a scalar radial shortcut. Existing estimators may intentionally be conservative or artistic approximations; a larger tracker in this example does not prove global safety. Document the convention and compare against finite-difference Jacobians over representative states. **GMT improvement: difficulty 7.5, effectiveness 7.4.** A 2D port should distinguish analytic, conjugate and piecewise-real derivative paths and should not inherit a generic `+1` blindly.

### F04 — Quaternion “Damping” extrapolates and has a sharp enable boundary

After computing the new iterate, the code adds `(new-old)/(2+100*abs(amount))`. The coefficient of the step from old to new is therefore greater than one: this is extrapolation, not an ordinary contraction toward the old state. It is disabled for amount<=0.001, but just above that boundary adds nearly 1/2.1 of the step. Increasing the amount then weakens the effect. The derivative tracker is computed before this adjustment and does not explicitly incorporate it.

The artistic map may be intentional; rename/explain it and preserve old scenes. If a continuously enabled damping effect is wanted, define a separate recurrence and derivative policy. **GMT improvement: difficulty 4.8, effectiveness 6.0.** For 2D, previous-state recurrences can be interesting, but require extra orbit state, precise parameter semantics and independent acceleration support.

### F05 — Shared sphere-fold bounds need an explicit radius-order policy

BoxBulb exposes min radius up to 1.5 and fixed radius down to 0.1 independently. The shared sphere fold uses `clamp(fixedR2/r2,1,fixedR2/minR2)`. For minR>fixedR, the upper clamp bound is below its lower bound, so the expression no longer supplies a well-defined ordered clamping contract. Flooring each radius to avoid division by zero does not resolve their ordering.

Trace all setters/import paths before deciding how to enforce this. Either validate minR<=fixedR, support reversed radii with an explicit piecewise definition, or expose the input as unsupported. **GMT improvement: difficulty 4.2, effectiveness 6.5.** The same constraint applies to a 2D fold sequence. Source: [math.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/shaders/chunks/math.ts) (`GLSL_SPHERE_FOLD`) and BoxBulb's parameter declarations.

### F06 — Rotation parameters and shared state need precise contracts

Bristorbrot and BoxBulb declare a three-component Rotation control but their inspected local code reads only x and z. UI generation or other injection may constrain/consume the remaining component, so this is a follow-up item rather than a proven whole-app inert control. Quaternion's vec2A default x=-6.44 is outside the declared [-6.28,6.28] range; check whether editing clamps or jumps an otherwise valid preset.

The shared Rodrigues precomputation only writes its global axis/sine/cosine when abs(params.z)>0.001. A second zero-angle call after a nonzero call in the same shader invocation retains earlier state. Current composition/renaming must be traced to establish reachability. The helper also halves params.z, which should be reflected in angle semantics. **GMT validation/cleanup: difficulty 4.8, effectiveness 6.1.** For RFF, prefer explicit transform inputs/local state and per-component availability over ambiguous generic vector controls.

Source: [transforms.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/features/geometry/transforms.ts) (`gmt_precalcRodrigues`, `gmt_applyRodrigues`).

### F07 — “Dampened” distance estimation can exceed the analytic estimate

The analytic branch uses coefficient 0.17328679 multiplying log2(r*r)*r/dr. The dampened branch uses 0.34657359 and denominator dr+8. Their ratio is approximately `2*dr/(dr+8)`, which exceeds one for dr>8 and approaches two for large dr. At dr=100 the ratio is about **1.85185**. This contradicts the adjacent description of a simple half-r-log-r numerator with an added damping denominator; it may enlarge the march step instead of reducing it.

Clarify the intended legacy estimator before changing coefficients, and compare silhouettes/oversteps across presets. Shared log smoothing also always uses an outer log2, while some formula powers differ from two; document it as a generic artistic mapping or provide formula-specific smoothing metadata. **GMT improvement: difficulty 5.5, effectiveness 6.9.** For RFF, give each formula a justified estimator/smoothing contract rather than sharing a power-two assumption.

Source: [estimators.ts](https://github.com/gamazama/GMT-fractals/blob/ad28b8bbca3c01b4d2cfebebd727c5bcd951a2e1/engine-gmt/engine/estimators.ts) (`analyticLog`, `dampened`, `generateGetDist`).

### Sine-family provenance and port boundary

The SineJulia3D header explicitly reports a Shadertoy-derived implementation and a noncommercial share-alike license, inferred there from platform defaults. That is source-reported provenance, not independently verified clearance. Do not treat the repository's root license as sufficient permission to copy this implementation into RFF. Its mathematically simple 2D sine recurrence can be independently implemented; the distinctive 3D derivative tracker, constants and transforms require separate provenance review. No external license research or legal compatibility determination has been completed here.

## Validation status

The implementation observations and plane reductions are source/math analysis. Formula-specific executable probes and coverage-ledger reconciliation are pending while the user-requested shader investigation takes priority; no formula probe results are claimed here. Migration behavior, UI constraints, GPU images and general DE conservativeness remain unverified. Other native formulas and the generated fold family remain in the full audit's worklist.
