# Difficulty and measured escape rates

[MazeRunner V5.4.3a wiki](Home.md)

## Exact difficulty parameters

All values below are read from `difficulty_settings`. Lower periods mean more frequent monster actions. Larger hint side lengths mean less precise information. Loop density is a budget multiplier, not an escape probability.

| Parameter | Low | Medium | High |
|---|---:|---:|---:|
| Minimum certified terrain solutions | 4 | 2 | 2 |
| Loop budget percentage | 225 | 135 | 100 |
| Monster movement interval, ms | 220 | 148 | 118 |
| Approximate moves per simulated second | 4.55 | 6.76 | 8.47 |
| Normal local sector side, cells | 9 | 5 | 5 |
| Local clue cooldown during continuous movement, ms | 200 | 0 | 0 |
| Nominal normal radio sector side, cells | 19 | 17 | 11 |
| Normal / relief radio cooldown, ms | 3,000 / 6,000 | 1,800 / 3,600 | 1,000 / 2,000 |
| Scent lifetime after last heard step, ms | 420 | 560 | 840 |
| Corridor hearing budget (base + steps per 100 cells of longer axis) | 18 + 10 | 18 + 14 | 28 + 18 |
| Hearing budget on 33 / 63 / 101-cell maps | 21 / 24 / 28 | 22 / 26 / 32 | 33 / 39 / 46 |
| Search sweep after losing the scent, ms | 1,500 | 2,500 | 3,500 |
| Maximum interceptors | 0 | 1 | 2 |
| Corridor sight, cells | off | off | 3–6 |
| Director confinement window, ms | 2,500 | 4,000 | 6,000 |
| Minimum relief hold, ms | 8,000 | 6,000 | 4,000 |
| Added padding per relief escalation, cells per side | 4 | 4 | 2 |
| Maximum padding, cells per side | 16 | 16 | 8 |
| Maximum nominal expanded local sector side, cells | 41 | 37 | 21 |
| Maximum nominal expanded radio sector side before map limits, cells | 51 | 49 | 27 |
| Auto-player action interval, ms | 40 | 40 | 40 |
| Tactical horizon / beam width | 12–18 / 64 | 12–18 / 64 | 12–18 / 64 |

### Measured escape rates

All 4,740 games used the bundled headless simulator. Calibration used five monsters on 21×21 and 31×31, seeds 1–400 per size and difficulty, at 60,000 simulated ms. Pooling the two sizes gives **751/800 Low (93.9%), 536/800 Medium (67.0%) and 273/800 High (34.1%)** against design targets of 92%, 65% and 35%. These rates apply to this seed protocol. The ten-monster presets are a separate, higher-pressure case.

| Sample | Rows × columns | Monsters | Difficulty | Escaped | Caught | At limit | Escape rate [95% CI] |
|---|---|---:|---|---:|---:|---:|---|
| calibration | 21 × 21 | 5 | Low | 359/400 | 40 | 1 | 89.8% [86.4, 92.3] |
| calibration | 21 × 21 | 5 | Medium | 254/400 | 143 | 3 | 63.5% [58.7, 68.1] |
| calibration | 21 × 21 | 5 | High | 116/400 | 281 | 3 | 29.0% [24.8, 33.6] |
| calibration | 31 × 31 | 5 | Low | 392/400 | 8 | 0 | 98.0% [96.1, 99.0] |
| calibration | 31 × 31 | 5 | Medium | 282/400 | 114 | 4 | 70.5% [65.8, 74.8] |
| calibration | 31 × 31 | 5 | High | 157/400 | 236 | 7 | 39.2% [34.6, 44.1] |
| presets | 33 × 33 | 10 | Low | 53/60 | 7 | 0 | 88.3% [77.8, 94.2] |
| presets | 33 × 33 | 10 | Medium | 30/60 | 30 | 0 | 50.0% [37.7, 62.3] |
| presets | 33 × 33 | 10 | High | 9/60 | 51 | 0 | 15.0% [8.1, 26.1] |
| presets | 63 × 63 | 10 | Low | 57/60 | 3 | 0 | 95.0% [86.3, 98.3] |
| presets | 63 × 63 | 10 | Medium | 27/60 | 33 | 0 | 45.0% [33.1, 57.5] |
| presets | 63 × 63 | 10 | High | 10/60 | 49 | 1 | 16.7% [9.3, 28.0] |
| presets | 101 × 101 | 10 | Low | 60/60 | 0 | 0 | 100.0% [94.0, 100.0] |
| presets | 101 × 101 | 10 | Medium | 43/60 | 17 | 0 | 71.7% [59.2, 81.5] |
| presets | 101 × 101 | 10 | High | 13/60 | 47 | 0 | 21.7% [13.1, 33.6] |
| setup | 21 × 31 | 5 | Medium | 260/400 | 136 | 4 | 65.0% [60.2, 69.5] |

The default setup is width 31, height 21, five monsters, Medium. It escaped in 260/400 games on seeds 401–800. Presets used ten monsters, seeds 1–60 and a 120,000-ms ceiling. A game still running at its ceiling counts as a non-escape. Intervals are two-sided 95% Wilson intervals for the observed fraction within each cell.

Fresh seeds 1201–1400 were run separately. The square maps pooled to 382/400 Low, 278/400 Medium and 144/400 High. The rectangular default held out at 146/200, giving 406/600 across its two disjoint cohorts. Seed sets overlap across profiles or map dimensions, so those rows should not be treated as independent replications of identical conditions.

| Sample | Rows × columns | Monsters | Difficulty | Escaped | Caught | At limit | Escape rate [95% CI] |
|---|---|---:|---|---:|---:|---:|---|
| holdout | 21 × 21 | 5 | Low | 186/200 | 14 | 0 | 93.0% [88.6, 95.8] |
| holdout | 21 × 21 | 5 | Medium | 135/200 | 62 | 3 | 67.5% [60.7, 73.6] |
| holdout | 21 × 21 | 5 | High | 55/200 | 144 | 1 | 27.5% [21.8, 34.1] |
| holdout | 31 × 31 | 5 | Low | 196/200 | 3 | 1 | 98.0% [95.0, 99.2] |
| holdout | 31 × 31 | 5 | Medium | 143/200 | 56 | 1 | 71.5% [64.9, 77.3] |
| holdout | 31 × 31 | 5 | High | 89/200 | 110 | 1 | 44.5% [37.8, 51.4] |
| setup-holdout | 21 × 31 | 5 | Medium | 146/200 | 54 | 0 | 73.0% [66.5, 78.7] |

Every game has an outcome, seed, simulated duration and step count. [Per-game CSV](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/artifacts/results/calibration-runs.csv) · [Summary and intervals](https://github.com/LeEz1219/MazeRunner/blob/b73842a26759c888d9919c48f04be167cca99ad2/artifacts/results/calibration.json)
