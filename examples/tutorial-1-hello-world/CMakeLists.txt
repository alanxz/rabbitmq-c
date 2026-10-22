
add_executable(send send.c)
target_link_libraries(send ${RMQ_LIBRARY_TARGET})

add_executable(recv recv.c)
target_link_libraries(recv ${RMQ_LIBRARY_TARGET})
