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

.PHONY: build install all clean realclean tests tarball docs

default: build

build:
	python ./setup.py build

install: build
	python ./setup.py install

all:
	@echo "WARNING: pyMIC now uses Python distutils to build"
	@echo "         GNU make has been deprecated."
	@echo "         Please use \"python setup.py build\" instead"
	@sleep 5
	make -C src all
	make -C pymic all
	make -C examples all
	make -C benchmarks all

tests: all
	make -C tests tests

pep8check:
	pep8 setup.py
	make -C pymic pep8check
	make -C tests pep8check
	make -C benchmarks pep8check

clean:
	make -C src clean
	make -C pymic clean
	make -C examples clean
	make -C benchmarks clean
	make -C tests clean

realclean:
	make -C src realclean
	make -C pymic realclean
	make -C examples realclean
	make -C benchmarks realclean
	make -C tests realclean
	rm -f README.html README.pdf CHANGELOG.html CHANGELOG.pdf
	rm -rf build

tarball: all
	make -C examples realclean
	tar cfj pymic-`date +%F`.tbz2 ../pymic/{README.txt,pymic,include,examples}

docs: README.html CHANGELOG.html

README.html: README.md
	pandoc --from=markdown_github --to=html -o README.html README.md

CHANGELOG.html: CHANGELOG.md
	pandoc --from=markdown_github --to=html -o CHANGELOG.html CHANGELOG.md
