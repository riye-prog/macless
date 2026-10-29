import os
import struct
import sys
import uuid


def main():
    try:
        import gi
        gi.require_version('Gst', '1.0')
        gi.require_version('GstVideo', '1.0')
        from gi.repository import Gio, GLib, Gst, GstVideo
    except (ImportError, ValueError) as failure:
        raise RuntimeError('Wayland capture needs python3-gi, GStreamer, the PipeWire GStreamer plugin and xdg-desktop-portal') from failure
    Gst.init(None)
    connection = Gio.bus_get_sync(Gio.BusType.SESSION, None)
    bus = 'org.freedesktop.portal.Desktop'
    path = '/org/freedesktop/portal/desktop'
    interface = 'org.freedesktop.portal.ScreenCast'
    loop = GLib.MainLoop()
    state = {'session': None, 'pipeline': None, 'error': None, 'fd': None}
    sender = connection.get_unique_name().lstrip(':').replace('.', '_')

    def available(name, fallback):
        try:
            value = connection.call_sync(bus, path, 'org.freedesktop.DBus.Properties', 'Get', GLib.Variant('(ss)', (interface, name)), GLib.VariantType.new('(v)'), Gio.DBusCallFlags.NONE, 5000, None).unpack()[0]
            return int(value)
        except (GLib.Error, TypeError, ValueError):
            return fallback

    def failed(message):
        state['error'] = RuntimeError(message)
        loop.quit()

    def request(method, signature, arguments, callback):
        token = 'macless_' + uuid.uuid4().hex
        options = arguments[-1]
        options['handle_token'] = GLib.Variant('s', token)
        handle = f'/org/freedesktop/portal/desktop/request/{sender}/{token}'
        subscription = None

        def response(conn, name, object_path, iface, signal, parameters):
            connection.signal_unsubscribe(subscription)
            code, result = parameters.unpack()
            if code != 0:
                failed('Capture selection was cancelled' if code == 1 else f'The desktop portal rejected {method}')
                return
            try:
                callback(result)
            except Exception as failure:
                failed(str(failure))

        subscription = connection.signal_subscribe(bus, 'org.freedesktop.portal.Request', 'Response', handle, None, Gio.DBusSignalFlags.NONE, response)
        connection.call_sync(bus, path, interface, method, GLib.Variant(signature, arguments), GLib.VariantType.new('(o)'), Gio.DBusCallFlags.NONE, -1, None)

    def sample(sink):
        sample = sink.emit('pull-sample')
        if not sample:
            return Gst.FlowReturn.ERROR
        info = GstVideo.VideoInfo.new_from_caps(sample.get_caps())
        width, height, stride = info.width, info.height, info.stride[0]
        if width < 1 or height < 1 or width * height > 16777216:
            failed('Invalid capture dimensions')
            return Gst.FlowReturn.ERROR
        buffer = sample.get_buffer()
        data = buffer.extract_dup(0, buffer.get_size())
        pixels = data if stride == width * 4 else b''.join(data[row * stride:row * stride + width * 4] for row in range(height))
        try:
            sys.stdout.buffer.write(struct.pack('>IIII', 0x4d434631, width, height, len(pixels)))
            sys.stdout.buffer.write(pixels)
            sys.stdout.buffer.flush()
            return Gst.FlowReturn.OK
        except (BrokenPipeError, OSError):
            loop.quit()
            return Gst.FlowReturn.EOS

    def started(result):
        streams = result.get('streams', [])
        if not streams:
            raise RuntimeError('No capture source was selected')
        reply, descriptors = connection.call_with_unix_fd_list_sync(bus, path, interface, 'OpenPipeWireRemote', GLib.Variant('(oa{sv})', (state['session'], {})), GLib.VariantType.new('(h)'), Gio.DBusCallFlags.NONE, -1, None, None)
        state['fd'] = descriptors.get(reply.unpack()[0])
        pipeline = Gst.parse_launch(f'pipewiresrc fd={state["fd"]} path={streams[0][0]} do-timestamp=true ! videoconvert ! videoscale ! video/x-raw,format=RGBA,width=[1,1920],height=[1,1080],pixel-aspect-ratio=1/1 ! appsink name=frames emit-signals=true max-buffers=1 drop=true sync=false')
        state['pipeline'] = pipeline
        pipeline.get_by_name('frames').connect('new-sample', sample)
        gst_bus = pipeline.get_bus()
        gst_bus.add_signal_watch()
        gst_bus.connect('message::error', lambda gst_bus, message: failed(str(message.parse_error()[0])))
        gst_bus.connect('message::eos', lambda gst_bus, message: loop.quit())
        pipeline.set_state(Gst.State.PLAYING)

    def selected(result):
        request('Start', '(osa{sv})', (state['session'], '', {}), started)

    def created(result):
        state['session'] = result['session_handle']
        types = available('AvailableSourceTypes', 3) & 3
        cursor_modes = available('AvailableCursorModes', 1)
        cursor = next((mode for mode in (1, 2, 4) if cursor_modes & mode), 1)
        if not types:
            raise RuntimeError('This desktop has no screen capture source')
        request('SelectSources', '(oa{sv})', (state['session'], {'types': GLib.Variant('u', types), 'multiple': GLib.Variant('b', False), 'cursor_mode': GLib.Variant('u', cursor)}), selected)

    try:
        request('CreateSession', '(a{sv})', ({'session_handle_token': GLib.Variant('s', 'macless_' + uuid.uuid4().hex)},), created)
        loop.run()
    finally:
        if state['pipeline']:
            state['pipeline'].set_state(Gst.State.NULL)
        if state['session']:
            try:
                connection.call_sync(bus, state['session'], 'org.freedesktop.portal.Session', 'Close', None, None, Gio.DBusCallFlags.NONE, 2000, None)
            except GLib.Error:
                pass
        if state['fd'] is not None:
            os.close(state['fd'])
    if state['error']:
        raise state['error']


try:
    main()
except (Exception, KeyboardInterrupt) as failure:
    print(str(failure), file=sys.stderr)
    sys.exit(1)
