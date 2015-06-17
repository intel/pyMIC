/******************************************************************************
** Copyright (c) 2014-2015, Intel Corporation                                **
** All rights reserved.                                                      **
**                                                                           **
** Redistribution and use in source and binary forms, with or without        **
** modification, are permitted provided that the following conditions        **
** are met:                                                                  **
** 1. Redistributions of source code must retain the above copyright         **
**    notice, this list of conditions and the following disclaimer.          **
** 2. Redistributions in binary form must reproduce the above copyright      **
**    notice, this list of conditions and the following disclaimer in the    **
**    documentation and/or other materials provided with the distribution.   **
** 3. Neither the name of the copyright holder nor the names of its          **
**    contributors may be used to endorse or promote products derived        **
**    from this software without specific prior written permission.          **
**                                                                           **
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       **
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         **
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR     **
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT      **
** HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,    **
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED  **
** TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR    **
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    **
** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING      **
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS        **
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              **
******************************************************************************/
/* Hans Pabst (Intel Corp.)
******************************************************************************/
#ifndef LIBXSTREAM_H
#define LIBXSTREAM_H

#include "libxstream_macros.h"
#if defined(LIBXSTREAM_OFFLOAD)
# pragma offload_attribute(push,target(mic))
#endif
#include <stdint.h>
#include <stddef.h>
#if defined(__cplusplus)
# include <complex>
#endif
#if defined(LIBXSTREAM_OFFLOAD)
# pragma offload_attribute(pop)
#endif


/** Boolean state; must match LIBXSTREAM_TYPE_BOOL. */
typedef int libxstream_bool;
/** Stream type. */
LIBXSTREAM_EXPORT_C typedef struct libxstream_stream libxstream_stream;
/** Event type. */
LIBXSTREAM_EXPORT_C typedef struct libxstream_event libxstream_event;
/** Enumeration of elemental "scalar" types. */
LIBXSTREAM_EXPORT_C typedef enum libxstream_type {
  /** special types: BOOL, BYTE, CHAR, VOID */
  LIBXSTREAM_TYPE_CHAR,
  /** signed integer types: I8, I16, I32, I64 */
  LIBXSTREAM_TYPE_I8,
  LIBXSTREAM_TYPE_I16,
  LIBXSTREAM_TYPE_I32, LIBXSTREAM_TYPE_BOOL = LIBXSTREAM_TYPE_I32,
  LIBXSTREAM_TYPE_I64,
  /** unsigned integer types: U8, U16, U32, U64 */
  LIBXSTREAM_TYPE_U8, LIBXSTREAM_TYPE_BYTE = LIBXSTREAM_TYPE_U8,
  LIBXSTREAM_TYPE_U16,
  LIBXSTREAM_TYPE_U32,
  LIBXSTREAM_TYPE_U64,
  /** floating point types: F32, F64, C32, C64 */
  LIBXSTREAM_TYPE_F32,
  LIBXSTREAM_TYPE_F64,
  LIBXSTREAM_TYPE_C32,
  LIBXSTREAM_TYPE_C64,
  /** terminates type list */
  LIBXSTREAM_TYPE_VOID,
  LIBXSTREAM_TYPE_INVALID
} libxstream_type;
/** Function call behavior (flags valid for binary combination). */
LIBXSTREAM_EXPORT_C typedef enum libxstream_call_flags {
  LIBXSTREAM_CALL_WAIT    = 1 /* synchronous function call */,
  LIBXSTREAM_CALL_NATIVE  = 2 /* native host/MIC function */,
  /** terminates the list */
  LIBXSTREAM_CALL_INVALID,
  /** collection of any valid flags from above */
  LIBXSTREAM_CALL_DEFAULT = 0
} libxstream_call_flags;
/** Function argument type. */
LIBXSTREAM_EXPORT_C typedef struct LIBXSTREAM_TARGET(mic) libxstream_argument libxstream_argument;
/** Function type of an offloadable function. */
typedef void (/*LIBXSTREAM_CDECL*/*libxstream_function)(LIBXSTREAM_VARIADIC);

/** Query the number of available devices. */
LIBXSTREAM_EXPORT_C int libxstream_get_ndevices(size_t* ndevices);
/** Query the device set active for this thread. */
LIBXSTREAM_EXPORT_C int libxstream_get_active_device(int* device);
/** Set the active device for this thread. */
LIBXSTREAM_EXPORT_C int libxstream_set_active_device(int device);

