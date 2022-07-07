# Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
# SPDX-License-Identifier: mit

cmake_minimum_required(VERSION 3.12...3.18)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Follow all steps below in order to calculate new ABI version when updating the library
# NOTE: THIS IS UNRELATED to the actual project version
#
# 1. If the library source code has changed at all since the last update, then increment revision
# 2. If any interfaces have been added, removed, or changed since the last update, increment current and set revision to 0.
# 3. If any interfaces have been added since the last public release, then increment age.
# 4. If any interfaces have been removed since the last public release, then set age to 0.

set(RMQ_SOVERSION_CURRENT   9)
set(RMQ_SOVERSION_REVISION  1)
set(RMQ_SOVERSION_AGE       5)

include(VersionFunctions)
get_library_version(RMQ_VERSION)
compute_soversion(${RMQ_SOVERSION_CURRENT} ${RMQ_SOVERSION_REVISION} ${RMQ_SOVERSION_AGE} RMQ_SOVERSION)

project(rabbitmq-c 
  VERSION ${RMQ_VERSION}
  DESCRIPTION "C RabbitMQ AMQP client library"
  LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

include(CheckSymbolExists)
include(CheckLibraryExists)
include(CMakePushCheckState)
include(GNUInstallDirs)

# Detect if we need to link against a socket library:
cmake_push_check_state()
if (WIN32)
  # Always use WinSock2 on Windows
  set(SOCKET_LIBRARIES ws2_32)
else ()
  # Is it in the default link?
  check_symbol_exists(getaddrinfo "sys/types.h;sys/socket.h;netdb.h" HAVE_GETADDRINFO)
  if (NOT (HAVE_GETADDRINFO EQUAL 1))
    SET(CMAKE_REQUIRED_LIBRARIES "socket")
    check_symbol_exists(getaddrinfo "sys/types.h;sys/socket.h;netdb.h" HAVE_GETADDRINFO2)
    if (HAVE_GETADDRINFO2 EQUAL 1)
      set(SOCKET_LIBRARIES socket)
    else ()
      SET(CMAKE_REQUIRED_LIBRARIES "socket;nsl")
      check_symbol_exists(getaddrinfo "sys/types.h;sys/socket.h;netdb.h" HAVE_GETADDRINFO3)
      if (HAVE_GETADDRINFO3 EQUAL 1)
        set(SOCKET_LIBRARIES socket nsl)
      else ()
        message(FATAL_ERROR "Cannot find name resolution library (containing symbol getaddrinfo)")
      endif ()
    endif ()
  endif ()

  set(CMAKE_REQUIRED_LIBRARIES ${SOCKET_LIBRARIES})
  check_symbol_exists(socket "sys/types.h;sys/socket.h" HAVE_SOCKET)
  if (NOT HAVE_SOCKET EQUAL 1)
    set(CMAKE_REQUIRED_LIBRARIES socket ${SOCKET_LIBRARIES})
    check_symbol_exists(socket "sys/types.h;sys/socket.h" HAVE_SOCKET2)
    if (HAVE_SOCKET2 EQUAL 1)
      set(SOCKET_LIBRARIES socket ${SOCKET_LIBRARIES})
    else ()
      set(CMAKE_REQUIRED_LIBRARIES socket nsl ${SOCKET_LIBRARIES})
      check_symbol_exists(socket "sys/types.h;sys/socket.h" HAVE_SOCKET3)
      if (HAVE_SOCKET3 EQUAL 1)
        set(SOCKET_LIBRARIES socket nsl ${SOCKET_LIBRARIES})
      else ()
        message(FATAL_ERROR "Cannot find socket library (containing symbol socket)")
      endif ()
    endif ()
  endif ()
endif ()
cmake_pop_check_state()

cmake_push_check_state()
set(CMAKE_REQUIRED_LIBRARIES ${SOCKET_LIBRARIES})
check_symbol_exists(poll poll.h HAVE_POLL)
if (NOT HAVE_POLL)
  if (WIN32)
    set(HAVE_SELECT 1)
  else()
    check_symbol_exists(select sys/select.h HAVE_SELECT)
  endif()
  if (NOT HAVE_SELECT)
    message(FATAL_ERROR "rabbitmq-c requires poll() or select() to be available")
  endif()
endif()
cmake_pop_check_state()

check_library_exists(rt clock_gettime "time.h" CLOCK_GETTIME_NEEDS_LIBRT)
check_library_exists(rt posix_spawnp "spawn.h" POSIX_SPAWNP_NEEDS_LIBRT)
if (CLOCK_GETTIME_NEEDS_LIBRT OR POSIX_SPAWNP_NEEDS_LIBRT)
  set(LIBRT rt)
endif()

option(ENABLE_SSL_SUPPORT "Enable SSL support" ON)

if (ENABLE_SSL_SUPPORT)
  find_package(OpenSSL 1.1.1 REQUIRED)

  cmake_push_check_state()
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  cmake_pop_check_state()
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include(CTest)
endif()

include(CMakeDependentOption)

option(BUILD_SHARED_LIBS "Build rabbitmq-c as a shared library" ON)
option(BUILD_STATIC_LIBS "Build rabbitmq-c as a static library" ON)

option(BUILD_EXAMPLES "Build Examples" OFF)
option(BUILD_TOOLS "Build Tools (requires POPT Library)" OFF)
cmake_dependent_option(BUILD_TOOLS_DOCS "Build man pages for tools (requires xmlto)" OFF "BUILD_TOOLS" OFF)
option(BUILD_API_DOCS "Build Doxygen API docs" OFF)
option(RUN_SYSTEM_TESTS "Run system tests (i.e. tests requiring an accessible RabbitMQ server instance on localhost)" OFF)
option(BUILD_LIBFUZZ "Build LibFuzzer" OFF)
option(BUILD_AFLFUZZ "Build AFLFuzzer" OFF)

if (NOT BUILD_SHARED_LIBS AND NOT BUILD_STATIC_LIBS)
    message(FATAL_ERROR "One or both of BUILD_SHARED_LIBS or BUILD_STATIC_LIBS must be set to ON to build")
endif()

set(targets_export_name rabbitmq-targets)

if(BUILD_LIBFUZZ)
  if (NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR "Need clang for libFuzzer support")
  endif()
  if (NOT BUILD_STATIC_LIBS)
    message(FATAL_ERROR "LibFuzzer can only be built against static libraries " "(set BUILD_STATIC_LIBS=ON)")
  endif ()

  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=fuzzer-no-link,address,undefined")
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address,undefined")
endif ()

if(BUILD_AFLFUZZ)
  if (NOT BUILD_STATIC_LIBS)
    message(FATAL_ERROR "AFL-Fuzzer can only be built against static libraries " "(set BUILD_STATIC_LIBS=ON)")
  endif ()

  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address,undefined")
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address,undefined")
endif ()

add_subdirectory(librabbitmq)

if(BUILD_EXAMPLES)
  if(NOT BUILD_SHARED_LIBS)
    message(FATAL_ERROR "Examples require -DBUILD_SHARED_LIBS=ON")
  endif()
  add_subdirectory(examples)
endif()

if(BUILD_TOOLS)
  find_package(POPT REQUIRED)
  if(BUILD_TOOLS_DOCS)
    find_package(XMLTO REQUIRED)
  endif()
  add_subdirectory(tools)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
  if (NOT BUILD_STATIC_LIBS)
    message(FATAL_ERROR
      "Tests can only be built against static libraries "
      "(set BUILD_STATIC_LIBS=ON)")
  endif ()
  add_subdirectory(tests)
endif ()

if(BUILD_LIBFUZZ OR BUILD_AFLFUZZ)
  add_subdirectory(fuzz)
endif ()

if (BUILD_API_DOCS)
  find_package(Doxygen REQUIRED)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/docs/Doxyfile @ONLY)

  add_custom_target(docs
    COMMAND ${DOXYGEN_EXECUTABLE}
    VERBATIM
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs
    DEPENDS rabbitmq
    COMMENT "Generating API documentation"
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in
    )
