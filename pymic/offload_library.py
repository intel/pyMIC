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

import sys
import os
import platform
import subprocess

from pymic.offload_error import OffloadError
from pymic._misc import _debug as debug
from pymic._misc import _config as config

from pymic._engine import pymic_library_load
from pymic._engine import pymic_library_unload
from pymic._engine import pymic_library_find_kernel


# retrieve the installation path of the module
pymic_dir = os.path.dirname(os.path.abspath(__file__))


class OffloadLibrary:
    """Manages loaded shared-object libraries with offload code on
       target devices.  For each kernel of the library, the instance
       provides an attribute that can be used with invoke
    """

    _library = None
    _tempfile = None
    _handle = None
    _device = None
    _device_id = None

    @staticmethod
    def _check_k1om(library):
        if platform.system() == 'Windows':
            # If we're running on a Windows machine, the .so
            # is expected to be a MIC native .so file.
            return True
        # Use readelf to detect the architecture flag of the .so
        # file to make sure we are only finding MIC native ones.
        p = subprocess.Popen(["readelf", '-h', library],
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        out, err = p.communicate()
        if not isinstance(out, str):
            out = str(out, 'ascii')
        # look for architecture flags
        # ('<unknown>' for older versions of binutils)
        return out.find('<unknown>: 0xb5') > 0 or out.find('Intel K1OM') > 0

    @staticmethod
    def _find_library(library):
        if os.path.isabs(library) and OffloadLibrary._check_k1om(library):
            abspath = library
        else:
            path_list = [pymic_dir]
            path_list.extend(config._search_path.split(os.pathsep))
            for path in path_list:
                debug(5, "    looking for {0} in {1}", library, path)
                abspath = os.path.join(path, library)

                if (os.path.isfile(abspath) and
                        OffloadLibrary._check_k1om(abspath)):
                    break
            else:
                return

        return abspath

    def __init__(self, library, device=None):
        """Initialize this OffloadLibrary instance.  This function is not to be
           called from outside pymic.
        """

        # safety checks
        assert device is not None

        # bookkeeping
        self._library = library
        self._device = device
        self._device_id = device._map_dev_id()
        self.unloader = pymic_library_unload
        self._cache = {}

        # locate the library on the host file system
        debug(5, "searching for {0} in {1}", library, config._search_path)
        filename = OffloadLibrary._find_library(library)
        if filename is None:
            debug(5, "no suitable library found for '{0}'", library)
            raise OffloadError("Cannot find library '{0}' "
                               "in PYMIC_LIBRARY_PATH".format(library))

        # load the library and memorize handle
        debug(5, "loading '{0}' on device {1}", filename, self._device_id)
        self._handle, self._tempfile = pymic_library_load(self._device_id,
                                                          filename)
        debug(5, "successfully loaded '{0}' on device {1} with handle 0x{2:x}",
              filename, self._device_id, self._handle)

    def __del__(self):
        # unload the library on the target device
        if self._handle is not None:
            self.unloader(self._device_id, self._handle, self._tempfile)

    def __repr__(self):
        return "OffloadLibrary('{0}'@0x{1:x}@mic:{2})".format(self._library,
                                                              self._handle,
                                                              self._device_id)

    def __str__(self):
        return "OffloadLibrary('{0}'@0x{1:x}@mic:{2})".format(self._library,
                                                              self._handle,
                                                              self._device_id)

    def __getattr__(self, attr):
        funcptr = self._cache.get(attr, None)
        if funcptr is None:
            funcptr = pymic_library_find_kernel(self._device_id,
                                                self._handle, attr)
            self._cache[attr] = funcptr

        return attr, funcptr, self._device, self
