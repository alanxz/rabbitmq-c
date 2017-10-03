$!
$! Copyright 2011, Richard Levitte <richard@levitte.org>
$! Copyright 2014, John Malmberg <wb8tyw@qsl.net>
$! Copyright 2016, Alexander Saprykin <xelfium@gmail.com>
$!
$! Permission to use, copy, modify, and/or distribute this software for any
$! purpose with or without fee is hereby granted, provided that the above
$! copyright notice and this permission notice appear in all copies.
$!
$! THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
$! WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
$! MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
$! ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
$! WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
$! ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
$! OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
$!
$!===========================================================================
$! Command-line options:
$!
$!    32            Compile with 32-bit pointers.
$!    BIGENDIAN     Compile for a big endian host.
$!    CCQUAL=x      Add "x" to the C compiler qualifiers.
$!    DEBUG         Build in debug mode.
$!    CLEAN         Only perform clean after the previous build.
$!    TESTS=(x)     Build library tests. Comma separated test names or leave
$!                  empty to build all the tests.
$!                  Example 1 (curtain tests): TESTS=(pmem,puthread)
$!                  Example 2 (all tests): TESTS
$!    RUN_TESTS     Runs all tests.
$!    BOOST_ROOT=x  Boost root directory.
$!                  Example: BOOST_ROOT=/SYS$COMMON/boost_1_43_1
$!    NOLIB         Skip library buidling. Useful when you want to rebuild
$!                  particular tests.
$!===========================================================================
$!
$!
$! Save the original default dev:[dir], and arrange for its restoration
$! at exit.
$!---------------------------------------------------------------------
$ orig_def = f$environment("DEFAULT")
$ on error then goto common_exit
$ on control_y then goto common_exit
$!
$ ctrl_y       = 1556
$ proc         = f$environment("PROCEDURE")
$ proc_fid     = f$file_attributes(proc, "FID")
$ proc_dev     = f$parse(proc, , , "DEVICE")
$ proc_dir     = f$parse(proc, , , "DIRECTORY")
$ proc_name    = f$parse(proc, , , "NAME")
$ proc_type    = f$parse(proc, , , "TYPE")
$ proc_dev_dir = proc_dev + proc_dir
$!
$! Have to manually parse the device for a search list.
$! Can not use the f$parse() as it will return the first name
$! in the search list.
$!
$ orig_def_dev = f$element(0, ":", orig_def) + ":"
$ if orig_def_dev .eqs. "::" then orig_def_dev = "sys$disk:"
$ test_proc = orig_def_dev + proc_dir + proc_name + proc_type
$!
$! If we can find this file using the default directory
$! then we know that we should use the original device from the
$! default directory which could be a search list.
$!
$ test_proc_fid = f$file_attributes(test_proc, "FID")
$!
$ if (test_proc_fid .eq. proc_fid)
$ then
$     proc_dev_dir = orig_def_dev + proc_dir
$ endif
$!
$! Verbose output message stuff. Define symbol to "write sys$output".
$! vo_c - verbose output for compile
$!
$ vo_c := "write sys$output"
$!
$! Determine the main distribution directory ("[--]") in an
$! ODS5-tolerant (case-insensitive) way.  (We do assume that the only
$! "]" or ">" is the one at the end.)
$!
$! Some non-US VMS installations report ">" for the directory delimiter
$! so do not assume that it is "]".
$!
$ orig_def_len = f$length(orig_def)
$ delim = f$extract(orig_def_len - 1, 1, orig_def)
$!
$ set default 'proc_dev_dir'
$ set default [-.librabbitmq]
$ base_src_dir = f$environment("default")
$ set default 'proc_dev_dir'
$!
$! Define the architecture-specific destination directory name
$! -----------------------------------------------------------
$!
$ if (f$getsyi("HW_MODEL") .lt. 1024)
$ then
$      'vo_c' "%LIBRABBITMQ-F-NOTSUP, VAX platform is not supported, sorry :("
$      goto common_exit
$ else
$      arch_name = ""
$      arch_name = arch_name + f$edit(f$getsyi("ARCH_NAME"), "UPCASE")
$!
$      if (arch_name .eqs. "") then arch_name = "UNK"
$!
$      node_swvers   = f$getsyi("node_swvers")
$      version_patch = f$extract(1, f$length(node_swvers), node_swvers)
$      maj_ver       = f$element(0, ".", version_patch)
$      min_ver_patch = f$element(1, ".", version_patch)
$      min_ver       = f$element(0, "-", min_ver_patch)
$      patch         = f$element(1, "-", min_ver_patch)
$!
$      if maj_ver .lts. "8" .or. min_ver .lts. "2"
$      then
$          'vo_c' "%LIBRABBITMQ-F-NOTSUP, only OpenVMS 8.2 and above are supported, sorry :("
$          goto common_exit
$      endif
$ endif
$!
$ objdir = proc_dev_dir - delim + ".''arch_name'" + delim
$ confdir = proc_dev_dir
$ ssldir = "SSL$ROOT:[INCLUDE]"
$!
$! Parse input arguments
$! ---------------------
$! Allow arguments to be grouped together with comma or separated by spaces
$! Do no know if we will need more than 8.
$ args = "," + p1 + "," + p2 + "," + p3 + "," + p4 + ","
$ args = args + p5 + "," + p6 + "," + p7 + "," + p8 + ","
$!
$! Provide lower case version to simplify parsing.
$ args_lower = f$edit(args, "LOWERCASE,COLLAPSE")
$!
$ args_len = f$length(args)
$ args_lower_len = f$length(args_lower)
$!
$ if f$locate(",clean,", args_lower) .lt. args_lower_len
$ then
$     'vo_c' "Cleaning up previous build..."
$     set default 'proc_dev_dir'
$!
$     if f$search("''arch_name'.DIR") .nes. ""
$     then
$         set prot=w:d []'arch_name'.DIR;*
$         delete/tree [.'arch_name'...]*.*;*
$         delete []'arch_name'.DIR;*
$     endif
$!
$     goto common_exit
$ endif
$!
$ build_64   = 1
$ if f$locate(",32,", args_lower) .lt. args_lower_len
$ then
$     build_64 = 0
$ endif
$!
$ big_endian = 0
$ if f$locate(",bigendian,", args_lower) .lt. args_lower_len
$ then
$     big_endian = 1
$ endif
$!
$ cc_extra = ""
$ args_loc = f$locate(",ccqual=", args_lower)
$ if args_loc .lt. args_lower_len
$ then
$     arg = f$extract(args_loc + 1, args_lower_len, args_lower)
$     arg_val = f$element(0, ",", arg)
$     cc_extra = f$element(1, "=", arg_val)
$ endif
$!
$ is_debug = 0
$ if f$locate(",debug,", args_lower) .lt. args_lower_len
$ then
$     is_debug = 1
$ endif
$!
$! Prepare build directory
$! -----------------------
$!
$! When building on a search list, need to do a create to make sure that
$! the output directory exists, since the clean procedure tries to delete
$! it.
$!
$ if f$search("''proc_dev_dir'''arch_name'.DIR") .eqs. ""
$ then
$     create/dir 'objdir'/prot=o:rwed
$ endif
$!
$ set default 'objdir'
$ if f$search("CXX_REPOSITORY.DIR") .nes. ""
$ then
$     set prot=w:d []CXX_REPOSITORY.DIR;*
$     delete/tree [.CXX_REPOSITORY...]*.*;*
$     delete []CXX_REPOSITORY.DIR;*
$ endif
$!
$ if f$locate(",nolib,", args_lower) .lt. args_lower_len
$ then
$     goto build_tests
$ endif
$!
$! Prepare sources for compilation
$! -------------------------------
$!
$ cc_link_params = ""
$ cc_params = "/NAMES=(UPPERCASE,SHORTENED)"
$ cc_params = cc_params + "/DEFINE=(HAVE_CONFIG_H,_REENTRANT,_POSIX_EXIT,_OPENVMS)"
$ cc_params = cc_params + "/INCLUDE_DIRECTORY=(''objdir',''confdir',''ssldir',''base_src_dir')"
$ cc_params = cc_params + "/FLOAT=IEEE/IEEE_MODE=DENORM_RESULTS"
$!
$ if build_64 .eqs. "1"
$ then
$     cc_params = cc_params + "/POINTER_SIZE=64"
$ else
$     cc_params = cc_params + "/POINTER_SIZE=32"
$ endif
$!
$ if cc_extra .nes. ""
$ then
$     cc_params = cc_params + " " + cc_extra
$ endif
$!
$ if is_debug .eqs. "1"
$ then
$     cc_params = cc_params + "/DEBUG/NOOPTIMIZE/LIST/SHOW=ALL"
$     cc_link_params = "/DEBUG/TRACEBACK"
$ else
$     cc_link_params = "/NODEBUG/NOTRACEBACK"
$ endif
$!
$ amqp_src = "amqp_api.c amqp_connection.c amqp_consumer.c amqp_framing.c amqp_hostcheck.c"
$ amqp_src = amqp_src + " amqp_mem.c amqp_socket.c amqp_table.c amqp_tcp_socket.c"
$ amqp_src = amqp_src + " amqp_time.c amqp_url.c amqp_openssl.c amqp_openssl_bio.c amqp_openssl_hostname_validation.c"
$!
$! Inform about building
$! ---------------------
$!
$ if build_64 .eqs. "1"
$ then
$     'vo_c' "Building for ''arch_name' (64-bit)"
$ else
$     'vo_c' "Building for ''arch_name' (32-bit)"
$ endif
$!
$! Compile library modules
$! -----------------------
$!
$ 'vo_c' "Compiling object modules..."
$ src_counter = 0
$ amqp_src = f$edit(amqp_src, "COMPRESS")
$ amqp_objs = ""
$!
$ src_loop:
$     next_src = f$element (src_counter, " ", amqp_src)
$     if next_src .nes. "" .and. next_src .nes. " "
$     then
$         'vo_c' "[CC] ''next_src'"
$         cc [--.LIBRABBITMQ]'next_src' 'cc_params'
$!
$         src_counter = src_counter + 1
$!
$         obj_file = f$extract (0, f$length (next_src) - 1, next_src) + "obj"
$         amqp_objs = amqp_objs + "''obj_file',"
$         purge 'obj_file'
$!
$         goto src_loop
$     endif
$!
$ amqp_objs = f$extract (0, f$length (amqp_objs) - 1, amqp_objs)
$!
$! Create library
$! --------------
$!
$ 'vo_c' "Creating object library..."
$ library/CREATE/INSERT/REPLACE /LIST=LIBRABBITMQ.LIS LIBRABBITMQ.OLB 'amqp_objs'
$ library/COMPRESS LIBRABBITMQ.OLB
$ purge LIBRABBITMQ.OLB
$ purge LIBRABBITMQ.LIS
$!
$ 'vo_c' "Creating shared library..."
$! 'vo_c' "Objects: ''amqp_objs'"
$ WRITE SYS$OUTPUT "LINK DO PAULO VITOR"
$ link/SHARE=LIBRABBITMQ.EXE /MAP=LIBRABBITMQ.MAP 'cc_link_params' 'amqp_objs', [-]librabbitmq.opt/OPTION
$ purge LIBRABBITMQ.EXE
$ purge LIBRABBITMQ.MAP
$!
$! Write link options file
$! -----------------------
$!
$ if f$search("librabbitmq_link.opt") .nes. "" then delete librabbitmq_link.opt;*
$!
$ open/write/error=link_write_end lhf librabbitmq_link.opt
$ write lhf "''objdir'LIBRABBITMQ.EXE/SHARE"
$ write lhf ""
$ link_write_end:
$     close lhf
$!
$ goto build_done
$!
$ build_done:
$     'vo_c' "Build done."
$!
$ common_exit:
$     set default 'orig_def'
$     exit
