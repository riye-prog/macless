import argparse
import hashlib
import json
import os
import pathlib
import re
import struct
import subprocess
import time


ROOT = pathlib.Path(__file__).resolve().parents[1]
VERSION = json.loads((ROOT / 'src/main/resources/fabric.mod.json').read_text())['version'].split('+')[0]
DIRECTORY = ROOT / 'verification' / ('gui-' + VERSION)
CASES = {
    'legacy': ['python3', 'tools/verify_prism.py', '--launcher-log', os.environ.get('MACLESS_PRISM_LOG',''), '--instance', os.environ.get('MACLESS_PRISM_INSTANCE',''), '--jar', str(ROOT / f'legacy/build/libs/macless-{VERSION}+1.16.1.jar')],
    'opengl': ['./gradlew', ':runClient', '-Pworld', '-Pfeatures', '-Pbackend=opengl', '--console=plain'],
    'vulkan': ['./gradlew', ':runClient', '-Pworld', '-Pfeatures', '-Pbackend=vulkan', '--console=plain'],
    'metal': ['./gradlew', ':runClient', '-Pworld', '-Pfeatures', '-Pbackend=opengl', '-Pmcopt='+os.environ.get('MACLESS_MCOPT',str(pathlib.Path.home()/'Downloads/mcopt-0.3.0-alpha.2.jar')), '--console=plain'],
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify(case, run):
    DIRECTORY.mkdir(parents=True, exist_ok=True)
    path = ROOT / 'verification' / f'prism-production-{VERSION}.log' if case == 'legacy' else DIRECTORY / f'world-{case}-final.log'
    if run:
        launch_log = DIRECTORY / 'prism-launch-final.log' if case == 'legacy' else path
        with launch_log.open('w') as output:
            result = subprocess.run(CASES[case], cwd=ROOT, stdout=output, stderr=subprocess.STDOUT, timeout=360)
        if result.returncode:
            raise RuntimeError(f'{case} exited with {result.returncode}; see {launch_log}')
    text = path.read_text()
    marker = 'TOOLSCREEN_LEGACY_PASS' if case == 'legacy' else 'TOOLSCREEN_WORLD_PASS backend=' + {'opengl': 'OpenGL', 'vulkan': 'Vulkan', 'metal': 'Metal'}[case]
    checks = ['TOOLSCREEN_UI_COLOR_PASS', 'TOOLSCREEN_NINJA_PASS', 'TOOLSCREEN_MIRRORS_PASS', 'TOOLSCREEN_INPUT_CONTEXT_PASS', 'TOOLSCREEN_F3_VISIBILITY_PASS visible=false', 'TOOLSCREEN_F3_VISIBILITY_PASS visible=true', 'TOOLSCREEN_CAMERA_SCALING_PASS modes=5 maximum=20 unified=true eyeOverride=godsens', 'TOOLSCREEN_MEASUREMENT_PAUSE_PASS', 'TOOLSCREEN_MEASUREMENT_RESUME_PASS', marker]
    if case != 'legacy':
        checks.append('BUILD SUCCESSFUL')
    missing = [check for check in checks if check not in text]
    if missing or re.search(r'TOOLSCREEN_\w*FAIL|Toolscreen renderer failed|NoSuchMethodError', text):
        raise RuntimeError(f'{case} failed checks: {missing}; see {path}')
    suffix = '1.16.1' if case == 'legacy' else case
    captures_directory = ROOT / f'verification/prism-client-{VERSION}/verification' if case == 'legacy' else ROOT / 'run/verification'
    captures = []
    for stage, dimensions in re.findall(r'TOOLSCREEN_(?:WORLD|LEGACY)_CAPTURE (\S+) source=(\d+x\d+)', text):
        image = captures_directory / f'world-{stage}-{suffix}.png'
        width, height = struct.unpack('>II', image.read_bytes()[16:24])
        if width < 640 or height < 360:
            raise RuntimeError(f'Invalid capture dimensions: {image}: {width}x{height}')
        captures.append({'stage': stage, 'source': dimensions, 'output': [width, height]})
    if len(captures) < 15:
        raise RuntimeError(f'Incomplete capture set for {case}: {len(captures)}')
    return {'passed': True, 'version': VERSION, 'log': str(path.relative_to(ROOT / 'verification')), 'checks': checks, 'captures': captures, 'capture_directory': str(captures_directory.relative_to(ROOT)), 'log_sha256': digest(path)}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('cases', nargs='*', choices=list(CASES))
    args = parser.parse_args()
    for case in args.cases:
        print(f'starting {case}', flush=True)
        verify(case, True)
        print(f'passed {case}', flush=True)
    results = {case: verify(case, False) for case in CASES}
    receipt = {'version': VERSION, 'completed_at': time.time(), 'native_sha256': digest(ROOT / 'build/native/libtoolscreen.dylib'), 'cases': results}
    (ROOT / 'verification/runtime-matrix.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print('runtime matrix passed', flush=True)
