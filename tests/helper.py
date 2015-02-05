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

import unittest

import pymic


# detect version of unittest
# TODO: fix this line for versions of unitttest that do no longer
#       carry __version__
# major, minor = unittest.__version__.split(".")


def skipNoDevice(func):
    def wrapper(*args):
        args[0].assertTrue(pymic.number_of_devices() > 0,
                           "Could not find Xeon Phi coprocessors to test with")
        func(*args)
    return wrapper


def skipNoMultipleDevices(func):
    def wrapper(*args):
        args[0].assertTrue(pymic.number_of_devices() > 1,
                           "Could not find multiple Xeon Phi coprocessors "
                           "to test with")
        func(*args)
    return wrapper


libraries = {}


def get_library(device, library_name):
    library = libraries.get((library_name, device.device_id), None)
    if library is None:
        library = device.load_library(library_name)
        libraries[(library_name, device.device_id)] = library
    return library
