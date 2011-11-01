include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${librabbitmq_SOURCE_DIR} ${librabbitmq_BINARY_DIR} ${POPT_INCLUDE_DIRS})

if (WIN32)
    set(PLATFORM_DIR win32)
    set(PLATFORM_SRCS
        windows/compat.c
        )
else (WIN32)
    set(PLATFORM_DIR unix)
endif (WIN32)

include_directories(${PLATFORM_DIR})

set(COMMON_SRCS
    common.h
    common.c
    ${PLATFORM_SRCS}
    )

add_executable(amqp-publish publish.c ${COMMON_SRCS})
target_link_libraries(amqp-publish rabbitmq ${POPT_LIBRARY})

add_executable(amqp-get get.c ${COMMON_SRCS})
target_link_libraries(amqp-get rabbitmq ${POPT_LIBRARY})

add_executable(amqp-consume consume.c ${PLATFORM_DIR}/process.c ${COMMON_SRCS})
target_link_libraries(amqp-consume rabbitmq ${POPT_LIBRARY})

add_executable(amqp-declare-queue declare_queue.c ${COMMON_SRCS})
target_link_libraries(amqp-declare-queue rabbitmq ${POPT_LIBRARY})

add_executable(amqp-delete-queue delete_queue.c ${COMMON_SRCS})
target_link_libraries(amqp-delete-queue rabbitmq ${POPT_LIBRARY})

