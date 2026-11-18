#!/usr/bin/env python3
"""Verify the developer kit and build its sorted, deterministic ZIP."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, help='archive destination (default: dist/)')
    args = parser.parse_args()
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_package.py')], check=True)
    manifest = json.loads((ROOT / 'manifest.json').read_text(encoding='utf-8'))
    folder = 'MazeRunner-V' + manifest['release']
    output = (args.output or ROOT / 'dist' / (folder + '-Documentation-and-Validation.zip')).resolve()
    paths = sorted([item['path'] for item in manifest['files']] + ['manifest.json', 'SHA256SUMS'])
    if output in {(ROOT / path).resolve() for path in paths}:
        parser.error('choose an output path outside the manifest payload')
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=output.parent, delete=False) as handle:
        temporary = Path(handle.name)
    try:
        with zipfile.ZipFile(temporary, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
            for name in paths:
                info = zipfile.ZipInfo(folder + '/' + name, date_time=(1980, 1, 1, 0, 0, 0))
                info.create_system = 3
                info.external_attr = 0o100644 << 16
                archive.writestr(info, (ROOT / name).read_bytes(), compress_type=zipfile.ZIP_DEFLATED, compresslevel=9)
        with zipfile.ZipFile(temporary) as archive:
            if archive.testzip() is not None:
                raise ValueError('archive failed its CRC check')
        temporary.replace(output)
    finally:
        if temporary.exists():
            temporary.unlink()
    print(f'{hashlib.sha256(output.read_bytes()).hexdigest()}  {output.name} ({len(paths)} files)')


if __name__ == '__main__':
    main()
