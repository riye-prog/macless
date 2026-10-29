import os
import pathlib
import platform
import shlex
import shutil
import subprocess


ROOT = pathlib.Path(__file__).resolve().parents[1]
if platform.system() != 'Linux':
    raise SystemExit('Use this configuration on a Linux build host')
zig = shutil.which('zig')
if not zig:
    raise SystemExit('Install Zig 0.15.2 before configuring the Linux build')
architecture = 'aarch64' if platform.machine() in ['aarch64', 'arm64'] else 'x86_64'
triple = architecture + '-linux-gnu.2.28'
folder = ROOT / 'build/linux-toolchain'
folder.mkdir(parents=True, exist_ok=True)
for name, command in [('cc', ['cc', '-target', triple]), ('cxx', ['c++', '-target', triple]), ('ar', ['ar']), ('ranlib', ['ranlib'])]:
    path = folder / name
    path.write_text('#!/bin/sh\nexec ' + shlex.join([zig] + command) + ' "$@"\n')
    path.chmod(0o755)
toolchain = folder / 'toolchain.cmake'
toolchain.write_text('set(CMAKE_SYSTEM_NAME Linux)\nset(CMAKE_SYSTEM_PROCESSOR ' + architecture + ')\n' + ''.join('set(CMAKE_' + key + ' "' + str(folder / value) + '")\n' for key, value in [('C_COMPILER', 'cc'), ('CXX_COMPILER', 'cxx'), ('AR', 'ar'), ('RANLIB', 'ranlib')]))
raise SystemExit(subprocess.call(['cmake', '-S', str(ROOT), '-B', str(ROOT / 'build/native'), '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_TOOLCHAIN_FILE=' + str(toolchain), '-DJAVA_HOME=' + os.environ['JAVA_HOME']]))
