# - Try to find the popt options processing library
# The module will set the following variables
#
#  POPT_FOUND - System has popt
#  POPT_INCLUDE_DIR - The popt include directory
#  POPT_LIBRARY - The libraries needed to use popt

# use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
  pkg_search_module(PC_POPT QUIET popt)
endif()

# Find the include directories
find_path(POPT_INCLUDE_DIR
  NAMES popt.h
  HINTS
    ${PC_POPT_INCLUDEDIR}
    ${PC_POPT_INCLUDE_DIRS}
  DOC "Path containing the popt.h include file"
  )

find_library(POPT_LIBRARY
  NAMES popt
  HINTS
    ${PC_POPT_LIBRARYDIR}
    ${PC_POPT_LIBRARY_DIRS}
  DOC "popt library path"
  )

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(POPT
  REQUIRED_VARS POPT_INCLUDE_DIR POPT_LIBRARY
  VERSION_VAR PC_POPT_VERSION)

mark_as_advanced(POPT_INCLUDE_DIR POPT_LIBRARY)

if(POPT_FOUND AND NOT TARGET popt::popt)
  add_library(popt::popt UNKNOWN IMPORTED)
  set_target_properties(popt::popt PROPERTIES
    IMPORTED_LINK_INTERFACE_LANGUAGES "C"
    IMPORTED_LOCATION "${POPT_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${POPT_INCLUDE_DIR}"
  )
endif()
