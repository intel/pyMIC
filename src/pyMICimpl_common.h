/* Copyright (c) 2014, Intel Corporation All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are 
 * met: 
 *
 * 1. Redistributions of source code must retain the above copyright 
 * notice, this list of conditions and the following disclaimer. 
 * 
 * 2. Redistributions in binary form must reproduce the above copyright 
 * notice, this list of conditions and the following disclaimer in the 
 * documentation and/or other materials provided with the distribution. 
 *
 * 3. Neither the name of the copyright holder nor the names of its 
 * contributors may be used to endorse or promote products derived from 
 * this software without specific prior written permission. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS 
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */
 
#ifndef PYMIC_COMMON_H
#define PYMIC_COMMON_H

#include "pyMICimpl_config.h"

// some definitions for much less typing
#define PYMIC_INTERFACE extern "C" static
#define PYMIC_ENTRYPOINT(entry, doc) 				\
	{"_" # entry, entry, METH_VARARGS, doc}
#define PYMIC_ENTRYPOINT_TERMINATOR 				\
	{NULL, NULL, 0, NULL}

// these macros map the "any" device to a physical device
#define PYMIC_ANY_DEVICE (-1)
#define PYMIC_MAP_ANY_DEVICE(device) 				\
	(((device) ==  PYMIC_ANY_DEVICE) ? PYMIC_DEFAULT_DEVICE : (device))
	
#endif 

