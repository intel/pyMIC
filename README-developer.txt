0. General Information
-----------------------

Maintainer: Michael Klemm, michael.klemm@intel.com, SSG-DRD EMEA HPC team

!!! SOME WORDS OF WARNING !!! SOME WORDS OF WARNING !!! 

This is very early code.  It is still under heavy development.  Please expect
some bugs, strange behavior, and irritating error messages.  I'm working on
getting all these things right, so I'm happy to receive your input.

There's also a certain lack of documentation.  If you have questions on how
to use this Python module, please feel free to drop me an email.

The two biggest limitations at this point are: 

   (1) pyMIC requires the data to be stored as numpy.array structures
   
   (2) the kernel needs to be written in C/C++ (and Fortran) and must be 
       compiled as a native shared object for KNC.

       
       
1. Requirements 
-----------------------

You need to have the following software packages:

- CPython 2.6.x or 2.7.x

- Intel Composer XE 2013 SP1 

- MPSS 3.3 or greater



2. Setup
-----------------------

To prepare pyMIC, please follow these steps

- $pymic is the base directory of the pyMIC checkout/download

- load the environment of the Intel Composer XE (if it has not been loaded yet):

  $> source /opt/intel/composerxe/bin/compilervars.sh intel64

- depending on the Python version installed, you may need to adjust the 
  variable PYTHON_INCLUDES in the makefiles to match your system

- change to the src directory and build the native parts of the module:

  $> cd $pymic/src
  $> make
  
- if you want to build the debugging version of pyMIC (enables PYMIC_DEBUG, see
  below), use the following commands:

  $> cd $pymic/src
  $> make debug=1
  
- you should now have a shared object file _pyMICimpl.so.  

- set the Python search path, so that Python find the pyMIC modules:"

  $> export PYTHONPATH=$PYTHONPATH:$pymic/src:$pymic/pyMIC

- you can set OFFLOAD_REPORT=<level> to see the offloads that are
  triggered by pyMIC.

- if you want to have even more fine-grained debugging output, 
  set the environment variable PYMIC_DEBUG=1.
   
 
3. Examples
-----------------------

There are a few (very few!) examples that you can use for your first steps.  You 
will find them in the $pymic/examples directory.  

After completing the setup steps, you should be able to build the native code of 
the examples:

  $> cd $pymic/examples/double_it
  $> make
  
This step creates a shared object for MIC native execution.  Please copy this 
file and all it's dependencies over to the MIC devices (you may need to 
do this as root):
  
  $> scp libdouble_it.so mic0:/lib64
  
Please also make sure that the library can be read and executed as this is 
required by the dynamic linker.

  $> ssh mic0 "chmod a+rx /lib64/libdouble_it.so"

Then you should be able to run the Python application and do some offloads:
  
  $> ./double_it.py
 

 
4. Debugging
-----------------------

If you are interested in what is going on inside the pyMIC module, you can
choose from several options to get a more verbose output.

You can set the OFFLOAD_REPORT environment variable to request an offload 
report from the Intel offload runtime.  Please have a loop at the article at
https://software.intel.com/en-us/node/510366 to see what values are accepted
for the environment variable and what effect they have.

You can also set PYMIC_DEBUG to enable the debugging output of pyMIC.  Here's 
the list of accepted values and what effect they have.  Please note that higher
levels include lower levels, that is, they increase verbosity of the output.

Level   Effect
0       disable debugging output
1       show data transfers and kernel invocation
2       show allocation/deallocation of buffers
5       show additional operations performed
10      extension module: data transfers
100     extension module: buffer management, kernel invocation
1000    extension module: function entry/exit
