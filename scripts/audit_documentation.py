#!/usr/bin/env python3
"""Re-record documentation coverage/audit results, manifest.json, and SHA256SUMS for the current tree.

Run after an intended change to the source, guides, tests, or regenerated artifacts.
It reads what is on disk; it does not build or run anything. Afterwards,
scripts/verify_package.py checks the tree against the records written here.
"""
import csv, datetime, hashlib, json, os, pathlib, re, struct, sys

from verify_package import function_coverage, release_paths

ROOT = pathlib.Path(__file__).resolve().parents[1]
RELEASE = '5.4.3c'
SOURCE = f'src/mazerunnerV{RELEASE}.cpp'
DOCUMENTS = ['01-Source-Line-by-Line.md', '02-AI-Graphics-and-Redesign.md', '03-Build-Test-and-Reproduce.md']
RESULTS = ROOT / 'artifacts/results'
# Files that describe the package rather than belong to it, and generated build output.
EXCLUDED_DIRS = {'.git', 'build', 'dist'}
EXCLUDED_FILES = {'manifest.json', 'SHA256SUMS'}

def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def report_unlisted_files(approved):
    for directory, children, filenames in os.walk(ROOT):
        parent = pathlib.Path(directory).relative_to(ROOT)
        children[:] = [name for name in children if name != '__pycache__' and
                       not (parent == pathlib.Path('.') and
                            (name in EXCLUDED_DIRS or name.startswith('cmake-build-')))]
        for name in sorted(filenames):
            rel = (parent / name).as_posix()
            if rel not in approved and rel not in EXCLUDED_FILES and not rel.endswith(('.tmp', '.ppm')):
                print('Excluded unlisted file: ' + rel, file=sys.stderr)

def calibration_matches():
    report = json.loads((RESULTS / 'calibration.json').read_text())
    if report['source_sha256'] != sha256(ROOT / SOURCE):
        return False
    groups = {}
    with (RESULTS / 'calibration-runs.csv').open(newline='') as records:
        for row in csv.DictReader(records):
            group = (row['cohort'], row['height'] + 'x' + row['width'], int(row['monsters']),
                     int(row['limit_ms']), row['difficulty'])
            seed, elapsed, moves = (int(row[k]) for k in ('seed', 'simulation_ms', 'auto_steps'))
            outcome = row['outcome']
            if outcome not in ('ESCAPED', 'CAUGHT', 'RUNNING') or not 0 <= elapsed <= group[3] or moves < 0:
                return False
            if outcome == 'RUNNING' and elapsed != group[3]:
                return False
            outcomes = groups.setdefault(group, {})
            if seed in outcomes:
                return False
            outcomes[seed] = outcome
    total = 0
    for cell in report['results']:
        group = (cell['cohort'], cell['size'], cell['monsters'], cell['limit_ms'], cell['difficulty'])
        outcomes = groups.pop(group, {})
        start, end = map(int, cell['seeds'].split('-'))
        if set(outcomes) != set(range(start, end + 1)) or len(outcomes) != cell['games']:
            return False
        counts = list(outcomes.values())
        if [counts.count(s) for s in ('ESCAPED', 'CAUGHT', 'RUNNING')] != [cell[k] for k in ('escaped','caught','timed_out')]:
            return False
        total += len(counts)
    return not groups and total == report['total_games']


