include_directories(${LIBRABBITMQ_INCLUDE_DIRS})

if (MSVC)
  # No version of MSVC has inttypes.h, this uses the msinttypes
  #  Note this includes stdint.h which is either in 
  #  ../librabbitmq/win32/msinttypes or in a standard location
  include_directories(win32/msinttypes)
endif (MSVC)

add_executable(test_parse_url test_parse_url.c)
target_link_libraries(test_parse_url rabbitmq)
add_test(parse_url test_parse_url)

add_executable(test_tables test_tables.c)
target_link_libraries(test_tables rabbitmq)
add_test(tables test_tables)
configure_file(test_tables.expected ${CMAKE_CURRENT_BINARY_DIR}/tests/test_tables.expected COPY_ONLY)
