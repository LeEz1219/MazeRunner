#!/usr/bin/env python3
"""Measure auto-solver escape rates with the headless simulator; write artifacts/results/calibration.json.

The calibration cohort matches the source's balancing note: 21x21 and 31x31 maps,
five monsters, a 60 s limit, and consecutive seeds starting at 1. A second cohort
covers the three setup presets with ten monsters and a 120 s limit. The actual
setup default (21 rows, 31 columns, five monsters, Medium) uses seeds 401–800.
Holdouts start at 1201. Results include one ordered CSV row per simulation.
"""
import argparse, concurrent.futures, csv, datetime, hashlib, json, math, os, pathlib, platform, re, statistics, subprocess, time

ROOT = pathlib.Path(__file__).resolve().parents[1]
os.chdir(ROOT)
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--compiler', default='g++')
p.add_argument('--seeds', type=int, default=400, help='seeds per calibration-map cell (default 400)')
p.add_argument('--preset-seeds', type=int, default=60, help='seeds per preset cell (default 60; 0 skips presets)')
p.add_argument('--holdout-seeds', type=int, default=200, help='fresh seeds per square map/profile and default Medium (0 skips)')
p.add_argument('--jobs', type=int, default=min(8, os.cpu_count() or 2))
args = p.parse_args()
if args.jobs < 1 or args.seeds < 1 or min(args.holdout_seeds, args.preset_seeds) < 0:
    p.error('jobs and seeds must be positive; optional cohort sizes must be nonnegative')

source = 'src/mazerunnerV5.4.4.cpp'
binary = ROOT / 'build/calibration/maze-headless'
binary.parent.mkdir(parents=True, exist_ok=True)
subprocess.run([args.compiler, source, '-std=c++17', '-O2', '-DNDEBUG', '-DMAZERUNNER_HEADLESS', '-pthread',
                '-o', str(binary)], check=True)

def simulate(job):
    cohort, height, width, monsters, limit, difficulty, seed = job
    out = subprocess.run([str(binary), '--simulate', str(height), str(width), str(monsters), str(seed), str(limit),
                          '--difficulty', difficulty, '--quiet'], stdout=subprocess.PIPE, text=True, check=True).stdout
    outcome = re.search(r'\| (ESCAPED|CAUGHT|RUNNING) \|', out).group(1)
    elapsed = int(re.search(r'simulation_ms=(\d+)', out).group(1))
    steps = int(re.search(r'auto_steps=(\d+)', out).group(1))
    return job, outcome, elapsed, steps

def wilson(k, n, z=1.96):
    if n == 0: return [0.0, 0.0]
    phat = k / n; denom = 1 + z * z / n
    centre = (phat + z * z / (2 * n)) / denom
    half = z * math.sqrt(phat * (1 - phat) / n + z * z / (4 * n * n)) / denom
    return [round(max(0.0, centre - half), 4), round(min(1.0, centre + half), 4)]

profiles = ('low', 'medium', 'high')
cohorts = [('calibration', s, s, 5, 60000, 1, args.seeds, profiles) for s in (21, 31)]
if args.preset_seeds:
    cohorts += [('presets', s, s, 10, 120000, 1, args.preset_seeds, profiles) for s in (33, 63, 101)]
cohorts.append(('setup', 21, 31, 5, 60000, 401, args.seeds, ('medium',)))
if args.holdout_seeds:
    cohorts += [('holdout', s, s, 5, 60000, 1201, args.holdout_seeds, profiles) for s in (21, 31)]
    cohorts.append(('setup-holdout', 21, 31, 5, 60000, 1201, args.holdout_seeds, ('medium',)))
jobs = [(cohort, height, width, monsters, limit, d, seed) for cohort, height, width, monsters, limit, start, count, ds in cohorts
        for d in ds for seed in range(start, start + count)]
started = time.monotonic()
tally = {}
output_dir = ROOT / 'artifacts/results'
output_dir.mkdir(parents=True, exist_ok=True)
raw_path = output_dir / 'calibration-runs.csv'
temporary = raw_path.with_suffix('.csv.tmp')
with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool, temporary.open('w', newline='') as raw:
    writer = csv.writer(raw, lineterminator='\n')
    writer.writerow(('cohort', 'height', 'width', 'monsters', 'limit_ms', 'difficulty', 'seed', 'outcome', 'simulation_ms', 'auto_steps'))
    for job, outcome, elapsed, steps in pool.map(simulate, jobs):
        writer.writerow((*job, outcome, elapsed, steps))
        key = job[:-1]
        cell = tally.setdefault(key, dict(ESCAPED=0, CAUGHT=0, RUNNING=0, escape_ms=[]))
        cell[outcome] += 1
        if outcome == 'ESCAPED': cell['escape_ms'].append(elapsed)

temporary.replace(raw_path)
rows = []
for cohort, height, width, monsters, limit, start, count, ds in cohorts:
    for d in ds:
        c = tally[(cohort, height, width, monsters, limit, d)]
        n = c['ESCAPED'] + c['CAUGHT'] + c['RUNNING']
        rows.append(dict(cohort=cohort, size=f'{height}x{width}', monsters=monsters, limit_ms=limit, difficulty=d,
                         seeds=f'{start}-{start + count - 1}', games=n, escaped=c['ESCAPED'], caught=c['CAUGHT'], timed_out=c['RUNNING'],
                         escape_rate=round(c['ESCAPED'] / n, 4), escape_rate_95ci=wilson(c['ESCAPED'], n),
                         median_escape_ms=statistics.median(c['escape_ms']) if c['escape_ms'] else None))
pooled = {}
for d in ('low', 'medium', 'high'):
    cal = [r for r in rows if r['cohort'] == 'calibration' and r['difficulty'] == d]
    k = sum(r['escaped'] for r in cal); n = sum(r['games'] for r in cal)
    pooled[d] = dict(escape_rate=round(k / n, 4), escape_rate_95ci=wilson(k, n), games=n)
report = dict(recorded_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
              source=source, source_sha256=hashlib.sha256((ROOT / source).read_bytes()).hexdigest(),
              total_games=len(jobs), raw_file=raw_path.name, platform=platform.platform(),
              compiler=subprocess.run([args.compiler, '--version'], stdout=subprocess.PIPE, text=True).stdout.splitlines()[0],
              targets={'low': 0.92, 'medium': 0.65, 'high': 0.35}, pooled_calibration=pooled,
              wall_seconds=round(time.monotonic() - started, 1), results=rows)
out = ROOT / 'artifacts/results/calibration.json'
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(json.dumps(report, indent=2) + '\n')
for r in rows:
    print(f"{r['cohort']:11} {r['size']:>7} m={r['monsters']:<2} {r['difficulty']:6} escaped {r['escape_rate']*100:5.1f}% "
          f"caught {r['caught']/r['games']*100:5.1f}% timeout {r['timed_out']/r['games']*100:4.1f}%")
print('pooled calibration:', {d: v['escape_rate'] for d, v in pooled.items()})
