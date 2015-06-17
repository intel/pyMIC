#ifndef PYMIC_INTERNAL_H
#define PYMIC_INTERNAL_H

#include <stdint.h>

#include <libxstream.h>

#if defined(__cplusplus)
extern "C" {
#endif

int pymic_internal_invoke_kernel(int device, libxstream_stream *stream, 
                                 void *funcptr, size_t argc, 
                                 const int64_t *dims, const int64_t *types, 
                                 void **ptrs, const size_t *sizes);
int pymic_internal_load_library(int device, const char *filename, 
                                int64_t *handle, char **tempfile);
int pymic_internal_unload_library(int device, int64_t handle, const char *tempfile);
int pymic_internal_find_kernel(int device, int64_t handle, const char *kernel_name, int64_t *kernel_ptr);
    
#if defined(__cplusplus)
}
#endif

#endif
