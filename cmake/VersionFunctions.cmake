function(get_library_version VERSION_ARG)
    file(STRINGS include/rabbitmq-c/amqp.h _API_VERSION_MAJOR REGEX "^#define AMQP_VERSION_MAJOR [0-9]+$")
    file(STRINGS include/rabbitmq-c/amqp.h _API_VERSION_MINOR REGEX "^#define AMQP_VERSION_MINOR [0-9]+$")
    file(STRINGS include/rabbitmq-c/amqp.h _API_VERSION_PATCH REGEX "^#define AMQP_VERSION_PATCH [0-9]+$")

    string(REGEX MATCH "[0-9]+" _API_VERSION_MAJOR ${_API_VERSION_MAJOR})
    string(REGEX MATCH "[0-9]+" _API_VERSION_MINOR ${_API_VERSION_MINOR})
    string(REGEX MATCH "[0-9]+" _API_VERSION_PATCH ${_API_VERSION_PATCH})

    # VERSION to match what is in autotools
    set(${VERSION_ARG} ${_API_VERSION_MAJOR}.${_API_VERSION_MINOR}.${_API_VERSION_PATCH} PARENT_SCOPE)
endfunction()

function(compute_soversion CURRENT REVISION AGE SOVERSION)
    math(EXPR MAJOR "${CURRENT} - ${AGE}")
    math(EXPR MINOR "${AGE}")
    math(EXPR PATCH "${REVISION}")

    set(${SOVERSION} ${MAJOR} PARENT_SCOPE)
endfunction()