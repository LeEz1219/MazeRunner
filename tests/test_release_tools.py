"""Regression tests for developer kit membership and CTest validation."""
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def run(*args, cwd, check=True):
    return subprocess.run(args, cwd=cwd, check=check, text=True, capture_output=True)


class ReleaseToolsTests(unittest.TestCase):
    def test_local_files_stay_out_of_the_kit(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            approved = (ROOT / 'scripts/devkit-files.txt').read_text().splitlines()
            for name in approved:
                destination = root / name
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(ROOT / name, destination)
            extras = ['local-review.md', 'artifacts/results/private-note.log',
                      'artifacts/screenshots/unpublished.png']
            for name in extras:
                (root / name).write_text('local only')
            audit = run(sys.executable, 'scripts/audit_documentation.py', cwd=root)
            for name in extras:
                self.assertIn('Excluded unlisted file: ' + name, audit.stderr)
            manifest = json.loads((root / 'manifest.json').read_text())
            self.assertEqual({e['path'] for e in manifest['files']}, set(approved))
            run(sys.executable, 'scripts/package_devkit.py', '--output', 'kit.zip', cwd=root)
            with zipfile.ZipFile(root / 'kit.zip') as archive:
                self.assertEqual(set(archive.namelist()), {
                    'MazeRunner-V' + manifest['release'] + '/' + name
                    for name in approved + ['manifest.json', 'SHA256SUMS']
                })

    @unittest.skipUnless(shutil.which('cmake') and shutil.which('ctest'), 'requires CMake and CTest')
    def test_cached_testing_off_and_empty_test_suite(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'scripts').mkdir()
            shutil.copyfile(ROOT / 'scripts/run_cmake_validation.py', root / 'scripts/run_cmake_validation.py')
            (root / 'CMakeLists.txt').write_text(
                'cmake_minimum_required(VERSION 3.20)\n'
                'project(ValidationFixture NONE)\n'
                'include(CTest)\n'
                'file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/bin")\n'
                'if(BUILD_TESTING AND NOT EMPTY_SUITE)\n'
                '  add_test(NAME example COMMAND "${CMAKE_COMMAND}" -E true)\n'
                'endif()\n'
            )
            generator = ['-G', 'Ninja'] if shutil.which('ninja') else []
            run('cmake', '-S', '.', '-B', 'build', *generator, '-DBUILD_TESTING=OFF', cwd=root)
            command = [sys.executable, 'scripts/run_cmake_validation.py', '--headless', '--build-dir', 'build']
            run(*command, cwd=root)
            report = root / 'artifacts/results/cmake/validation.json'
            self.assertEqual(json.loads(report.read_text())['summary']['tests_total'], 1)

            run('cmake', '-S', '.', '-B', 'build', '-DEMPTY_SUITE=ON', cwd=root)
            self.assertNotEqual(run(*command, cwd=root, check=False).returncode, 0)
            result = json.loads(report.read_text())
            self.assertEqual(result['cmake_configure_build_ctest_status'], 'failed')
            ctest = next(item for item in result['checks'] if item['name'] == 'ctest')
            self.assertNotEqual(ctest['exit_code'], 0)


if __name__ == '__main__':
    unittest.main()
