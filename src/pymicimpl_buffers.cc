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

#include <stdio.h>
 
#include "pymicimpl_config.h"
#include "pymicimpl_common.h"
#include "pymicimpl_buffers.h"
#include "pymicimpl_misc.h"

#if PYMIC_USE_XSTREAM
#include "libxstream.h"
#endif

#include "debug.h"
 
namespace pymic {


unsigned char * buffer_allocate(int device, void * stream, size_t size, size_t alignment) {
	debug_enter();
	uintptr_t host_ptr = 0;
	uintptr_t device_ptr = 0;
    unsigned char * dummy = NULL;
    
#if PYMIC_USE_XSTREAM
    void * memory = NULL;
    if (libxstream_mem_allocate(device, &memory, size, alignment) != LIBXSTREAM_ERROR_NONE) {
        printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
        throw internal_exception("XSTREAM FAILED", __FILE__, __LINE__);
    }
    dummy = static_cast<unsigned char *>(memory);
#else    
    // We fake the allocation by allocating a descriptor in the host memory
    // and let LEO use it as the host part of the device allocation
    dummy = static_cast<unsigned char *>(mmap(0, std::max(size, sizeof(buffer_descriptor)), 
                                              PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    host_ptr = reinterpret_cast<uintptr_t>(dummy);
#pragma offload target(mic:device) out(device_ptr) nocopy(dummy:length(size) align(alignment) alloc_if(1) free_if(0))
	{
		device_ptr = reinterpret_cast<uintptr_t>(dummy);
    }
    
	// record the buffer's address on the target device so that we can find it later
	debug(100, "%s: recording device pointer 0x%lx for host pointer 0x%lx", 
	      __FUNCTION__, device_ptr, host_ptr);
    buffer_descriptor * bd = reinterpret_cast<buffer_descriptor *>(dummy);
    bd->pointer = device_ptr;
    bd->size = size;
#endif
	
    debug_leave();
    return dummy;
}


void buffer_release(int device, void * stream, unsigned char * dummy) {
	debug_enter();
	uintptr_t host_ptr = reinterpret_cast<uintptr_t>(dummy);
    size_t size;
    
#if PYMIC_USE_XSTREAM    
    if (libxstream_mem_deallocate(device, dummy) != LIBXSTREAM_ERROR_NONE) {
        printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
        throw internal_exception("XSTREAM FAILED", __FILE__, __LINE__);
    }
#else    
    buffer_descriptor * bd = reinterpret_cast<buffer_descriptor *>(dummy);
    size = bd->size;

#pragma offload_transfer target(mic:device) nocopy(dummy:length(size) alloc_if(0) free_if(1))
    
	debug(100, "%s: removing device pointer for host pointer 0x%lx", 
	      __FUNCTION__, host_ptr);
    munmap(const_cast<unsigned char *>(dummy), size);
#endif
    
    debug_leave();
}


void buffer_copy_to_target(int device, void * stream, 
                           unsigned char * src, unsigned char * dst, size_t size, 
                           size_t offset_host, size_t offset_device) {
	debug_enter();
	uintptr_t hptr = reinterpret_cast<uintptr_t>(src);
    uintptr_t dptr = *reinterpret_cast<uintptr_t *>(dst);

    unsigned char * src_offs = src + offset_host;
    unsigned char * dst_offs = dst + offset_device;

    debug(100, "%s: transferring %ld bytes from host pointer 0x%lp into device pointer 0x%lx",
          __FUNCTION__, size, hptr, dptr);

#if PYMIC_USE_XSTREAM
    if (libxstream_memcpy_h2d(src_offs, dst_offs, size, static_cast<libxstream_stream *>(stream)) != LIBXSTREAM_ERROR_NONE) {
        printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
        throw internal_exception("XSTREAM FAILED", __FILE__, __LINE__);
    }
#else    
    buffer_descriptor *bd = reinterpret_cast<buffer_descriptor *>(dst);
    

#pragma offload_transfer target(mic:device) \
        in(src_offs:length(size) into(dst_offs) alloc_if(0) free_if(0))
#endif

	// we do not need to update the buffer map here, the buffers stay in their current state
	debug_leave();
}


void buffer_copy_to_host(int device, void * stream, 
                         unsigned char * src, unsigned char * dst, size_t size,
                         size_t offset_device, size_t offset_host) {
	debug_enter();

	uintptr_t hptr = reinterpret_cast<uintptr_t>(dst);
    uintptr_t dptr = *reinterpret_cast<uintptr_t *>(src);
    
    unsigned char * src_offs = src + offset_device;
    unsigned char * dst_offs = dst + offset_host;
    
    debug(100, "%s: transferring %ld bytes from device pointer 0x%lp into host pointer 0x%lx",
          __FUNCTION__, size, dptr, hptr);
#if PYMIC_USE_XSTREAM
    if (libxstream_memcpy_d2h(src_offs, dst_offs, size, static_cast<libxstream_stream *>(stream)) != LIBXSTREAM_ERROR_NONE) {
        printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
        throw internal_exception("XSTREAM FAILED", __FILE__, __LINE__);
    }
#else 
#pragma offload_transfer target(mic:device) \
        out(src_offs:length(size) into(dst_offs) alloc_if(0) free_if(0))
#endif

    // we do not need to update the buffer map here, the buffers stay in their current state
	debug_leave();
}


void buffer_copy_on_device(int device, void * stream, 
                           unsigned char * src, unsigned char * dst, size_t size,
                           size_t offset_device_src, size_t offset_device_dst) {
	debug_enter();

#if PYMIC_USE_XSTREAM
    unsigned char * src_ptr = src + offset_device_src;
    unsigned char * dst_ptr = dst + offset_device_dst;
    if (libxstream_memcpy_d2d(src_ptr, dst_ptr, size, static_cast<libxstream_stream *>(stream)) != LIBXSTREAM_ERROR_NONE) {
        printf("WHOOOOP at %s:%d\n", __FUNCTION__, __LINE__);
        throw internal_exception("XSTREAM FAILED", __FILE__, __LINE__);
    }
#else          
    buffer_descriptor *bd_src = reinterpret_cast<buffer_descriptor *>(src);
    buffer_descriptor *bd_dst = reinterpret_cast<buffer_descriptor *>(dst);
    
    debug(100, "%s: copying %ld bytes from device pointer 0x%lx to device pointer 0x%lx",
          __FUNCTION__, size, bd_src->pointer, bd_dst->pointer);

          uintptr_t src_ptr = bd_dst->pointer + offset_device_src;
    uintptr_t dst_ptr = bd_src->pointer + offset_device_dst;
#pragma offload target(mic:device) in(size) in(src_ptr) in(dst_ptr)
    {
        // TODO: we might want to execute this statement in parallel 
        //       to obtain the full memory bandwidth of KNC
        memcpy(reinterpret_cast<void *>(src_ptr), 
               reinterpret_cast<void *>(dst_ptr), 
               size);
    }
#endif

    // we do not need to update the buffer map here, the buffers stay in their current state
	debug_leave();
}


uintptr_t buffer_translate_pointer(unsigned char * device_ptr) {
    debug_enter();
    
    buffer_descriptor *bd = reinterpret_cast<buffer_descriptor *>(device_ptr);
    
    debug_leave();
    return bd->pointer;
}

} // namespace pymic
