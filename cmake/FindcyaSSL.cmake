# - Try to find the cyaSSL SSL Library
# The module will set the following variables
#
#  CYASSL_FOUND - System has popt
#  CYASSL_INCLUDE_DIR - The popt include directory
#  CYASSL_LIBRARIES - The libraries needed to use popt

# Find the include directories
FIND_PATH(CYASSL_INCLUDE_DIR
    NAMES cyassl/ssl.h
    DOC "Path containing the cyassl/ssl.h include file"
    )

FIND_LIBRARY(CYASSL_LIBRARIES
    NAMES cyassl
    DOC "cyassl library path"
    )

include(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(CYASSL
    REQUIRED_VARS CYASSL_INCLUDE_DIR CYASSL_LIBRARIES
  )
