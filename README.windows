# Using rabbitmq-c on Windows

There are three approaches to building rabbitmq-c under Windows:

- THE PREFERRED METHOD:
  Build using CMake, which will cover building with MSVC or MinGW
  See the README file for details on how to build with cmake.

- Build using the MinGW/MSYS (MinGW/MSYS is a port of the GNU
  toolchain and utilities to Windows, including the gcc compiler).
  The results of building in this way are native Windows DLLs and
  EXEs, and can be used without having MinGW installed.  The drawback
  to this approach is that you cannot safely call the resulting
  librabbitmq DLL from code compiled with Microsoft's C compiler.  The
  advantage is that the whole of rabbitmq-c can be built under
  Windows, including the tools.

- Build using Microsoft's C compiler.  You will still need to install
  MinGW/MSYS in order to run the rabbitmq-c build scripts, but
  Microsoft's compiler is used to compile the code.  The resulting
  librabbitmq DLL can be used from code compiled with Microsoft's C
  compiler (i.e. code developed in Visual Studio).  The downside to
  this approach is that the rabbitmq-c tools cannot be built, due to
  dependencies on other libraries.


## Common steps

With either of the approaches, the initial steps are the same: You
should download and install MinGW/MSYS and Python.

Installing installing the relevant parts of MinGW/MSYS can be fairly
time consuming - there are dozens of files to be downloaded and
unpacked.  To make it easier, we provide a bash script that automates
this process, in `rabbitmq-c/etc/install-mingw.sh`.  You can run this
script under cygwin or Linux (obviously if you use Linux you'll need
to transfer the resulting files over to the Windows machine).

Note that some MinGW packages are .tar.lzma files, so it requires a
system with the xz compression utility and a tar that supports the -J
option.  Recent cygwin and Linux distros should be fine here.

Run the install-mingw.sh script specifying the destination directory,
e.g.

  $ etc/install-mingw.sh mingw

This will download all the required MinGW/MSYS packages, and unpack
them into the `mingw` directory.

The other prerequisite for the rabbitmq-c build is Python.  The
Windows installer from python.org for the latest 2.x version of Python
will do fine.

You will also need to copy the source code for rabbitmq-c and
rabbitmq-codegen somewhere under your `mingw` directory.

Then to start the MSYS bash shell, open a `cmd` window, and ensure
that both the MinGW bin directory and the python install directory are
in the path, e.g.

  C:\>set PATH=%PATH%;C:\mingw\bin;C:\Python27

Then start bash, and run the following mount command (substituting the
Windows path of your MinGW install if it isn't `C:\mingw`):

  C:\>bash
  bash-3.1$ mount 'C:\mingw' /mingw

Finally, go to wherever you copied the rabbitmq-c source.

  bash-3.1$ cd /rabbitmq-c


## Building rabbitmq-c with Microsoft's C compiler

The Microsoft C/C++ compiler is part of MS Visual Studio, including
the gratis Visual Studio Express.  Visual Studio 2005 and higher are
known to work.

Start by following the steps in the previous section.  The GNU build
tools have limited support for Microsoft toolchain, but the
install-mingw.sh script will install versions of the packages that are
known to be suitable.  In particular, only libtool version 2.2.7a is
known to work; later versions have been reported to introduce
problems.

Once you are at the bash prompt, build rabbitmq-c by running the
script in `rabbitmq-c/etc/build-ms.sh`:

  bash-3.1$ etc/build-ms.sh

You should end up with a directory `build` containing the librabbitmq
DLL, the corresponding .lib file, and header files.  These are
sufficient to create applications using librabbitmq within Visual
Studio.

build-ms.sh produces 32-bit binaries by default.  If you have an
appropriate version of Visual Studio (e.g. VS2010), you can build
64-bit binaries with:

  bash-3.1$ etc/build-ms.sh --enable-64-bit


## Building rabbitmq-c with gcc

There is no script to build rabbitmq-c with gcc, but it is as
documented in the README file:

  bash-3.1$ autoreconf -i && ./configure && make

You can run the resulting tool EXEs without needing the rest of MinGW.  To do
this, copy the following files into a single directory:

- rabbitmq-c/tools/.libs/*.exe

- rabbitmq-c/librabbitmq/.libs/librabbitmq-0.dll

- /bin/libpopt-0.dll

- /bin/libiconv-2.dll

- /bin/libintl-8.dll


