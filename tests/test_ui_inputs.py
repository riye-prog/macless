import ctypes
import unittest
from test_native import LIBRARY,UiSession


LIBRARY.ts_ui_text.argtypes=[ctypes.c_uint]


class UiInputTests(unittest.TestCase):
    def setUp(self):
        LIBRARY.ts_reset()
        LIBRARY.ts_ui_display_scale(1)
        LIBRARY.ts_ui_focus(0)
        self.ui=UiSession()
        LIBRARY.ts_ui_focus(1)
        self.ui.frame(2)

    def click_control(self,control):
        left,top,right,bottom=self.ui.bounds(control)
        self.ui.click((left+right)/2,(top+bottom)/2)

    def enter(self,control,value):
        self.click_control(control)
        self.ui.key(ord('a'),192)
        for character in str(value):LIBRARY.ts_ui_text(ord(character))
        self.ui.frame(2)
        self.ui.key(13)
        self.ui.frame(2)

    def test_resolution_dimensions_accept_typing_and_disable_automatic(self):
        self.ui.key(ord('3'),192)
        self.click_control(801)
        self.enter(1,478)
        self.assertEqual(LIBRARY.ts_get(1),478)
        self.assertEqual(LIBRARY.ts_get(36),0)
        self.enter(2,674)
        self.assertEqual(LIBRARY.ts_get(2),674)

    def test_interface_scale_accepts_direct_typing_and_preserves_range(self):
        self.click_control(500)
        self.enter(34,1.375)
        self.assertAlmostEqual(LIBRARY.ts_get(34),1.375)
        self.enter(34,4)
        self.assertEqual(LIBRARY.ts_get(34),2)
        self.enter(34,.1)
        self.assertEqual(LIBRARY.ts_get(34),.75)

    def test_add_layer_menu_has_spacious_rows(self):
        self.ui.key(ord('1'),192)
        self.click_control(9200)
        previous=None
        for control in range(9201,9207):
            left,top,right,bottom=self.ui.bounds(control)
            self.assertGreaterEqual(right-left,200)
            self.assertGreaterEqual(bottom-top,38)
            if previous is not None:self.assertGreaterEqual((top+bottom)/2-previous,44)
            previous=(top+bottom)/2

    def test_dragging_a_dimension_field_does_not_change_its_value(self):
        self.ui.key(ord('3'),192)
        self.click_control(801)
        left,top,right,bottom=self.ui.bounds(1)
        self.ui.press((left+right)/2,(top+bottom)/2)
        self.ui.point(right-4,(top+bottom)/2)
        self.ui.release()
        self.assertEqual(LIBRARY.ts_get(1),384)
        self.assertEqual(LIBRARY.ts_get(36),1)


if __name__=='__main__':
    unittest.main()
