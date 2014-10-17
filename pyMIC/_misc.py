# Copyright (c) 2014, Intel Corporation All rights reserved. 
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

# determine the integer value for PYMIC_DEBUG
_debug_level = os.environ.get("PYMIC_DEBUG")
if _debug_level is not None:
    try:
        _debug_level = int(_debug_level)
        print("DBG: debug level set to {0}".format(_debug_level), file=sys.stderr)
    except:
        _debug_level = None

def _debug(level, *objs):
    if _debug_level >= level:
        print("DBG:", *objs, file=sys.stderr)
    return None
    
def _deprecated(func):
    def wrapper(*args):
        print("Warning: function '{0}' has been deprecated".format(func.__name__))
        func(*args)
    return wrapper
