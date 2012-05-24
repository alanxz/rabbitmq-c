include_directories(${LIBRABBITMQ_INCLUDE_DIRS})

if (WIN32)
    set(PLATFORM_DIR win32)
else (WIN32)
    set(PLATFORM_DIR unix)
endif (WIN32)

set(COMMON_SRCS
    utils.h
    utils.c
    ${PLATFORM_DIR}/platform_utils.c
    )

add_executable(amqp_sendstring amqp_sendstring.c ${COMMON_SRCS})
target_link_libraries(amqp_sendstring rabbitmq)

add_executable(amqp_exchange_declare amqp_exchange_declare.c ${COMMON_SRCS})
target_link_libraries(amqp_exchange_declare rabbitmq)

add_executable(amqp_listen amqp_listen.c ${COMMON_SRCS})
target_link_libraries(amqp_listen rabbitmq)

add_executable(amqp_producer amqp_producer.c ${COMMON_SRCS})
target_link_libraries(amqp_producer rabbitmq)

add_executable(amqp_consumer amqp_consumer.c ${COMMON_SRCS})
target_link_libraries(amqp_consumer rabbitmq)

add_executable(amqp_unbind amqp_unbind.c ${COMMON_SRCS})
target_link_libraries(amqp_unbind rabbitmq)

add_executable(amqp_bind amqp_bind.c ${COMMON_SRCS})
target_link_libraries(amqp_bind rabbitmq)

add_executable(amqp_listenq amqp_listenq.c ${COMMON_SRCS})
target_link_libraries(amqp_listenq rabbitmq)
