import ctypes
import os
import math
import pathlib
import random
import shlex
import struct
import tempfile
import json
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
LIBRARY = ctypes.CDLL(os.environ.get("TOOLSCREEN_TEST_LIBRARY",str(ROOT / "build/native/libtoolscreen.dylib")))
LIBRARY.ts_ui_display_scale.argtypes=[ctypes.c_float]
LIBRARY.ts_sensitivity.argtypes=[ctypes.POINTER(ctypes.c_double)]
LIBRARY.ts_set.argtypes = [ctypes.c_int, ctypes.c_double]
LIBRARY.ts_get.argtypes = [ctypes.c_int]
LIBRARY.ts_get.restype = ctypes.c_double
LIBRARY.ts_load.argtypes = [ctypes.c_char_p]
LIBRARY.ts_viewport.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.POINTER(ctypes.c_int)]
LIBRARY.ts_plan.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_double, ctypes.POINTER(ctypes.c_float), ctypes.c_int]
LIBRARY.ts_ui_frame.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_float, ctypes.POINTER(ctypes.c_uint8), ctypes.c_int]
LIBRARY.ts_ui_mouse.argtypes = [ctypes.c_float, ctypes.c_float]
LIBRARY.ts_ui_button.argtypes = [ctypes.c_int, ctypes.c_int]
LIBRARY.ts_ui_key.argtypes = [ctypes.c_int,ctypes.c_int,ctypes.c_int]
LIBRARY.ts_ui_rect.argtypes = [ctypes.c_int, ctypes.POINTER(ctypes.c_float)]
LIBRARY.ts_add_mirror.argtypes=[ctypes.c_char_p,ctypes.c_int]
LIBRARY.ts_edit_mirror.argtypes=[ctypes.c_int,ctypes.c_int,ctypes.c_double]
LIBRARY.ts_key_event.argtypes=[ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_int]
LIBRARY.ts_add_image.argtypes=[ctypes.c_char_p]
LIBRARY.ts_add_background.argtypes=[ctypes.c_char_p]
LIBRARY.ts_image_size.argtypes=[ctypes.c_int,ctypes.c_int,ctypes.c_int]
LIBRARY.ts_edit_image.argtypes=[ctypes.c_int,ctypes.c_int,ctypes.c_double]
LIBRARY.ts_bind_remap.argtypes=[ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_int]
LIBRARY.ts_remap_key.argtypes=[ctypes.c_int,ctypes.c_int,ctypes.POINTER(ctypes.c_int)]

LIBRARY.ts_ui_physical_key.argtypes=[ctypes.c_int]*4
LIBRARY.ts_input_event.argtypes=[ctypes.c_int]*5
LIBRARY.ts_bind_keys.argtypes=[ctypes.c_int,ctypes.POINTER(ctypes.c_int),ctypes.c_int]
LIBRARY.ts_plan_output.argtypes=[ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_double,ctypes.c_int,ctypes.POINTER(ctypes.c_float),ctypes.c_int]
LIBRARY.ts_overlay_frame.argtypes=[ctypes.c_int]*5+[ctypes.POINTER(ctypes.c_uint8),ctypes.c_int]
LIBRARY.ts_feature_probe.argtypes=[ctypes.POINTER(ctypes.c_double),ctypes.c_int]
LIBRARY.ts_debug_counter.argtypes=[ctypes.c_char_p]
LIBRARY.ts_debug_pie.argtypes=[ctypes.c_char_p,ctypes.c_char_p,ctypes.POINTER(ctypes.c_double),ctypes.c_int]
LIBRARY.ts_ninja_update.argtypes=[ctypes.c_char_p,ctypes.c_char_p]


LIBRARY.ts_mirror_obs_override.argtypes=[ctypes.c_int,ctypes.c_int]
LIBRARY.ts_edit_mirror_output.argtypes=[ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_double]
LIBRARY.ts_edit_overlay_style.argtypes=[ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_double]
LIBRARY.ts_copy_mirror_output.argtypes=[ctypes.c_int,ctypes.c_int,ctypes.c_int]
LIBRARY.ts_mirror_output.argtypes=[ctypes.c_int,ctypes.c_int,ctypes.POINTER(ctypes.c_double)]
LIBRARY.ts_overlay_extent.argtypes=[ctypes.c_int]*4+[ctypes.POINTER(ctypes.c_float)]

def mirror_output(index,route=0):
    values=(ctypes.c_double*67)();assert LIBRARY.ts_mirror_output(index,route,values)==67;return list(values)

def overlay_extent(index,route=0,width=1440,height=900):
    values=(ctypes.c_float*4)();assert LIBRARY.ts_overlay_extent(index,route,width,height,values)==4;return list(values)

def profile_rows(names,path=b'root/frame/extraction/block_entities'):
    values=(ctypes.c_double*(1+len(names)*3))(100,*[value for _ in names for value in [25,2.5,0xffaabb]])
    LIBRARY.ts_debug_pie(path,'\n'.join(names).encode(),values,len(names))


def bind(action,keys):
    data=(ctypes.c_int*len(keys))(*keys)
    LIBRARY.ts_bind_keys(action,data,len(keys))


def input_event(scan,modifiers=0,action=1,gameplay=True,editor=True):
    return LIBRARY.ts_input_event(scan,modifiers,action,gameplay,editor)


def generic_mirror(name,preset):
    index=LIBRARY.ts_add_mirror(name,preset)
    LIBRARY.ts_edit_mirror(index,33,0)
    LIBRARY.ts_edit_mirror(index,15,1)
    if preset==1:
        LIBRARY.ts_edit_mirror(index,31,8)
    return index


def probe():
    data=(ctypes.c_double*10)()
    LIBRARY.ts_feature_probe(data,10)
    return list(data)


def overlay(route=0):
    data=(ctypes.c_uint8*4194304)()
    count=LIBRARY.ts_overlay_frame(1440,900,1440,900,route,data,len(data))
    return bytes(data[:count])


def viewport(width=1920, height=1080, limit=16384, suspended=False):
    result = (ctypes.c_int * 2)()
    LIBRARY.ts_viewport(width, height, limit, suspended, result)
    return list(result)


def plan(width=1920, height=1080, view_width=384, view_height=1080, suspended=False):
    result = (ctypes.c_float * 4096)()
    count = LIBRARY.ts_plan(width, height, view_width, view_height, suspended, 0, result, len(result))
    return [list(result[i:i+20]) for i in range(0, count, 20)]


