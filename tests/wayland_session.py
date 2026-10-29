import json
import os
import pathlib
import shutil
import subprocess
import sys
import tempfile
import time


if '--session' not in sys.argv:
    raise SystemExit(subprocess.call(['dbus-run-session', '--', sys.executable, __file__, '--session']))

root = pathlib.Path(__file__).resolve().parents[1]
processes = []
logs = []
destination = root / 'verification/wayland-session'
destination.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='macless-wayland-') as folder:
    runtime = pathlib.Path(folder)
    environment = dict(os.environ)
    environment.update({'XDG_RUNTIME_DIR': str(runtime), 'XDG_CONFIG_HOME': str(runtime / 'config'), 'XDG_CURRENT_DESKTOP': 'sway', 'WLR_BACKENDS': 'headless', 'WLR_RENDERER': os.environ.get('MACLESS_WAYLAND_RENDERER', 'pixman'), 'WLR_RENDERER_ALLOW_SOFTWARE': '1', 'WLR_LIBINPUT_NO_DEVICES': '1', 'WLR_HEADLESS_OUTPUTS': '1'})
    config = runtime / 'sway.conf'
    config.write_text('output * mode 800x600\noutput * bg #123456 solid_color\nxwayland disable\n')
    portal = runtime / 'config/xdg-desktop-portal-wlr'
    portal.mkdir(parents=True)
    (portal / 'config').write_text('[screencast]\noutput_name=HEADLESS-1\nchooser_type=none\nmax_fps=30\n')

    def start(name, arguments):
        log = (destination / (name + '.log')).open('w')
        logs.append(log)
        process = subprocess.Popen(arguments, env=environment, stdout=log, stderr=subprocess.STDOUT)
        processes.append(process)
        return process

    try:
        compositor = start('sway', ['sway', '-d', '-c', str(config)])
        display = None
        for _ in range(100):
            display = next((path for path in runtime.glob('wayland-*') if not path.name.endswith('.lock')), None)
            if display or compositor.poll() is not None:
                break
            time.sleep(.1)
        if not display:
            raise RuntimeError('Headless Wayland compositor did not start')
        environment['WAYLAND_DISPLAY'] = display.name
        start('pipewire', ['pipewire'])
        time.sleep(.5)
        start('wireplumber', ['wireplumber'])
        start('portal-wlr', ['/usr/libexec/xdg-desktop-portal-wlr', '-l', 'DEBUG'])
        time.sleep(.5)
        start('portal', ['/usr/libexec/xdg-desktop-portal'])
        time.sleep(1)
        result = subprocess.run([sys.executable, str(root / 'tests/linux_system_contract.py'), 'wayland'], env=environment, capture_output=True, text=True, timeout=30)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        print(result.stdout.strip())
    except Exception as failure:
        print(json.dumps({'passed': False, 'error': str(failure), 'logs': str(destination)}))
        raise SystemExit(1)
    finally:
        for process in reversed(processes):
            if process.poll() is None:
                process.terminate()
        for process in reversed(processes):
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
        for log in logs:
            log.close()
