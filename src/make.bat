@echo off 

REM # Copyright (c) 2014-2016, Intel Corporation All rights reserved. 
REM # 
REM # Redistribution and use in source and binary forms, with or without 
REM # modification, are permitted provided that the following conditions are 
REM # met: 
REM # 
REM # 1. Redistributions of source code must retain the above copyright 
REM # notice, this list of conditions and the following disclaimer. 
REM #
REM # 2. Redistributions in binary form must reproduce the above copyright 
REM # notice, this list of conditions and the following disclaimer in the 
REM # documentation and/or other materials provided with the distribution. 
REM #
REM # 3. Neither the name of the copyright holder nor the names of its 
REM # contributors may be used to endorse or promote products derived from 
REM # this software without specific prior written permission. 
REM #
REM # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS 
REM # IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
REM # TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
REM # PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
REM # HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
REM # SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
REM # TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
REM # PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
REM # LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
REM # NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
REM # SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

REM build LIBXSTREAM
FOR %%s IN (libxstream.cpp libxstream_alloc.cpp libxstream_argument.cpp libxstream_context.cpp libxstream_event.cpp libxstream_offload.cpp libxstream_stream.cpp libxstream_workitem.cpp libxstream_workqueue.cpp) DO icl /nologo /EHsc /MD /GR- /DLIBXSTREAM_EXPORTED /Qoffload=mandatory /Qoffload-option,mic,compiler,"-fPIC" /O2 /Qansi-alias /Ilibxstream\include /c /Fo libxstream\src\%%s

REM build the Cython code
echo pymic_libxstream.pyx
cython -2 pymic_libxstream.pyx
icl /nologo /Qwd167 /EHsc /MD /GR- /DLIBXSTREAM_EXPORTED /O2 /Qansi-alias /I. /IC:\Anaconda\include /Ilibxstream\include /c pymic_libxstream.c

REM build supplementary sourcs
icl /nologo /EHsc /MD /GR- /DLIBXSTREAM_EXPORTED /DPYMIC_LOAD_LIBRARY_WORKAROUND /Qoffload=mandatory /Qoffload-option,mic,compiler,"-fPIC" /O2 /Qansi-alias /I..\include /Ilibxstream\include /c /Fo pymic_internal.cc
icl /nologo /EHsc /MD /GR- /DLIBXSTREAM_EXPORTED /DPYMIC_LOAD_LIBRARY_WORKAROUND /Qoffload=mandatory /Qoffload-option,mic,compiler,"-fPIC" /O2 /Qansi-alias /I..\include /Ilibxstream\include /c /Fo pymicimpl_misc.cc

REM build supporting kernel library
echo offload_array.c
icl -nologo -Qmic -I..\include -O2 -fPIC -shared -o liboffload_array.so offload_array.c

REM link everything
icl /nologo /Qoffload-option,mic,link,"--no-undefined -lpthread" /LD pymic_libxstream.obj libxstream.obj libxstream_alloc.obj libxstream_argument.obj libxstream_context.obj libxstream_event.obj libxstream_offload.obj libxstream_stream.obj libxstream_workitem.obj libxstream_workqueue.obj pymic_internal.obj pymicimpl_misc.obj c:\anaconda\libs\python27.lib  
copy /B pymic_libxstream.dll pymic_libxstream.pyd > NUL
copy /B pymic_libxstream.pyd ..\pymic > NUL
del pymic_libxstream.dll > NUL
copy /B liboffload_array.so ..\pymic > NUL
