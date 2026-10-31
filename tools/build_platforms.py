import argparse
import hashlib
import json
import os
import pathlib
import shlex
import shutil
import subprocess
import tarfile
import urllib.request


ROOT = pathlib.Path(__file__).resolve().parents[1]
VERSION = json.loads((ROOT / 'src/main/resources/fabric.mod.json').read_text())['version'].split('+')[0]
DEPS = ROOT / 'build/portable-deps'
(DEPS / 'tmp').mkdir(parents=True, exist_ok=True)
os.environ['TMPDIR'] = str(DEPS / 'tmp')
os.environ['ZIG_GLOBAL_CACHE_DIR'] = str(DEPS / 'zig-cache')
JNI_HASHES = {'jni.h': '1266aea5b9f5d5db1cb6f8e5c6c43cfa7f80bc4f72d7fe42c6131bb939dc70f4', 'linux/jni_md.h': '88cb5c33e306900dd35a78d5a439087123b8e91b0986bb5acb42cc9bd2fcc42e', 'windows/jni_md.h': 'dbf96659c4c840b15ef40237db0c65657eca7a70904225fc984deb38999df515'}


def download(url, destination, expected):
    if destination.exists() and hashlib.sha256(destination.read_bytes()).hexdigest() == expected:
        return
    destination.parent.mkdir(parents=True, exist_ok=True)
    with urllib.request.urlopen(url, timeout=60) as response:
        data = response.read()
    if hashlib.sha256(data).hexdigest() != expected:
        raise RuntimeError('Checksum mismatch: ' + url)
    destination.write_bytes(data)


def command(arguments, log):
    with log.open('a') as output:
        result = subprocess.run([str(value) for value in arguments], cwd=ROOT, stdout=output, stderr=subprocess.STDOUT)
    if result.returncode:
        raise RuntimeError('Build failed: ' + str(log))


def wrapper(name, arguments):
    path = DEPS / 'bin' / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text('#!/bin/sh\nexec ' + shlex.join([str(value) for value in arguments]) + ' "$@"\n')
    path.chmod(0o755)
    return path


def build(target):
    directory = ROOT / 'build' / ('native-' + target)
    directory.mkdir(parents=True, exist_ok=True)
    log = ROOT / 'verification' / ('macless-' + VERSION) / ('build-' + target + '.log')
    log.parent.mkdir(parents=True, exist_ok=True)
    log.write_text('')
    options = ['-DMACLESS_BUNDLED_FREETYPE=ON', '-DCMAKE_BUILD_TYPE=Release', '-DFETCHCONTENT_SOURCE_DIR_IMGUI=' + str(ROOT / 'build/native/_deps/imgui-src')]
    if target == 'macos-arm64':
        directory = ROOT / 'build/native'
    else:
        source = DEPS / 'freetype-VER-2-14-1'
        archive = DEPS / 'freetype.tar.gz'
        download('https://codeload.github.com/freetype/freetype/tar.gz/refs/tags/VER-2-14-1', archive, '44bd69d1f0750603410cfd4f26f1c5523c5a3a087a2dfa8639b757dc7c6677be')
        if not source.exists():
            with tarfile.open(archive) as data:
                data.extractall(DEPS, filter='data')
        options += ['-DFETCHCONTENT_SOURCE_DIR_FREETYPE=' + str(source)]
    if target == 'macos-x64':
        options += ['-DCMAKE_OSX_ARCHITECTURES=x86_64']
    elif target.startswith(('windows-', 'linux-')):
        system = target.split('-')[0]
        for name, expected in JNI_HASHES.items():
            part = 'share' if name == 'jni.h' else 'unix' if name.startswith('linux') else 'windows'
            url = 'https://raw.githubusercontent.com/openjdk/jdk17u/jdk-17.0.16-ga/src/java.base/' + part + '/native/include/' + pathlib.Path(name).name
            download(url, DEPS / 'jni' / name, expected)
        options += ['-DMACLESS_JNI_INCLUDE_DIRS=' + str(DEPS / 'jni') + ';' + str(DEPS / 'jni' / system)]
        if system == 'windows':
            c = shutil.which('x86_64-w64-mingw32-gcc')
            cxx = shutil.which('x86_64-w64-mingw32-g++')
            if not c or not cxx:
                raise RuntimeError('Install the MinGW-w64 x86_64 toolchain')
            toolchain = f'set(CMAKE_SYSTEM_NAME Windows)\nset(CMAKE_C_COMPILER "{c}")\nset(CMAKE_CXX_COMPILER "{cxx}")\n'
        else:
            zig = next(DEPS.glob('zig-aarch64-macos-0.15.2/zig'), None)
            if not zig:
                url = 'https://ziglang.org/download/0.15.2/zig-aarch64-macos-0.15.2.tar.xz'
                archive = DEPS / 'zig.tar.xz'
                download(url, archive, '3cc2bab367e185cdfb27501c4b30b1b0653c28d9f73df8dc91488e66ece5fa6b')
                with tarfile.open(archive) as data:
                    data.extractall(DEPS, filter='data')
                zig = DEPS / 'zig-aarch64-macos-0.15.2/zig'
            triple = 'aarch64-linux-gnu.2.28' if target.endswith('arm64') else 'x86_64-linux-gnu.2.28'
            c = wrapper(target + '-cc', [zig, 'cc', '-target', triple])
            cxx = wrapper(target + '-cxx', [zig, 'c++', '-target', triple])
            ar = wrapper('zig-ar', [zig, 'ar'])
            ranlib = wrapper('zig-ranlib', [zig, 'ranlib'])
            toolchain = f'set(CMAKE_SYSTEM_NAME Linux)\nset(CMAKE_SYSTEM_PROCESSOR {"aarch64" if target.endswith("arm64") else "x86_64"})\nset(CMAKE_C_COMPILER "{c}")\nset(CMAKE_CXX_COMPILER "{cxx}")\nset(CMAKE_AR "{ar}")\nset(CMAKE_RANLIB "{ranlib}")\n'
        toolchain += 'set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)\n'
        path = directory / 'toolchain.cmake'
        path.write_text(toolchain)
        options += ['-DCMAKE_TOOLCHAIN_FILE=' + str(path)]
    command(['cmake', '-S', ROOT, '-B', directory, *options], log)
    command(['cmake', '--build', directory, '--config', 'Release', '--parallel', '2'], log)
    destination = ROOT / 'build/natives' / target
    destination.mkdir(parents=True, exist_ok=True)
    library = 'toolscreen.dll' if target.startswith('windows') else 'libtoolscreen.dylib' if target.startswith('macos') else 'libtoolscreen.so'
    shutil.copy2(directory / library, destination / library)
    if target.startswith('windows'):
        shutil.copy2(directory / 'macless-capture.exe', destination / 'macless-capture.exe')
    return {'target': target, 'library': str(destination / library), 'sha256': hashlib.sha256((destination / library).read_bytes()).hexdigest(), 'log': str(log)}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('targets', nargs='+', choices=['macos-arm64', 'macos-x64', 'windows-x64', 'linux-x64', 'linux-arm64'])
    args = parser.parse_args()
    for target in args.targets:
        print('building ' + target, flush=True)
        print(json.dumps(build(target)), flush=True)
