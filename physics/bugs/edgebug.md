# Edgebug: getting the nose down by leaving an edge

Back to the [catalogue](README.md). Used to reach the nose position of the [nosebug](nosebug.md). Related: [bluebug](bluebug.md), [rammstein](rammstein.md).

Status: verified [sim] for the measured cases below (exactly reproduced runs on a community map); mechanism inferred. The community term "edge bug" only says that the car "jumps" at sharp block edges ([community] https://tmnf.miraheze.org/wiki/UberBug); the wiki lists the nosebug set-up "rotation left over from another trick (edge bug)" ([community] https://www.trackmania.wiki/wiki/Nosebug). The usage below is my own: the rear of the car is lifted by catching an edge as the car leaves it, so the car pitches **nose down** in the following flight and can land in the nose position.

## Measured [sim]

| case | contact at take-off | speed at the kick | pitch after 0.5 s | landing |
|---|---|---|---|---|
| A | rear-right wheel on the corner of the edge | 413.6 to 400.7 km/h | +40 deg | front wheels, +66 deg, 387 km/h |
| B | rear-right wheel on the corner | 413.4 to 400.0 km/h | +37 deg | front-left, +62 deg, 387 km/h |
| C | rear wheels hit the edge **after** take-off | 394 km/h | **-44 deg (nose up)** | on the rear wheel |
| D (search from C's state) | earlier, before the corner | 400 to 386 km/h | +30 deg | front-left, **+90.0 deg**, 373 km/h, but no nosebug (no brake at the landing) |

The rear-wheel kick costs about 13 km/h and gives a steady forward rotation of about 70 deg/s; with about 1 s of flight the nose reaches 60-90 deg. A rear contact **after** take-off does the opposite and lifts the nose. Gas, and brake at the landing, decide whether the landing becomes a nosebug (case D lacked the brake).

See [nosebug](nosebug.md) for why the pitch must be settled at the landing, and [bluebug](bluebug.md) for the seam-stacking variant of lifting the rear.
