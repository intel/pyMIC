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

#include <unordered_map>
 
#include "pyMICimpl_config.h"
#include "pyMICimpl_common.h"
#include "pyMICimpl_buffers.h"

#include "debug.h"
 
namespace pyMIC {

static std::unordered_map<uintptr_t, uintptr_t> buffers[PYMIC_MAX_DEVICES];

uintptr_t buffer_lookup_device_ptr(int device, void* host_ptr) {
	int target = PYMIC_MAP_ANY_DEVICE(device);
	return buffer_lookup_device_ptr(device, reinterpret_cast<uintptr_t>(host_ptr));
}
 
uintptr_t buffer_lookup_device_ptr(int device, uintptr_t host_ptr) {
	int target = PYMIC_MAP_ANY_DEVICE(device);
	return buffers[target][host_ptr];
} 
 
void buffer_allocate(int device, char* backing_store, size_t size) {
	debug_enter();
	int target = PYMIC_MAP_ANY_DEVICE(device);
	uintptr_t host_ptr = reinterpret_cast<uintptr_t>(backing_store);
	uintptr_t device_ptr = 0;
#pragma offload target(mic:target) out(device_ptr) nocopy(backing_store:length(size) align(64) alloc_if(1) free_if(0))
	{
		device_ptr = reinterpret_cast<uintptr_t>(backing_store);
	}
	// record the buffers address on the target device so that we can find it later
	debug(100, "%s: recording device pointer 0x%lx for host pointer 0x%lx", 
	      __FUNCTION__, device_ptr, host_ptr);
	buffers[target][host_ptr] = device_ptr;
	debug_leave();
}

void buffer_release(int device, char* backing_store, size_t size) {
	debug_enter();
	int target = PYMIC_MAP_ANY_DEVICE(device);
	uintptr_t host_ptr = reinterpret_cast<uintptr_t>(backing_store);
#ifdef UNOPTIMIZED_TRANSFERS
	uintptr_t device_ptr = 0;
#pragma offload target(mic:target) out(device_ptr) nocopy(backing_store:length(size) align(64) alloc_if(0) free_if(1))
	{
		device_ptr = reinterpret_cast<uintptr_t>(backing_store);
	}
#else
#pragma offload_transfer target(mic:target) nocopy(backing_store:length(size) align(64) alloc_if(0) free_if(1))
#endif
	// the buffer has been deallocated on the target, so remove it from the buffer tracker
	debug(100, "%s: removing device pointer for host pointer 0x%lx", 
	      __FUNCTION__, host_ptr);
	buffers[target].erase(host_ptr);
	debug_leave();
}

void buffer_update_on_target(int device, char* backing_store, size_t size) {
	debug_enter();
	int target = PYMIC_MAP_ANY_DEVICE(device);
	uintptr_t host_ptr = reinterpret_cast<uintptr_t>(backing_store);
#ifdef UNOPTIMIZED_TRANSFERS
	uintptr_t device_ptr = 0;
#pragma offload target(mic:target) out(device_ptr) in(backing_store:length(size) align(64) alloc_if(0) free_if(0))
	{
		device_ptr = reinterpret_cast<uintptr_t>(backing_store);
	}
#else
#pragma offload_transfer target(mic:target) in(backing_store:length(size) align(64) alloc_if(0) free_if(0))
#endif
	// we do not need to update the buffer map here, the buffers stay in their current state
	debug_leave();
}

void buffer_update_on_host(int device, char* backing_store, size_t size) {
	debug_enter();
	int target = PYMIC_MAP_ANY_DEVICE(device);
	uintptr_t host_ptr = reinterpret_cast<uintptr_t>(backing_store);
#ifdef UNOPTIMIZED_TRANSFERS
	uintptr_t device_ptr = 0;
#pragma offload target(mic:target) out(device_ptr) out(backing_store:length(size) align(64) alloc_if(0) free_if(0))
	{
		device_ptr = reinterpret_cast<uintptr_t>(backing_store);
	}
#else
#pragma offload_transfer target(mic:target) out(backing_store:length(size) align(64) alloc_if(0) free_if(0))
#endif
	// we do not need to update the buffer map here, the buffers stay in their current state
	debug_leave();
}

void buffer_alloc_and_copy_to_target(int device, char* backing_store, size_t size) {
	debug_enter();
	int target = PYMIC_MAP_ANY_DEVICE(device);
	uintptr_t host_ptr = reinterpret_cast<uintptr_t>(backing_store);
	uintptr_t device_ptr = 0;
#pragma offload target(mic:target) out(device_ptr) in(backing_store:length(size) align(64) alloc_if(1) free_if(0))
	{
		device_ptr = reinterpret_cast<uintptr_t>(backing_store);
	}
	// a buffer as been allocated, so add it to the tracker
	debug(100, "%s: recording device pointer 0x%lx for host pointer 0x%lx", 
	      __FUNCTION__, device_ptr, host_ptr);
	buffers[target][host_ptr] = device_ptr;
	debug_leave();
}

void buffer_copy_from_target_and_release(int device, char* backing_store, size_t size) {
	debug_enter();
	int target = PYMIC_MAP_ANY_DEVICE(device);
	uintptr_t host_ptr = reinterpret_cast<uintptr_t>(backing_store);
#ifdef UNOPTIMIZED_TRANSFERS    
	uintptr_t device_ptr = 0;
#pragma offload target(mic:target) out(device_ptr) out(backing_store:length(size) align(64) alloc_if(0) free_if(1))
	{
		device_ptr = reinterpret_cast<uintptr_t>(backing_store);
	}
#else
#pragma offload_transfer target(mic:target) out(backing_store:length(size) align(64) alloc_if(0) free_if(1))
#endif
	// the buffer has been deallocated on the target, so remove it from the buffer tracker
	debug(100, "%s: removing device pointer 0x%lx for host pointer 0x%lx", 
	      __FUNCTION__, buffers[target][host_ptr], host_ptr);
	buffers[target].erase(host_ptr);
	debug_leave();
}

} // namespace pyMIC