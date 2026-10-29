import argparse
import json
import pathlib
import shutil
import time


ROOT = pathlib.Path(__file__).resolve().parents[1]
VERSION = json.loads((ROOT / 'legacy/src/main/resources/fabric.mod.json').read_text())['version'].split('+')[0]
FOLDER = ROOT / 'verification' / ('prism-visual-' + VERSION) / 'verification'


def send(command):
    value = str(time.time_ns()) + '|' + command
    (FOLDER / 'visual-input').write_text(value)
    deadline = time.monotonic() + 10
    while time.monotonic() < deadline:
        if (FOLDER / 'visual-ack').exists() and (FOLDER / 'visual-ack').read_text() == value:
            return
        time.sleep(.05)
    raise RuntimeError('Visual client did not acknowledge: ' + command)


def capture(destination):
    image = FOLDER / 'visual-window.png'
    previous = image.stat().st_mtime_ns if image.exists() else 0
    time.sleep(.25)
    send('capture')
    deadline = time.monotonic() + 12
    while time.monotonic() < deadline:
        if image.exists() and image.stat().st_mtime_ns != previous:
            data=image.read_bytes()
            if data.endswith(bytes.fromhex('0000000049454e44ae426082')):
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes(data)
                return
        time.sleep(.05)
    raise RuntimeError('Visual client did not produce a snapshot')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--page', type=int)
    parser.add_argument('--command', action='append', default=[])
    parser.add_argument('--capture', type=pathlib.Path)
    arguments = parser.parse_args()
    if arguments.page is not None:
        (FOLDER / 'visual-page').write_text(str(arguments.page))
        time.sleep(.5)
    for command in arguments.command:
        send(command)
    if arguments.capture:
        capture(arguments.capture)
        print(arguments.capture)
    else:
        print('Visual input delivered')
