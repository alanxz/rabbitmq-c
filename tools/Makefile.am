SUBDIRS=doc

bin_PROGRAMS = amqp-publish amqp-get amqp-consume amqp-declare-queue amqp-delete-queue

# The -I to srcdir's librabbitmq is for the main amqp header
# files. The -I to builddir's librabbitmq is less obvious; it's for
# VPATH-based builds: the amqp_framing.h gets generated into the
# *build* directory, not the source directory.
AM_CFLAGS = -I$(top_srcdir)/librabbitmq -I$(srcdir)/$(PLATFORM_DIR) -I$(top_builddir)/librabbitmq
AM_LDFLAGS = $(top_builddir)/librabbitmq/librabbitmq.la

LDADD=$(LIBPOPT)

noinst_HEADERS = common.h $(PLATFORM_DIR)/process.h

COMMON_SOURCES = common.c

if WINDOWS
COMMON_SOURCES += windows/compat.c
endif

amqp_publish_SOURCES = publish.c $(COMMON_SOURCES)
amqp_get_SOURCES = get.c $(COMMON_SOURCES)
amqp_consume_SOURCES = consume.c $(PLATFORM_DIR)/process.c $(COMMON_SOURCES)
amqp_declare_queue_SOURCES = declare_queue.c $(COMMON_SOURCES)
amqp_delete_queue_SOURCES = delete_queue.c $(COMMON_SOURCES)

EXTRA_DIST = \
	unix/process.c unix/process.h \
	windows/process.c windows/process.h \
	windows/compat.c windows/compat.h
