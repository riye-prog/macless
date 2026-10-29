import argparse
import ctypes as c
import ctypes.util
import json
import os
import pathlib
import select
import struct
import subprocess
import time


def exact(stream, length, timeout=15):
    result = bytearray()
    deadline = time.monotonic() + timeout
    while len(result) < length:
        remaining = deadline - time.monotonic()
        if remaining <= 0 or not select.select([stream], [], [], remaining)[0]:
            raise RuntimeError('Capture frame timed out')
        data = os.read(stream.fileno(), min(1048576, length - len(result)))
        if not data:
            raise RuntimeError('Capture ended before a frame')
        result.extend(data)
    return result


def frame(process):
    magic, width, height, length = struct.unpack('>IIII', exact(process.stdout, 16))
    if magic != 0x4d434631 or width < 1 or height < 1 or length != width * height * 4:
        raise RuntimeError('Invalid capture packet')
    return width, height, exact(process.stdout, length)


def x11(root):
    environment = dict(os.environ)
    environment['DISPLAY'] = ':83'
    server = subprocess.Popen(['Xvfb', ':83', '-screen', '0', '800x600x24', '-ac'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    capture = None
    display = None
    x = c.CDLL(ctypes.util.find_library('X11'))
    x.XOpenDisplay.restype = c.c_void_p
    x.XOpenDisplay.argtypes = [c.c_char_p]
    x.XDefaultRootWindow.restype = c.c_ulong
    x.XDefaultRootWindow.argtypes = [c.c_void_p]
    x.XCreateSimpleWindow.restype = c.c_ulong
    x.XCreateSimpleWindow.argtypes = [c.c_void_p, c.c_ulong, c.c_int, c.c_int, c.c_uint, c.c_uint, c.c_uint, c.c_ulong, c.c_ulong]
    x.XStoreName.argtypes = [c.c_void_p, c.c_ulong, c.c_char_p]
    x.XMapWindow.argtypes = [c.c_void_p, c.c_ulong]
    x.XFlush.argtypes = [c.c_void_p]
    x.XCloseDisplay.argtypes = [c.c_void_p]
    try:
        for _ in range(50):
            display = x.XOpenDisplay(b':83')
            if display:
                break
            time.sleep(.1)
        if not display:
            raise RuntimeError('Xvfb did not start')
        window = x.XCreateSimpleWindow(display, x.XDefaultRootWindow(display), 40, 40, 640, 360, 0, 0, 0x123456)
        x.XStoreName(display, window, b'Macless capture fixture')
        x.XMapWindow(display, window)
        x.XFlush(display)
        time.sleep(.3)
        helper = root / 'shared/resources/macless/capture_x11.py'
        listing = subprocess.run(['python3', str(helper), '--list'], env=environment, capture_output=True, text=True, timeout=10)
        if listing.returncode or f'{window}\t' not in listing.stdout:
            raise RuntimeError('X11 window enumeration failed: ' + listing.stderr)
        capture = subprocess.Popen(['python3', str(helper), '--capture', str(window)], env=environment, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        width, height, data = frame(capture)
        at = ((height // 2) * width + width // 2) * 4
        if data[at:at + 4] != bytes([18, 52, 86, 255]):
            raise RuntimeError('X11 colors differ: ' + repr(data[at:at + 4]))
        frame(capture)
        return {'mode': 'x11', 'passed': True, 'width': width, 'height': height, 'checks': ['enumeration', 'RGBA colors', 'fresh frames']}
    finally:
        if capture:
            capture.terminate()
            capture.wait(timeout=5)
        if display:
            x.XCloseDisplay(display)
        server.terminate()
        server.wait(timeout=5)


def wayland(root):
    helper = root / 'shared/resources/macless/capture_portal.py'
    process = subprocess.Popen(['python3', str(helper), '--capture', 'portal'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    try:
        width, height, data = frame(process)
        at = ((height // 2) * width + width // 2) * 4
        if data[at:at + 3] != bytes([18, 52, 86]):
            raise RuntimeError('Wayland colors differ: ' + repr(data[at:at + 4]))
        frame(process)
        return {'mode': 'wayland', 'passed': True, 'width': width, 'height': height, 'checks': ['portal session', 'PipeWire FD', 'GStreamer RGBA colors', 'fresh frames']}
    except Exception as failure:
        process.terminate()
        process.wait(timeout=5)
        raise RuntimeError(str(failure) + ': ' + process.stderr.read().decode(errors='replace')[:1500]) from failure
    finally:
        if process.poll() is None:
            process.terminate()
            process.wait(timeout=5)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('mode', choices=['x11', 'wayland'])
    parser.add_argument('--root', type=pathlib.Path, default=pathlib.Path(__file__).resolve().parents[1])
    arguments = parser.parse_args()
    print(json.dumps((x11 if arguments.mode == 'x11' else wayland)(arguments.root)))
