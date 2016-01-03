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
import platform

_loaded = False

# retrieve the installation path of the module
if platform.system() == 'Linux':
    pymic_dir = os.path.dirname(os.path.abspath(__file__))
    ld_library_path = os.environ['LD_LIBRARY_PATH']
    if ld_library_path is None:
        ld_library_path = pymic_dir
    else:
        ld_library_path = ld_library_path + ":" + pymic_dir
    os.environ['LD_LIBRARY_PATH'] = ld_library_path

# try to load LIBXSTREAM offload engine
if not _loaded:
    try:
        # debug(1, 'Trying to load LIBXSTREAM as offload engine')
        from pymic.pymic_libxstream import pymic_get_ndevices
        from pymic.pymic_libxstream import pymic_library_load
        from pymic.pymic_libxstream import pymic_library_unload
        from pymic.pymic_libxstream import pymic_library_find_kernel
        from pymic.pymic_libxstream import pymic_stream_create
        from pymic.pymic_libxstream import pymic_stream_destroy
        from pymic.pymic_libxstream import pymic_stream_sync
        from pymic.pymic_libxstream import pymic_stream_allocate
        from pymic.pymic_libxstream import pymic_stream_deallocate
        from pymic.pymic_libxstream \
            import pymic_stream_translate_device_pointer
        from pymic.pymic_libxstream import pymic_stream_memcpy_h2d
        from pymic.pymic_libxstream import pymic_stream_memcpy_d2h
        from pymic.pymic_libxstream import pymic_stream_memcpy_d2d
        from pymic.pymic_libxstream import pymic_stream_invoke_kernel
        _loaded = True
        # debug(1, 'Successfully loaded LIBXSTREAM as offload engine')
    except ImportError as exc:
        # debug(1, 'Failed to load LIBXSTREAM as offload engine')
        # debug(1, 'Reason:')
        # debug(1, '{0}', exc.message())
        print(exc)

if not _loaded:
    raise ImportError('No suitable offload engine found.')
