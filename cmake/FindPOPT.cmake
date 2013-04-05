# vim:set ts=2 sw=2 sts=2 et:
# - Try to find the popt options processing library
# The module will set the following variables
#
#  POPT_FOUND - System has popt
#  POPT_INCLUDE_DIR - The popt include directory
#  POPT_LIBRARIES - The libraries needed to use popt

# use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls

find_package(PkgConfig QUIET)
pkg_search_module(PC_POPT QUIET popt)

# Find the include directories
FIND_PATH(POPT_INCLUDE_DIR
    NAMES popt.h
    HINTS
          ${PC_POPT_INCLUDEDIR}
          ${PC_POPT_INCLUDE_DIRS}
    DOC "Path containing the popt.h include file"
    )

FIND_LIBRARY(POPT_LIBRARY
    NAMES popt
    HINTS
          ${PC_POPT_LIBRARYDIR}
          ${PC_POPT_LIBRARY_DIRS}
    DOC "popt library path"
    )

include(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(POPT
  REQUIRED_VARS POPT_INCLUDE_DIR POPT_LIBRARY
  VERSION_VAR PC_POPT_VERSION)

MARK_AS_ADVANCED(POPT_INCLUDE_DIR POPT_LIBRARY)
