cmake_minimum_required(VERSION 2.8)
project(rabbitmq-c "C")

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

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
include(TestCInline)

option(FETCH_CODEGEN_FROM_GIT "Fetch codegen directly from the git repository" OFF)

set(CODEGEN_GIT_TAG rabbitmq_v2_5_1 CACHE STRING "Git tag in rabbitmq-codegen to fetch with FETCH_CODEGEN_FROM_GIT")
if (FETCH_CODEGEN_FROM_GIT)
    include(ExternalProject)
    ExternalProject_Add(
        amqp_codegen
        GIT_REPOSITORY https://github.com/rabbitmq/rabbitmq-codegen.git
        GIT_TAG ${CODEGEN_GIT_TAG}
        CONFIGURE_COMMAND ""
        UPDATE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        TEST_COMMAND "")

    ExternalProject_Get_Property(amqp_codegen SOURCE_DIR)
    set(AMQP_CODEGEN_DIR ${SOURCE_DIR} CACHE PATH "Path to rabbitmq-codegen" FORCE)
    SET(AMQP_CODEGEN_TARGET amqp_codegen)
    message(STATUS "Using amqp_codegen.py in ${AMQP_CODEGEN_DIR}")
else (FETCH_CODEGEN_FROM_GIT)
    find_path(RABBITMQ_CODEGEN_DIR
        amqp_codegen.py
        PATHS ${CMAKE_SOURCE_DIR}/rabbitmq-codegen 
              ${CMAKE_SOURCE_DIR}/../rabbitmq-codegen
        DOC "Path to directory containing amqp_codegen.py (rabbitmq-codegen)"
        NO_DEFAULT_PATH
        )

    if (RABBITMQ_CODEGEN_DIR STREQUAL "RABBITMQ_CODEGEN_DIR-NOTFOUND")
        message(SEND_ERROR "Cannot find rabbitmq-codegen, set RABBITMQ_CODEGEN_DIR to a rabbitmq-codegen checkout, or set FETCH_CODEGEN_FROM_GIT to download it from git automatically")
    else ()
        message(STATUS "Using amqp_codegen.py in ${RABBITMQ_CODEGEN_DIR}")
    endif()

    set(AMQP_CODEGEN_DIR ${RABBITMQ_CODEGEN_DIR} CACHE PATH "Path to rabbitmq-codegen" FORCE)
endif (FETCH_CODEGEN_FROM_GIT)

mark_as_advanced(AMQP_CODEGEN_DIR)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

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

