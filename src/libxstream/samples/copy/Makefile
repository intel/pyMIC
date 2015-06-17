ARCH = intel64

ROOTDIR = $(abspath $(dir $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))))
DEPDIR = $(ROOTDIR)/../..
INCDIR = $(ROOTDIR)
SRCDIR = $(ROOTDIR)
BLDDIR = build/$(ARCH)
OUTDIR = .

CXXFLAGS = $(NULL)
CFLAGS = $(NULL)
DFLAGS = $(NULL)
IFLAGS = -I$(INCDIR) -I$(DEPDIR)/include

STATIC ?= 0
OMP ?= 1
DBG ?= 0
IPO ?= 0

OUTNAME = $(shell basename $(ROOTDIR))
HEADERS = $(shell ls -1 $(INCDIR)/*.h   2> /dev/null | tr "\n" " ") \
          $(shell ls -1 $(SRCDIR)/*.hpp 2> /dev/null | tr "\n" " ") \
          $(shell ls -1 $(SRCDIR)/*.hxx 2> /dev/null | tr "\n" " ") \
          $(shell ls -1 $(SRCDIR)/*.hh 2>  /dev/null | tr "\n" " ")
CPPSRCS = $(shell ls -1 $(SRCDIR)/*.cpp 2> /dev/null | tr "\n" " ")
CXXSRCS = $(shell ls -1 $(SRCDIR)/*.cxx 2> /dev/null | tr "\n" " ")
CCXSRCS = $(shell ls -1 $(SRCDIR)/*.cc  2> /dev/null | tr "\n" " ")
CSOURCS = $(shell ls -1 $(SRCDIR)/*.c   2> /dev/null | tr "\n" " ")
SOURCES = $(CPPSRCS) $(CXXSRCS) $(CCXSRCS) $(CSOURCS)
CPPOBJS = $(patsubst %,$(BLDDIR)/%,$(notdir $(CPPSRCS:.cpp=-cpp.o)))
CXXOBJS = $(patsubst %,$(BLDDIR)/%,$(notdir $(CXXSRCS:.cxx=-cxx.o)))
CCXOBJS = $(patsubst %,$(BLDDIR)/%,$(notdir $(CCXSRCS:.cc=-cc.o)))
COBJCTS = $(patsubst %,$(BLDDIR)/%,$(notdir $(CSOURCS:.c=-c.o)))
OBJECTS = $(CPPOBJS) $(CXXOBJS) $(CCXOBJS) $(COBJCTS)

ICPC = $(notdir $(shell which icpc 2> /dev/null))
ICC = $(notdir $(shell which icc 2> /dev/null))
GPP = $(notdir $(shell which g++ 2> /dev/null))
GCC = $(notdir $(shell which gcc 2> /dev/null))

ifneq (,$(ICPC))
	CXX = $(ICPC)
	ifeq (,$(ICC))
		CC = $(CXX)
	endif
else
	CXX = $(GPP)
endif
ifneq (,$(ICC))
	CC = $(ICC)
	ifeq (,$(ICPC))
		CXX = $(CC)
	endif
else
	CC = $(GCC)
endif
ifneq ($(CXX),)
	LD = $(CXX)
endif
ifeq ($(LD),)
	LD = $(CC)
endif

ifneq (,$(filter icpc icc,$(CXX) $(CC)))
	CXXFLAGS += -fPIC -Wall -std=c++0x
	CFLAGS += -fPIC -Wall -std=c99
	ifeq (0,$(DBG))
		CXXFLAGS += -fno-alias -ansi-alias -O2
		CFLAGS += -fno-alias -ansi-alias -O2
		DFLAGS += -DNDEBUG
		ifneq ($(IPO),0)
			CXXFLAGS += -ipo
			CFLAGS += -ipo
		endif
		ifeq ($(AVX),1)
			CXXFLAGS += -xAVX
			CFLAGS += -xAVX
		else ifeq ($(AVX),2)
			CXXFLAGS += -xCORE-AVX2
			CFLAGS += -xCORE-AVX2
		else ifeq ($(AVX),3)
			CXXFLAGS += -xCOMMON-AVX512
			CFLAGS += -xCOMMON-AVX512
		else
			CXXFLAGS += -xHost
			CFLAGS += -xHost
		endif
	else ifneq (1,$(DBG))
		CXXFLAGS += -O0 -g3 -gdwarf-2 -debug inline-debug-info
		CFLAGS += -O0 -g3 -gdwarf-2 -debug inline-debug-info
	else
		CXXFLAGS += -O0 -g
		CFLAGS += -O0 -g
	endif
	ifneq ($(OMP),0)
		CXXFLAGS += -openmp
		CFLAGS += -openmp
		LDFLAGS += -openmp
	endif
	ifeq (0,$(OFFLOAD))
		CXXFLAGS += -no-offload
		CFLAGS += -no-offload
	else
		#CXXFLAGS += -offload-option,mic,compiler,"-O2 -opt-assume-safe-padding"
		#CFLAGS += -offload-option,mic,compiler,"-O2 -opt-assume-safe-padding"
	endif
	LDFLAGS += -fPIC
	ifneq ($(STATIC),0)
		ifneq ($(STATIC),)
			LDFLAGS += -no-intel-extensions -static-intel
		endif
	endif
else # GCC assumed
	CXXFLAGS += -Wall
	CFLAGS += -Wall
	ifeq (0,$(DBG))
		CXXFLAGS += -O2
		CFLAGS += -O2
		DFLAGS += -DNDEBUG
		ifeq ($(AVX),1)
			CXXFLAGS += -mavx
			CFLAGS += -mavx
		else ifeq ($(AVX),2)
			CXXFLAGS += -mavx2
			CFLAGS += -mavx2
		else ifeq ($(AVX),3)
			CXXFLAGS += -mavx512f
			CFLAGS += -mavx512f
		else
			CXXFLAGS += -march=native
			CFLAGS += -march=native
		endif
	else ifneq (1,$(DBG))
		CXXFLAGS += -O0 -g3 -gdwarf-2
		CFLAGS += -O0 -g3 -gdwarf-2
	else
		CXXFLAGS += -O0 -g
		CFLAGS += -O0 -g
	endif
	ifneq ($(OMP),0)
		CXXFLAGS += -fopenmp
		CFLAGS += -fopenmp
		LDFLAGS += -fopenmp
	endif
	ifneq ($(OS),Windows_NT)
		CXXFLAGS += -fPIC
		CFLAGS += -fPIC
		LDFLAGS += -fPIC
	endif
	ifneq ($(STATIC),0)
		ifneq ($(STATIC),)
			LDFLAGS += -static
		endif
	endif
endif

ifeq (,$(CXXFLAGS))
	CXXFLAGS = $(CFLAGS)
endif
ifeq (,$(CFLAGS))
	CFLAGS = $(CXXFLAGS)
endif

ifneq ("","$(wildcard $(DEPDIR)/lib/$(ARCH)/libxstream.a)")
	LIBEXT = a
else
	LIBEXT = so
endif

parent = $(subst ?, ,$(firstword $(subst /, ,$(subst $(NULL) ,?,$(patsubst ./%,%,$1)))))

.PHONY: all
all: $(OUTDIR)/$(OUTNAME)

$(OUTDIR)/$(OUTNAME): $(OBJECTS) $(DEPDIR)/lib/$(ARCH)/libxstream.$(LIBEXT)
	@mkdir -p $(OUTDIR)
	$(LD) -o $@ $(LDFLAGS) $^

$(BLDDIR)/%-c.o: $(SRCDIR)/%.c $(HEADERS) $(ROOTDIR)/Makefile
	@mkdir -p $(BLDDIR)
	$(CC) $(CFLAGS) $(DFLAGS) $(IFLAGS) -c $< -o $@

$(BLDDIR)/%-cpp.o: $(SRCDIR)/%.cpp $(HEADERS) $(ROOTDIR)/Makefile
	@mkdir -p $(BLDDIR)
	$(CXX) $(CXXFLAGS) $(DFLAGS) $(IFLAGS) -c $< -o $@

.PHONY: clean
clean:
ifneq ($(abspath $(call parent,$(BLDDIR))),$(ROOTDIR))
ifneq ($(abspath $(call parent,$(BLDDIR))),$(abspath .))
	@rm -rf $(call parent,$(BLDDIR))
else
	@rm -f $(OBJECTS)
endif
else
	@rm -f $(OBJECTS)
endif

.PHONY: realclean
realclean: clean
ifneq ($(abspath $(call parent,$(OUTDIR))),$(ROOTDIR))
ifneq ($(abspath $(call parent,$(OUTDIR))),$(abspath .))
	@rm -rf $(call parent,$(OUTDIR))
else
	@rm -f $(OUTDIR)/$(OUTNAME)
endif
else
	@rm -f $(OUTDIR)/$(OUTNAME)
endif
	@rm -f $(OUTDIR)/libxstream.so

install: all clean
	@cp $(DEPDIR)/lib/$(ARCH)/libxstream.so $(OUTDIR) 2> /dev/null || true

