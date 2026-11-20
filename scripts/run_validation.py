#!/usr/bin/env python3
"""Rebuild and test the V5.4.4 source; run from any working directory."""
import argparse, datetime, hashlib, json, os, pathlib, platform, re, subprocess, sys, time

ROOT = pathlib.Path(__file__).resolve().parents[1]
os.chdir(ROOT)
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--core', action='store_true', help='headless, full UI/gameplay regressions, sanitizer, CLI smoke')
p.add_argument('--graphics', action='store_true', help='Linux EGL rendering, injected input lifecycle, syntax checks')
p.add_argument('--all', action='store_true', help='core and graphics')
p.add_argument('--compiler', default='g++')
p.add_argument('--skip-sanitizers', action='store_true', help='record sanitizer check as skipped')
args = p.parse_args()
if not (args.core or args.graphics or args.all): args.core = True
build = ROOT/'build'; build.mkdir(exist_ok=True)
results = ROOT/'artifacts/results'; results.mkdir(parents=True, exist_ok=True)
shots = ROOT/'artifacts/screenshots'; shots.mkdir(parents=True, exist_ok=True)
records = []
env = dict(os.environ, LC_ALL='C')
HOME = os.path.expanduser('~')
# Longest first: the checkout usually lives inside the home directory. CMake prints
# forward slashes on Windows, so each path is also matched in that spelling.
PRIVATE_PREFIXES = sorted({(spelling, label)
                           for path, label in ((str(ROOT), '.'), (HOME, '~'), (os.path.realpath(HOME), '~'))
                           for spelling in (path, path.replace('\\', '/'))},
                          key=lambda item: -len(item[0]))

def scrub(text):
    """Replace this machine's checkout and home paths so recorded logs can be published."""
    for prefix, label in PRIVATE_PREFIXES:
        if prefix not in ('', '/', '.'):
            text = re.sub(re.escape(prefix) + r'(?![^/\\\s"\'])', label, text)
    return text

def run(name, command, timeout=300, extra_env=None):
    started=time.monotonic()
    process=subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           text=True, env={**env, **(extra_env or {})}, timeout=timeout)
    (results/(name+'.log')).write_text(scrub(process.stdout))
    records.append(dict(name=name, command=[scrub(part) for part in command], exit_code=process.returncode,
                        seconds=round(time.monotonic()-started,3), status='passed' if process.returncode==0 else 'failed'))
    print(name+': '+records[-1]['status'], flush=True)
    if process.returncode: raise RuntimeError(name+' failed; see artifacts/results/'+name+'.log')
    return process.stdout

def ppm_to_png(path):
    # Standard-library encoder: GL capture emits P6 RGB with one simple header.
    import struct,zlib
    data=path.read_bytes();magic,dims,maximum,pixels=data.split(b'\n',3)
    assert magic==b'P6' and maximum==b'255'
    w,h=map(int,dims.split());assert len(pixels)==w*h*3
    def chunk(kind,body):
        return struct.pack('>I',len(body))+kind+body+struct.pack('>I',zlib.crc32(kind+body)&0xffffffff)
    scan=b''.join(b'\0'+pixels[y*w*3:(y+1)*w*3] for y in range(h))
    output=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(scan,9))+chunk(b'IEND',b'')
    path.with_suffix('.png').write_bytes(output)

source='src/mazerunnerV5.4.4.cpp'
flags=['-std=c++17','-Wall','-Wextra','-Wpedantic','-Werror','-pthread']
status=0
try:
    compiler=run('compiler',[args.compiler,'--version'])
    (results/'environment.json').write_text(json.dumps(dict(
        recorded_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        platform=platform.platform(),machine=platform.machine(),python=sys.version,
        compiler=compiler.splitlines()[0],source_sha256=hashlib.sha256(pathlib.Path(source).read_bytes()).hexdigest(),
        scope='Linux host execution; no native macOS or Windows execution; desktop GLFW startup not executed',
        sanitizer_leak_detection=False),indent=2)+'\n')
    if args.core or args.all:
        run('build-headless',[args.compiler,source,*flags,'-O2','-DMAZERUNNER_HEADLESS','-o','build/maze-headless'])
        run('help',['./build/maze-headless','--help'])
        radio = run('director-radio',['./build/maze-headless','--simulate','63','63','5','42','50','--difficulty','medium'])
        if 'Remote receivers: 5 | Using local evidence: 0 | Lead investigators: 1' not in radio:
            raise RuntimeError('Expected one lead for five remote receivers on the first report')
        relief = run('director-relief',['./build/maze-headless','--simulate','21','31','5','26','32000','--difficulty','medium'])
        for marker in ('RELIEF started', 'BACKSTAGE starts', 'WITHDRAWING to', 'BACKSTAGE ends',
                       'NORMAL precision restored', '| ESCAPED |'):
            if marker not in relief:
                raise RuntimeError('Missing director transition: '+marker)
        if not re.search(r'withdrawal_steps=[1-9]\d*', relief):
            raise RuntimeError('The investigator must physically leave the encounter')
        run('build-regressions',[args.compiler,'tests/ui_tests.cpp',*flags,'-O2','-o','build/regressions'])
        run('gameplay-and-ui',['./build/regressions'])
        if args.skip_sanitizers:
            records.append(dict(name='sanitizers',status='skipped',reason='--skip-sanitizers requested'))
        else:
            run('build-sanitizers',[args.compiler,'tests/ui_tests.cpp',*flags,'-O1','-g','-fsanitize=address,undefined','-o','build/regressions-asan'])
            run('sanitizers',['./build/regressions-asan'],extra_env={'ASAN_OPTIONS':'detect_leaks=0','UBSAN_OPTIONS':'halt_on_error=1'})
        smoke=[]
        for size in (33,63,101):
            for difficulty in ('low','medium','high'):
                command=['./build/maze-headless','--simulate',str(size),str(size),'10','42','1000','--difficulty',difficulty,'--quiet']
                a=run(f'preset-{size}-{difficulty}',command)
                b=run(f'preset-{size}-{difficulty}-repeat',command)
                assert a.split(' | compute_ms=')[0]==b.split(' | compute_ms=')[0]
                assert f'| {size}x{size} |' in a and '| monsters=10 |' in a
                smoke.append(dict(size=size,difficulty=difficulty,monsters=10,seed=42,limit_ms=1000,state=a.split(' | compute_ms=')[0].strip(),repeat_equal=True))
        (results/'preset-smoke.json').write_text(json.dumps(smoke,indent=2)+'\n')
    if args.graphics or args.all:
        if platform.system()!='Linux': raise RuntimeError('The packaged EGL harness is Linux-specific; use --core and the manual desktop checklist on this OS.')
        run('graphical-syntax',[args.compiler,source,*flags,'-Itests/support/syntax','-fsyntax-only'])
        for name,include in [('render','egl'),('app','app')]:
            run('build-'+name,[args.compiler,'tests/'+name+'_tests.cpp',*flags,'-O2','-Itests/support/'+include,'-l:libEGL.so.1','-l:libGL.so.1','-o','build/'+name+'-tests'])
            run(name,['./build/'+name+'-tests'])
        for path in shots.glob('*.ppm'): ppm_to_png(path)
except Exception as exc:
    status=1
    records.append(dict(name='runner',status='failed',error=scrub(str(exc))))
    print(str(exc),file=sys.stderr)
finally:
    (results/'validation-summary.json').write_text(json.dumps(dict(status='passed' if status==0 else 'failed',checks=records),indent=2)+'\n')
sys.exit(status)
