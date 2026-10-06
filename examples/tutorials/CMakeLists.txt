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
