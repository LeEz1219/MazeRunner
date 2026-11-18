#!/usr/bin/env python3
"""Measure auto-solver escape rates with the headless simulator; write artifacts/results/calibration.json.

The calibration cohort matches the source's balancing note: 21x21 and 31x31 maps,
five monsters, a 60 s limit, and consecutive seeds starting at 1. A second cohort
covers the three setup presets with ten monsters and a 120 s limit.
"""
import argparse, concurrent.futures, datetime, json, math, os, pathlib, platform, re, subprocess, sys, time

ROOT = pathlib.Path(__file__).resolve().parents[1]
os.chdir(ROOT)
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--compiler', default='g++')
p.add_argument('--seeds', type=int, default=400, help='seeds per calibration-map cell (default 400)')
p.add_argument('--preset-seeds', type=int, default=60, help='seeds per preset cell (default 60; 0 skips presets)')
p.add_argument('--jobs', type=int, default=os.cpu_count() or 2)
args = p.parse_args()

source = 'src/mazerunnerV5.4.2.cpp'
binary = ROOT / 'build/calibration/maze-headless'
binary.parent.mkdir(parents=True, exist_ok=True)
subprocess.run([args.compiler, source, '-std=c++17', '-O2', '-DNDEBUG', '-DMAZERUNNER_HEADLESS', '-pthread',
                '-o', str(binary)], check=True)

def simulate(job):
    size, monsters, limit, difficulty, seed = job
    out = subprocess.run([str(binary), '--simulate', str(size), str(size), str(monsters), str(seed), str(limit),
                          '--difficulty', difficulty, '--quiet'], stdout=subprocess.PIPE, text=True, check=True).stdout
    outcome = re.search(r'\| (ESCAPED|CAUGHT|RUNNING) \|', out).group(1)
    elapsed = int(re.search(r'simulation_ms=(\d+)', out).group(1))
    return job, outcome, elapsed

def wilson(k, n, z=1.96):
    if n == 0: return [0.0, 0.0]
    phat = k / n; denom = 1 + z * z / n
    centre = (phat + z * z / (2 * n)) / denom
    half = z * math.sqrt(phat * (1 - phat) / n + z * z / (4 * n * n)) / denom
    return [round(max(0.0, centre - half), 4), round(min(1.0, centre + half), 4)]

cohorts = [('calibration', s, 5, 60000, args.seeds) for s in (21, 31)]
if args.preset_seeds:
    cohorts += [('presets', s, 10, 120000, args.preset_seeds) for s in (33, 63, 101)]
jobs = [(size, monsters, limit, d, seed) for _, size, monsters, limit, count in cohorts
        for d in ('low', 'medium', 'high') for seed in range(1, count + 1)]
started = time.monotonic()
tally = {}
with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
    for job, outcome, elapsed in pool.map(simulate, jobs):
        key = job[:4]
        cell = tally.setdefault(key, dict(ESCAPED=0, CAUGHT=0, RUNNING=0, escape_ms=[]))
        cell[outcome] += 1
        if outcome == 'ESCAPED': cell['escape_ms'].append(elapsed)

rows = []
for cohort, size, monsters, limit, count in cohorts:
    for d in ('low', 'medium', 'high'):
        c = tally[(size, monsters, limit, d)]
        n = c['ESCAPED'] + c['CAUGHT'] + c['RUNNING']
        rows.append(dict(cohort=cohort, size=f'{size}x{size}', monsters=monsters, limit_ms=limit, difficulty=d,
                         seeds=f'1-{count}', games=n, escaped=c['ESCAPED'], caught=c['CAUGHT'], timed_out=c['RUNNING'],
                         escape_rate=round(c['ESCAPED'] / n, 4), escape_rate_95ci=wilson(c['ESCAPED'], n),
                         median_escape_ms=sorted(c['escape_ms'])[len(c['escape_ms']) // 2] if c['escape_ms'] else None))
pooled = {}
for d in ('low', 'medium', 'high'):
    cal = [r for r in rows if r['cohort'] == 'calibration' and r['difficulty'] == d]
    k = sum(r['escaped'] for r in cal); n = sum(r['games'] for r in cal)
    pooled[d] = dict(escape_rate=round(k / n, 4), escape_rate_95ci=wilson(k, n), games=n)
report = dict(recorded_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
              source=source, platform=platform.platform(),
              compiler=subprocess.run([args.compiler, '--version'], stdout=subprocess.PIPE, text=True).stdout.splitlines()[0],
              targets={'low': 0.92, 'medium': 0.65, 'high': 0.35}, pooled_calibration=pooled,
              wall_seconds=round(time.monotonic() - started, 1), results=rows)
out = ROOT / 'artifacts/results/calibration.json'
out.write_text(json.dumps(report, indent=2) + '\n')
for r in rows:
    print(f"{r['cohort']:11} {r['size']:>7} m={r['monsters']:<2} {r['difficulty']:6} escaped {r['escape_rate']*100:5.1f}% "
          f"caught {r['caught']/r['games']*100:5.1f}% timeout {r['timed_out']/r['games']*100:4.1f}%")
print('pooled calibration:', {d: v['escape_rate'] for d, v in pooled.items()})
