/* Copyright (c) 2014-2016, Intel Corporation All rights reserved.
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
 
#ifndef PYMIC_DEBUG_H
#define PYMIC_DEBUG_H

#ifdef DEBUG

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>

#define DEBUG_SUFFIX     "PYMIC: "
#define DEBUG_SUFFIX_MIC "*MIC* "
#define DEBUG_BUFSZ      2048

static void debug(int level, const char *fmt, ...) {
	char buffer[DEBUG_BUFSZ];
	char *env;
    int debug_level;
	size_t pos = 0;
	env = getenv("PYMIC_DEBUG");
    if (env) {
        debug_level = atoi(env);
        if (level <= debug_level) {
            va_list vargs;
            va_start(vargs, fmt);
            pos += snprintf(buffer, DEBUG_BUFSZ - pos, DEBUG_SUFFIX);
#ifdef __MIC__	
            pos += snprintf(buffer + pos, DEBUG_BUFSZ - pos, DEBUG_SUFFIX_MIC);
#endif
            pos = vsnprintf(buffer + pos, DEBUG_BUFSZ - pos, fmt, vargs);
            fprintf(stderr, "%s\n", buffer);
            va_end(vargs);
        }
    }
}

static void debugmsg(int level, const char *msg) {
	debug(level, "%s", msg);
}

#define debug_enter() { \
	debug(1000, "entering %s at %s:%d", __FUNCTION__, __FILE__, __LINE__); \
}

#define debug_leave() { \
	debug(1000, "leaving %s at %s:%d", __FUNCTION__, __FILE__, __LINE__); \
}

#else

#define debug(fmt, ...)
#define debugmsg(msg)
#define debug_enter()
#define debug_leave()

#endif

#endif
