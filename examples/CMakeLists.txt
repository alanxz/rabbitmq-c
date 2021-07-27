# Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
# SPDX-License-Identifier: mit

add_library(examples-common OBJECT)
target_sources(examples-common PRIVATE
  utils.h
  utils.c)
if(WIN32)
  target_sources(examples-common PRIVATE win32/platform_utils.c)
else()
  target_sources(examples-common PRIVATE unix/platform_utils.c)
endif()
target_link_libraries(examples-common PRIVATE rabbitmq::rabbitmq)

add_executable(amqp_sendstring amqp_sendstring.c)
target_link_libraries(amqp_sendstring PRIVATE examples-common rabbitmq::rabbitmq)

add_executable(amqp_rpc_sendstring_client amqp_rpc_sendstring_client.c)
target_link_libraries(amqp_rpc_sendstring_client PRIVATE examples-common rabbitmq::rabbitmq)

add_executable(amqp_exchange_declare amqp_exchange_declare.c)
target_link_libraries(amqp_exchange_declare PRIVATE examples-common rabbitmq::rabbitmq)

add_executable(amqp_listen amqp_listen.c)
target_link_libraries(amqp_listen PRIVATE examples-common rabbitmq::rabbitmq)

add_executable(amqp_producer amqp_producer.c)
target_link_libraries(amqp_producer PRIVATE examples-common rabbitmq::rabbitmq)

add_executable(amqp_connect_timeout amqp_connect_timeout.c)
target_link_libraries(amqp_connect_timeout PRIVATE examples-common rabbitmq::rabbitmq)

add_executable(amqp_consumer amqp_consumer.c)
target_link_libraries(amqp_consumer PRIVATE examples-common rabbitmq::rabbitmq)

add_executable(amqp_unbind amqp_unbind.c)
target_link_libraries(amqp_unbind PRIVATE examples-common rabbitmq::rabbitmq)

add_executable(amqp_bind amqp_bind.c)
target_link_libraries(amqp_bind PRIVATE examples-common rabbitmq::rabbitmq)

add_executable(amqp_listenq amqp_listenq.c)
target_link_libraries(amqp_listenq PRIVATE examples-common rabbitmq::rabbitmq)

if (ENABLE_SSL_SUPPORT)
  add_executable(amqp_ssl_connect amqp_ssl_connect.c)
  target_link_libraries(amqp_ssl_connect PRIVATE examples-common rabbitmq::rabbitmq)
endif (ENABLE_SSL_SUPPORT)
