# - Try to find the PolarSSL SSL Library
# The module will set the following variables
#
#  POLARSSL_FOUND - System has popt
#  POLARSSL_INCLUDE_DIR - The popt include directory
#  POLARSSL_LIBRARIES - The libraries needed to use popt

# Find the include directories
FIND_PATH(POLARSSL_INCLUDE_DIR
    NAMES polarssl/ssl.h
    DOC "Path containing the polarssl/ssl.h include file"
    )

FIND_LIBRARY(POLARSSL_LIBRARIES
    NAMES polarssl
    DOC "polarssl library path"
    )

include(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(POLARSSL
    REQUIRED_VARS POLARSSL_INCLUDE_DIR POLARSSL_LIBRARIES
  )
