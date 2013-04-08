# vim:set ts=2 sw=2 sts=2 et:
#
# This module install PDB files.
#
# Based on users posts:
# http://www.cmake.org/pipermail/cmake/2007-October/016924.html
#
#  Copyright (c) 2006-2011 Mathieu Malaterre <mathieu.malaterre@gmail.com>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

macro (install_pdb library)
  if (MSVC)
    if(CMAKE_CONFIGURATION_TYPES)
      # Visual Studio
      # The following does not work with LOCATION keyword. See:
      # http://www.cmake.org/pipermail/cmake/2011-February/042579.html
      foreach(cfg ${CMAKE_CONFIGURATION_TYPES})
        get_target_property(library_dll ${library} LOCATION_${cfg})
        string(REPLACE .dll .pdb library_pdb ${library_dll})
        string(TOLOWER ${cfg} lcfg)
        if(lcfg STREQUAL "debug" OR lcfg STREQUAL "relwithdebinfo")
          install (FILES ${library_pdb}
            DESTINATION bin
            CONFIGURATIONS ${cfg}
            )
        endif()
      endforeach()
    else()
      # nmake
      # Same as above we need the explicit location_<config> variable to account for
      # the value of CMAKE_DEBUG_POSTFIX
      get_target_property(library_dll ${library} LOCATION_${CMAKE_BUILD_TYPE})
      string(REPLACE .dll .pdb library_pdb ${library_dll})
      string(TOLOWER ${CMAKE_BUILD_TYPE} lcfg)
      if(lcfg STREQUAL "debug" OR lcfg STREQUAL "relwithdebinfo")
        install (FILES ${library_pdb}
          DESTINATION bin
          )
      endif()
    endif()
  endif ()
endmacro ()
