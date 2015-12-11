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

import os
import sys
import timeit
import inspect

from pymic._misc import _debug as debug
from pymic._misc import _config as config


_pymic_modules = [
    "offload_device",
    "offload_stream",
    "offload_array",
    "_tracing",
    "_misc"
]


class _TraceDatabase:
    events = None

    @staticmethod
    def _shorten_arg(arg):
        clazz = arg.__class__.__name__
        if clazz in ["ndarray", "OffloadArray"]:
            shape = getattr(arg, "shape")
            dtype = getattr(arg, "dtype")
            return "{0}<{1},{2}>".format(clazz, shape, dtype)
        else:
            return str(arg)

    @staticmethod
    def _shorten_karg(karg):
        return "{0}={1}".format(*karg)

    def __init__(self):
        self.events = []

    def __del__(self):
        print("PYMIC:TRACE:function;tstart;tend;tdiff;args;"
              "keyword_args;stack_info")
        for i in self.events:
            print(i)

    def register(self, func, tstart, tend, args, kargs, stack_info):
        args = map(self._shorten_arg, args)
        kargs = map(self._shorten_karg, kargs.iteritems())
        i = (func, tstart, tend, tend - tstart, args, kargs, stack_info)
        msg = "PYMIC:TRACE:{0};{1};{2};{3};{4};{5};{6}".format(*i)
        self.events.append(msg)


def _stack_walk_none():
    return None


def _stack_walk_compact():
    filename = None
    lineno = None
    current_frame = inspect.currentframe()
    # unwind until we hit a frame that's not from pymic's call stack
    while current_frame is not None:
        traceback = inspect.getframeinfo(current_frame)
        module = inspect.getmoduleinfo(traceback.filename)
        # TODO: is this test sufficient?
        if module.name not in _pymic_modules:
            filename = traceback.filename
            lineno = traceback.lineno
            break
        current_frame = current_frame.f_back
    return (filename, lineno)


def _stack_walk_full():
    stack = []
    current_frame = inspect.currentframe()
    # unwind until we hit a frame that's not from pymic's call stack
    while current_frame is not None:
        traceback = inspect.getframeinfo(current_frame)
        module = inspect.getmoduleinfo(traceback.filename)
        # TODO: is this test sufficient?
        if module.name not in _pymic_modules:
            break
        current_frame = current_frame.f_back
    # continue to unwind, collection all stack frames seen
    while current_frame is not None:
        traceback = inspect.getframeinfo(current_frame)
        stack.append((traceback.function,
                     (traceback.filename, traceback.lineno)))
        current_frame = current_frame.f_back
    return stack


def _trace_func(func):
    # this is the do-nothing-wrapper
    def wrapper(*args, **kwargs):
        return func(*args, **kwargs)
    return wrapper


if config._trace_level == 1:
    debug(5, "tracing is enabled", config._trace_level)
elif config._trace_level is not None:
    debug(5, "tracing is disabled", config._trace_level)

if config._trace_level >= 1:
    _stack_walk_func = _stack_walk_compact
    if config._collect_stacks_str.lower() == "none":
        debug(5, "stack collection is set to '{0}'",
              config._collect_stacks_str)
        _stack_walk_func = _stack_walk_none
    elif config._collect_stacks_str.lower() == "compact":
        debug(5, "stack collection is set to '{0}'",
              config._collect_stacks_str)
        _stack_walk_func = _stack_walk_compact
    elif config._collect_stacks_str.lower() == "full":
        debug(5, "stack collection is set to '{0}'",
              config._collect_stacks_str)
        _stack_walk_func = _stack_walk_full
    else:
        debug(5, "stack collection is set to '{0}', using 'compact'",
              config._collect_stacks_str)

    # create statistics database
    _trace_database = _TraceDatabase()

    # create the proper decorator for gathering the traces
    def _trace_func(func):
        funcname = func.__name__
        debug(5, "collecting statistics for {0}", funcname)

        def wrapper(*args, **kwargs):
            tstart = timeit.default_timer()
            rv = func(*args, **kwargs)
            tend = timeit.default_timer()
            stack_info = _stack_walk_func()
            _trace_database.register(funcname, tstart, tend,
                                     args, kwargs, stack_info)
            return rv
        return wrapper


_trace = _trace_func