def main():
    files = release_paths()
    report_unlisted_files(set(files))
    now = datetime.datetime.now(datetime.timezone.utc).isoformat()
    source = (ROOT / SOURCE).read_bytes()
    lines = source.decode('utf-8').splitlines()
    guide = (ROOT / DOCUMENTS[0]).read_text(encoding='utf-8')
    coverage = dict(source=SOURCE, source_sha256=hashlib.sha256(source).hexdigest(), physical_lines=len(lines),
                    **function_coverage(source.decode('utf-8'), guide, SOURCE))
    (RESULTS / 'documentation-coverage.json').write_text(json.dumps(coverage, indent=2) + '\n')

    checks = {
        'source_matches_function_guide': coverage['exact_match'],
        'exactly_three_markdown_files': sorted(name for name in files if '/' not in name and name.endswith('.md')) == DOCUMENTS,
        'eighteen_png_screenshots': sum(pathlib.Path(name).parent.as_posix() == 'artifacts/screenshots' and name.endswith('.png') for name in files) == 18,
        'cmake_file_included': (ROOT / 'CMakeLists.txt').is_file(),
    }
    validation = json.loads((RESULTS / 'validation-summary.json').read_text())
    cmake_path = RESULTS / 'cmake/validation.json'
    cmake = json.loads(cmake_path.read_text()) if cmake_path.is_file() else {}
    cmake_summary = cmake.get('summary', {})
    names = {check['name'] for check in validation['checks'] if check['status'] == 'passed'}
    checks['raw_validation_passed'] = (validation['status'] == 'passed' and
        {'gameplay-and-ui', 'app', 'render', 'sanitizers', 'director-radio', 'director-relief'} <= names)
    checks['cmake_configure_build_ctest_passed'] = cmake.get('cmake_configure_build_ctest_status') == 'passed'
    binaries = {name.removesuffix('.exe') for name in cmake_summary.get('executables', [])}
    checks['cmake_built_release_target'] = (
        'maze-headless' in binaries and
        (cmake_summary.get('graphical_target') is False or f'mazerunnerV{RELEASE}' in binaries))
    checks['raw_calibration_matches_all_groups_and_seeds'] = calibration_matches()
    checks['director_evidence'] = (
        'Remote receivers: 5 | Using local evidence: 0 | Lead investigators: 1' in (RESULTS / 'director-radio.log').read_text()
        and all(marker in (RESULTS / 'director-relief.log').read_text()
                for marker in ('RELIEF started','WITHDRAWING to','BACKSTAGE ends','NORMAL precision restored')))
    try:
        smoke = json.loads((RESULTS / 'preset-smoke.json').read_text())
        checks['smoke_matrix_complete'] = len(smoke) == 9 and all(r['repeat_equal'] for r in smoke)
    except (OSError, ValueError, KeyError):
        checks['smoke_matrix_complete'] = False
    for document in DOCUMENTS:
        text = (ROOT / document).read_text(encoding='utf-8')
        checks[f'{document}_fences_balanced'] = text.count('```') % 2 == 0
        checks[f'{document}_no_internal_paths'] = not re.search(r'/(tmp|root|home)/[A-Za-z]', text)
    audit = dict(recorded_utc=now, release=RELEASE, status='passed' if all(checks.values()) else 'failed', checks=checks,
                 visual_review=['setup.png', 'hud.png', 'game.png', 'result.png'],
                 review_notes=['The release HUD fixture uses seed 8 during withdrawal. The result fixture records a zero-monster escape.',
                               'Native GLFW desktop startup and macOS/Windows builds were not executed.',
                               'Portable/regression assertion counters are cumulative, not independent simulation totals.'])
    (RESULTS / 'documentation-audit.json').write_text(json.dumps(audit, indent=2) + '\n')

    env = json.loads((RESULTS / 'environment.json').read_text()) if (RESULTS / 'environment.json').is_file() else {}
    render = (RESULTS / 'render.log').read_text().splitlines()[0] if (RESULTS / 'render.log').is_file() else None
    calibration = json.loads((RESULTS / 'calibration.json').read_text()) if (RESULTS / 'calibration.json').is_file() else {}
    manifest = dict(
        package=f'MazeRunner V{RELEASE} Documentation and Validation', release=RELEASE, previous_release='5.4.3b',
        prepared_date=now[:10],
        source=dict(path=SOURCE, bytes=len(source), physical_lines=len(lines), sha256=coverage['source_sha256']),
        validation=dict(platform=env.get('platform'), compiler=env.get('compiler'), renderer=render,
                        direct_runner_checks=len(json.loads((RESULTS / 'validation-summary.json').read_text())['checks']),
                        benchmark_games=calibration['total_games'], native_glfw_startup_executed=False, macos_execution=False, windows_execution=False,
                        sanitizer_leak_detection=env.get('sanitizer_leak_detection'),
                        preset_smoke_configurations=9, smoke_invocations=18,
                        cmake_configure_build_ctest=cmake.get('cmake_configure_build_ctest_status'),
                        ctest_tests=cmake.get('summary', {}).get('tests_total'),
                        calibration_benchmark_performed=bool(calibration),
                        calibration_pooled_escape_rates={k: v['escape_rate'] for k, v in calibration.get('pooled_calibration', {}).items()}),
        entry_guide='03-Build-Test-and-Reproduce.md',
        excluded_generated_files=['build/', 'dist/', 'artifacts/screenshots/*.ppm'],
        build_system=dict(file='CMakeLists.txt', minimum_cmake='3.20',
                          targets=['mazerunner', 'maze-headless', 'maze-regressions', 'maze-render-tests', 'maze-app-tests'],
                          full_package_ctest_cases=13, ctest_cases_without_egl=11,
                          configuration_executed=cmake.get('cmake_configure_build_ctest_status') == 'passed',
                          validation_record='artifacts/results/cmake/validation.json'),
        files=[dict(path=f, bytes=(ROOT / f).stat().st_size, sha256=sha256(ROOT / f)) for f in files])
    (ROOT / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    sums = [f'{sha256(ROOT / f)}  {f}' for f in files + ['manifest.json']]
    (ROOT / 'SHA256SUMS').write_text('\n'.join(sums) + '\n')
    print(f"audit {audit['status']}: {len(lines)} lines, {coverage['documented_functions']} documented functions; "
          f"manifest lists {len(files)} files")
    for key, ok in checks.items():
        if not ok: print('FAILED CHECK:', key, file=sys.stderr)
    return 0 if audit['status'] == 'passed' else 1

if __name__ == '__main__':
    sys.exit(main())
