project(librabbitmq "C")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(COPY "${AMQP_CODEGEN_DIR}/amqp_codegen.py" "codegen.py"
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)
file(COPY "${AMQP_CODEGEN_DIR}/amqp_codegen.py"
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)
set(CODEGEN_PY "${CMAKE_CURRENT_BINARY_DIR}/codegen.py")

#generate amqp_framing.h/amqp_framing.c
add_custom_command(
  OUTPUT "amqp_framing.h" "amqp_framing.c"
  COMMAND ${PYTHON_EXECUTABLE} ARGS ${CODEGEN_PY} header ${AMQP_SPEC_JSON_PATH} "amqp_framing.h"
  COMMAND ${PYTHON_EXECUTABLE} ARGS ${CODEGEN_PY} body ${AMQP_SPEC_JSON_PATH} "amqp_framing.c"
  DEPENDS ${AMQP_SPEC_JSON_PATH}
  VERBATIM)

#prepare config.h
file(WRITE "config.h" "#define VERSION \"0.0.1\"")

include_directories("unix")

set(RABBITMQ_SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/amqp_framing.h
    ${CMAKE_CURRENT_BINARY_DIR}/amqp_framing.c
    amqp_api.c  amqp.h 
    amqp_connection.c  amqp_mem.c  amqp_private.h  amqp_socket.c  amqp_table.c
    unix/socket.h unix/socket.c
)

add_library(rabbitmq ${RABBITMQ_SOURCES})