/** Query the memory metrics of the device (valid to pass one NULL pointer). */
LIBXSTREAM_EXPORT_C int libxstream_mem_info(int device, size_t* allocatable, size_t* physical);
/** Allocate aligned memory (0: automatic alignment) on the device (-1: host, 0<: coprocessor). */
LIBXSTREAM_EXPORT_C int libxstream_mem_allocate(int device, void** memory, size_t size, size_t alignment);
/** Deallocate memory; shall match the device where the memory was allocated. */
LIBXSTREAM_EXPORT_C int libxstream_mem_deallocate(int device, const void* memory);
/** Fill memory with zeros; allocated memory can carry an offset. */
LIBXSTREAM_EXPORT_C int libxstream_memset_zero(void* memory, size_t size, libxstream_stream* stream);
/** Copy memory from the host to the device; addresses can carry an offset. */
LIBXSTREAM_EXPORT_C int libxstream_memcpy_h2d(const void* host_mem, void* dev_mem, size_t size, libxstream_stream* stream);
/** Copy memory from the device to the host; addresses can carry an offset. */
LIBXSTREAM_EXPORT_C int libxstream_memcpy_d2h(const void* dev_mem, void* host_mem, size_t size, libxstream_stream* stream);
/** Copy memory from device to device; cross-device copies are allowed as well. */
LIBXSTREAM_EXPORT_C int libxstream_memcpy_d2d(const void* src, void* dst, size_t size, libxstream_stream* stream);

/** Query the range of valid priorities (inclusive bounds). */
LIBXSTREAM_EXPORT_C int libxstream_stream_priority_range(int* least, int* greatest);
/** Create a stream on a given device. */
LIBXSTREAM_EXPORT_C int libxstream_stream_create(libxstream_stream** stream, int device, int priority, const char* name);
/** Destroy a stream; pending work must be completed if results are needed. */
LIBXSTREAM_EXPORT_C int libxstream_stream_destroy(const libxstream_stream* stream);
/** Wait for pending work (blocking); NULL to synchronize all streams. */
LIBXSTREAM_EXPORT_C int libxstream_stream_wait(libxstream_stream* stream);
/** Wait for an event inside of the specified stream; a NULL-stream designates all streams. */
LIBXSTREAM_EXPORT_C int libxstream_stream_wait_event(libxstream_stream* stream, const libxstream_event* event);
/** Query the device the given stream is constructed for. */
LIBXSTREAM_EXPORT_C int libxstream_stream_device(const libxstream_stream* stream, int* device);

/** Create an event; event can be recorded multiple times. */
LIBXSTREAM_EXPORT_C int libxstream_event_create(libxstream_event** event);
/** Destroy an event; does not implicitly wait for the event to complete. */
LIBXSTREAM_EXPORT_C int libxstream_event_destroy(const libxstream_event* event);
/** Record an event; an event can be re-recorded multiple times. */
LIBXSTREAM_EXPORT_C int libxstream_event_record(libxstream_event* event, libxstream_stream* stream);
/** Check whether an event has occurred or not (non-blocking). */
LIBXSTREAM_EXPORT_C int libxstream_event_query(const libxstream_event* event, libxstream_bool* occurred);
/** Wait for the event i.e., waiting for work queued prior to recording the event. */
LIBXSTREAM_EXPORT_C int libxstream_event_wait(libxstream_event* event);

/** Receive thread-local signature with capacity of LIBXSTREAM_MAX_NARGS. */
LIBXSTREAM_EXPORT_C int libxstream_fn_signature(libxstream_argument** signature);
/** Reset function signature to start over with less arguments (arity). */
LIBXSTREAM_EXPORT_C int libxstream_fn_clear_signature(libxstream_argument* signature);
/** Construct an input argument from device data, dimensionality, and shape. */
LIBXSTREAM_EXPORT_C int libxstream_fn_input(libxstream_argument* signature, size_t arg, const void* in, libxstream_type type, size_t dims, const size_t shape[]);
/** Construct an output argument from device data, dimensionality, and shape. */
LIBXSTREAM_EXPORT_C int libxstream_fn_output(libxstream_argument* signature, size_t arg, void* out, libxstream_type type, size_t dims, const size_t shape[]);
/** Construct an in-out argument from device data, dimensionality, and shape. */
LIBXSTREAM_EXPORT_C int libxstream_fn_inout(libxstream_argument* signature, size_t arg, void* inout, libxstream_type type, size_t dims, const size_t shape[]);
/** Query the capacity of the function signature (maximum possible arity). */
LIBXSTREAM_EXPORT_C int libxstream_fn_nargs(const libxstream_argument* signature, size_t* nargs);
/** Call a user function along with the signature. */
LIBXSTREAM_EXPORT_C int libxstream_fn_call(libxstream_function function, const libxstream_argument* signature, libxstream_stream* stream, int flags);

