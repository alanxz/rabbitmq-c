#!/bin/bash

# Build rabbitmq-c using Microsoft's C compiler

set -e

vs64=
sdk64=

while [ $# -gt 0 ] ; do
  case $1 in
  --enable-64-bit)
    vs64=/amd64
    sdk64=/x64
    ;;
  *)
    echo "Usage: build-ms.sh [ --enable-64-bit ]" 1>&2
    exit 1
  esac
  shift
done

# Locate the necessary lib and include directories

drive=$(echo "$SYSTEMDRIVE" | sed 's|^\([A-Za-z]\):$|/\1|')

for vsvers in 10.0 9.0 8 ; do
  vsdir="$drive/Program Files/Microsoft Visual Studio $vsvers"
  [ -x "$vsdir/VC/bin$vs64/cl.exe" ] && break

  vsdir="$drive/Program Files (x86)/Microsoft Visual Studio $vsvers"
  [ -x "$vsdir/VC/bin$vs64/cl.exe" ] && break

  vsdir=
done

if [ -z "$vsdir" ] ; then
  echo "Couldn't find a suitable Visual Studio installation"
  exit 1
fi

echo "Using Visual Studio install at $vsdir"

for sdkpath in "Microsoft SDKs/Windows/"{v7.0A,v6.0A} "Microsoft Visual Studio 8/VC/PlatformSDK" ; do
  sdkdir="$drive/Program Files/$sdkpath"
  [ -d "$sdkdir/lib$sdk64" -a -d "$sdkdir/include" ] && break

  sdkdir="$drive/Program Files (x86)/$sdkpath"
  [ -d "$sdkdir/lib$sdk64" -a -d "$sdkdir/include" ] && break

  sdkdir=
done

if [ -z "$sdkdir" ] ; then
  echo "Couldn't find suitable Windows SDK installation"
  exit 1
fi

echo "Using Windows SDK install at $sdkdir"

PATH="$PATH:$vsdir/VC/bin$vs64:$vsdir/Common7/IDE"
LIB="$vsdir/VC/lib$vs64:$sdkdir/lib$sdk64"
INCLUDE="$vsdir/VC/include:$sdkdir/include"
export PATH LIB INCLUDE

# Do the build
set -x
autoreconf -i
./configure CC=cl.exe LD=link.exe CFLAGS='-nologo'
sed -i -e 's/^fix_srcfile_path=.*$/fix_srcfile_path=""/;s/^deplibs_check_method=.*$/deplibs_check_method=pass_all/;/^archive_cmds=/s/-link -dll/& -implib:\\$libname.\\$libext/' libtool
make

# Copy the results of the build into one place, as "make install"
# isn't too useful here.
mkdir -p build/lib build/include build/bin
cp -a librabbitmq/.libs/*.dll examples/.libs/*.exe build/bin
cp -a librabbitmq/win32/msinttypes/*.h librabbitmq/amqp.h librabbitmq/amqp_framing.h build/include
cp -a librabbitmq/*.exp librabbitmq/*.lib build/lib
