noinst_PROGRAMS = amqp_sendstring amqp_exchange_declare amqp_listen amqp_producer amqp_consumer amqp_unbind amqp_bind amqp_listenq

# The -I to srcdir's librabbitmq is for the main amqp header
# files. The -I to builddir's librabbitmq is less obvious; it's for
# VPATH-based builds: the amqp_framing.h gets generated into the
# *build* directory, not the source directory.
AM_CFLAGS = -I$(top_srcdir)/librabbitmq -I$(top_builddir)/librabbitmq

if GCC
# Because we want to build under Microsoft's C compiler (for which
# there is apparently no demand for C99 support), it's a good idea
# to have gcc tell us when we stray from the old standard.
AM_CFLAGS += -ansi -pedantic
endif

if USE_MSINTTYPES
AM_CFLAGS += -I$(top_srcdir)/msinttypes
endif

AM_LDFLAGS = $(top_builddir)/librabbitmq/librabbitmq.la

noinst_HEADERS = utils.h

COMMON_SOURCES = utils.c $(PLATFORM_DIR)/platform_utils.c

amqp_sendstring_SOURCES = amqp_sendstring.c $(COMMON_SOURCES)
amqp_exchange_declare_SOURCES = amqp_exchange_declare.c $(COMMON_SOURCES)
amqp_listen_SOURCES = amqp_listen.c $(COMMON_SOURCES)
amqp_producer_SOURCES = amqp_producer.c $(COMMON_SOURCES)
amqp_consumer_SOURCES = amqp_consumer.c $(COMMON_SOURCES)
amqp_unbind_SOURCES = amqp_unbind.c $(COMMON_SOURCES)
amqp_bind_SOURCES = amqp_bind.c $(COMMON_SOURCES)
amqp_listenq_SOURCES = amqp_listenq.c $(COMMON_SOURCES)
