/* Copyright (c) 2014-2015, Intel Corporation All rights reserved. 
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
 
#include <cstddef> 
#include <unordered_map>
#include <string>

#include "pymicimpl_config.h"
#include "pymicimpl_common.h"
#include "pymicimpl_misc.h"
#include "pymicimpl_invoke.h"
#include "pymicimpl_buffers.h"

#include <pymic_kernel.h>

#include "debug.h"
#include <assert.h>

namespace pymic {

void target_invoke_kernel(const int device, const uintptr_t funcptr, const std::vector<std::pair<uintptr_t, size_t> >  & arguments) {
	debug_enter();
	int argc = arguments.size();
	uintptr_t *device_ptrs = new uintptr_t[argc];
	size_t *sizes = new size_t[argc];
	int i = 0;
		
    for (auto it = arguments.begin(); it != arguments.end(); ++it, ++i) {
        auto arg = *it;
        // Do pointer translation
#if PYMIC_USE_XSTREAM
        // TODO: implement the proper xstream translation here
#else
        // The real device pointer is stored in the descriptor object.
        buffer_descriptor * bd = reinterpret_cast<buffer_descriptor *>(arg.first);
        device_ptrs[i] = bd->pointer;
        assert(bd->size == arg.second);
#endif    
        sizes[i] = arg.second;
    }
    
#pragma offload target(mic:device) in(argc) in(device_ptrs:length(argc)) in(funcptr) in(sizes:length(argc))
	{
		pymic_kernel_t kernel_ptr = reinterpret_cast<pymic_kernel_t>(funcptr);
		kernel_ptr(argc, device_ptrs, sizes);
	}
	
	delete[] device_ptrs;
	delete[] sizes;
	
	debug_leave();
}

} // namespace pymic