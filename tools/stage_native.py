import hashlib
import json
import pathlib
import platform
import shutil


ROOT = pathlib.Path(__file__).resolve().parents[1]
system = {'Darwin': 'macos', 'Windows': 'windows', 'Linux': 'linux'}[platform.system()]
architecture = 'arm64' if platform.machine().lower() in ['arm64', 'aarch64'] else 'x64'
target = system + '-' + architecture
destination = ROOT / 'build/native-artifacts' / target
destination.mkdir(parents=True, exist_ok=True)
library = 'toolscreen.dll' if system == 'windows' else 'libtoolscreen.dylib' if system == 'macos' else 'libtoolscreen.so'
names = [library] + (['macless-capture.exe'] if system == 'windows' else [])
files = {}
for name in names:
    shutil.copy2(ROOT / 'build/native' / name, destination / name)
    files[name] = hashlib.sha256((destination / name).read_bytes()).hexdigest()
(destination / 'manifest.json').write_text(json.dumps({'target': target, 'files': files}, indent=2) + '\n')
print(json.dumps({'target': target, 'files': files}))
