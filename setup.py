# Copyright (c) 2014-2016, Intel Corporation All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1. Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from __future__ import print_function

import os
import sys
import types

import distutils.ccompiler

from distutils import sysconfig
from distutils.core import setup
from distutils.extension import Extension
from distutils.ccompiler import CCompiler
from distutils.unixccompiler import UnixCCompiler
from Cython.Distutils import build_ext

MAJOR = 0
MINOR = 6
VERSION = '{0}.{1}'.format(MAJOR, MINOR)


# compiler driver for Intel Composer for C/C++
class IntelCompiler(UnixCCompiler, object):
    """Compiler wrapper for Intel Composer for C/C++"""

    def __init__(self, verbose=0, dry_run=0, force=0):
        super(self.__class__, self).__init__(verbose, dry_run, force)
        print('creating IntelCompiler instance')

    def set_executables(self, **args):
        # basically, we ignore all the tool chain coming in
        super(self.__class__, self).set_executables(compiler='icc',
                                                    compiler_so='icc',
                                                    compiler_cxx='icpc',
                                                    linker_exe='icc',
                                                    linker_so='icc -shared')

    def library_dir_option(self, dir):
        # We have to filter out -L/usr/lib64
        if dir == '/usr/lib64':
            return ''
        else:
            return super(self.__class__, self).library_dir_option(dir)

    def library_option(self, lib):
        # We have to filter out -lpythonX.Y
        if lib[0:6] == 'python':
            return ''
        else:
            return super(self.__class__, self).library_option(lib)

    def _fix_lib_args(self, libraries, library_dirs, runtime_library_dirs):
        # we need to have this method here, to avoid an endless
        # recursion in UnixCCompiler._fix_lib_args.
        libraries, library_dirs, runtime_library_dirs = \
            CCompiler._fix_lib_args(self, libraries, library_dirs,
                                    runtime_library_dirs)
        libdir = sysconfig.get_config_var('LIBDIR')
        if runtime_library_dirs and (libdir in runtime_library_dirs):
            runtime_library_dirs.remove(libdir)
        return libraries, library_dirs, runtime_library_dirs


# register Intel Composer for C/C++ as the main compiler for pyMIC
def my_new_compiler(plat=None, compiler=None, verbose=0, dry_run=0, force=0):
    compiler = IntelCompiler(verbose, dry_run, force)
    return compiler


# override compiler construction
print('creating compiler instance for Intel Composer for C/C++')
distutils.ccompiler.new_compiler = my_new_compiler


print('done with customizations')
# define the source of LIBXSTREAM and the pyMIC offload engine
libxstream_src = map(lambda x: 'src/libxstream/src/' + x,
                     ['libxstream.cpp', 'libxstream_alloc.cpp',
                      'libxstream_argument.cpp', 'libxstream_context.cpp',
                      'libxstream_event.cpp', 'libxstream_offload.cpp',
                      'libxstream_stream.cpp', 'libxstream_workitem.cpp',
                      'libxstream_workqueue.cpp'])
sources = ['src/pymic_libxstream.pyx', 'src/pymic_internal.cc',
           'src/pymicimpl_misc.cc']
sources.extend(libxstream_src)

engine_libxstream = Extension('pymic.pymic_libxstream',
                              sources,
                              language='c++',
                              include_dirs=['./src/libxstream/include',
                                            './include/'],
                              extra_compile_args=['-fPIC', '-std=c++0x',
                                                  '-DPYMIC_USE_XSTREAM=1',
                                                  '-DLIBXSTREAM_EXPORTED',
                                                  '-offload=mandatory',
                                                  '-Wall', '-pthread',
                                                  '-g', '-O2', '-ansi-alias'],
                              extra_link_args=['-pthread'])
liboffload_array = Extension('pymic.liboffload_array',
                             ['src/offload_array.c'],
                             language='c',
                             include_dirs=['./include/'],
                             extra_compile_args=['-fPIC', '-mmic',
                                                 '-std=c99', '-g', '-O2'],
                             extra_link_args=['-mmic'])

setup(
    name='Python Offload Infrastructure for the '
         'Intel(R) Many Integrated Core Architecture',
    version=VERSION,
    author='Michael Klemm',
    author_email='michael.klemm@intel.com',
    maintainer='Michael Klemm',
    maintainer_email='michael.klemm@intel.com',
    url='https://github.com/01org/pyMIC',
    description='A Python module to offload computation to the '
                'Intel(R) Xeon Phi(tm) coprocessor',
    long_description=""
                     "pyMIC is a Python module to offload computation in a "
                     "Python program to the Intel Xeon Phi coprocessor. It "
                     "contains offloadable arrays and device management "
                     "functions. It supports invocation of native kernels "
                     "(C/C++, Fortran) and blends in with Numpy's array "
                     "types for float, complex, and int data types. For "
                     "more information and downloads please visit pyMIC's "
                     "Github page: https://github.com/01org/pyMIC. You can "
                     "find pyMIC's mailinglist at "
                     "https://lists.01.org/mailman/listinfo/pymic. If you "
                     "want to report a bug, please send an email to the "
                     "mailinglist or file an issue at Github.",
    platforms=['Linux'],
    license='BSD-3',
    cmdclass={'build_ext': build_ext},
    packages=['pymic'],
    ext_modules=[engine_libxstream, liboffload_array],
)