/** Query the size of the elemental type (Byte). */
LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_typesize(libxstream_type type, size_t* typesize);
/** Select a type according to the typesize; suiteable to transport the requested amount of Bytes. */
LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_autotype(size_t typesize, libxstream_type start, libxstream_type* autotype);
/** Query the name of the elemental type (string does not need to be buffered). */
LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_typename(libxstream_type type, const char** name);
/** Query the argument's 0-based position within the signature; needs a pointer variable (not from a by-value variable). */
LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_argument(const void* variable, size_t* arg);
/** Query the arity of the function signature (actual number of arguments). A NULL-signature designates the call context. */
LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_arity(const libxstream_argument* signature, size_t* arity);
/** Query the argument's data. A NULL-signature designates the call context. */
LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_data(const libxstream_argument* signature, size_t arg, const void** data);
/** Query a textual representation; thread safe (valid until next call). A NULL-signature designates the call context. */
LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_string(const libxstream_argument* signature, size_t arg, const char** value);
/** Query the elemental type of the argument. A NULL-signature designates the call context. */
LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_type(const libxstream_argument* signature, size_t arg, libxstream_type* type);
/** Query the dimensionality of the argument; an elemental arg. is 0-dimensional. A NULL-signature designates the call context. */
LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_dims(const libxstream_argument* signature, size_t arg, size_t* dims);
/** Query the extent of the argument; an elemental argument has an 0-extent. A NULL-signature designates the call context. */
LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_shape(const libxstream_argument* signature, size_t arg, size_t shape[]);
/** Query the number of elements of the argument. A NULL-signature designates the call context. */
LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_size(const libxstream_argument* signature, size_t arg, size_t* size);
/** Query the size of the element type of the argument (Byte). A NULL-signature designates the call context. */
LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_elemsize(const libxstream_argument* signature, size_t arg, size_t* size);
/** Query the data size of the argument (Byte). A NULL-signature designates the call context. */
LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_datasize(const libxstream_argument* signature, size_t arg, size_t* size);

/** Query the internal verbosity level. */
LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_get_verbosity(int* level);
/** Set the internal verbosity level. */
LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_set_verbosity(int level);
/** Prints to the standard error stream. Locks the stream in order to avoid intermixed characters. */
LIBXSTREAM_EXPORT_C LIBXSTREAM_TARGET(mic) int libxstream_print(int verbosity, const char* message, ...);

#if defined(__cplusplus)
template<typename TYPE> struct libxstream_map_to { static libxstream_type type() {/** select a type by type-size; bool goes here! */
     libxstream_type autotype = LIBXSTREAM_TYPE_VOID; libxstream_get_autotype(sizeof(TYPE), LIBXSTREAM_TYPE_CHAR, &autotype); return autotype;   } };
