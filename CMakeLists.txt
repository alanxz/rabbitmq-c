cmake_minimum_required(VERSION 2.6)
project(rabbitmq-c "C")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(VERSION "0.2")

if (MSVC)
  set(CMAKE_C_FLAGS "/W4 /nologo ${CMAKE_C_FLAGS}")
else ()
  set(CMAKE_C_FLAGS "-Wall -Wextra -pedantic -Wstrict-prototypes -Wcast-align -Wno-unused-function -fno-common -fvisibility=hidden ${CMAKE_C_FLAGS}")
endif ()

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

#find python
find_package(PythonInterp REQUIRED)

#check for json or simplejson
execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import json"
  RESULT_VARIABLE CHECK_PYTHON_JSON_FAILED
)

if (CHECK_PYTHON_JSON_FAILED)
  execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import simplejson"
    RESULT_VARIABLE CHECK_PYTHON_SIMPLEJSON_FAILED
  )

  if (CHECK_PYTHON_SIMPLEJSON_FAILED)
    message(FATAL_ERROR "could not find a python that can 'import simplejson")
  endif (CHECK_PYTHON_SIMPLEJSON_FAILED)
endif (CHECK_PYTHON_JSON_FAILED)

include(TestCInline)

find_path(RABBITMQ_CODEGEN_DIR
  amqp_codegen.py
  PATHS ${CMAKE_CURRENT_SOURCE_DIR}/codegen
        ${CMAKE_CURRENT_SOURCE_DIR}/rabbitmq-codegen 
        ${CMAKE_CURRENT_SOURCE_DIR}/../rabbitmq-codegen
        DOC "Path to directory containing amqp_codegen.py (rabbitmq-codegen)"
        NO_DEFAULT_PATH
        )

if (RABBITMQ_CODEGEN_DIR STREQUAL "RABBITMQ_CODEGEN_DIR-NOTFOUND")
  message(SEND_ERROR "Cannot find amqp_codegen.py, did you forget to:\n\ngit submodule init\ngit submodule update\n?")
else ()
  message(STATUS "Using amqp_codegen.py in ${RABBITMQ_CODEGEN_DIR}")
endif()

set(AMQP_CODEGEN_DIR ${RABBITMQ_CODEGEN_DIR} CACHE PATH "Path to rabbitmq-codegen" FORCE)
mark_as_advanced(AMQP_CODEGEN_DIR)

find_package(POPT)
find_package(XmlTo)

if (POPT_FOUND AND XmlTo_FOUND)
  set(DO_DOCS ON)
endif()

option(BUILD_SHARED_LIBS "Build rabbitmq-c as a shared library" ON)
option(BUILD_EXAMPLES "Build Examples" ON)
option(BUILD_TOOLS "Build Tools (requires POPT Library)" ${POPT_FOUND})
option(BUILD_TOOLS_DOCS "Build man pages for Tools (requires xmlto)" ${DO_DOCS})
option(BUILD_TESTS "Build tests (run tests with make test)" ON)

if (WIN32 AND NOT BUILD_SHARED_LIBS)
  message(FATAL_ERROR "The rabbitmq-c library cannot be built as a static library on Win32. Set BUILD_SHARED_LIBS=ON to get around this.")
endif()

add_subdirectory(librabbitmq)

if (BUILD_EXAMPLES)
  add_subdirectory(examples)
endif ()

if (BUILD_TOOLS)
  if (POPT_FOUND)
    add_subdirectory(tools)
  else ()
    message(WARNING "POpt library was not found. Tools will not be built")
  endif ()
endif ()

if (BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif (BUILD_TESTS)

set(prefix ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${CMAKE_INSTALL_PREFIX}/bin)
set(libdir ${CMAKE_INSTALL_PREFIX}/lib)
set(includedir ${CMAKE_INSTALL_PREFIX}/include)

configure_file(librabbitmq.pc.in ${CMAKE_CURRENT_BINARY_DIR}/librabbitmq.pc @ONLY)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/librabbitmq.pc
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig
  )
