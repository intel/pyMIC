# Copyright (c) 2014-2015, Intel Corporation All rights reserved. 
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

import numpy

from _pymicimpl import _pymic_impl_offload_number_of_devices

from _misc import _debug as debug
from _misc import _get_order as get_order
from _tracing import _trace as trace

from offload_library import OffloadLibrary
from offload_stream import OffloadStream


def number_of_devices():
    """Returns the number of devices available for offloading.
    
       Parameters
       ----------
              
       Returns
       -------
       out : int
           Number of offload devices in the system
       
       See Also
       --------
              
       Examples
       --------
       >>> pymic.number_of_devices()
       2
    """
    return _pymic_impl_offload_number_of_devices()


class OffloadDevice:
    def __init__(self, device_id):
        self.device_id = device_id
        self.default_stream = OffloadStream(self)

    def __eq__(self, other):
        return self.device_id == other.device_id
        
    def __repr__(self):
        if self.device_id == any:
            s = "any"
        else:
            s = str(self.device_id)
        return "OffloadDevice({0})".format(s)
        
    def __str__(self):
        if self.device_id == any:
            s = "any"
        else:
            s = str(self.device_id)
        return "target(mic:{0})".format(s)
    
    def _is_real(self):
        return self.device_id != any
    
    def _map_dev_id(self):
        if self.device_id == any:
            return -1
        else:
            return int(self.device_id)

    def get_default_stream(self):
        """Return the default stream for the current device.

           Parameters
           ----------
           n/a

           Returns
           -------
           out : offload_stream
               The default stream object for this device.

           See Also
           --------
           create_stream
        """
        return self.default_stream

    def create_stream(self):
        """Create a new stream object for this device.  All streams on the same
           device will execute enqueued requests concurrently.

           Parameters
           ----------
           n/a

           Returns
           -------
           out : offload_stream
               The default stream object for this device.

           See Also
           --------
           get_default_stream
        """
        return OffloadStream(self)

    @trace    
    def load_library(self, *libraries, **kwargs):
        """Load one or multiple shared-object library that each contains one 
           or multiple native kernels for invocation through invoke.
           
           Parameters
           ----------
           libraries : str, typle of str, or list of str
               Names of the shared-object libraries to load on the device
           
           Returns
           -------
           One OffloadLibrary object per loaded library.
           
           See Also
           --------
           OffloadStream.invoke
           
           Examples
           --------
           >>> device.load_library("kernels")
           >>> device.load_library("blas", "mykernels")
        """   

        if len(libraries) == 0:
            raise ValueError("no argument given")
        
        # if called from wrapper, actual arguments are wrapped 
        # in an extra tuple, so we unpack them
        while type(libraries[0]) is tuple:
            libraries = libraries[0]
            
        if type(libraries) is tuple and len(libraries) == 1:
            return OffloadLibrary(libraries[0], device=self)
        else:
            return map(lambda l: OffloadLibrary(l, device=self), 
                       libraries)


def _init_devices():
    """Internal function that is used to initialize the Python mapping of 
       physical offload targets to a Python dictionary.
    """
    no_of_devices = number_of_devices()
    debug(5, "found {0} device(s)", no_of_devices)
    device_dict = {any: OffloadDevice(any)}
    for d in range(0, no_of_devices):
        device_dict[d] = OffloadDevice(d)
    return device_dict

    
devices = _init_devices()           
