# Copyright (c) 2014-2016, Intel Corporation All rights reserved. 
# 
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following conditions are 
# met: 
# 
# 1. Redistributions of source code must retain the above copyright 
# notice, this list of conditions and the following disclaimer. 
#
# 2. Redistributions in binary form must reproduce the above copyright 
# notice, this list of conditions and the following disclaimer in the 
# documentation and/or other materials provided with the distribution. 
#
# 3. Neither the name of the copyright holder nor the names of its 
# contributors may be used to endorse or promote products derived from 
# this software without specific prior written permission. 
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS 
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 

.PHONY: all clean realclean

ifeq ($(debug),0)
DEBUG=
else
# DEBUG=-DDEBUG
endif

LIBXSTREAM=./libxstream

PYMIC_SOURCE=pymic_internal.cc pymicimpl_misc.cc
LIBXSTREAM_SOURCE=$(shell ls $(LIBXSTREAM)/src/*.cpp)

PYMIC_OBJECTS=$(PYMIC_SOURCE:.cc=.o)
LIBXSTREAM_OBJECTS=$(LIBXSTREAM_SOURCE:.cpp=.o)

PYTHON_CONFIG=python3.4-config
PYTHON_INCLUDES+=$(shell $(PYTHON_CONFIG) --includes)

CYTHON=cython
CYTHONFLAGS=-3

CC=icc
CFLAGS=$(DEBUG) -DPYMIC_USE_XSTREAM=1 -DLIBXSTREAM_EXPORTED -offload=mandatory -Wall -pthread -g -O2 -ansi-alias -fPIC -I. -I../include -I$(LIBXSTREAM)/include $(PYTHON_INCLUDES)

CC_MIC=icc
CFLAGS_MIC=-DPYMIC_USE_XSTREAM=1 -I../include -fPIC -shared -mmic -g -O2 -o

CXX=icpc
CXXFLAGS=$(DEBUG) -DPYMIC_USE_XSTREAM=1 -DLIBXSTREAM_EXPORTED -offload=mandatory -std=c++0x -Wall -pthread -g -O2 -ansi-alias -fPIC -I. -I../include -I$(LIBXSTREAM)/include $(PYTHON_INCLUDES)

LD=icpc
LDFLAGS=-shared -pthread -offload-option,mic,link,"--no-undefined"

all: new_module

# old_module: ../pymic/_pymicimpl.so _pymicimpl.so ../pymic/liboffload_array.so liboffload_array.so
new_module: ../pymic/pymic_libxstream.so pymic_libxstream.so ../pymic/liboffload_array.so liboffload_array.so

pymic_libxstream.so: pymic_libxstream.o pymic_internal.o pymicimpl_misc.o $(LIBXSTREAM_OBJECTS)
	$(LD) $(LDFLAGS) -o $@ pymic_libxstream.o pymic_internal.o pymicimpl_misc.o $(LIBXSTREAM_OBJECTS)
	
../pymic/pymic_libxstream.so: pymic_libxstream.so
	cp pymic_libxstream.so ../pymic/pymic_libxstream.so

pymic_libxstream.c: pymic_libxstream.pyx
	$(CYTHON) $(CYTHONFLAGS) $<
	
liboffload_array.so: offload_array.c
	$(CC_MIC) $(CFLAGS_MIC) $@ $< -lm
	chmod a+rX liboffload_array.so

pymic_internal.cc: pymic_internal.h pymicimpl_misc.h
pymicimpl_misc.cc: pymicimpl_misc.h
offload_array.c: ../include/pymic_kernel.h
	
../pymic/liboffload_array.so: liboffload_array.so
	cp liboffload_array.so ../pymic/liboffload_array.so
	
clean:
	rm -f $(PYMIC_OBJECTS)
	rm -f $(LIBXSTREAM_OBJECTS)
	rm -f pymic_libxstream.c pymic_libxstream.o

realclean: clean
	rm -f *~ *.bak
	rm -f *.so
