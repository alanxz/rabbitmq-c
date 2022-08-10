include_directories(
  ${LIBRABBITMQ_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}/../librabbitmq/
  ${CMAKE_CURRENT_SOURCE_DIR}/../librabbitmq/)

add_definitions(-DHAVE_CONFIG_H)
add_definitions(-DAMQP_STATIC)

SET(CMAKE_EXE_LINKER_FLAGS "${LIB_FUZZING_ENGINE}")

if(BUILD_OSSFUZZ)
  add_executable(fuzz_url fuzz_url.c)
  target_link_libraries(fuzz_url rabbitmq-static)

  add_executable(fuzz_table fuzz_table.c)
  target_link_libraries(fuzz_table rabbitmq-static)

  add_executable(fuzz_server fuzz_server.c)
  target_link_libraries(fuzz_server rabbitmq-static)
endif ()
