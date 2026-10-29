import argparse
import json
import os
import pathlib
import shutil
import subprocess
import time


ROOT = pathlib.Path(__file__).resolve().parents[1]
FOLDER = ROOT / 'build/linux-vm'
SSH = ['ssh', '-p', '22284', '-i', str(FOLDER / 'id_ed25519'), '-o', 'IdentitiesOnly=yes', '-o', 'StrictHostKeyChecking=accept-new', '-o', 'UserKnownHostsFile=' + str(FOLDER / 'known_hosts'), '-o', 'ConnectTimeout=5', 'macless@127.0.0.1']


def start():
    FOLDER.mkdir(parents=True, exist_ok=True)
    key = FOLDER / 'id_ed25519'
    if not key.exists():
        subprocess.run(['ssh-keygen', '-q', '-t', 'ed25519', '-N', '', '-f', str(key)], check=True)
    seed = FOLDER / 'seed'
    seed.mkdir(exist_ok=True)
    public = key.with_suffix('.pub').read_text().strip()
    packages = ['python3', 'python3-gi', 'gir1.2-gstreamer-1.0', 'gir1.2-gst-plugins-base-1.0', 'gstreamer1.0-pipewire', 'gstreamer1.0-plugins-base', 'xdg-desktop-portal', 'xdg-desktop-portal-wlr', 'sway', 'pipewire', 'wireplumber', 'dbus-user-session', 'xvfb', 'x11-apps', 'libxcomposite1', 'libgl1-mesa-dri', 'libvulkan1', 'mesa-vulkan-drivers', 'openjdk-17-jdk-headless', 'libgoogle-gson-java', 'chromium']
    data = '#cloud-config\nusers:\n  - name: macless\n    groups: [sudo, video, render]\n    sudo: ALL=(ALL) NOPASSWD:ALL\n    shell: /bin/bash\n    ssh_authorized_keys:\n      - ' + public + '\nssh_pwauth: false\ndisable_root: true\npackage_update: true\npackages:\n' + ''.join('  - ' + name + '\n' for name in packages)
    (seed / 'user-data').write_text(data)
    (seed / 'meta-data').write_text('instance-id: macless-port-080\nlocal-hostname: macless-test\n')
    image = FOLDER / 'debian-12-genericcloud-arm64.qcow2'
    if not image.exists():
        raise RuntimeError('The verified Debian image has not finished downloading')
    disk = FOLDER / 'test.qcow2'
    if not disk.exists():
        subprocess.run(['qemu-img', 'create', '-f', 'qcow2', '-F', 'qcow2', '-b', str(image), str(disk), '20G'], check=True, stdout=subprocess.DEVNULL)
    iso = FOLDER / 'seed.iso'
    iso.unlink(missing_ok=True)
    subprocess.run(['hdiutil', 'makehybrid', '-o', str(iso), '-iso', '-joliet', '-default-volume-name', 'cidata', str(seed)], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    firmware = pathlib.Path('/opt/homebrew/share/qemu')
    variables = FOLDER / 'vars.fd'
    if not variables.exists():
        shutil.copy2(firmware / 'edk2-arm-vars.fd', variables)
    arguments = ['qemu-system-aarch64', '-machine', 'virt,accel=hvf', '-cpu', 'host', '-smp', '2', '-m', '4096', '-display', 'none', '-device', 'virtio-gpu-pci,addr=0x10', '-serial', 'file:' + str(FOLDER / 'console.log'), '-drive', 'if=pflash,format=raw,readonly=on,file=' + str(firmware / 'edk2-aarch64-code.fd'), '-drive', 'if=pflash,format=raw,file=' + str(variables), '-drive', 'if=virtio,format=qcow2,file=' + str(disk), '-drive', 'if=none,id=cidata,media=cdrom,format=raw,file=' + str(iso), '-device', 'virtio-scsi-pci', '-device', 'scsi-cd,drive=cidata', '-netdev', 'user,id=net,hostfwd=tcp:127.0.0.1:22284-:22', '-device', 'virtio-net-pci,netdev=net', '-fsdev', 'local,id=source,path=' + str(ROOT) + ',security_model=none,readonly=on', '-device', 'virtio-9p-pci,fsdev=source,mount_tag=macless']
    temporary = FOLDER / 'tmp'
    temporary.mkdir(exist_ok=True)
    environment = dict(os.environ)
    environment['TMPDIR'] = str(temporary)
    with (FOLDER / 'qemu.log').open('w') as output:
        process = subprocess.Popen(arguments, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT, start_new_session=True, env=environment)
    (FOLDER / 'pid').write_text(str(process.pid))
    return {'pid': process.pid, 'console': str(FOLDER / 'console.log'), 'port': 22284}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('action', choices=['start', 'ssh', 'stop'])
    parser.add_argument('command', nargs='*')
    args = parser.parse_args()
    if args.action == 'start':
        print(json.dumps(start()))
    elif args.action == 'ssh':
        raise SystemExit(subprocess.call(SSH + args.command))
    else:
        subprocess.call(SSH + ['sudo poweroff'])
