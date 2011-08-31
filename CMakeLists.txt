cmake_minimum_required(VERSION 2.8)
project(rabbitmq-c)

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

include(ExternalProject)
ExternalProject_Add(
	amqp_codegen
	GIT_REPOSITORY https://github.com/rabbitmq/rabbitmq-codegen.git
	GIT_TAG rabbitmq_v2_5_1
	SOURCE_DIR amqp_codegen
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
	TEST_COMMAND "")

ExternalProject_Get_Property(amqp_codegen SOURCE_DIR)
set(AMQP_CODEGEN_DIR ${SOURCE_DIR})

add_subdirectory(librabbitmq)
add_subdirectory(examples)

