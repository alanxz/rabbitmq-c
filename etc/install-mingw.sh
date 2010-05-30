#!/bin/bash

if [ $# -ne 1 ] ; then
    echo "usage: install-mingw.sh <destination directory>" 1>&2
    exit 1
fi    

unpack_dir=$1

if [ -eb "$unpack_dir" ] ; then
    echo "Destination directory already exists; please delete it if you are sure" 1>&2
    exit 1
fi

set -e

download_dir=/tmp/install-mingw.$$
mkdir -p $download_dir $unpack_dir

while read f ; do
    wget -P $download_dir -N http://switch.dl.sourceforge.net/project/mingw/$f
done <<EOF
GCC%20Version%204/gcc-4.5.0-1/gcc-core-4.5.0-1-mingw32-bin.tar.lzma
GCC%20Version%204/gcc-4.5.0-1/libgcc-4.5.0-1-mingw32-dll-1.tar.lzma
GNU%20Binutils/binutils-2.20.1/binutils-2.20.1-2-mingw32-bin.tar.gz
MinGW%20API%20for%20MS-Windows/w32api-3.14/w32api-3.14-mingw32-dev.tar.gz
MinGW%20gettext/gettext-0.17-1/gettext-0.17-1-mingw32-dev.tar.lzma
MinGW%20gettext/gettext-0.17-1/libintl-0.17-1-mingw32-dll-8.tar.lzma
MinGW%20gmp/gmp-5.0.1-1/libgmp-5.0.1-1-mingw32-dll-10.tar.lzma
MinGW%20gmp/gmp-5.0.1-1/libgmpxx-5.0.1-1-mingw32-dll-4.tar.lzma
MinGW%20libiconv/libiconv-1.13.1-1/libcharset-1.13.1-1-mingw32-dll-1.tar.lzma
MinGW%20libiconv/libiconv-1.13.1-1/libiconv-1.13.1-1-mingw32-dev.tar.lzma
MinGW%20libiconv/libiconv-1.13.1-1/libiconv-1.13.1-1-mingw32-dll-2.tar.lzma
MinGW%20mpc/mpc-0.8.1-1/libmpc-0.8.1-1-mingw32-dll-2.tar.lzma
MinGW%20mpfr/mpfr-2.4.1-1/libmpfr-2.4.1-1-mingw32-dll-1.tar.lzma
MinGW%20popt/popt-1.15-1/libpopt-1.15-1-mingw32-dev.tar.lzma
MinGW%20popt/popt-1.15-1/libpopt-1.15-1-mingw32-dll-0.tar.lzma
MinGW%20pthreads-w32/pthreads-w32-2.8.0-3/libpthread-2.8.0-3-mingw32-dll-2.tar.lzma
MinGW%20Runtime/mingwrt-3.18/mingwrt-3.18-mingw32-dev.tar.gz
MinGW%20Runtime/mingwrt-3.18/mingwrt-3.18-mingw32-dll.tar.gz
MSYS%20Base%20System/msys-1.0.14-1/msysCORE-1.0.14-1-msys-1.0.14-bin.tar.lzma
MSYS%20bash/bash-3.1.17-2/bash-3.1.17-2-msys-1.0.11-bin.tar.lzma
MSYS%20coreutils/coreutils-5.97-2/coreutils-5.97-2-msys-1.0.11-bin.tar.lzma
MSYS%20diffutils/diffutils-2.8.7.20071206cvs-2/diffutils-2.8.7.20071206cvs-2-msys-1.0.11-bin.tar.lzma
MSYS%20gawk/gawk-3.1.7-1/gawk-3.1.7-1-msys-1.0.11-bin.tar.lzma
MSYS%20grep/grep-2.5.4-1/grep-2.5.4-1-msys-1.0.11-bin.tar.lzma
MSYS%20libtool/libtool-2.2.7a-1/libtool-2.2.7a-1-msys-1.0.11-bin.tar.lzma
MSYS%20make/make-3.81-2/make-3.81-2-msys-1.0.11-bin.tar.lzma
MSYS%20sed/sed-4.2.1-1/sed-4.2.1-1-msys-1.0.11-bin.tar.lzma
EOF

for f in $download_dir/* ; do
    case $f in
    *.tar.gz)
            tar -C $unpack_dir -xzf $f
            ;;

    *.tar.lzma)
            tar -C $unpack_dir -xJf $f
            ;;
        
    *)
            echo "Don't know how to unpack $f" 1>&2
            exit 1
            ;;
    esac
done

rm -rf $download_dir
