lib_LTLIBRARIES = librabbitmq.la

AM_CFLAGS = -I$(srcdir)/$(PLATFORM_DIR) -DBUILDING_LIBRABBITMQ

if GCC
# Because we want to build under Microsoft's C compiler (for which
# there is apparently no demand for C99 support), it's a good idea
# to have gcc tell us when we stray from the old standard.
AM_CFLAGS += -ansi -pedantic
endif

librabbitmq_la_SOURCES = amqp_mem.c amqp_table.c amqp_connection.c amqp_socket.c amqp_debug.c amqp_api.c $(PLATFORM_DIR)/socket.c
librabbitmq_la_LDFLAGS = -no-undefined
librabbitmq_la_LIBADD = $(EXTRA_LIBS)
nodist_librabbitmq_la_SOURCES = amqp_framing.c
include_HEADERS = amqp_framing.h amqp.h
noinst_HEADERS = amqp_private.h $(PLATFORM_DIR)/socket.h
BUILT_SOURCES = amqp_framing.h amqp_framing.c
CLEANFILES = amqp_framing.h amqp_framing.c
EXTRA_DIST = \
	codegen.py \
	unix/socket.c unix/socket.h \
	windows/socket.c windows/socket.h

CODEGEN_PY=$(srcdir)/codegen.py

amqp_framing.h: $(AMQP_SPEC_JSON_PATH) $(CODEGEN_PY)
	PYTHONPATH=$(AMQP_CODEGEN_DIR) $(PYTHON) $(CODEGEN_PY) header $< $@

amqp_framing.c: $(AMQP_SPEC_JSON_PATH) $(CODEGEN_PY)
	PYTHONPATH=$(AMQP_CODEGEN_DIR) $(PYTHON) $(CODEGEN_PY) body $< $@
