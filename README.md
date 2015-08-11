# General Information

pyMIC is a Python module to offload computation in a Python program to the Intel Xeon Phi coprocessor. It contains offloadable arrays and device management functions. It supports invocation of native kernels (C/C++, Fortran) and blends in with Numpy's array types for `float`, `complex`, and `int` data types. 

For more information and downloads please visit pyMIC's Github page: https://github.com/01org/pyMIC. You can find pyMIC's mailinglist at https://lists.01.org/mailman/listinfo/pymic. If you want to report a bug, please send an email to the mailinglist or file an issue at Github.

The pyMIC module is early code and is still under development. Please expect some bugs, strange behavior, and irritating error messages. We're working hard on getting all these things right, so we're more than happy to receive your input.



# Contributors

*   Michael Klemm, Intel, Germany (maintainer)

*   Jussi Enkovaara, CSC, Finland  (developer)

*   Freddie Witherden, Imperial College, United Kindom (developer)



# Requirements 

## Linux

The pyMIC module still has some special requirements for the build system.  You will need to have the following software packages installed on your system:

*   CPython 2.6.x or 2.7.x (<https://www.python.org/>)

*   Intel(R) Composer XE 2013 SP1 or Intel Composer XE 2016 Beta (<https://software.intel.com/en-us/intel-parallel-studio-xe>)

*   MPSS 3.3 or greater (<https://software.intel.com/en-us/articles/intel-manycore-platform-software-stack-mpss>)


## Windows

Support for Windows is experimental and does not have a proper build system yet.  You will to have the following software packages installed on your system:

*   Anaconda Python with Python 2.7.x compiled for 64 bit(<http://continuum.io/downloads>)

*   Intel(R) Composer XE 2013 SP1 or Intel Composer XE 2016 Beta (<https://software.intel.com/en-us/intel-parallel-studio-xe>)

*   MPSS 3.3 or greater (<https://software.intel.com/en-us/articles/intel-manycore-platform-software-stack-mpss>)


# Setting up the pyMIC module for Linux

## Compilation

To compile pyMIC, please follow these steps carefully:

*   `$pymic` is the base directory of the pyMIC checkout/download.

*   Load the environment of the Intel Composer XE (if it has not been loaded yet):

    ``$> source /opt/intel/composerxe/bin/compilervars.sh intel64``

*   Go to the pyMIC directory (replace `$pymic` with the right path for your system):

    ```
    $> cd $pymic
    ```

*   Load the pyMIC enviroment by sourcing the enviroment script, to set all required path variables for pyMIC:

    ```
    $> source env.sh
    ```

*   Depending on the Python version installed, you may need to adjust the variable `PYTHON_INCLUDES` in the makefiles to match your system

*   Change to the pyMIC directory and build the native parts of the module:

    ```
    $> cd $PYMIC_BASE
    $> make
    ```

*   You should now have the shared object files `pymic_libxstream.so` and `liboffload_array.so` in the `pymic` subdirectory.


## Usage
  
To use the pyMIC module in a Python application on Linux, please follow these steps:

*   `$pymic` is the base directory of the pyMIC checkout/download

*   Load the environment of the Intel Composer XE (if it has not been loaded yet):

    ```
    $> source /opt/intel/composerxe/bin/compilervars.sh intel64
    ```

*   Load the pyMIC enviroment by sourcing the enviroment script, to set all required path variables for pyMIC:

    ```
    $> cd $pymic
    $> source env.sh
    ```

*   Alternatively, set the Python search path, so that Python can find the pyMIC modules:

    ```
    $> export PYTHONPATH=$PYTHONPATH:$pymic
    ```

*   You can set `OFFLOAD_REPORT=<level>` to see the offloads that are triggered by pyMIC.

*   If you want to have even more fine-grained debugging output, set the environment variable `PYMIC_DEBUG=<level>`.

Please see the example codes in `$pymic/examples` and the API documentation for each pyMIC method on how to use the pyMIC module from your Python application.
 


# Setting up the pyMIC Module for Windows

## Compilation

The build system is based on a couple of Windows batch files.  To compile pyMIC for Windows, please follow these steps:

*   `%pymic%` is the base directory of the pyMIC checkout/download

*   Start a command window with support for Intel compiler and 64-bit compilation.

*   Go the pyMIC directory (replace `%pymic%` with the right path for your system):

    ```
    > cd %pymic%
    ```
 
*   Load the pyMIC environment:

    ```
    > env.bat
    ```

*   Invoke the build:

    ```
    > make.bat
    ```

*   You should now find the Python extension `pymic_libxstream.pyd` and the shared object file `liboffload_array.so` in the `pymic` subdirectory.


## Usage

To use the pyMIC module in a Python application, please follow these steps:

*   `%pymic%` is the base directory of the pyMIC checkout/download.

*   Start a command window with support for Intel compiler and 64-bit compilation.

*   Go the pyMIC directory (replace `%pymic%` with the right path for your system) and load the enviroment:

    ```
    > cd %pymic%
    > env.bat
    ```

*   Alternatively, set the Python search path, so that Python can find the pyMIC modules:

    ```
    > set PYTHONPATH=%PYTHONPATH%:%pymic%
    ```

*   You can set `OFFLOAD_REPORT=<level>` to see the offloads that are triggered by pyMIC.

*   If you want to have even more fine-grained debugging output, set the environment variable `PYMIC_DEBUG=<level>`.

Please see the example codes in `%pymic%/examples` and the API documentation for each pyMIC method on how to use the pyMIC module from your Python application.



# Example Programs

There are a few examples programs that you can use for your first steps.  You will find them in the `$pymic/examples` directory.  

To run the examples, pleae make sure that the environment of Intel Composer XE and pyMIC have been loaded:

*   Linux:

    ```
    $> source /opt/intel/composerxe/bin/compilervars.sh intel64
    $> cd $pymic
    $> source env.sh
    ```

*   Windows:

    ```
    > cd %pymic%
    > env.bat
    ```

After completing the setup steps for the environment, you should now be able to build the native code of the examples.  For instance:

*   Linux:

    ```
    $> cd $PYMIC_BASE/examples/double_it
    $> make
    ```

*   Windows:

    ```
    > cd %PYMIC_BASE\examples\double_it
    > make.bat
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