class UiSession:
    def __init__(self):
        self.packet=(ctypes.c_uint8*4194304)()
        self.frame(2)

    def frame(self,count=1):
        for _ in range(count):LIBRARY.ts_ui_frame(1440,900,1/60,self.packet,len(self.packet))

    def key(self,key,modifiers=0):
        LIBRARY.ts_ui_key(key,modifiers,1);self.frame();LIBRARY.ts_ui_key(key,0,0);self.frame()

    def bounds(self,control):
        result=(ctypes.c_float*4)()
        if not LIBRARY.ts_ui_rect(control,result):raise AssertionError('Missing control '+str(control))
        return list(result)

    def point(self,x,y):
        LIBRARY.ts_ui_mouse(x,y);self.frame()

    def press(self,x,y):
        self.point(x,y);LIBRARY.ts_ui_button(0,1);self.frame()

    def release(self):
        LIBRARY.ts_ui_button(0,0);self.frame(2)

    def click(self,x,y):
        self.press(x,y);self.release()

    def layout(self):
        self.key(ord('1'),192)
        left,top,right,bottom=self.bounds(501)
        self.press(right-4,bottom-4)
        self.point(min(1432,left+960)-4,min(892,top+560)-4);self.release();self.frame(2)

    def source_bounds(self,values):
        left,top,right,bottom=self.bounds(400)
        fit=min((right-left-24)/1440,(bottom-top-24)/900)
        width,height=1440*fit,900*fit
        x,y=left+(right-left-width)/2,top+(bottom-top-height)/2
        return [x+values[5]*width,y+values[6]*height,values[7]*width,values[8]*height]


def saved_image_values(path,source):
    LIBRARY.ts_save()
    for line in path.read_text().splitlines():
        parts=shlex.split(line)
        if len(parts)==43 and parts[0]==str(source):return [float(value) for value in parts[3:]]
    raise AssertionError('Missing saved image')


