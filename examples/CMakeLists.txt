include_directories(${librabbitmq_SOURCE_DIR} ${librabbitmq_BINARY_DIR})

add_executable(amqp_sendstring amqp_sendstring.c utils.h utils.c)
target_link_libraries(amqp_sendstring rabbitmq)
