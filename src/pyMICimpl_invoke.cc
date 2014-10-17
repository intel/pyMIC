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
 
#include <cstddef> 
#include <unordered_map>
#include <string>

#include "pyMICimpl_config.h"
#include "pyMICimpl_common.h"
#include "pyMICimpl_misc.h"
#include "pyMICimpl_invoke.h"
#include "pyMICimpl_buffers.h"

#include <pymic_kernel.h>

#include "debug.h"
#include <assert.h>

namespace pyMIC {

std::unordered_map<std::string, uintptr_t> kernel_cache[PYMIC_MAX_DEVICES];

void target_invoke_kernel(int device, std::string kernel_name, std::vector<std::pair<char *, size_t> > arguments) {
	debug_enter();
	int target = PYMIC_MAP_ANY_DEVICE(device);
	
	uintptr_t kernel_fct_ptr;
	int argc = arguments.size();
	uintptr_t *device_ptrs = new uintptr_t[argc];
	size_t *sizes = new size_t[argc];
	bool *present = new bool[argc];
	int i = 0;
		
	// find the function pointer for the kernel to invoke
	kernel_fct_ptr = kernel_cache[target][kernel_name];
	if (!kernel_fct_ptr) {
		// we have not found the kernel in the cache, so try to load its symbol
		kernel_fct_ptr = find_kernel(target, kernel_name);
		if (!kernel_fct_ptr) {
			// TODO: panic here if we do not find the proper kernel
			fprintf(stderr, "PANIC!!! could not find kernel function for kernel %s\n", kernel_name.c_str());
			assert(false);
		}
		else {
			// memorize the kernel's device pointer 
			kernel_cache[target][kernel_name] = kernel_fct_ptr;
		}
	}
	
	// check if all buffers have been allocated on the target already
	// if not, do copyin, copyout semantics
	for (auto it = arguments.begin(); it != arguments.end(); ++it,++i) {
		auto arg = *it;
		present[i] = !!buffer_lookup_device_ptr(target, arg.first);
		if (!present[i]) {
			debug(100, "buffer with backing store %p not available on target", arg.first);
			buffer_alloc_and_copy_to_target(target, arg.first, arg.second);
		}
	}	
	
	// construct the argument list with device pointers from the actual arguments
	i = 0;
	for (auto it = arguments.begin(); it != arguments.end(); ++it, ++i) {
		auto arg = *it;
		device_ptrs[i] = buffer_lookup_device_ptr(target, arg.first);
		sizes[i] = arg.second;
		debug(100, "invoke: argument %d: device ptr %p, size %ld", i, device_ptrs[i], sizes[i]);
	}
	
#pragma offload target(mic:target) in(argc) in(device_ptrs:length(argc)) in(kernel_fct_ptr) in(sizes:length(argc))
	{
		pymic_kernel_t kernel_ptr = reinterpret_cast<pymic_kernel_t>(kernel_fct_ptr);
		kernel_ptr(argc, device_ptrs, sizes);
	}
	
	// if we have copied any buffers with copyin/copyout retrieve them from the device
	for (i = 0; i < argc; ++i) {
		if (!present[i]) {
			auto arg = arguments[i];
			buffer_copy_from_target_and_release(target, arg.first, arg.second);
		}
	}
	
	delete[] device_ptrs;
	delete[] sizes;
	
	debug_leave();
}

} // namespace pyMIC