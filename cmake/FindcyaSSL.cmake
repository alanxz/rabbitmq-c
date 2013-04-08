# - Try to find the PolarSSL SSL Library
# The module will set the following variables
#
#  CYASSL_FOUND - System has popt
#  CYASSL_INCLUDE_DIR - The popt include directory
#  CYASSL_LIBRARIES - The libraries needed to use popt

# Find the include directories
FIND_PATH(CYASSL_INCLUDE_DIR
    NAMES polarssl/ssl.h
    DOC "Path containing the polarssl/ssl.h include file"
    )

FIND_LIBRARY(CYASSL_LIBRARY
    NAMES polarssl
    DOC "polarssl library path"
    )

include(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(CYASSL
  REQUIRED_VARS CYASSL_INCLUDE_DIR CYASSL_LIBRARY
  )