template<> struct libxstream_map_to<int8_t>                                       { static libxstream_type type() { return LIBXSTREAM_TYPE_I8;   } };
template<> struct libxstream_map_to<uint8_t>                                      { static libxstream_type type() { return LIBXSTREAM_TYPE_U8;   } };
template<> struct libxstream_map_to<int16_t>                                      { static libxstream_type type() { return LIBXSTREAM_TYPE_I16;  } };
template<> struct libxstream_map_to<uint16_t>                                     { static libxstream_type type() { return LIBXSTREAM_TYPE_U16;  } };
template<> struct libxstream_map_to<int32_t>                                      { static libxstream_type type() { return LIBXSTREAM_TYPE_I32;  } };
template<> struct libxstream_map_to<uint32_t>                                     { static libxstream_type type() { return LIBXSTREAM_TYPE_U32;  } };
template<> struct libxstream_map_to<int64_t>                                      { static libxstream_type type() { return LIBXSTREAM_TYPE_I64;  } };
template<> struct libxstream_map_to<uint64_t>                                     { static libxstream_type type() { return LIBXSTREAM_TYPE_U64;  } };
template<> struct libxstream_map_to<float>                                        { static libxstream_type type() { return LIBXSTREAM_TYPE_F32;  } };
template<> struct libxstream_map_to<double>                                       { static libxstream_type type() { return LIBXSTREAM_TYPE_F64;  } };
template<> struct libxstream_map_to<float[2]>                                     { static libxstream_type type() { return LIBXSTREAM_TYPE_C32;  } };
template<> struct libxstream_map_to<double[2]>                                    { static libxstream_type type() { return LIBXSTREAM_TYPE_C64;  } };
template<> struct libxstream_map_to<std::complex<float> >                         { static libxstream_type type() { return LIBXSTREAM_TYPE_C32;  } };
template<> struct libxstream_map_to<std::complex<double> >                        { static libxstream_type type() { return LIBXSTREAM_TYPE_C64;  } };
template<> struct libxstream_map_to<char>                                         { static libxstream_type type() { return LIBXSTREAM_TYPE_CHAR; } };
template<> struct libxstream_map_to<void>                                         { static libxstream_type type() { return LIBXSTREAM_TYPE_VOID; } };
template<typename TYPE> struct libxstream_map_to<TYPE*>                           { static libxstream_type type() { return libxstream_map_to<TYPE>::type(); } };
template<typename TYPE> struct libxstream_map_to<const TYPE*>                     { static libxstream_type type() { return libxstream_map_to<TYPE>::type(); } };
template<typename TYPE> struct libxstream_map_to<TYPE**>                          { static libxstream_type type() { return libxstream_map_to<uintptr_t>::type(); } };
template<typename TYPE> struct libxstream_map_to<const TYPE**>                    { static libxstream_type type() { return libxstream_map_to<uintptr_t>::type(); } };
template<typename TYPE> struct libxstream_map_to<TYPE*const*>                     { static libxstream_type type() { return libxstream_map_to<uintptr_t>::type(); } };
template<typename TYPE> struct libxstream_map_to<const TYPE*const*>               { static libxstream_type type() { return libxstream_map_to<uintptr_t>::type(); } };

template<typename TYPE> libxstream_type libxstream_map_to_type(const TYPE&)       { return libxstream_map_to<TYPE>::type(); }
template<typename TYPE> libxstream_type libxstream_map_to_type(TYPE*)             { return libxstream_map_to<TYPE*>::type(); }
template<typename TYPE> libxstream_type libxstream_map_to_type(const TYPE*)       { return libxstream_map_to<const TYPE*>::type(); }
template<typename TYPE> libxstream_type libxstream_map_to_type(TYPE**)            { return libxstream_map_to<TYPE**>::type(); }
template<typename TYPE> libxstream_type libxstream_map_to_type(const TYPE**)      { return libxstream_map_to<const TYPE**>::type(); }
template<typename TYPE> libxstream_type libxstream_map_to_type(TYPE*const*)       { return libxstream_map_to<TYPE*const*>::type(); }
template<typename TYPE> libxstream_type libxstream_map_to_type(const TYPE*const*) { return libxstream_map_to<const TYPE*const*>::type(); }

template<libxstream_type VALUE> struct libxstream_map_from                        { /** compile-time error expected */ };
template<> struct libxstream_map_from<LIBXSTREAM_TYPE_I8>                         { typedef int8_t type; };
template<> struct libxstream_map_from<LIBXSTREAM_TYPE_U8>                         { typedef uint8_t type; };
template<> struct libxstream_map_from<LIBXSTREAM_TYPE_I16>                        { typedef int16_t type; };
template<> struct libxstream_map_from<LIBXSTREAM_TYPE_U16>                        { typedef uint16_t type; };
template<> struct libxstream_map_from<LIBXSTREAM_TYPE_I32>                        { typedef int32_t type; };
template<> struct libxstream_map_from<LIBXSTREAM_TYPE_U32>                        { typedef uint32_t type; };
template<> struct libxstream_map_from<LIBXSTREAM_TYPE_I64>                        { typedef int64_t type; };
template<> struct libxstream_map_from<LIBXSTREAM_TYPE_U64>                        { typedef uint64_t type; };
template<> struct libxstream_map_from<LIBXSTREAM_TYPE_F32>                        { typedef float type; };
template<> struct libxstream_map_from<LIBXSTREAM_TYPE_F64>                        { typedef double type; };
template<> struct libxstream_map_from<LIBXSTREAM_TYPE_C32>                        { typedef std::complex<float>  type; typedef float  ctype[2]; };
template<> struct libxstream_map_from<LIBXSTREAM_TYPE_C64>                        { typedef std::complex<double> type; typedef double ctype[2]; };
template<> struct libxstream_map_from<LIBXSTREAM_TYPE_CHAR>                       { typedef char type; };
template<> struct libxstream_map_from<LIBXSTREAM_TYPE_VOID>                       { typedef void type; };
#endif /*__cplusplus*/

#endif /*LIBXSTREAM_H*/
