import ctypes as c
import ctypes.util
import math
import os
import struct
import sys
import time


class Image(c.Structure):
    _fields_ = [('width', c.c_int), ('height', c.c_int), ('xoffset', c.c_int), ('format', c.c_int), ('data', c.c_void_p), ('byte_order', c.c_int), ('bitmap_unit', c.c_int), ('bitmap_bit_order', c.c_int), ('bitmap_pad', c.c_int), ('depth', c.c_int), ('bytes_per_line', c.c_int), ('bits_per_pixel', c.c_int), ('red_mask', c.c_ulong), ('green_mask', c.c_ulong), ('blue_mask', c.c_ulong), ('obdata', c.c_void_p), ('functions', c.c_void_p * 6)]


def library(name):
    path = ctypes.util.find_library(name)
    if not path:
        raise RuntimeError('Install the X11 and XComposite runtime libraries')
    return c.CDLL(path)


def signature(lib, name, result, arguments):
    function = getattr(lib, name)
    function.restype = result
    function.argtypes = arguments
    return function


def main():
    x = library('X11')
    pointer = c.c_void_p
    window = c.c_ulong
    signature(x, 'XOpenDisplay', pointer, [c.c_char_p])
    signature(x, 'XDefaultRootWindow', window, [pointer])
    signature(x, 'XInternAtom', window, [pointer, c.c_char_p, c.c_int])
    signature(x, 'XGetWindowProperty', c.c_int, [pointer, window, window, c.c_long, c.c_long, c.c_int, window, c.POINTER(window), c.POINTER(c.c_int), c.POINTER(window), c.POINTER(window), c.POINTER(pointer)])
    signature(x, 'XQueryTree', c.c_int, [pointer, window, c.POINTER(window), c.POINTER(window), c.POINTER(pointer), c.POINTER(c.c_uint)])
    signature(x, 'XGetGeometry', c.c_int, [pointer, window, c.POINTER(window), c.POINTER(c.c_int), c.POINTER(c.c_int), c.POINTER(c.c_uint), c.POINTER(c.c_uint), c.POINTER(c.c_uint), c.POINTER(c.c_uint)])
    signature(x, 'XGetImage', c.POINTER(Image), [pointer, window, c.c_int, c.c_int, c.c_uint, c.c_uint, c.c_ulong, c.c_int])
    signature(x, 'XDestroyImage', c.c_int, [c.POINTER(Image)])
    signature(x, 'XFree', c.c_int, [pointer])
    signature(x, 'XCloseDisplay', c.c_int, [pointer])
    signature(x, 'XFreePixmap', c.c_int, [pointer, window])
    handler_type = c.CFUNCTYPE(c.c_int, pointer, pointer)
    handler = handler_type(lambda display, event: 0)
    x.XSetErrorHandler.argtypes = [handler_type]
    x.XSetErrorHandler(handler)
    display = x.XOpenDisplay(None)
    if not display:
        raise RuntimeError('No X11 display is available')

    def property_value(target, name):
        atom = x.XInternAtom(display, name.encode(), 1)
        actual, count, remaining, data = window(), window(), window(), pointer()
        format_bits = c.c_int()
        if not atom or x.XGetWindowProperty(display, target, atom, 0, 4096, 0, 0, c.byref(actual), c.byref(format_bits), c.byref(count), c.byref(remaining), c.byref(data)) or not data:
            return None
        try:
            if format_bits.value == 32:
                return list(c.cast(data, c.POINTER(window * count.value)).contents)
            return c.string_at(data, count.value).decode('utf-8', 'replace').replace('\t', ' ').replace('\n', ' ')
        finally:
            x.XFree(data)

    def geometry(target):
        root, left, top, width, height, border, depth = window(), c.c_int(), c.c_int(), c.c_uint(), c.c_uint(), c.c_uint(), c.c_uint()
        if not x.XGetGeometry(display, target, c.byref(root), c.byref(left), c.byref(top), c.byref(width), c.byref(height), c.byref(border), c.byref(depth)):
            raise RuntimeError('The selected window is unavailable')
        return width.value, height.value

    try:
        if sys.argv[1:] == ['--list']:
            root = x.XDefaultRootWindow(display)
            windows = property_value(root, '_NET_CLIENT_LIST')
            if not windows:
                parent, returned, children, count = window(), window(), pointer(), c.c_uint()
                x.XQueryTree(display, root, c.byref(returned), c.byref(parent), c.byref(children), c.byref(count))
                windows = list(c.cast(children, c.POINTER(window * count.value)).contents) if children else []
                if children:
                    x.XFree(children)
            for target in windows[:512]:
                title = property_value(target, '_NET_WM_NAME') or property_value(target, 'WM_NAME')
                if not isinstance(title, str) or not title:
                    continue
                width, height = geometry(target)
                if width < 20 or height < 20:
                    continue
                pid = (property_value(target, '_NET_WM_PID') or [0])[0]
                try:
                    with open(f'/proc/{pid}/comm') as file:
                        application = file.read().strip().replace('\t', ' ')
                except OSError:
                    application = 'Window'
                print(f'{target}\t{pid}\t{application}\t{title}')
            return
        if len(sys.argv) != 3 or sys.argv[1] != '--capture':
            raise RuntimeError('Expected --list or --capture window')
        target = int(sys.argv[2])
        composite = library('Xcomposite')
        signature(composite, 'XCompositeNameWindowPixmap', window, [pointer, window])
        while True:
            started = time.monotonic()
            width, height = geometry(target)
            if width < 1 or height < 1 or width * height > 16777216:
                raise RuntimeError('Invalid window size')
            pixmap = composite.XCompositeNameWindowPixmap(display, target)
            source = pixmap or target
            image = x.XGetImage(display, source, 0, 0, width, height, c.c_ulong(-1), 2)
            if not image and pixmap:
                image = x.XGetImage(display, target, 0, 0, width, height, c.c_ulong(-1), 2)
            if not image:
                if pixmap:
                    x.XFreePixmap(display, pixmap)
                time.sleep(.1)
                continue
            try:
                value = image.contents
                if value.bits_per_pixel != 32 or value.red_mask != 0xff0000 or value.blue_mask != 0xff:
                    raise RuntimeError('Window capture requires a 24 or 32-bit TrueColor X11 display')
                data = c.string_at(value.data, value.bytes_per_line * height)
                step = max(1, math.ceil(max(width / 1920, height / 1080)))
                output_width, output_height = math.ceil(width / step), math.ceil(height / step)
                rgba = bytearray(output_width * output_height * 4)
                for row in range(output_height):
                    pixels = data[row * step * value.bytes_per_line:row * step * value.bytes_per_line + width * 4]
                    start = row * output_width * 4
                    rgba[start:start + output_width * 4:4] = pixels[2 if value.byte_order == 0 else 1::4 * step]
                    rgba[start + 1:start + output_width * 4:4] = pixels[1 if value.byte_order == 0 else 2::4 * step]
                    rgba[start + 2:start + output_width * 4:4] = pixels[0 if value.byte_order == 0 else 3::4 * step]
                    rgba[start + 3:start + output_width * 4:4] = b'\xff' * output_width
                sys.stdout.buffer.write(struct.pack('>IIII', 0x4d434631, output_width, output_height, len(rgba)))
                sys.stdout.buffer.write(rgba)
                sys.stdout.buffer.flush()
            finally:
                x.XDestroyImage(image)
                if pixmap:
                    x.XFreePixmap(display, pixmap)
            time.sleep(max(0, 1 / 30 - (time.monotonic() - started)))
    finally:
        x.XCloseDisplay(display)


try:
    main()
except (Exception, KeyboardInterrupt) as failure:
    print(str(failure), file=sys.stderr)
    sys.exit(1)