endif ()

foreach (lib ${SOCKET_LIBRARIES})
    set(libs_private "${libs_private} -l${lib}")
endforeach(lib)
set(libs_private "${libs_private} -l${LIBRT}")
if (ENABLE_SSL_SUPPORT)
  set(libs_private "${libs_private} -lssl -lcrypto ${CMAKE_THREAD_LIBS_INIT}")
endif()

set(prefix ${CMAKE_INSTALL_PREFIX})
set(exec_prefix "\${prefix}")
set(libdir "\${exec_prefix}/${CMAKE_INSTALL_LIBDIR}")
set(includedir "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")

configure_file(cmake/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/librabbitmq/config.h)
configure_file(librabbitmq.pc.in ${CMAKE_CURRENT_BINARY_DIR}/librabbitmq.pc @ONLY)

include(CMakePackageConfigHelpers)
set(RMQ_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/rabbitmq-c)
set(project_config "${CMAKE_CURRENT_BINARY_DIR}/rabbitmq-c-config.cmake")
set(version_config "${CMAKE_CURRENT_BINARY_DIR}/rabbitmq-c-config-version.cmake")

write_basic_package_version_file(
    "${version_config}"
    VERSION ${RMQ_VERSION}
    COMPATIBILITY AnyNewerVersion)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/rabbitmq-c-config.cmake.in"
    "${project_config}"
    INSTALL_DESTINATION "${RMQ_CMAKE_DIR}")


if(BUILD_SHARED_LIBS)
    list(APPEND INSTALL_TARGETS rabbitmq)
endif()
if(BUILD_STATIC_LIBS)
    list(APPEND INSTALL_TARGETS rabbitmq-static)
endif()

export(TARGETS ${INSTALL_TARGETS} 
    NAMESPACE rabbitmq:: 
    FILE ${PROJECT_BINARY_DIR}/${targets_export_name}.cmake)

install(FILES ${project_config} ${version_config}
    DESTINATION ${RMQ_CMAKE_DIR}
    COMPONENT rabbitmq-c-development
    )

install(EXPORT ${targets_export_name} 
    DESTINATION ${RMQ_CMAKE_DIR}
    NAMESPACE rabbitmq::
    COMPONENT rabbitmq-c-development
    )

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/librabbitmq.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
  )

if (BUILD_SHARED_LIBS)
  message(STATUS "Building rabbitmq as a shared library - yes")
else ()
  message(STATUS "Building rabbitmq as a shared library - no")
endif ()

if (BUILD_STATIC_LIBS)
  message(STATUS "Building rabbitmq as a static library - yes")
else ()
  message(STATUS "Building rabbitmq as a static library - no")
endif ()
