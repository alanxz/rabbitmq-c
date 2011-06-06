cmake_minimum_required(VERSION 2.6)
project(rabbitmq-c)

#detect amqp_codegen directory
find_path(AMQP_CODEGEN_DIR "amqp-rabbitmq-0.9.1.json"
  PATHS "${CMAKE_CURRENT_SOURCE_DIR}/../rabbitmq-codegen" "${CMAKE_CURRENT_SOURCE_DIR}/../codegen"
  NO_DEFAULT_PATH)

if(NOT AMQP_CODEGEN_DIR)
  message(FATAL_ERROR "rabbitmq-codegen not found")
else()
  message("rabbitmq-codegen found at ${AMQP_CODEGEN_DIR}")
  set(AMQP_SPEC_JSON_PATH "${AMQP_CODEGEN_DIR}/amqp-rabbitmq-0.9.1.json")
endif()

#find python
find_package(PythonInterp REQUIRED)

#check simplejson
execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import json"
  RESULT_VARIABLE CHECK_PYTHON_JSON_FAILED
)
execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import simplejson"
  RESULT_VARIABLE CHECK_PYTHON_SIMPLEJSON_FAILED
)
if(CHECK_PYTHON_JSON_FAILED AND CHECK_PYTHON_SIMPLEJSON_FAILED)
  message(FATAL_ERROR "could not find a python that can 'import simplejson")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
include(TestCInline)

add_subdirectory(librabbitmq)
add_subdirectory(examples)
