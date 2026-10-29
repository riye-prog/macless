import os
import pathlib
import subprocess
import sys
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]


class ApprovalTests(unittest.TestCase):
    def test_runtime_limits_survive_settings_reset_and_allow_window_sizes(self):
        code = '''
import ctypes
import math
import os
library=ctypes.CDLL(os.environ['TOOLSCREEN_TEST_LIBRARY'])
library.ts_set.argtypes=[ctypes.c_int,ctypes.c_double]
library.ts_runtime_context.argtypes=[ctypes.c_int,ctypes.c_double]
library.ts_viewport.argtypes=[ctypes.c_int]*4+[ctypes.POINTER(ctypes.c_int)]
library.ts_sensitivity.argtypes=[ctypes.POINTER(ctypes.c_double)]
size=(ctypes.c_int*2)()
scale=(ctypes.c_double*2)()
library.ts_reset()
library.ts_runtime_context(1,.5)
library.ts_viewport(1440,900,32768,0,size)
assert library.ts_set(0,3)==1
library.ts_viewport(1440,900,32768,0,size)
assert list(size)==[384,16384],list(size)
assert library.ts_set(5,400)==0
assert library.ts_set(0,4)==1
library.ts_set(0,0)
library.ts_reset()
assert library.ts_set(5,400)==1
assert library.ts_set(0,3)==0
for mode in [0,1,2]:
 assert library.ts_set(0,mode)==1
library.ts_runtime_context(0,.5)
assert library.ts_set(0,3)==0
library.ts_set(39,20)
library.ts_sensitivity(scale)
assert all(math.isclose(value,4.096) for value in scale),list(scale)
library.ts_set(39,.000001)
library.ts_sensitivity(scale)
assert all(math.isclose(value,.064) for value in scale),list(scale)
for base in [0,.2,.5,.8,1]:
 library.ts_runtime_context(0,base)
 for requested in [.000001,.15,1,20]:
  library.ts_set(39,requested)
  library.ts_sensitivity(scale)
  effective=(base*.6+.2)**3*scale[0]
  assert .2**3-1e-12<=effective<=.8**3+1e-12,(base,requested,effective)
print('session resolution lock, window modes, reset persistence, menu persistence, sensitivity bounds passed')
'''
        environment=os.environ.copy()
        environment.setdefault('TOOLSCREEN_TEST_LIBRARY',str(ROOT/'build/native/libtoolscreen.dylib'))
        result=subprocess.run([sys.executable,'-c',code],env=environment,capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)


if __name__ == '__main__':
    unittest.main()
