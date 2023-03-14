import os
import sys
from distutils.core import setup, Extension

owd = os.getcwd()
os.chdir("c_module")

sys.argv.append('build_ext')
sys.argv.append('--inplace')

setup(name='myModule', version='1.0', ext_modules=[Extension('myModule', sources=['python_interface_mcts.c'],
                                                             extra_compile_args=["/arch:AVX2", "/openmp"])])

os.chdir(owd)
