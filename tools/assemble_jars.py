import hashlib
import json
import os
import pathlib
import shutil
import struct
import subprocess
import zipfile


ROOT = pathlib.Path(__file__).resolve().parents[1]
VERSION = json.loads((ROOT / 'src/main/resources/fabric.mod.json').read_text())['version'].split('+')[0]
TARGETS = {'macos-arm64': 'libtoolscreen.dylib', 'macos-x64': 'libtoolscreen.dylib', 'windows-x64': 'toolscreen.dll', 'linux-x64': 'libtoolscreen.so', 'linux-arm64': 'libtoolscreen.so'}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def assemble():
    destination = ROOT / 'build/natives/macos-arm64'
    destination.mkdir(parents=True, exist_ok=True)
    shutil.copy2(ROOT / 'build/native/libtoolscreen.dylib', destination / 'libtoolscreen.dylib')
    natives = {}
    for target, name in TARGETS.items():
        path = ROOT / 'build/natives' / target / name
        data = path.read_bytes()
        if target.startswith('macos'):
            assert struct.unpack_from('<II', data) == (0xfeedfacf, 0x100000c if target.endswith('arm64') else 0x1000007)
        elif target.startswith('linux'):
            assert data[:5] == b'\x7fELF\x02' and struct.unpack_from('<H', data, 18)[0] == (183 if target.endswith('arm64') else 62)
        else:
            offset = struct.unpack_from('<I', data, 60)[0]
            assert data[:2] == b'MZ' and data[offset:offset + 4] == b'PE\0\0' and struct.unpack_from('<H', data, offset + 4)[0] == 0x8664
        natives[target] = {'library': name, 'sha256': digest(path)}
    helper = ROOT / 'build/natives/windows-x64/macless-capture.exe'
    assert helper.read_bytes()[:2] == b'MZ'
    natives['windows-x64']['capture_sha256'] = digest(helper)
    log = ROOT / 'verification' / ('macless-' + VERSION) / 'assemble-universal.log'
    with log.open('w') as output:
        result = subprocess.run(['./gradlew', 'jar', ':legacy:remapJar', '-PprebuiltNatives=' + str(ROOT / 'build/natives'), '--console=plain'], cwd=ROOT, stdout=output, stderr=subprocess.STDOUT)
    if result.returncode:
        raise RuntimeError('Universal JAR assembly failed: ' + str(log))
    jars = {}
    for version, folder, major in [('26.3', 'build/libs', 69), ('1.16.1', 'legacy/build/libs', 61)]:
        jar = ROOT / folder / f'macless-{VERSION}+{version}.jar'
        with zipfile.ZipFile(jar) as archive:
            assert archive.testzip() is None
            metadata = json.loads(archive.read('fabric.mod.json'))
            assert json.loads(archive.read('macless-build.json')) == {'verification': False, 'approvalProfile': 'v8-review-1'}
            forbidden=['SmokeHarness','WorldHarness','ControlsProbe','RuntimeVerification','VerificationFixtures','LegacyHarness','LegacyVisualProbe','LegacyVisualBenchmark','LegacyControlsProbe','ModernVisualProbe','RenderColorProbe']
            assert not any(entry.startswith('dev/riye/toolscreen/'+helper) and entry.endswith('.class') for entry in archive.namelist() for helper in forbidden)
            assert not any('fabric-api' in entry or 'fabric-resource-loader' in entry for entry in archive.namelist())
            assert not metadata.get('jars')
            assert metadata['id'] == 'toolscreen-native' and metadata['version'] == VERSION + '+' + version
            for target, details in natives.items():
                assert hashlib.sha256(archive.read('natives/' + target + '/' + details['library'])).hexdigest() == details['sha256']
            assert hashlib.sha256(archive.read('natives/windows-x64/macless-capture.exe')).hexdigest() == digest(helper)
            assert struct.unpack_from('>H', archive.read('dev/riye/toolscreen/DesktopPlatform.class'), 6)[0] == major
            for name in ['imgui', 'toolscreen', 'freetype', 'libpng', 'inter', 'jetbrains-mono', 'monocraft', 'nlohmann-json']:
                assert archive.read('licenses/' + name + '.txt')
            for name in ['capture_portal.py', 'capture_x11.py']:
                assert archive.read('macless/' + name)
        jars[jar.name] = {'path': str(jar), 'sha256': digest(jar)}
    receipt = {'version': VERSION, 'natives': natives, 'jars': jars}
    (ROOT / 'verification' / ('macless-' + VERSION) / 'artifacts.json').write_text(json.dumps(receipt, indent=2) + '\n')
    return receipt


if __name__ == '__main__':
    print(json.dumps(assemble()))
