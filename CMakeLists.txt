cmake_minimum_required(VERSION 2.6)
project(rabbitmq-c "C")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

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
  PATHS ${CMAKE_SOURCE_DIR}/codegen
        ${CMAKE_SOURCE_DIR}/rabbitmq-codegen 
        ${CMAKE_SOURCE_DIR}/../rabbitmq-codegen
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

option(BUILD_SHARED_LIBS "Build rabbitmq-c as a shared library" ON)

if (WIN32 AND NOT BUILD_SHARED_LIBS)
  message(FATAL_ERROR "The rabbitmq-c library cannot be built as a static library on Win32. Set BUILD_SHARED_LIBS=ON to get around this.")
endif()

add_subdirectory(librabbitmq)
add_subdirectory(examples)

if (POPT_FOUND)
    add_subdirectory(tools)
endif (POPT_FOUND)

