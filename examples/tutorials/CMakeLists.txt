# Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
# SPDX-License-Identifier: mit

# Examples that follow the tutorials at https://www.rabbitmq.com/tutorials
add_library(tutorials-common OBJECT)
target_sources(tutorials-common PRIVATE
  tutorial_common.h
  tutorial_common.c)
target_link_libraries(tutorials-common PUBLIC rabbitmq::rabbitmq)
if(ENABLE_SSL_SUPPORT)
  target_compile_definitions(tutorials-common PRIVATE AMQP_TUTORIALS_HAVE_SSL)
endif()

add_executable(hello_world_send hello_world_send.c)
target_link_libraries(hello_world_send PRIVATE tutorials-common rabbitmq::rabbitmq)

add_executable(hello_world_receive hello_world_receive.c)
target_link_libraries(hello_world_receive PRIVATE tutorials-common rabbitmq::rabbitmq)

add_executable(work_queues_new_task work_queues_new_task.c)
target_link_libraries(work_queues_new_task PRIVATE tutorials-common rabbitmq::rabbitmq)

add_executable(work_queues_worker work_queues_worker.c)
target_link_libraries(work_queues_worker PRIVATE tutorials-common rabbitmq::rabbitmq)

add_executable(publish_subscribe_emit_log publish_subscribe_emit_log.c)
target_link_libraries(publish_subscribe_emit_log PRIVATE tutorials-common rabbitmq::rabbitmq)

add_executable(publish_subscribe_receive_logs publish_subscribe_receive_logs.c)
target_link_libraries(publish_subscribe_receive_logs PRIVATE tutorials-common rabbitmq::rabbitmq)

add_executable(routing_emit_log_direct routing_emit_log_direct.c)
target_link_libraries(routing_emit_log_direct PRIVATE tutorials-common rabbitmq::rabbitmq)

add_executable(routing_receive_logs_direct routing_receive_logs_direct.c)
target_link_libraries(routing_receive_logs_direct PRIVATE tutorials-common rabbitmq::rabbitmq)

add_executable(topics_emit_log_topic topics_emit_log_topic.c)
target_link_libraries(topics_emit_log_topic PRIVATE tutorials-common rabbitmq::rabbitmq)

add_executable(topics_receive_logs_topic topics_receive_logs_topic.c)
target_link_libraries(topics_receive_logs_topic PRIVATE tutorials-common rabbitmq::rabbitmq)

add_executable(rpc_server rpc_server.c)
target_link_libraries(rpc_server PRIVATE tutorials-common rabbitmq::rabbitmq)

add_executable(rpc_client rpc_client.c)
target_link_libraries(rpc_client PRIVATE tutorials-common rabbitmq::rabbitmq)

add_executable(publisher_confirms publisher_confirms.c)
target_link_libraries(publisher_confirms PRIVATE tutorials-common rabbitmq::rabbitmq)
