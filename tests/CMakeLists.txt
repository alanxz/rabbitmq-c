include_directories(${LIBRABBITMQ_INCLUDE_DIRS})

# Needed because no version of MSVC has inttypes.h
if (MSVC)
  include_directories(${CMAKE_SOURCE_DIR}/librabbitmq/win32/msinttypes)
endif()

add_executable(test_parse_url test_parse_url.c)
target_link_libraries(test_parse_url rabbitmq)
add_test(parse_url test_parse_url)

add_executable(test_tables test_tables.c)
target_link_libraries(test_tables rabbitmq)
add_test(tables test_tables)
configure_file(test_tables.expected ${CMAKE_CURRENT_BINARY_DIR}/tests/test_tables.expected COPY_ONLY)
