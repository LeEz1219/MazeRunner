#!/usr/bin/env python3
"""Configure, build, and test MazeRunner with CMake/CTest; record logs under artifacts/results/cmake."""
import argparse, datetime, json, os, pathlib, re, shutil, subprocess, sys, time

ROOT = pathlib.Path(__file__).resolve().parents[1]
os.chdir(ROOT)
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--build-dir', default='build/cmake-validation')
p.add_argument('--generator', default='Ninja' if shutil.which('ninja') else None,
               help='CMake generator (default: Ninja when available)')
p.add_argument('--headless', action='store_true', help='build CLI and tests without GLFW/GLEW development packages')
p.add_argument('--jobs', type=int, default=2, help='concurrent compiler processes')
args = p.parse_args()
if args.jobs < 1:
    p.error('--jobs must be positive')

results = ROOT / 'artifacts/results/cmake'
results.mkdir(parents=True, exist_ok=True)
build = pathlib.Path(args.build_dir)
records = []
env = dict(os.environ, LC_ALL='C')

def run(name, command, timeout=1800):
    started = time.monotonic()
    process = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
                             env=env, timeout=timeout)
    (results / (name + '.log')).write_text(process.stdout)
    records.append(dict(name=name, command=command, exit_code=process.returncode,
                        seconds=round(time.monotonic() - started, 3),
                        status='passed' if process.returncode == 0 else 'failed'))
    print(name + ': ' + records[-1]['status'], flush=True)
    if process.returncode:
        raise RuntimeError(name + ' failed; see artifacts/results/cmake/' + name + '.log')
    return process.stdout

status = 0
summary = {}
try:
    version = run('cmake-version', ['cmake', '--version']).splitlines()[0]
    configure = ['cmake', '-S', '.', '-B', str(build), '-DCMAKE_BUILD_TYPE=Release',
                 '-DBUILD_TESTING=ON', '-DMAZERUNNER_WARNINGS_AS_ERRORS=ON']
    configure.append('-DMAZERUNNER_BUILD_GRAPHICS=' + ('OFF' if args.headless else 'ON'))
    if args.generator:
        configure[1:1] = ['-G', args.generator]
    run('configure', configure)
    run('build', ['cmake', '--build', str(build), '--parallel', str(args.jobs)])
    output = run('ctest', ['ctest', '--test-dir', str(build), '--output-on-failure', '--no-tests=error'])
    match = re.search(r'(\d+)% tests passed, (\d+) tests failed out of (\d+)', output)
    if not match or int(match.group(3)) == 0:
        raise RuntimeError('CTest did not report a positive test count; see artifacts/results/cmake/ctest.log')
    tests = re.findall(r'Test\s+#\d+: (\S+) \.+\s+(\w+)', output)
    summary = dict(cmake=version, graphical_target=not args.headless, generator=args.generator or 'default',
                   tests_total=int(match.group(3)),
                   tests_failed=int(match.group(2)),
                   tests=[dict(name=n, result=r) for n, r in tests],
                   executables=sorted(x.name for x in (build / 'bin').iterdir()))
except Exception as exc:
    status = 1
    records.append(dict(name='runner', status='failed', error=str(exc)))
    print(str(exc), file=sys.stderr)
finally:
    (results / 'validation.json').write_text(json.dumps(dict(
        recorded_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        cmake_configure_build_ctest_status='passed' if status == 0 else 'failed',
        scope='Real CMake configure, Release build with warnings as errors, and CTest run on this Linux host.',
        summary=summary, checks=records), indent=2) + '\n')
sys.exit(status)
