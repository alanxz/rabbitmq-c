project(librabbitmq "C")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Stuff dealing with code generation
set(AMQP_CODEGEN_PY "${CMAKE_CURRENT_BINARY_DIR}/amqp_codegen.py")
set(CODEGEN_PY "${CMAKE_CURRENT_BINARY_DIR}/codegen.py")
set(AMQP_SPEC_JSON_PATH "${AMQP_CODEGEN_DIR}/amqp-rabbitmq-0.9.1.json")

#generate amqp_framing.h/amqp_framing.c

if (PYTHON_VERSION_MAJOR GREATER 2)
  set(CONVERT_CODEGEN ${PYTHON_2TO3_EXECUTABLE} -w ${CODEGEN_PY} > codegen_2to3.out)
  set(CONVERT_AMQP_CODEGEN ${PYTHON_2TO3_EXECUTABLE} -w ${AMQP_CODEGEN_PY} > amqp_codegen_2to3.out)
else ()
  set(CONVERT_CODEGEN "")
  set(CONVERT_AMQP_CODEGEN "")
endif ()

add_custom_command(
  OUTPUT ${CODEGEN_PY}
  COMMAND ${CMAKE_COMMAND} ARGS -E copy ${CMAKE_CURRENT_SOURCE_DIR}/codegen.py ${CODEGEN_PY}
  COMMAND ${CONVERT_CODEGEN}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/codegen.py
  VERBATIM)

add_custom_command(
  OUTPUT ${AMQP_CODEGEN_PY}
  COMMAND ${CMAKE_COMMAND} ARGS -E copy ${AMQP_CODEGEN_DIR}/amqp_codegen.py ${AMQP_CODEGEN_PY}
  COMMAND ${CONVERT_AMQP_CODEGEN}
  DEPENDS ${AMQP_CODEGEN_DIR}/amqp_codegen.py ${AMQP_CODEGEN_TARGET}
  VERBATIM)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/amqp_framing.h
  COMMAND ${PYTHON_EXECUTABLE} ARGS ${CODEGEN_PY} header ${AMQP_SPEC_JSON_PATH} ${CMAKE_CURRENT_BINARY_DIR}/amqp_framing.h
  DEPENDS ${AMQP_SPEC_JSON_PATH} ${CODEGEN_PY} ${AMQP_CODEGEN_PY}
  VERBATIM)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/amqp_framing.c
  COMMAND ${PYTHON_EXECUTABLE} ARGS ${CODEGEN_PY} body ${AMQP_SPEC_JSON_PATH} ${CMAKE_CURRENT_BINARY_DIR}/amqp_framing.c
  DEPENDS ${AMQP_SPEC_JSON_PATH} ${CODEGEN_PY} ${AMQP_CODEGEN_PY}
  VERBATIM)

if(WIN32)
  set(SOCKET_IMPL "win32")
else(WIN32)
  set(SOCKET_IMPL "unix")
endif(WIN32)

if(MSVC)
  if(MSVC_VERSION LESS 1600)
    set(MSINTTYPES_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/win32/msinttypes")
    set(STDINT_H_INSTALL_FILE "${CMAKE_CURRENT_SOURCE_DIR}/win32/msinttypes/stdint.h")
  endif(MSVC_VERSION LESS 1600)
endif(MSVC)

set(LIBRABBITMQ_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${SOCKET_IMPL}
	${MSINTTYPES_INCLUDE}
	)

include_directories(${LIBRABBITMQ_INCLUDE_DIRS})

set(LIBRABBITMQ_INCLUDE_DIRS
	${LIBRABBITMQ_INCLUDE_DIRS}
	PARENT_SCOPE)

add_definitions(-DHAVE_CONFIG_H)

set(RABBITMQ_SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/amqp_framing.h
    ${CMAKE_CURRENT_BINARY_DIR}/amqp_framing.c
    amqp_api.c  amqp.h 
    amqp_connection.c  amqp_mem.c  amqp_private.h  amqp_socket.c  amqp_table.c
    amqp_url.c
    ${SOCKET_IMPL}/socket.h ${SOCKET_IMPL}/socket.c
)

add_definitions(-DAMQP_BUILD)

if (BUILD_SHARED_LIBS)
    add_library(rabbitmq SHARED ${RABBITMQ_SOURCES})

    if (WIN32)
        set_target_properties(rabbitmq PROPERTIES VERSION ${RMQ_VERSION} OUTPUT_NAME rabbitmq.${RMQ_SOVERSION})
    else (WIN32)
        set_target_properties(rabbitmq PROPERTIES VERSION ${RMQ_VERSION} SOVERSION ${RMQ_SOVERSION})
    endif (WIN32)

    if (DEFINED SOCKET_LIBRARIES)
      target_link_libraries(rabbitmq ${SOCKET_LIBRARIES})
    endif ()

    install(TARGETS rabbitmq
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        )
    set(RMQ_LIBRARY_TARGET rabbitmq)
endif (BUILD_SHARED_LIBS)

if (BUILD_STATIC_LIBS)
    add_library(rabbitmq-static STATIC ${RABBITMQ_SOURCES})

    set_target_properties(rabbitmq-static PROPERTIES COMPILE_DEFINITIONS AMQP_STATIC)
    if (WIN32)
        set_target_properties(rabbitmq-static PROPERTIES VERSION ${RMQ_VERSION} OUTPUT_NAME librabbitmq.${RMQ_SOVERSION})
    else (WIN32)
        set_target_properties(rabbitmq-static PROPERTIES VERSION ${RMQ_VERSION} SOVERSION ${RMQ_SOVERSION} OUTPUT_NAME rabbitmq)
    endif (WIN32)

    if (DEFINED SOCKET_LIBRARIES)
      target_link_libraries(rabbitmq-static ${SOCKET_LIBRARIES})
    endif ()

    install(TARGETS rabbitmq-static
        ARCHIVE DESTINATION lib
        )

    if (NOT DEFINED RMQ_LIBRARY_TARGET)
        set(RMQ_LIBRARY_TARGET rabbitmq-static)
    endif ()
endif (BUILD_STATIC_LIBS)

install(FILES amqp.h ${CMAKE_CURRENT_BINARY_DIR}/amqp_framing.h ${STDINT_H_INSTALL_FILE}
	DESTINATION include
    )

set(RMQ_LIBRARY_TARGET ${RMQ_LIBRARY_TARGET} PARENT_SCOPE)

