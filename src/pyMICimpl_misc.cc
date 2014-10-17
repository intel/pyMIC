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

#pragma offload_attribute(push, target(mic))
	#include <string>
	#include <unordered_map>
#pragma offload_attribute(pop)

#include <cstdio>
#include <cstring>
#include <dlfcn.h>
 
#include "pyMICimpl_common.h"
#include "pyMICimpl_misc.h"

#include <pymic_kernel.h>

#include "debug.h"

namespace pyMIC {

int get_number_of_devices() {
	return _Offload_number_of_devices();
}

std::unordered_map<std::string,uintptr_t> loaded_libraries[PYMIC_MAX_DEVICES];

bool target_load_library(int device, const std::string &library, std::string &error) {
	debug_enter();

	int target = PYMIC_MAP_ANY_DEVICE(device);
	const size_t bufsz = 256;
	char buffer[bufsz];
	const char *library_cstr = library.c_str();
	size_t library_sz = strlen(library_cstr) + 1;
	uintptr_t handle_device_ptr;
	
	
	debug(10, "loading library '%s' on target", library_cstr);
	
#pragma offload target(mic:target) in(library_cstr:length(library_sz)) out(handle_device_ptr) in(bufsz) 
	{
		// load library and register
		void *handle = NULL;
		handle = dlopen(library_cstr, RTLD_NOW | RTLD_GLOBAL);
		if (!handle) {
			snprintf(buffer, bufsz, "dlopen failed (error: '%s')", dlerror());
			// TODO: remove the following two lines in the final code
			printf("LD_LIBRARY_PATH: '%s'\n", getenv("LD_LIBRARY_PATH"));
			printf("MIC_LD_LIBRARY_PATH: '%s'\n", getenv("MIC_LD_LIBRARY_PATH"));
		}
		handle_device_ptr = reinterpret_cast<uintptr_t>(handle);
	}

	if (!handle_device_ptr) {
		error = buffer;
		debug(10, "library load for '%s' failed on target: %s", library_cstr, buffer);
	}
	else {
		loaded_libraries[device][library] = handle_device_ptr;
		debug(10, "library load for '%s' succeeded on target, handle %p", library_cstr, handle_device_ptr);
	}
	
	debug_leave();
	return !!handle_device_ptr;
}

uintptr_t find_kernel(int device, const std::string &kernel_name) {
	int target = PYMIC_MAP_ANY_DEVICE(device);
	const size_t bufsz = 256;
	char buffer[bufsz];
	const char *kernel_cstr = kernel_name.c_str();
	size_t kernel_sz = strlen(kernel_cstr) + 1;
	uintptr_t fct_ptr = NULL;
	
	for (auto it = loaded_libraries[device].begin(); it != loaded_libraries[device].end(); ++it) {
		auto entry = *it;
		uintptr_t handle_device_ptr = entry.second;
		debug(100, "looking for '%s' in '%s' (handle %p)", kernel_cstr, entry.first.c_str(), handle_device_ptr);
#pragma offload target(mic:target) in(kernel_cstr:length(kernel_sz)) in(handle_device_ptr) out(fct_ptr) 
		{
			void *handle = reinterpret_cast<void*>(handle_device_ptr);
			void *ptr = NULL;
			dlerror();
			ptr = dlsym(handle, kernel_cstr);
			dlerror();
			if (!ptr) {
				dlerror();
			}
			else {
				fct_ptr = reinterpret_cast<uintptr_t>(ptr);
			}
		}
	}

	debug(100, "function pointer for '%s': %p", kernel_cstr, fct_ptr);
	
	return fct_ptr;
}

} // namespace pyMIC