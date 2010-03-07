bin_PROGRAMS = amqp-publish amqp-get amqp-consume

AM_CFLAGS = -I$(top_srcdir)/librabbitmq
AM_LDFLAGS = $(top_builddir)/librabbitmq/librabbitmq.la

LDADD=$(LIBPOPT)

noinst_HEADERS = common.h common_consume.h

amqp_publish_SOURCES = publish.c common.c
amqp_get_SOURCES = get.c common.c common_consume.c
amqp_consume_SOURCES = consume.c common.c common_consume.c
