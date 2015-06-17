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
#ifndef LIBXSTREAM_CONFIG_H
#define LIBXSTREAM_CONFIG_H

#ifndef LIBXSTREAM_MACROS_H
# error Do not include <libxstream_config.h> directly (use <libxstream_macros.h>)!
#endif

#if !defined(LIBXSTREAM_CONFIG_EXTERNAL)


/**
 * Debug-time error checks are usually disabled for production code (NDEBUG).
 * The LIBXSTREAM_DEBUG symbol ultimately controls this (see libxstream_macros.h).
 */
#define LIBXSTREAM_ERROR_DEBUG

/**
 * Runtime error checks and error handling code is usually enabled.
 * The LIBXSTREAM_CHECK symbol ultimately controls this (see libxstream_macros.h).
 */
#define LIBXSTREAM_ERROR_CHECK

/**
 * Enables printing trace information. Valid choices:
 * - #define LIBXSTREAM_TRACE: enables default (1) behavior
 * - #define LIBXSTREAM_TRACE 0: no trace information
 * - #define LIBXSTREAM_TRACE 1: enabled
 * - #define LIBXSTREAM_TRACE 2: enabled (debug build)
 * If the trace information is enabled, the environment variable
 * LIBXSTREAM_VERBOSITY can be used to adjust the verbosity level.
 */
#define LIBXSTREAM_TRACE

/**
 * Enables the kind of offload mechanism. Valid choices:
 * - #define LIBXSTREAM_ASYNC: enables default (1) behavior
 * - #define LIBXSTREAM_ASYNC 0: native (KNL) - not implemented yet / must be disabled
 * - #define LIBXSTREAM_ASYNC 1: synchronous offloads
 * - #define LIBXSTREAM_ASYNC 2: compiler offload
 * - #define LIBXSTREAM_ASYNC 3: compiler streams
 */
#define LIBXSTREAM_ASYNC

/** Not implemented yet. Must be disabled. */
/*#define LIBXSTREAM_ASYNCHOST*/

/** SIMD width in Byte (actual alignment might be smaller). */
#define LIBXSTREAM_MAX_SIMD 64

/** Alignment in Byte (actual alignment might be smaller). */
#define LIBXSTREAM_MAX_ALIGN (2 * 1024 * 1024)

/** Maximum number of devices (POT). */
#define LIBXSTREAM_MAX_NDEVICES 4

/** Maximum number of streams per device (POT). */
#define LIBXSTREAM_MAX_NSTREAMS 32

/** Maximum dimensionality of arrays. */
#define LIBXSTREAM_MAX_NDIMS 4

/** Maximum number of arguments in offload structure. */
#define LIBXSTREAM_MAX_NARGS 16

/** Maximum number of executions in the queue (POT). */
#define LIBXSTREAM_MAX_QSIZE 2048

/** Maximum number of host threads. */
#define LIBXSTREAM_MAX_NTHREADS 512

/** Maximum number of locks (POT). */
#define LIBXSTREAM_MAX_NLOCKS 16

/**
 * Number of CPU cycles to actively wait. A positive value translates to cpu cycles
 * whereas a negative value translates into milliseconds. A value of zero designates
 * a passive wait rather than polling for a condition. For example:
 * #define LIBXSTREAM_SPIN_CYCLES 2000000
 */
#define LIBXSTREAM_SPIN_CYCLES 2000000

/**
 * Duration a thread may sleep when waiting. A value of zero yields the thread.
 * For a non-zero value, a non-zero LIBXSTREAM_SPIN_CYCLES is highly recommended.
 * For example:
 * #define LIBXSTREAM_SLEEP_MS 20.
 */
#define LIBXSTREAM_SLEEP_MS 20

/** Allows client-side to sleep when waiting. */
#define LIBXSTREAM_SLEEP_CLIENT

/** Prefers OpenMP based locking primitives. */
/*#define LIBXSTREAM_PREFER_OPENMP*/

#endif /*LIBXSTREAM_CONFIG_EXTERNAL*/
#endif /*LIBXSTREAM_CONFIG_H*/
