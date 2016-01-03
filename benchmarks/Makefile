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
 
.SUFFIXES: .csv .py
.PHONY: all benchmarks clean realclean
 
BENCHMARKS=$(shell find . -maxdepth 1 -name \*.py )
CSV=$(BENCHMARKS:.py=.csv)
 
all: libbenchmark_kernels.so

benchmarks: libbenchmark_kernels.so dgemm_c.x $(CSV)
	
libbenchmark_kernels.so: benchmark_kernels.c
	icc -I../include -I$(MKLROOT)/include -fPIC -shared -mmic -g -L$(MKLROOT)/lib/mic -lmkl_intel_lp64 -lmkl_core -lmkl_intel_thread -lpthread -o $@ $<
	chmod a+rX libbenchmark_kernels.so

dgemm_c.x: dgemm_c.c
	icc -openmp -L$(MKLROOT)/lib/intel64 -lmkl_intel_lp64 -lmkl_core -lmkl_intel_thread -lpthread -lm -g -O2 -o dgemm_c.x dgemm_c.c

pep8check:
	pep8 --ignore=W293,W291 *.py

clean:
	rm -f libbenchmark_kernels.so
	rm -f dgemm_c.x
	
realclean: clean
	rm -f *~
	rm -f *.csv

dgemm_c.csv: dgemm_c.x
	./dgemm_c.x

%.csv: %.py
	MIC_USE_2MB_BUFFERS=16k PYMIC_KMP_AFFINITY=balanced PYMIC_OMP_NUM_THREADS=240 ./$< 
