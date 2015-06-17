# General Information

pyMIC is a Python module to offload computation in a Python program to the Intel Xeon Phi coprocessor. It contains offloadable arrays and device management functions. It supports invocation of native kernels (C/C++, Fortran) and blends in with Numpy's array types for `float`, `complex`, and `int` data types. 

For more information and downloads please visit pyMIC's Github page: https://github.com/01org/pyMIC. You can find pyMIC's mailinglist at https://lists.01.org/mailman/listinfo/pymic. 

The project is maintained by Michael Klemm, michael.klemm@intel.com, with the Software and Services Group at Intel Corporation. 

The pyMIC module is very early code and is still under heavy development. Please expect some bugs, strange behavior, and irritating error messages. We're working hard on getting all these things right, so we're more than happy to receive your input. 



# Requirements 

The pyMIC module still has some special requirements for the build system.  You will need to have the following software packages installed on your system:

*   CPython 2.6.x or 2.7.x (<https://www.python.org/>)

*   Intel Composer XE 2013 SP1 (<https://software.intel.com/en-us/intel-parallel-studio-xe>)

*   MPSS 3.3 or greater (<https://software.intel.com/en-us/articles/intel-manycore-platform-software-stack-mpss>)



# Setting up the pyMIC Module

## Compilation

To compile pyMIC, please follow these steps carefully:

*   `$pymic` is the base directory of the pyMIC checkout/download

*   load the environment of the Intel Composer XE (if it has not been loaded yet):

    ``$> source /opt/intel/composerxe/bin/compilervars.sh intel64``

*   depending on the Python version installed, you may need to adjust the variable `PYTHON_INCLUDES` in the makefiles to match your system

*   change to the src directory and build the native parts of the module:

    ```
    $> cd $pymic/src
    $> make
    ```
  
*   if you want to build the debugging version of pyMIC (enables the environment variable `PYMIC_DEBUG` for the native code, see below), use the following commands:

    ```
    $> cd $pymic/src
    $> make debug=1
    ```
  
*   you should now have a shared object file `_pyMICimpl.so`.  

*   set the Python search path, so that Python find the pyMIC modules:

    ```
    $> export PYTHONPATH=$PYTHONPATH:$pymic/src:$pymic/pymic
    ```

*   you can set `OFFLOAD_REPORT=<level>` to see the offloads that are triggered by pyMIC.

*   if you want to have even more fine-grained debugging output, set the environment variable `PYMIC_DEBUG=<level>`.


## Using pyMIC
  
To use the pyMIC module in a Python application, please follow these steps:

*   `$pymic` is the base directory of the pyMIC checkout/download

*   load the environment of the Intel Composer XE (if it has not been loaded yet):

    ```
    $> source /opt/intel/composerxe/bin/compilervars.sh intel64
    ```

*   set the Python search path, so that Python find the pyMIC modules:

    ```
    $> export PYTHONPATH=$PYTHONPATH:$pymic/src
    ```

*   you can set `OFFLOAD_REPORT=<level>` to see the offloads that are triggered by pyMIC.

*   if you want to have even more fine-grained debugging output, set the environment variable `PYMIC_DEBUG=<level>`.

Please see the example codes in `$pymic/examples` and the API documentation for each pyMIC method on how to use the pyMIC module from your Python application.
 
 
# Example Programs

There are a few examples programs that you can use for your first steps.  You will find them in the `$pymic/examples` directory.  

To run the examples, pleae make sure that the environment of Intel Composer XE and pyMIC have been loaded:

```
$> source /opt/intel/composerxe/bin/compilervars.sh intel64
$> cd $pymic
$> source env.sh
```

After completing the setup steps for the environment, you should now be able to build the native code of the examples.  For instance:

```
$> cd $pymic/examples/double_it
$> make
```
  
The `make` command creates a shared object for the examples kernel code for native execution on the Intel(R) Xeon Phi(tm) Coprocessor.  After this step has been completed, you can run the example code:

```
$> python double_it.py
```

Side note: if the shared object created for the example depends on additional libraries (e.g., the examples dgemm and svd) you need to make sure that the directories hosting these libraries are visible on the coprocessor via NFS or have been installed to the `/lib` directory on the coprocessor's ramdisk.

 

# Tracing & Debugging

If you are interested in what is going on inside the pyMIC module, you can choose from several options to get a more verbose output.

You can set the `OFFLOAD_REPORT` environment variable to request an offload report from the Intel offload runtime.  Please have a loop at the article at <https://software.intel.com/en-us/node/510366> to see what values are accepted for the environment variable and what effect they have.

The pyMIC module also supports more specific tracing and debugging.


## Tracing

The pyMIC module can collect a trace of all performance relevant calls into the module.  The trace consists of the called functions' name, timings, argument list, and (if collected) the source code location of the invocation.

To enable tracing, set `PYMIC_TRACE=1` before launching the Python application.  Shortly before the program finishes, the tracing information will be printed to `stdout` as Python maps and lists that have been converted to plain text.  You can then run any desired analysis on the trace data.

For each trace record, pyMIC records its source code location of the invocation.  This is called "compact" format (`PYMIC_TRACE_STACKS=commpact`).  If the full call stack of the invocation is needed, `PYMIC_TRACE_STACKS=full` will collect the full call stack from the call site of a pyMIC function up to the top of the application code.  You can turn of stack collection (to increase performance while tracing) by setting `PYMIC_TRACE_STACKS=none`.
 

# Debugging

You can set the `PYMIC_DEBUG` environment variable to enable the debugging output of pyMIC.  Here's the list of accepted values and what effect they have.  Please note that higher levels include lower levels, that is, they increase verbosity of the output.

| Level | Effect                                                 |
|------:|--------------------------------------------------------|
|0      | disable debugging output                               |
|1      | show data transfers and kernel invocation              |
|2      | show allocation/deallocation of buffers                |
|3      | show argument conversions                              |
|5      | show additional operations performed                   | 
|10     | extension module: data transfers                       |
|100    | extension module: buffer management, kernel invocation |
|1000   | extension module: function entry/exit                  |

Please note that the pyMIC extension modules has to be compiled with debugging support to enable debugging level 10 and above.