class NativeTests(unittest.TestCase):
    def setUp(self):
        LIBRARY.ts_reset()
        LIBRARY.ts_ui_display_scale(1)
        LIBRARY.ts_ui_focus(0)
        packet=(ctypes.c_uint8*4194304)()
        for _ in range(2):
            LIBRARY.ts_ui_frame(1440,900,1/60,packet,len(packet))
        LIBRARY.ts_ui_focus(1)
        LIBRARY.ts_ui_frame(1440,900,1/60,packet,len(packet))
        LIBRARY.ts_debug_clear()
        LIBRARY.ts_ninja_update(b"",b"offline")
        for index in range(3):
            LIBRARY.ts_edit_mirror(index,0,0)
        LIBRARY.ts_edit_mirror(0,33,0)

    def test_unified_sensitivity_applies_to_both_axes_and_all_modes(self):
        values=(ctypes.c_double*2)();LIBRARY.ts_set(145,0);LIBRARY.ts_set(39,1.5)
        self.assertEqual(LIBRARY.ts_get(40),1.5)
        for mode in range(5):
            LIBRARY.ts_set(0,mode);LIBRARY.ts_sensitivity(values);self.assertEqual(list(values),[1.5,1.5])
        LIBRARY.ts_set(40,.12345678);LIBRARY.ts_sensitivity(values);self.assertEqual(list(values),[.12345678,.12345678])
        LIBRARY.ts_set(38,0);LIBRARY.ts_sensitivity(values);self.assertEqual(list(values),[1,1])
        LIBRARY.ts_set(39,1.5);self.assertEqual(LIBRARY.ts_get(38),1)
        LIBRARY.ts_set(128,0);LIBRARY.ts_sensitivity(values);self.assertEqual(list(values),[1,1])

    def test_eyezoom_replaces_normal_sensitivity_and_restores_it(self):
        values=(ctypes.c_double*2)();LIBRARY.ts_set(39,20);LIBRARY.ts_set(146,.01234567);LIBRARY.ts_set(0,3)
        LIBRARY.ts_sensitivity(values);self.assertAlmostEqual(values[0],.064);self.assertAlmostEqual(values[1],.064)
        LIBRARY.ts_set(38,0);LIBRARY.ts_sensitivity(values);self.assertAlmostEqual(values[0],.064);self.assertAlmostEqual(values[1],.064)
        LIBRARY.ts_set(145,0);LIBRARY.ts_sensitivity(values);self.assertEqual(list(values),[1,1])
        LIBRARY.ts_set(146,.1);self.assertEqual(LIBRARY.ts_get(145),1)
        LIBRARY.ts_set(38,1);LIBRARY.ts_set(0,2);LIBRARY.ts_sensitivity(values);self.assertAlmostEqual(values[0],4.096);self.assertAlmostEqual(values[1],4.096)
        LIBRARY.ts_set(0,3);LIBRARY.ts_set(128,0);LIBRARY.ts_sensitivity(values);self.assertEqual(list(values),[1,1])

    def test_sensitivity_supports_twenty_and_precise_right_click_entry(self):
        for key in [39,40,146]:
            LIBRARY.ts_set(key,20);self.assertEqual(LIBRARY.ts_get(key),20)
            LIBRARY.ts_set(key,21);self.assertEqual(LIBRARY.ts_get(key),20)
        LIBRARY.ts_set(38,1);LIBRARY.ts_ui_focus(1)
        packet=(ctypes.c_uint8*4194304)();bounds=(ctypes.c_float*4)()
        def frame():
            LIBRARY.ts_ui_frame(1440,900,1/60,packet,len(packet))
        LIBRARY.ts_ui_key(ord('2'),192,1);LIBRARY.ts_ui_key(ord('2'),0,0);frame();frame()
        self.assertEqual(LIBRARY.ts_ui_rect(39,bounds),1)
        LIBRARY.ts_ui_mouse(700,650);frame()
        LIBRARY.ts_ui_scroll.argtypes=[ctypes.c_float,ctypes.c_float]
        for _ in range(4):
            if bounds[1]<780:break
            LIBRARY.ts_ui_scroll(0,-3);frame();frame();LIBRARY.ts_ui_rect(39,bounds)
        LIBRARY.ts_ui_mouse((bounds[0]+bounds[2])/2,(bounds[1]+bounds[3])/2);frame();LIBRARY.ts_ui_button(1,1);frame();LIBRARY.ts_ui_button(1,0);frame();frame()
        self.assertEqual(LIBRARY.ts_ui_wants_text(),1)
        LIBRARY.ts_ui_key(ord('a'),192,1);frame();LIBRARY.ts_ui_key(ord('a'),0,0)
        for character in '2.34567891':LIBRARY.ts_ui_text(ord(character))
        frame();LIBRARY.ts_ui_key(13,0,1);frame();LIBRARY.ts_ui_key(13,0,0);frame()
        self.assertAlmostEqual(LIBRARY.ts_get(39),2.34567891,places=8)
        LIBRARY.ts_ui_focus(0);frame()

    def test_command_palette_finds_sensitivity_and_accepts_keyboard_selection(self):
        packet=(ctypes.c_uint8*4194304)();bounds=(ctypes.c_float*4)()
        def frame():
            LIBRARY.ts_ui_frame(1440,900,1/60,packet,len(packet))
        LIBRARY.ts_ui_focus(1);LIBRARY.ts_ui_key(ord('1'),192,1);LIBRARY.ts_ui_key(ord('1'),0,0);frame();frame()
        LIBRARY.ts_ui_key(ord('k'),192,1);LIBRARY.ts_ui_key(ord('k'),0,0);frame();frame()
        self.assertEqual(LIBRARY.ts_ui_rect(300,bounds),1)
        self.assertEqual(LIBRARY.ts_ui_escape_active(),1)
        for char in 'sensitivity':LIBRARY.ts_ui_text(ord(char))
        frame();LIBRARY.ts_ui_key(13,0,1);frame();LIBRARY.ts_ui_key(13,0,0);frame();frame()
        self.assertEqual(LIBRARY.ts_ui_rect(39,bounds),1)
        self.assertEqual(LIBRARY.ts_ui_rect(146,bounds),1)
        LIBRARY.ts_ui_focus(0);frame()

    def test_retina_input_controls_receive_precise_mouse_input(self):
        LIBRARY.ts_ui_display_scale(2);packet=(ctypes.c_uint8*4194304)();bounds=(ctypes.c_float*4)()
        def frame():
            LIBRARY.ts_ui_frame(2880,1482,1/60,packet,len(packet))
        LIBRARY.ts_ui_focus(1);LIBRARY.ts_ui_key(ord('2'),192,1);LIBRARY.ts_ui_key(ord('2'),0,0);frame();frame();frame()
        self.assertEqual(LIBRARY.ts_ui_rect(146,bounds),1)
        self.assertTrue(0<=bounds[0]<bounds[2]<=2880)
        self.assertTrue(0<=bounds[1]<bounds[3]<1380)
        LIBRARY.ts_ui_mouse((bounds[0]+bounds[2])/2,(bounds[1]+bounds[3])/2);frame();LIBRARY.ts_ui_button(1,1);frame();LIBRARY.ts_ui_button(1,0);frame();frame()
        self.assertEqual(LIBRARY.ts_ui_wants_text(),1)
        LIBRARY.ts_ui_key(ord('a'),192,1);frame();LIBRARY.ts_ui_key(ord('a'),0,0)
        for char in '0.03141592':LIBRARY.ts_ui_text(ord(char))
        frame();LIBRARY.ts_ui_key(13,0,1);frame();LIBRARY.ts_ui_key(13,0,0);frame()
        self.assertAlmostEqual(LIBRARY.ts_get(146),.03141592,places=8)
        LIBRARY.ts_ui_focus(0);LIBRARY.ts_ui_display_scale(1);frame()

    def test_true_render_dimensions_and_menu_restore(self):
        LIBRARY.ts_set(0, 1)
        self.assertEqual(viewport(), [330, 1026])
        LIBRARY.ts_set(36,0)
        self.assertEqual(viewport(), [384,1080])
        self.assertEqual(viewport(suspended=True), [1920, 1080])
        LIBRARY.ts_set(0, 3)
        self.assertEqual(viewport(), [384, 16384])

    def test_dimension_limit_preserves_aspect(self):
        LIBRARY.ts_set(0, 3)
        width, height = viewport(limit=4096)
        self.assertEqual([width, height], [96, 4096])

    def test_gpu_budget(self):
        LIBRARY.ts_set(0, 3)
        LIBRARY.ts_set(5, 16384)
        LIBRARY.ts_set(6, 16384)
        width, height = viewport()
        self.assertLessEqual(width * height, 16777216)

    def test_letterbox_keeps_source_aspect(self):
        base = plan()[0]
        self.assertAlmostEqual(base[4], 0.4, places=5)
        self.assertAlmostEqual(base[6], 0.2, places=5)
        self.assertEqual(base[0:4], [0, 0, 1, 1])

    def test_eyezoom_uses_centered_crop(self):
        LIBRARY.ts_set(0, 3)
        layers = plan(view_height=8192)
        self.assertEqual(len(layers), 2)
        self.assertAlmostEqual(layers[1][0] + layers[1][2] / 2, 0.5, places=6)
        self.assertAlmostEqual(layers[1][1] + layers[1][3] / 2, 0.5, places=6)
        self.assertEqual(layers[1][12], 576)
        self.assertEqual(layers[1][18],30)

    def test_mirror_crop_stays_in_source(self):
        LIBRARY.ts_set(20, 1)
        LIBRARY.ts_set(21, 0.99)
        LIBRARY.ts_set(22, 0.99)
        mirror = plan()[-1]
        self.assertLessEqual(mirror[0] + mirror[2], 1.000001)
        self.assertGreaterEqual(mirror[1], 0)
        self.assertLessEqual(mirror[1] + mirror[3], 1.000001)

    def test_invalid_updates_do_not_corrupt_state(self):
        self.assertEqual(LIBRARY.ts_set(-1, 1), 0)
        self.assertEqual(LIBRARY.ts_set(0, math.nan), 0)
        self.assertEqual(LIBRARY.ts_set(5, math.inf), 0)
        self.assertEqual(LIBRARY.ts_get(0), 0)

    def test_persistence_is_atomic_on_invalid_load(self):
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / "settings.conf"
            self.assertEqual(LIBRARY.ts_load(str(path).encode()), 1)
            LIBRARY.ts_set(0, 3)
            self.assertEqual(LIBRARY.ts_save(), 1)
            LIBRARY.ts_reset()
            self.assertEqual(LIBRARY.ts_load(str(path).encode()), 1)
            self.assertEqual(LIBRARY.ts_get(0), 3)
            path.write_text("toolscreen 1\n2\nnope\n")
            self.assertEqual(LIBRARY.ts_load(str(path).encode()), 0)
            self.assertEqual(LIBRARY.ts_get(0), 3)

    def test_plan_capacity_is_checked(self):
        result = (ctypes.c_float * 1)(91)
        self.assertEqual(LIBRARY.ts_plan(1920, 1080, 384, 1080, 0, 0, result, 1), -20)
        self.assertEqual(result[0], 91)

    def test_random_configurations_have_finite_bounded_crops(self):
        generator = random.Random(26)
        for _ in range(300):
            for index in range(36):
                LIBRARY.ts_set(index, generator.uniform(-100, 20000))
            width, height = viewport(limit=8192)
            self.assertTrue(1 <= width <= 8192)
            self.assertTrue(1 <= height <= 8192)
            for layer in plan(view_width=width, view_height=height):
                self.assertTrue(all(math.isfinite(value) for value in layer))
                self.assertGreaterEqual(layer[0], 0)
                self.assertGreaterEqual(layer[1], -0.000001)
                self.assertLessEqual(layer[0] + layer[2], 1.000001)
                self.assertLessEqual(layer[1] + layer[3], 1.000001)

    def test_imgui_draw_packets_have_valid_vertices_and_clips(self):
        packet = (ctypes.c_uint8 * 4194304)()
        for width, height in [(854, 480), (1920, 1080), (480, 320)]:
            LIBRARY.ts_ui_frame(width, height, 1/60, packet, len(packet))
            count = LIBRARY.ts_ui_frame(width, height, 1/60, packet, len(packet))
            self.assertGreater(count, 8)
            data = bytes(packet[:count])
            vertices, commands = struct.unpack_from("II", data)
            self.assertGreater(vertices, 100)
            self.assertGreater(commands, 0)
            self.assertEqual(count, 8 + vertices * 24 + commands * 32)
            for offset in range(8, 8 + vertices * 24, 24):
                vertex = struct.unpack_from("5fI", data, offset)
                self.assertTrue(all(math.isfinite(value) for value in vertex[:5]))
            for offset in range(8 + vertices * 24, count, 32):
                first, amount, x, y, clip_width, clip_height, texture, reserved = struct.unpack_from("II4fII", data, offset)
                self.assertLessEqual(first + amount, vertices)
                self.assertTrue(all(math.isfinite(value) for value in [x, y, clip_width, clip_height]))
                self.assertTrue(texture in [1,2,3,4] or texture>=100)

    def test_imgui_keyboard_page_navigation(self):
        packet=(ctypes.c_uint8*4194304)()
        bounds=(ctypes.c_float*4)()
        LIBRARY.ts_ui_key(ord('2'),192,1)
        LIBRARY.ts_ui_key(ord('2'),0,0)
        LIBRARY.ts_ui_frame(1440,900,1/60,packet,len(packet))
        self.assertEqual(LIBRARY.ts_ui_rect(39,bounds),1)
        LIBRARY.ts_ui_key(ord('1'),192,1)
        LIBRARY.ts_ui_key(ord('1'),0,0)
        LIBRARY.ts_ui_frame(1440,900,1/60,packet,len(packet))
        self.assertEqual(LIBRARY.ts_ui_rect(39,bounds),0)

    def test_imgui_mouse_input_changes_native_settings(self):
        packet = (ctypes.c_uint8 * 4194304)()
        bounds = (ctypes.c_float * 4)()

        def frame():
            LIBRARY.ts_ui_frame(854, 480, 1/60, packet, len(packet))

        def click(control):
            self.assertEqual(LIBRARY.ts_ui_rect(control, bounds), 1)
            LIBRARY.ts_ui_mouse((bounds[0]+bounds[2])/2, (bounds[1]+bounds[3])/2)
            frame()
            LIBRARY.ts_ui_button(0, 1)
            frame()
            LIBRARY.ts_ui_button(0, 0)
            frame()

        frame()
        frame()
        click(-2)
        self.assertEqual(LIBRARY.ts_get(128), 1)
        click(128)
        self.assertEqual(LIBRARY.ts_get(128), 0)

    def test_eyezoom_native_target_is_cropped_instead_of_scaled(self):
        LIBRARY.ts_set(0,3)
        base=plan(view_height=16384)[0]
        self.assertAlmostEqual(base[6]*1920,384,places=4)
        self.assertAlmostEqual(base[7]*1080,1080,places=4)
        self.assertAlmostEqual(base[3],1080/16384,places=6)

    def test_auto_projector_never_overlaps_game_across_displays(self):
        LIBRARY.ts_set(0,3)
        for width,height in [(800,600),(1280,720),(1920,1080),(2560,1440),(3440,1440),(360,640),(1441,901),(801,601)]:
            layers=plan(width,height,384,16384)
            if len(layers)>1:
                base,projector=layers[:2]
                self.assertLess(projector[4]+projector[6],base[4])
                self.assertGreaterEqual(projector[4],0)
                self.assertGreaterEqual(projector[5],0)
                self.assertLessEqual(projector[5]+projector[7],1)
                self.assertAlmostEqual(projector[7]*height,height-2*(height//8),places=3)

    def test_windows_wide_preset_and_manual_override(self):
        LIBRARY.ts_set(0,2)
        self.assertEqual(viewport(),[1882,270])
        self.assertEqual(viewport(1440,900),[1412,225])
        LIBRARY.ts_set(37,0)
        LIBRARY.ts_set(3,1600)
        LIBRARY.ts_set(4,400)
        self.assertEqual(viewport(),[1600,400])

    def test_odd_dimensions_follow_windows_integer_centering(self):
        LIBRARY.ts_set(0,3)
        LIBRARY.ts_set(12,31)
        LIBRARY.ts_set(13,1301)
        base,projector=plan(1441,901,384,16384)
        self.assertAlmostEqual(base[4]*1441,1441//2-384//2,places=3)
        self.assertAlmostEqual((1-base[1]-base[3])*16384,16384//2-901//2,places=3)
        self.assertAlmostEqual(projector[0]*384,384//2-31//2,places=3)
        self.assertAlmostEqual((1-projector[1]-projector[3])*16384,16384//2-1301//2,places=3)

    def test_background_image_is_clipped_around_game(self):
        LIBRARY.ts_set(0,1)
        source=LIBRARY.ts_add_background(b"/tmp/background.png")
        LIBRARY.ts_image_size(source,400,200)
        for sizing in range(3):
            LIBRARY.ts_set(122,sizing)
            layers=plan(1440,900,330,856)
            base=layers[0]
            self.assertGreater(len(layers),1)
            for layer in layers[1:]:
                self.assertEqual(layer[19],source)
                overlap_x=min(base[4]+base[6],layer[4]+layer[6])-max(base[4],layer[4])
                overlap_y=min(base[5]+base[7],layer[5]+layer[7])-max(base[5],layer[5])
                self.assertLessEqual(min(overlap_x,overlap_y),0.000001)
                self.assertGreaterEqual(layer[0],0)
                self.assertLessEqual(layer[0]+layer[2],1.000001)
                self.assertGreaterEqual(layer[1],0)
                self.assertLessEqual(layer[1]+layer[3],1.000001)
        self.assertEqual(LIBRARY.ts_add_background(b"/tmp/second.png"),source)
        self.assertEqual(len(plan(1440,900,330,856)),1)

    def test_gradient_and_border_suspend_for_menus(self):
        LIBRARY.ts_set(0,3)
        LIBRARY.ts_set(124,1)
        layers=plan(1440,900,384,16384)
        self.assertEqual(int(layers[0][12])&128,128)
        self.assertEqual(sum(bool(int(layer[12])&32) for layer in layers),2)
        menu=plan(1440,900,1440,900,True)
        self.assertEqual(len(menu),1)
        self.assertEqual(int(menu[0][12]),512)

    def test_undo_preserves_loaded_background_dimensions(self):
        LIBRARY.ts_set(0,1)
        source=LIBRARY.ts_add_background(b"/tmp/undo-background.png")
        LIBRARY.ts_edit_image(source,9,.5)
        LIBRARY.ts_image_size(source,640,480)
        LIBRARY.ts_undo()
        self.assertGreater(len(plan(1440,900,330,856)),1)

    def test_fixed_pixel_counter_crop_and_scale_survive_resizing(self):
        generic_mirror(b"E Counter",1)
        for width,height in [(1440,900),(330,856),(384,16384)]:
            mirror=plan(1440,900,width,height)[-1]
            self.assertAlmostEqual(mirror[0]*width,14,places=3)
            self.assertAlmostEqual((1-mirror[1]-mirror[3])*height,38,delta=.001)
            self.assertAlmostEqual(mirror[2]*width,23,places=3)
            self.assertAlmostEqual(mirror[3]*height,7,places=3)
            self.assertAlmostEqual(mirror[6]*1440,184,places=3)
            self.assertAlmostEqual(mirror[7]*900,56,places=3)
            self.assertEqual(int(mirror[12])&260,260)
            self.assertAlmostEqual(mirror[8],221/255,places=5)

    def test_source_anchor_and_destination_resize_override_magnification(self):
        index=generic_mirror(b"Anchored",1)
        for field,value in [(21,-3),(22,-4),(23,40),(24,12),(25,3)]:
            LIBRARY.ts_edit_mirror(index,field,value)
        mirror=plan(1440,900,330,856)[-1]
        self.assertAlmostEqual(mirror[0]*330,287,places=3)
        self.assertAlmostEqual((1-mirror[1]-mirror[3])*856,840,places=3)
        LIBRARY.ts_edit_mirror(index,7,.25)
        mirror=plan(1440,900,330,856)[-1]
        self.assertAlmostEqual(mirror[6],.25,places=5)

    def test_previous_source_format_is_migrated(self):
        for version,fields in [(2,20),(3,32)]:
            with tempfile.TemporaryDirectory() as directory:
                path=pathlib.Path(directory)/"legacy.conf"
                LIBRARY.ts_load(str(path).encode())
                asset=LIBRARY.ts_add_image(b"/tmp/image.png")
                LIBRARY.ts_save()
                lines=path.read_text().splitlines()
                cursor=258
                mode_count=int(lines[cursor]);mode_lines=lines[cursor:cursor+mode_count+1];cursor+=mode_count+1
                mirror_count=int(lines[cursor]);cursor+=1
                mirrors=[]
                for line in lines[cursor:cursor+mirror_count]:
                    parts=shlex.split(line);mirrors.append(json.dumps(parts[0])+' '+' '.join(parts[1:1+fields]))
                cursor+=mirror_count;image_count=int(lines[cursor]);cursor+=1
                images=[]
                for line in lines[cursor:cursor+image_count]:
                    parts=shlex.split(line);images.append(parts[0]+' '+json.dumps(parts[1])+' '+json.dumps(parts[2])+' '+' '.join(parts[3:3+fields]))
                cursor+=image_count
                legacy=[f'toolscreen {version}','128']+lines[2:130]+mode_lines+[str(mirror_count)]+mirrors+[str(image_count)]+images+lines[cursor:cursor+1+int(lines[cursor])]
                path.write_text('\n'.join(legacy)+'\n')
                LIBRARY.ts_reset()
                self.assertEqual(LIBRARY.ts_load(str(path).encode()),1)
                self.assertEqual(LIBRARY.ts_save(),1)
                self.assertTrue(path.read_text().startswith('toolscreen 6\n'))
                self.assertIn('"Profiler"',path.read_text())
                self.assertEqual(int(plan()[-1][19]),asset)

    def test_multiple_mirrors_keep_separate_visibility_and_border(self):
        first=generic_mirror(b"Counter",1)
        second=generic_mirror(b"Pie",2)
        self.assertEqual(len(plan()),5)
        LIBRARY.ts_edit_mirror(first,14,1<<3)
        self.assertEqual(len(plan()),3)
        LIBRARY.ts_edit_mirror(second,0,0)
        self.assertEqual(len(plan()),1)

    def test_hold_binding_restores_previous_mode_even_if_panel_opens(self):
        LIBRARY.ts_set(99,1)
        self.assertEqual(LIBRARY.ts_key_event(1073741890,0,1,0),2)
        self.assertEqual(LIBRARY.ts_get(0),3)
        self.assertEqual(LIBRARY.ts_key_event(1073741890,0,0,1),2)
        self.assertEqual(LIBRARY.ts_get(0),0)

    def test_modifier_binding_only_fires_for_matching_chord(self):
        LIBRARY.ts_set(83,4)
        self.assertEqual(LIBRARY.ts_key_event(1073741890,0,1,0),0)
        self.assertEqual(LIBRARY.ts_key_event(1073741890,768,1,0),2)
        self.assertEqual(LIBRARY.ts_get(0),3)

    def test_undo_redo_preserves_source_objects_and_properties(self):
        LIBRARY.ts_set(5,600)
        self.assertEqual(LIBRARY.ts_get(5),600)
        self.assertEqual(LIBRARY.ts_undo(),1)
        self.assertEqual(LIBRARY.ts_get(5),384)
        self.assertEqual(LIBRARY.ts_redo(),1)
        self.assertEqual(LIBRARY.ts_get(5),600)

    def test_sources_survive_config_round_trip(self):
        with tempfile.TemporaryDirectory() as directory:
            path=pathlib.Path(directory)/"profile.conf"
            LIBRARY.ts_load(str(path).encode())
            asset=LIBRARY.ts_add_image(b"browser:https://example.com/overlay")
            LIBRARY.ts_edit_image(asset,5,.2)
            LIBRARY.ts_save()
            LIBRARY.ts_reset()
            self.assertEqual(LIBRARY.ts_load(str(path).encode()),1)
            layers=plan()
            self.assertEqual(int(layers[-1][19]),asset)
            self.assertAlmostEqual(layers[-1][4],.2,places=5)

    def test_corrupt_primary_recovers_last_valid_backup(self):
        with tempfile.TemporaryDirectory() as directory:
            path=pathlib.Path(directory)/"profile.conf"
            LIBRARY.ts_load(str(path).encode())
            LIBRARY.ts_set(0,3)
            LIBRARY.ts_save()
            LIBRARY.ts_set(0,1)
            LIBRARY.ts_save()
            path.write_text("corrupt")
            self.assertEqual(LIBRARY.ts_load(str(path).encode()),1)
            self.assertEqual(LIBRARY.ts_get(0),3)

    def test_remapping_is_opt_in_and_does_not_chain(self):
        LIBRARY.ts_bind_remap(4,97,5,98)
        LIBRARY.ts_bind_remap(5,98,6,99)
        result=(ctypes.c_int*2)()
        LIBRARY.ts_remap_key(4,97,result)
        self.assertEqual(list(result),[4,97])
        LIBRARY.ts_set(112,1)
        LIBRARY.ts_remap_key(4,97,result)
        self.assertEqual(list(result),[4,97])
        LIBRARY.ts_remap_key(5,98,result)
        self.assertEqual(list(result),[6,99])
        LIBRARY.ts_remap_key(6,99,result)
        self.assertEqual(list(result),[5,98])
        LIBRARY.ts_remap_key(7,100,result)
        self.assertEqual(list(result),[7,100])

    def test_remapping_configuration_survives_reload(self):
        with tempfile.TemporaryDirectory() as directory:
            path=pathlib.Path(directory)/"keys.conf"
            LIBRARY.ts_load(str(path).encode())
            LIBRARY.ts_bind_remap(4,97,80,1073741904)
            LIBRARY.ts_set(112,1)
            LIBRARY.ts_save()
            LIBRARY.ts_reset()
            self.assertEqual(LIBRARY.ts_load(str(path).encode()),1)
            result=(ctypes.c_int*2)()
            LIBRARY.ts_remap_key(4,97,result)
            self.assertEqual(list(result),[80,1073741904])

    def test_every_physical_keyboard_scan_can_bind(self):
        for scan in range(1,512):
            LIBRARY.ts_reset()
            for action in range(17):
                bind(action,[])
            bind(3,[scan])
            self.assertEqual(input_event(scan,editor=False),2,scan)
            self.assertEqual(LIBRARY.ts_get(0),3,scan)
            input_event(scan,action=0,editor=False)

    def test_multiple_key_chords_and_exact_modifiers(self):
        bind(1,[4]);bind(2,[4,5]);bind(3,[6,224,229])
        self.assertEqual(input_event(4),2)
        self.assertEqual(input_event(5),2)
        self.assertEqual(LIBRARY.ts_get(0),2)
        input_event(4,action=0);input_event(5,action=0)
        self.assertEqual(input_event(6,64),0)
        input_event(6,64,0)
        self.assertEqual(input_event(6,64|2),2)
        self.assertEqual(LIBRARY.ts_get(0),3)

    def test_repeat_and_release_are_consumed_without_retriggering(self):
        self.assertEqual(input_event(66),2)
        for _ in range(10):
            self.assertEqual(input_event(66,action=2),3)
            self.assertEqual(LIBRARY.ts_get(0),3)
        self.assertEqual(input_event(66,action=0),3)
        self.assertEqual(input_event(66),2)
        self.assertEqual(LIBRARY.ts_get(0),0)

    def test_gameplay_bindings_are_blocked_in_chat_menus_and_when_disabled(self):
        for scan in [62,63,64,66,67]:
            self.assertEqual(input_event(scan,gameplay=False,editor=False),0)
            input_event(scan,action=0,gameplay=False,editor=False)
        self.assertEqual(LIBRARY.ts_get(0),0)
        LIBRARY.ts_set(128,0)
        self.assertEqual(input_event(66),0)
        self.assertEqual(LIBRARY.ts_get(0),0)
        self.assertEqual(input_event(65,gameplay=False),1)

    def test_f3_debug_chords_do_not_trigger_toolscreen(self):
        input_event(60)
        self.assertEqual(input_event(66),0)
        self.assertEqual(input_event(65),0)
        bind(3,[60,66])
        self.assertEqual(input_event(66),2)

    def test_hold_chord_releases_modifier_to_vanilla(self):
        bind(3,[7,224]);LIBRARY.ts_set(99,1)
        self.assertEqual(input_event(224,64),0)
        self.assertEqual(input_event(7,64),2)
        self.assertEqual(LIBRARY.ts_get(0),3)
        self.assertEqual(input_event(224,action=0),4)
        self.assertEqual(LIBRARY.ts_get(0),0)
        self.assertEqual(input_event(7,action=0),3)

    def test_hold_configuration_saves_the_restored_mode(self):
        with tempfile.TemporaryDirectory() as directory:
            path=pathlib.Path(directory)/"hold.conf"
            LIBRARY.ts_load(str(path).encode());bind(3,[66]);LIBRARY.ts_set(99,1)
            input_event(66);LIBRARY.ts_save()
            self.assertEqual(float(path.read_text().splitlines()[2]),0)
            LIBRARY.ts_reset();LIBRARY.ts_load(str(path).encode())
            self.assertEqual(LIBRARY.ts_get(0),0)

    def test_binding_conflicts_and_chords_survive_reload(self):
        with tempfile.TemporaryDirectory() as directory:
            path=pathlib.Path(directory)/"chords.conf"
            LIBRARY.ts_load(str(path).encode());bind(2,[4,5,231]);bind(3,[4,5,231]);LIBRARY.ts_save()
            LIBRARY.ts_reset();self.assertEqual(LIBRARY.ts_load(str(path).encode()),1)
            input_event(4,2048);self.assertEqual(input_event(5,2048),2)
            self.assertEqual(LIBRARY.ts_get(0),3)

    def test_display_and_obs_plans_have_independent_objects(self):
        display=LIBRARY.ts_add_image(b"/tmp/display.png");capture=LIBRARY.ts_add_image(b"/tmp/capture.png")
        LIBRARY.ts_edit_image(display,32,1);LIBRARY.ts_edit_image(capture,32,2)
        for route,expected,absent in [(0,display,capture),(1,capture,display)]:
            data=(ctypes.c_float*4096)()
            count=LIBRARY.ts_plan_output(1440,900,1440,900,0,0,route,data,len(data))
            ids=[int(data[index+19]) for index in range(0,count,20)]
            self.assertIn(expected,ids);self.assertNotIn(absent,ids)
            self.assertTrue(int(data[12])&512)
            self.assertFalse(int(data[count-8])&512)

    def test_eyezoom_routing_and_opacity_preserve_game_color(self):
        LIBRARY.ts_set(0,3);LIBRARY.ts_set(132,1)
        data=(ctypes.c_float*4096)()
        count=LIBRARY.ts_plan_output(1440,900,384,16384,0,0,1,data,len(data))
        self.assertEqual(count,20)
        self.assertTrue(int(data[12])&512)
        self.assertEqual(len(plan(1440,900,384,16384)),2)

    def test_f3_counter_appears_only_with_real_data_and_routes(self):
        LIBRARY.ts_edit_mirror(0,33,1);LIBRARY.ts_edit_mirror(0,0,1);LIBRARY.ts_edit_mirror(0,32,1)
        self.assertEqual(struct.unpack_from('II',overlay())[0],0)
        LIBRARY.ts_debug_counter(b'E: 7/65')
        self.assertGreater(struct.unpack_from('II',overlay())[0],0)
        self.assertEqual(struct.unpack_from('II',overlay(1))[0],0)
        LIBRARY.ts_debug_clear()
        self.assertEqual(struct.unpack_from('II',overlay())[0],0)

    def test_mapless_uses_only_the_current_profiler_selection(self):
        for name,color in [(b'blockentities',0xec6e4e),(b'blockEntities',0xc6c66e)]:
            LIBRARY.ts_debug_clear()
            values=(ctypes.c_double*7)(80,7.5,6,color,92.5,74,0xeeeeee)
            LIBRARY.ts_debug_pie(b'root/render/entities',name+b'\nunspecified',values,2)
            self.assertEqual(probe()[2:4],[1,6])
            LIBRARY.ts_debug_pie(b'root',b'tick\nrender',values,2)
            self.assertEqual(probe()[2],0)
        LIBRARY.ts_debug_clear();self.assertEqual(probe()[2],0)

    def test_modern_signed_argb_profiler_colors_are_preserved(self):
        LIBRARY.ts_edit_mirror(0,33,2);LIBRARY.ts_edit_mirror(0,0,1)
        values=(ctypes.c_double*4)(100,100,100,0xffc6c66e-0x100000000)
        LIBRARY.ts_debug_pie(b'root',b'blockEntities',values,1)
        data=overlay();vertices=struct.unpack_from('I',data)[0]
        colors=[struct.unpack_from('I',data,index+20)[0]&0xffffff for index in range(8,8+vertices*24,24)]
        self.assertIn(0x6ec6c6,colors)

    def test_ninjabrain_schema_coordinates_confidence_and_disconnect(self):
        payload={'stronghold':{'resultType':'TRIANGULATION','playerPosition':{'xInOverworld':1200,'zInOverworld':-300,'horizontalAngle':-40},'predictions':[{'chunkX':100,'chunkZ':-50,'certainty':.987,'overworldDistance':640}]}}
        LIBRARY.ts_ninja_update(json.dumps(payload).encode(),b'')
        self.assertEqual(probe()[4:9],[1,1,1604,-796,.987])
        self.assertTrue(-180<=probe()[9]<=180)
        LIBRARY.ts_ninja_update(b'',b'offline')
        self.assertEqual(probe()[4:6],[0,0])
        LIBRARY.ts_ninja_update(b'{}',b'')
        self.assertEqual(probe()[4:6],[0,0])
        payload['stronghold']['predictions']=[{}, {'chunkX':1,'chunkZ':2,'certainty':4}]
        LIBRARY.ts_ninja_update(json.dumps(payload).encode(),b'')
        self.assertEqual(probe()[4:9],[1,1,20,36,1])

    def test_escape_can_be_captured_as_a_gameplay_binding(self):
        packet=(ctypes.c_uint8*4194304)();bounds=(ctypes.c_float*4)()
        def frame():
            LIBRARY.ts_ui_frame(1440,900,1/60,packet,len(packet))
        LIBRARY.ts_ui_focus(1);LIBRARY.ts_ui_key(ord('2'),192,1);LIBRARY.ts_ui_key(ord('2'),0,0);frame();frame()
        self.assertEqual(LIBRARY.ts_ui_rect(203,bounds),1)
        LIBRARY.ts_ui_mouse((bounds[0]+bounds[2])/2,(bounds[1]+bounds[3])/2);frame();LIBRARY.ts_ui_button(0,1);frame();LIBRARY.ts_ui_button(0,0);frame()
        self.assertEqual(LIBRARY.ts_ui_physical_key(41,27,0,1),1)
        self.assertEqual(LIBRARY.ts_ui_physical_key(41,27,0,0),1)
        self.assertEqual(input_event(41,editor=False),2)
        self.assertEqual(LIBRARY.ts_get(0),3)
        LIBRARY.ts_ui_focus(0);frame()

    def test_actual_ninjabrain_1_5_2_triangulation_response(self):
        data=json.loads((ROOT/'tests/fixtures/ninjabrain-1.5.2.json').read_text())
        LIBRARY.ts_ninja_update(json.dumps(data).encode(),b'')
        first=data['stronghold']['predictions'][0]
        result=probe()
        self.assertEqual(result[4:8],[5,1,first['chunkX']*16+4,first['chunkZ']*16+4])
        self.assertAlmostEqual(result[8],first['certainty'])
        self.assertEqual(result[9],0)

    def test_hud_rendering_preserves_editor_text_focus(self):
        packet=(ctypes.c_uint8*4194304)();bounds=(ctypes.c_float*4)()
        LIBRARY.ts_ui_focus(1);LIBRARY.ts_ui_key(ord('1'),192,1);LIBRARY.ts_ui_key(ord('1'),0,0)
        def frame():
            LIBRARY.ts_ui_frame(1440,900,1/60,packet,len(packet))
        frame();frame()
        LIBRARY.ts_ui_key(ord('k'),192,1);LIBRARY.ts_ui_key(ord('k'),0,0);frame();frame()
        for character in 'mapless overlay':LIBRARY.ts_ui_text(ord(character))
        frame();frame();LIBRARY.ts_ui_key(13,0,1);frame();LIBRARY.ts_ui_key(13,0,0);frame();frame()
        self.assertEqual(LIBRARY.ts_ui_rect(21,bounds),1)
        LIBRARY.ts_ui_mouse((bounds[0]+bounds[2])/2,(bounds[1]+bounds[3])/2);frame()
        LIBRARY.ts_ui_button(0,1);frame();LIBRARY.ts_ui_button(0,0);frame();frame()
        self.assertEqual(LIBRARY.ts_ui_wants_text(),1)
        for _ in range(8):
            overlay();overlay(1);frame()
        self.assertEqual(LIBRARY.ts_ui_wants_text(),1)
        LIBRARY.ts_ui_text(ord('z'));frame();frame()
        name=ctypes.create_string_buffer(512);LIBRARY.ts_mirror_name(2,name,len(name))
        self.assertIn(b'z',name.value.lower())
        LIBRARY.ts_ui_focus(0);frame()

    def test_canvas_drag_is_one_undoable_action(self):
        with tempfile.TemporaryDirectory() as directory:
            path=pathlib.Path(directory)/'settings.conf';LIBRARY.ts_load(str(path).encode())
            for index in range(3):LIBRARY.ts_edit_mirror(index,0,0)
            source=LIBRARY.ts_add_image(b'/tmp/drag-fixture.png');before=saved_image_values(path,source)
            ui=UiSession();ui.layout();x,y,width,height=ui.source_bounds(before)
            LIBRARY.ts_ui_key(0,768,1);ui.frame();ui.press(x+width*.5,y+height*.5);ui.point(x+width*.5+24,y+height*.5+17);ui.release();LIBRARY.ts_ui_key(0,0,0);ui.frame()
            after=saved_image_values(path,source);self.assertNotEqual(after[5:7],before[5:7]);self.assertNotEqual(after[5],before[5]);self.assertNotEqual(after[6],before[6])
            self.assertEqual(LIBRARY.ts_undo(),1);restored=saved_image_values(path,source)
            self.assertEqual(restored[5:9],before[5:9])

    def test_canvas_top_left_resize_keeps_opposite_corner(self):
        with tempfile.TemporaryDirectory() as directory:
            path=pathlib.Path(directory)/'settings.conf';LIBRARY.ts_load(str(path).encode())
            for index in range(3):LIBRARY.ts_edit_mirror(index,0,0)
            source=LIBRARY.ts_add_image(b'/tmp/resize-fixture.png');before=saved_image_values(path,source)
            ui=UiSession();ui.layout();x,y,width,height=ui.source_bounds(before);ui.click(x+width*.5,y+height*.5)
            LIBRARY.ts_ui_key(0,768,1);ui.frame();ui.press(x,y);ui.point(x-20,y-14);ui.release();LIBRARY.ts_ui_key(0,0,0);ui.frame()
            after=saved_image_values(path,source)
            self.assertLess(after[5],before[5]);self.assertLess(after[6],before[6]);self.assertGreater(after[7],before[7]);self.assertGreater(after[8],before[8])
            self.assertAlmostEqual(after[5]+after[7],before[5]+before[7],places=6);self.assertAlmostEqual(after[6]+after[8],before[6]+before[8],places=6)
            self.assertEqual(LIBRARY.ts_undo(),1);self.assertEqual(saved_image_values(path,source)[5:9],before[5:9])

    def test_escape_cancels_canvas_drag(self):
        with tempfile.TemporaryDirectory() as directory:
            path=pathlib.Path(directory)/'settings.conf';LIBRARY.ts_load(str(path).encode())
            for index in range(3):LIBRARY.ts_edit_mirror(index,0,0)
            source=LIBRARY.ts_add_image(b'/tmp/cancel-fixture.png');before=saved_image_values(path,source)
            ui=UiSession();ui.layout();x,y,width,height=ui.source_bounds(before)
            ui.press(x+width*.5,y+height*.5);ui.point(x+width*.5+30,y+height*.5+20)
            self.assertEqual(LIBRARY.ts_ui_escape_active(),1);ui.key(27);ui.release()
            self.assertEqual(saved_image_values(path,source)[5:9],before[5:9])

    def test_ninjabrain_data_does_not_draw_a_runtime_layer(self):
        data=(ROOT/'tests/fixtures/ninjabrain-1.5.2.json').read_bytes();LIBRARY.ts_ninja_update(data,b'');LIBRARY.ts_set(134,1);LIBRARY.ts_set(135,3)
        self.assertEqual(len(overlay()),8);self.assertEqual(len(overlay(1)),8)

    def test_v4_migration_keeps_an_immutable_original(self):
        with tempfile.TemporaryDirectory() as directory:
            path=pathlib.Path(directory)/'settings.conf';LIBRARY.ts_load(str(path).encode());LIBRARY.ts_save()
            original=path.read_text().replace('toolscreen 6\n','toolscreen 4\n',1);path.write_text(original)
            LIBRARY.ts_reset();self.assertEqual(LIBRARY.ts_load(str(path).encode()),1);self.assertEqual(LIBRARY.ts_save(),1)
            backup=pathlib.Path(str(path)+'.v4.bak');self.assertEqual(backup.read_text(),original)
            LIBRARY.ts_set(39,2);LIBRARY.ts_save();self.assertEqual(backup.read_text(),original)
            self.assertTrue(path.read_text().startswith('toolscreen 6\n'))

    def test_overlay_long_names_render_every_glyph(self):
        index=LIBRARY.ts_add_mirror(b'Profiler',2)
        for field in [9,10,11,12,13,14,15,16,23,24]:LIBRARY.ts_edit_overlay_style(index,0,field,0)
        for other in range(index):LIBRARY.ts_edit_mirror(other,0,0)
        LIBRARY.ts_edit_mirror(index,7,.08)
        profile_rows(['AA']);short=struct.unpack_from('I',overlay())[0]
        profile_rows(['gjpqy'*24]);packet=overlay();long=struct.unpack_from('I',packet)[0]
        self.assertEqual(long-short,118*6)
        x,y,width,height=overlay_extent(index)
        points=[struct.unpack_from('ff',packet,8+i*24) for i in range(long)]
        self.assertGreaterEqual(min(a for a,b in points),x-2)
        self.assertLessEqual(max(a for a,b in points),x+width+2)
        self.assertGreaterEqual(min(b for a,b in points),y-2)
        self.assertLessEqual(max(b for a,b in points),y+height+2)

    def test_keybind_header_has_clear_space_above_first_button(self):
        ui=UiSession();ui.key(ord('2'),192);ui.frame(3)
        header=ui.bounds(710);first=ui.bounds(200)
        self.assertGreaterEqual(first[1]-header[3],6)

    def test_overlay_long_paths_render_every_glyph(self):
        index=LIBRARY.ts_add_mirror(b'Profiler',2)
        for other in range(index):LIBRARY.ts_edit_mirror(other,0,0)
        for field in [9,10,12,13,14,15,16,23,24]:LIBRARY.ts_edit_overlay_style(index,0,field,0)
        LIBRARY.ts_edit_overlay_style(index,0,11,1);LIBRARY.ts_edit_mirror(index,7,.12)
        profile_rows([],b'AA');short=struct.unpack_from('I',overlay())[0]
        profile_rows([],b'gjpqy'*80);long=struct.unpack_from('I',overlay())[0]
        self.assertEqual(long-short,398*6)

    def test_overlay_all_rows_fit_a_small_manual_panel(self):
        index=LIBRARY.ts_add_mirror(b'Profiler',2)
        for other in range(index):LIBRARY.ts_edit_mirror(other,0,0)
        for field in [9,10,11,12,13,14,15,16,23,24]:LIBRARY.ts_edit_overlay_style(index,0,field,0)
        LIBRARY.ts_edit_mirror(index,7,.12);LIBRARY.ts_edit_mirror(index,8,.08);LIBRARY.ts_edit_mirror(index,38,0)
        profile_rows(['A'*40]*32);packet=overlay();count=struct.unpack_from('I',packet)[0]
        self.assertGreaterEqual(count,32*40*6)
        x,y,width,height=overlay_extent(index)
        points=[struct.unpack_from('ff',packet,8+i*24) for i in range(count)]
        self.assertLessEqual(max(a for a,b in points),x+width+2)
        self.assertLessEqual(max(b for a,b in points),y+height+2)

    def test_overlay_near_screen_edge_moves_inward(self):
        index=LIBRARY.ts_add_mirror(b'Profiler',2);profile_rows(['long section name']*32)
        LIBRARY.ts_edit_mirror(index,5,.99);LIBRARY.ts_edit_mirror(index,6,.99)
        x,y,width,height=overlay_extent(index,width=640,height=360)
        self.assertGreaterEqual(x,0);self.assertGreaterEqual(y,0)
        self.assertLessEqual(x+width,640.01);self.assertLessEqual(y+height,360.01)

    def test_overlay_customization_roundtrips_and_undoes(self):
        with tempfile.TemporaryDirectory() as directory:
            path=pathlib.Path(directory)/'settings.conf';LIBRARY.ts_load(str(path).encode())
            index=LIBRARY.ts_add_mirror(b'Custom profiler',2)
            LIBRARY.ts_edit_mirror_output(index,0,7,.42);LIBRARY.ts_edit_mirror(index,38,17);LIBRARY.ts_edit_mirror(index,39,1.4)
            LIBRARY.ts_edit_overlay_style(index,0,17,6);LIBRARY.ts_edit_overlay_style(index,0,6,.27)
            display=mirror_output(index)
            self.assertEqual(LIBRARY.ts_save(),1);LIBRARY.ts_reset();self.assertEqual(LIBRARY.ts_load(str(path).encode()),1)
            self.assertEqual(mirror_output(index),display)
            LIBRARY.ts_edit_overlay_style(index,0,21,1);self.assertEqual(LIBRARY.ts_undo(),1)
            self.assertEqual(mirror_output(index),display)

    def test_v5_profiles_migrate_with_a_backup(self):
        with tempfile.TemporaryDirectory() as directory:
            path=pathlib.Path(directory)/'settings.conf';LIBRARY.ts_load(str(path).encode());LIBRARY.ts_save()
            original=path.read_text().replace('toolscreen 6\n','toolscreen 5\n',1);path.write_text(original)
            self.assertEqual(LIBRARY.ts_load(str(path).encode()),1);self.assertEqual(LIBRARY.ts_save(),1)
            self.assertEqual(pathlib.Path(str(path)+'.v5.bak').read_text(),original)
            self.assertEqual(mirror_output(1)[40:],mirror_output(1,1)[40:])


if __name__ == "__main__":
    unittest.main()
